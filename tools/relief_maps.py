"""Build the relief (tangent-space normal) maps in assets/textures/ (SPEC A164).

Sources are measured global topography, all public-domain NASA/USGS works:

  Moon     LRO LOLA, 16 px/degree float32 kilometres (NASA SVS CGI Moon Kit,
           ldem_16.tif; area-averaged from the LOLA LDEM).
  Mars     MGS MOLA MEGDR, 16 px/degree big-endian int16 metres
           (PDS Geosciences Node, megt90n000eb.img; left edge 0 E).
  Mercury  MESSENGER USGS global DEM 665 m, little-endian int16 metres
           (USGS Astrogeology, PDS Imaging Node bucket). The file is 530 MB
           of uncompressed strips, so only the rows needed are range-read
           (two per output row, ~94 MB).

Each height field is reduced to 2048 x 1024 (Mercury 1024 x 512) by area averaging, recentred on
longitude 0 with east to the right (the layout render_build_sphere expects),
turned into slopes on the sphere (east: dh / (R cos(lat) dlon), north:
dh / (R dlat)) and stored as a tangent-space normal (east, north, up) in RGB,
n = normalize(-k dh/dx, -k dh/dy, 1), encoded n * 0.5 + 0.5. At ~5 km per
texel the true slopes of all but the largest structures are a few degrees, so
the relief is exaggerated by k (RELIEF_EXAGGERATION below) to read on screen;
the shapes and their placement are measured, the height scale is
presentation. Near the poles, where a texel's east-west width shrinks to
nothing, the east slope is faded out with cos(latitude).

Usage:
  python3 tools/relief_maps.py            # convert from the cache in build/relief-maps/
  python3 tools/relief_maps.py --refresh  # download missing sources first (~200 MB)
"""
import argparse
import concurrent.futures
import math
import struct
import sys
import urllib.request
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
CACHE = ROOT / "build" / "relief-maps"
OUTPUT = ROOT / "assets" / "textures"
SIZE = (2048, 1024)
RELIEF_EXAGGERATION = 4.0

MOON_URL = "https://svs.gsfc.nasa.gov/vis/a000000/a004700/a004720/ldem_16.tif"
MARS_URL = "https://pds-geosciences.wustl.edu/mgs/mgs-m-mola-5-megdr-l3-v1/mgsl_300x/meg016/megt90n000eb.img"
MERCURY_URL = "https://asc-pds-services.s3.us-west-2.amazonaws.com/mosaic/Mercury_Messenger_USGS_DEM_Global_665m_v2.tif"

# name, mean radius (m) for the slope scale, left-edge east longitude, size.
# Mercury is rarely framed up close, so its map is halved to save download.
BODIES = {
    "moon": (1737.4e3, -180.0, SIZE),
    "mars": (3389.5e3, 0.0, SIZE),
    # The DEM's equirectangular projection is centred on 180 E, so its
    # left edge (tiepoint -pi R) is 0 E: Caloris (31 N, 170 E) sits there.
    "mercury": (2439.4e3, 0.0, (1024, 512)),
}


def fetch(url: str, path: Path) -> None:
    if path.exists() and path.stat().st_size > 0:
        return
    CACHE.mkdir(parents=True, exist_ok=True)
    partial = path.with_suffix(".part")
    with urllib.request.urlopen(url, timeout=120) as response, open(partial, "wb") as out:
        while chunk := response.read(1 << 20):
            out.write(chunk)
    partial.replace(path)


def read_range(url: str, start: int, length: int) -> bytes:
    request = urllib.request.Request(url, headers={"Range": f"bytes={start}-{start + length - 1}"})
    with urllib.request.urlopen(request, timeout=120) as response:
        data = response.read()
    if len(data) != length:
        raise RuntimeError(f"short range read at {start}: {len(data)} of {length} bytes")
    return data


def tiff_layout(head: bytes, url: str) -> dict:
    """Width, height, strip offsets and rows per strip of a classic or Big
    TIFF holding one uncompressed band (the only layout these sources use)."""
    order = "<" if head[:2] == b"II" else ">"
    big = struct.unpack(order + "H", head[2:4])[0] == 43
    offset = struct.unpack(order + ("Q" if big else "I"), head[8:16] if big else head[4:8])[0]
    count_size, entry_size = (8, 20) if big else (2, 12)
    count = struct.unpack(order + ("Q" if big else "H"), head[offset:offset + count_size])[0]
    sizes = {3: 2, 4: 4, 16: 8}
    tags = {}
    for k in range(count):
        entry = head[offset + count_size + k * entry_size: offset + count_size + (k + 1) * entry_size]
        tag, kind = struct.unpack(order + "HH", entry[:4])
        n = struct.unpack(order + ("Q" if big else "I"), entry[4:12] if big else entry[4:8])[0]
        value = entry[12:20] if big else entry[8:12]
        if kind not in sizes:
            continue  # only the integer tags below are needed
        width = sizes[kind]
        if n * width <= len(value):
            items = [struct.unpack(order + {2: "H", 4: "I", 8: "Q"}[width], value[i * width:(i + 1) * width])[0] for i in range(n)]
        else:
            where = struct.unpack(order + ("Q" if big else "I"), value[:8] if big else value[:4])[0]
            blob = head[where:where + n * width] if where + n * width <= len(head) else read_range(url, where, n * width)
            items = list(struct.unpack(order + {2: "H", 4: "I", 8: "Q"}[width] * n, blob))
        tags[tag] = items
    if tags.get(259, [1])[0] != 1:
        raise RuntimeError("compressed TIFF: unsupported")
    return {"order": order, "width": tags[256][0], "height": tags[257][0], "offsets": tags[273],
            "rows_per_strip": tags.get(278, [tags[257][0]])[0], "bits": tags[258][0]}


def mercury_rows() -> Image.Image:
    """Range-read two rows per output row of the USGS DEM (int16 metres)."""
    path = CACHE / "mercury_rows.raw"
    head = read_range(MERCURY_URL, 0, 1 << 20)
    layout = tiff_layout(head, MERCURY_URL)
    width, height = layout["width"], layout["height"]
    rows = [min(height - 1, int((y + 0.5) * height / (SIZE[1] * 2))) for y in range(SIZE[1] * 2)]
    if not (path.exists() and path.stat().st_size == len(rows) * width * 2):
        CACHE.mkdir(parents=True, exist_ok=True)

        def row_bytes(row: int) -> bytes:
            strip, within = divmod(row, layout["rows_per_strip"])
            return read_range(MERCURY_URL, layout["offsets"][strip] + within * width * 2, width * 2)

        with concurrent.futures.ThreadPoolExecutor(16) as pool:
            data = list(pool.map(row_bytes, rows))
        path.with_suffix(".part").write_bytes(b"".join(data))
        path.with_suffix(".part").replace(path)
    raw = "F;16S" if layout["order"] == "<" else "F;16BS"
    return Image.frombytes("F", (width, len(rows)), path.read_bytes(), "raw", raw)


def height_field(name: str) -> Image.Image:
    """Heights in metres as a 32-bit float image, before recentring."""
    if name == "moon":
        path = CACHE / "ldem_16.tif"
        data = path.read_bytes()
        layout = tiff_layout(data, MOON_URL)  # its directory sits at the end
        width, height = layout["width"], layout["height"]
        rows = bytearray()
        for row in range(height):
            strip, within = divmod(row, layout["rows_per_strip"])
            start = layout["offsets"][strip] + within * width * 4
            rows += data[start:start + width * 4]
        raw = "F;32F" if layout["order"] == "<" else "F;32BF"
        image = Image.frombytes("F", (width, height), bytes(rows), "raw", raw)
        return image.point(lambda km: km * 1000.0)
    if name == "mars":
        return Image.frombytes("F", (5760, 2880), (CACHE / "megt90n000eb.img").read_bytes(), "raw", "F;16BS")
    return mercury_rows()


def recentre(image: Image.Image, left_east: float) -> Image.Image:
    """Roll columns so the left edge is longitude -180 (centre 0)."""
    shift = round((left_east + 180) / 360 * image.width) % image.width
    if not shift:
        return image
    rolled = Image.new(image.mode, image.size)
    rolled.paste(image.crop((0, 0, image.width - shift, image.height)), (shift, 0))
    rolled.paste(image.crop((image.width - shift, 0, image.width, image.height)), (0, 0))
    return rolled


def normal_map(heights: Image.Image, radius_m: float) -> Image.Image:
    width, height = heights.size
    h = list(heights.get_flattened_data()) if hasattr(heights, "get_flattened_data") else list(heights.getdata())
    d_lon = 2 * math.pi / width
    d_lat = math.pi / height
    out = bytearray(width * height * 3)
    k = RELIEF_EXAGGERATION
    for y in range(height):
        latitude = math.pi / 2 - (y + 0.5) * d_lat
        cos_lat = math.cos(latitude)
        east_scale = cos_lat / (2 * radius_m * max(cos_lat, 0.05) * d_lon)  # fades with cos(lat) at the poles
        north_scale = 1 / (2 * radius_m * d_lat)
        up, down = max(y - 1, 0), min(y + 1, height - 1)
        row, row_up, row_down = y * width, up * width, down * width
        for x in range(width):
            left, right = (x - 1) % width, (x + 1) % width
            sx = (h[row + right] - h[row + left]) * east_scale
            sy = (h[row_up + x] - h[row_down + x]) * north_scale * (2 if up == y or down == y else 1)
            nx, ny, nz = -k * sx, -k * sy, 1.0
            norm = math.sqrt(nx * nx + ny * ny + nz * nz)
            i = (row + x) * 3
            out[i] = round((nx / norm * 0.5 + 0.5) * 255)
            out[i + 1] = round((ny / norm * 0.5 + 0.5) * 255)
            out[i + 2] = round((nz / norm * 0.5 + 0.5) * 255)
    return Image.frombytes("RGB", (width, height), bytes(out))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--refresh", action="store_true", help="download missing sources into build/relief-maps/")
    args = parser.parse_args()
    if args.refresh:
        fetch(MOON_URL, CACHE / "ldem_16.tif")
        fetch(MARS_URL, CACHE / "megt90n000eb.img")
        mercury_rows()
    for name, (radius, left_east, size) in BODIES.items():
        heights = height_field(name).resize(size, Image.Resampling.BOX)
        heights = recentre(heights, left_east)
        image = normal_map(heights, radius)
        target = OUTPUT / f"{name}_relief.jpg"
        image.save(target, "JPEG", quality=90, subsampling=0, optimize=True)
        print(f"{target.relative_to(ROOT)}: {image.width} x {image.height}, {target.stat().st_size // 1024} KB")
    return 0


if __name__ == "__main__":
    sys.exit(main())
