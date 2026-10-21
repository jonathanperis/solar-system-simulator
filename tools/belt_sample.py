"""Build the small-body cloud the 3D view draws (SPEC A121).

The pinned atlas (docs/public/catalog/, 1,564,244 heliocentric orbits at JD
2461200.5, the scene epoch) is far too large to draw. This takes a
deterministic sample per orbit class, evenly spaced through each class in
catalog order, sized so the structure reads: the main belt with its Kirkwood
gaps, both Jupiter Trojan swarms, the Centaurs and trans-Neptunian objects,
and the near-Earth groups. Orbits with e >= 0.97 (including hyperbolic ones)
or a > 1,000 AU are skipped.

Output: assets/belt/small_bodies.bin, little-endian
  header (16 bytes): b"SOLBELT1", uint32 count, uint32 reserved (0)
  record (16 bytes): float32 a [AU]; uint16 e, i, node, periapsis, M0;
                     uint8 class code; uint8 reserved (0)
e is a fraction of 1, i a fraction of pi, the other angles fractions of 2 pi,
each scaled to 0..65535 (resolution 1.5e-5 and 0.0055 deg); M0 is the mean
anomaly at the epoch, n (epoch - tp) with n from the simulator's solar GM.
Class codes follow CLASSES below; src/render/scene_style.c decodes them.

Usage: python3 tools/belt_sample.py [--check]
"""
import argparse
import gzip
import json
import math
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
CATALOG = ROOT / "docs" / "public" / "catalog"
OUTPUT = ROOT / "assets" / "belt" / "small_bodies.bin"
# The simulator's solar GM (src/sim/constants.h, SOLAR_SUN_GM_M3PS2) and day,
# so mean motions match catalog_coordinate exactly.
SUN_GM = 1.32712440041279419e20  # m^3/s^2
AU = 149597870700.0
DAY = 86400.0
EPOCH = 2461200.5

# class, code, how many to keep (0 = all). Codes: 0 main belt, 1 Trojans,
# 2 near-Earth, 3 Centaurs and trans-Neptunian, 4 Mars-crossers.
CLASSES = [
    ("MBA", 0, 20000), ("IMB", 0, 1500), ("OMB", 0, 3000), ("MCA", 4, 1500),
    ("TJN", 1, 9000), ("CEN", 3, 0), ("TNO", 3, 0),
    ("APO", 2, 2500), ("AMO", 2, 1500), ("ATE", 2, 600), ("IEO", 2, 0),
    ("AST", 0, 0), ("HYA", 3, 0),
]
MAGIC = b"SOLBELT1"


def rows_of(cls):
    for path in sorted(CATALOG.glob(f"{cls}-*.json.gz")):
        if path.name.endswith("-index.json.gz"):
            continue
        with gzip.open(path, "rt") as stream:
            yield from json.load(stream)


def record(row, code):
    _, _, _, _, epoch, q, e, inc, node, peri, tp = row[:11]
    # Each conic is placed from its perihelion time tp, as the atlas does
    # (catalog_coordinate), whatever epoch its elements osculate at.
    # The shader's six Newton steps for Kepler's equation need e < 0.97; the
    # few orbits beyond that, or beyond 1,000 AU, never reach the screen.
    if not (0 <= e < 0.97) or not (q > 0) or q / (1 - e) > 1000:
        return None
    a = q / (1 - e)
    n = math.sqrt(SUN_GM / (a * AU) ** 3) * DAY  # rad/day
    m0 = (n * (EPOCH - tp)) % (2 * math.pi)
    turn = lambda value, period: min(65535, max(0, round(value % period / period * 65535)))
    return struct.pack("<fHHHHHBB", a, min(65535, round(e * 65535)), min(65535, round(math.radians(inc) / math.pi * 65535)),
                       turn(math.radians(node), 2 * math.pi), turn(math.radians(peri), 2 * math.pi),
                       turn(m0, 2 * math.pi), code, 0)


def build():
    records = []
    for cls, code, keep in CLASSES:
        usable = [r for r in (record(row, code) for row in rows_of(cls)) if r]
        stride = max(1, len(usable) // keep) if keep else 1
        chosen = usable[::stride][: keep or None]
        records.extend(chosen)
        print(f"{cls}: {len(chosen)} of {len(usable)}")
    return MAGIC + struct.pack("<II", len(records), 0) + b"".join(records)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--check", action="store_true", help="verify the committed file matches the catalog")
    args = parser.parse_args()
    data = build()
    if args.check:
        if not OUTPUT.exists() or OUTPUT.read_bytes() != data:
            print(f"{OUTPUT.relative_to(ROOT)} is stale; run tools/belt_sample.py", file=sys.stderr)
            return 1
        print(f"{OUTPUT.relative_to(ROOT)} matches the catalog")
        return 0
    OUTPUT.parent.mkdir(parents=True, exist_ok=True)
    OUTPUT.write_bytes(data)
    print(f"{OUTPUT.relative_to(ROOT)}: {(len(data) - 16) // 16} bodies, {len(data) // 1024} KB")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
