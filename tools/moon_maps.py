"""Build the moon and small-world surface maps in assets/textures/ (SPEC A109).

Sources are USGS Astrogeology global mosaics (Voyager, Galileo, Cassini,
New Horizons, Mars Express and Dawn data), public-domain U.S. Government
works served from the PDS Imaging Node's open bucket. Each map is reduced to
1024 x 512, rotated so longitude 0 sits at the image centre with east to the
right (the layout render_build_sphere expects), and saved as a baseline JPEG.

Panchromatic mosaics carry albedo but no colour. They are tinted with an
approximate true-colour hue (Triton too: its Voyager colour
composite has a green filter cast) so that the map's mean equals that hue; the
features are measured, the hue is a presentation choice (assets/textures/
README.md lists both). Areas no spacecraft imaged (Pluto's and Charon's far
south, Vesta's north, Triton's north) are filled with the mean surface
colour rather than black.

Usage:
  python3 tools/moon_maps.py            # convert from the cache in build/moon-maps/
  python3 tools/moon_maps.py --refresh  # download missing sources first (~2.2 GB)
"""
import argparse
import sys
import urllib.request
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
CACHE = ROOT / "build" / "moon-maps"
OUTPUT = ROOT / "assets" / "textures"
BUCKET = "https://asc-pds-services.s3.us-west-2.amazonaws.com/mosaic/"
SIZE = (1024, 512)

# name, USGS file stem, left-edge east longitude (from the ISIS label:
# PositiveEast maps start at MinimumLongitude, PositiveWest maps at
# -MaximumLongitude; both keep east to the right), tint (None = colour map).
MAPS = [
    ("io", "Io_GalileoSSI-Voyager_Global_Mosaic_ClrMerge_1km", -180, None),
    ("europa", "Europa_Voyager_GalileoSSI_global_mosaic_500m", 0, (214, 206, 186)),
    ("ganymede", "Ganymede_Voyager_GalileoSSI_Global_ClrMosaic_1435m", 0, None),
    ("callisto", "Callisto_Voyager_GalileoSSI_global_mosaic_1km", 0, (150, 138, 122)),
    ("enceladus", "Enceladus_Cassini_mosaic_global_110m", -180, (238, 241, 244)),
    ("tethys", "Tethys_Cassini_mosaic_global_293m", -180, (226, 224, 220)),
    ("dione", "Dione_Cassini_Voyager_mosaic_global_154m", -180, (214, 212, 208)),
    ("rhea", "Rhea_Cassini_Voyager_mosaic_global_417m", -180, (206, 202, 196)),
    ("iapetus", "Iapetus_Cassini_Voyager_mosaic_global_783m", -180, (196, 182, 160)),
    ("triton", "Triton_Voyager2_ClrMosaic_GlobalFill_600m", -180, (220, 202, 192)),
    ("pluto", "Pluto_NewHorizons_Global_Mosaic_300m_Jul2017_8bit", 0, (222, 188, 150)),
    ("charon", "Charon_NewHorizons_Global_Mosaic_300m_Jul2017_8bit", -180, (176, 170, 164)),
    ("phobos", "Phobos_ME_SRC_Mosaic_Global_16ppd", -180, (132, 118, 104)),
    ("vesta", "Vesta_Dawn_FC_HAMO_Mosaic_Global_74ppd", -180, (176, 168, 158)),
]


def source_path(name: str) -> Path:
    return CACHE / f"{name}.tif"


def download(name: str, stem: str) -> None:
    path = source_path(name)
    if path.exists() and path.stat().st_size > 0:
        return
    CACHE.mkdir(parents=True, exist_ok=True)
    partial = path.with_suffix(".part")
    with urllib.request.urlopen(BUCKET + stem + ".tif", timeout=120) as response, open(partial, "wb") as out:
        while chunk := response.read(1 << 20):
            out.write(chunk)
    partial.replace(path)


def recentre(image: Image.Image, left_east: float) -> Image.Image:
    """Roll columns so the left edge is longitude -180 (centre 0)."""
    shift = round((left_east + 180) / 360 * image.width) % image.width
    if not shift:
        return image
    rolled = Image.new(image.mode, image.size)
    rolled.paste(image.crop((0, 0, image.width - shift, image.height)), (shift, 0))
    rolled.paste(image.crop((image.width - shift, 0, image.width, image.height)), (0, 0))
    return rolled


def pixels_of(image: Image.Image) -> list:
    """Flat pixel list (get_flattened_data from Pillow 12, getdata before)."""
    return list(getattr(image, "get_flattened_data", image.getdata)())


def filled(image: Image.Image, tint) -> Image.Image:
    """Reduce to SIZE, filling no-data (black) with the mean of the imaged
    surface; with a tint, scale a panchromatic map so its mean is the tint."""
    # Normalized convolution: area-average the data and its validity mask
    # together and divide, so the edges of unimaged areas are not darkened by
    # averaging in zeros.
    mask = image.convert("L").point(lambda v: 255 if v > 0 else 0)
    cover = pixels_of(mask.resize(SIZE, Image.Resampling.BOX))
    data = pixels_of(image.resize(SIZE, Image.Resampling.BOX))
    if image.mode == "L":
        data = [(d,) for d in data]
    values = [tuple(x / c * 255 for x in d) if c > 32 else None for d, c in zip(data, cover)]
    known = [v for v in values if v is not None]
    mean = tuple(sum(v[k] for v in known) / len(known) for k in range(len(known[0])))
    pixels = []
    for v in values:
        v = mean if v is None else v
        if tint is None:
            pixels.append(tuple(min(255, round(x)) for x in v))
        else:
            pixels.append(tuple(min(255, round(channel * v[0] / mean[0])) for channel in tint))
    out = Image.new("RGB", SIZE)
    out.putdata(pixels)
    return out


def convert(name: str, left_east: float, tint) -> Path:
    Image.MAX_IMAGE_PIXELS = None  # the mosaics are up to 357 megapixels
    with Image.open(source_path(name)) as image:
        image = recentre(image, left_east)
        result = filled(image.convert("RGB" if tint is None else "L"), tint)
    target = OUTPUT / f"{name}.jpg"
    result.save(target, "JPEG", quality=84, optimize=True)
    return target


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--refresh", action="store_true", help="download missing sources into build/moon-maps/")
    args = parser.parse_args()
    for name, stem, left_east, tint in MAPS:
        if args.refresh:
            download(name, stem)
        if not source_path(name).exists():
            print(f"missing {source_path(name)}; run with --refresh", file=sys.stderr)
            return 1
        target = convert(name, left_east, tint)
        print(f"{target.relative_to(ROOT)} {target.stat().st_size // 1024} KB")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
