#!/usr/bin/env python3
"""Pin the real sky of the scene epoch; offline generation/check on normal builds.

Every scene starts at JD 2461200.5 TDB (2026-06-09), the epoch of
data/planet_epoch.json (SPEC A92). Planets come from that file. This tool pins
the rest: every catalogued moon relative to its primary's centre, Earth's Moon,
Phobos and Deimos, and the heliocentric states of Vesta, the Pluto system
barycenter and the Didymos and Patroclus systems.

  --refresh   fetch Horizons vectors (one serialized request per body) and
              rewrite data/scene_epoch.json
  --check     verify src/sim/scene_epoch.inc against the snapshot, offline

Mean elements cannot replace this: propagated from their epochs to 2026 they
miss Horizons by up to 178 degrees (resonances, forced eccentricities). A body
Horizons does not serve under its own name stays undated and keeps its
mean-element state; the snapshot lists it explicitly.
"""
import argparse
from datetime import datetime, timezone
import json
import math
from pathlib import Path
import re
import time
from urllib.parse import urlencode
from urllib.request import urlopen

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "data/scene_epoch.json"
GENERATED = ROOT / "src/sim/scene_epoch.inc"
EPOCH = 2461200.5
HORIZONS = "https://ssd.jpl.nasa.gov/api/horizons.api"
# Moon catalogs and the Horizons centre each moon is measured from.
CATALOGS = [("jovian", 599), ("saturnian", 699), ("uranian", 799), ("neptunian", 899),
            ("plutonian", 999), ("didymos", 920065803), ("patroclus", 920000617)]
# Bodies outside the moon catalogs: (code used by the C scene, Horizons
# command, centre, expected name). Vesta and the Pluto, Didymos and Patroclus
# system barycenters are heliocentric; the C scene assigns those states to the
# family barycenters (V6).
# Earth's Moon, Phobos and Deimos keep their legacy scene IDs (4, 6, 7). Their
# centre must be "@399"/"@499": a bare "499" is not read as Mars's centre and
# returns a state ~3e8 km away (caught in review of PR #40).
EXTRA = [(4, "301", "@399", "Moon"), (6, "401", "@499", "Phobos"), (7, "402", "@499", "Deimos"),
         (8, "4;", "500@10", "Vesta"), (999, "9", "500@10", "Pluto Barycenter"),
         (920065803, "65803;", "500@10", "Didymos"), (920000617, "617;", "500@10", "Patroclus")]


def normalized(name):
    return re.sub(r"[^a-z0-9]", "", name.lower())


def tokens(name):
    """Letter and number runs, numbers without zero padding: Horizons writes
    S/2023 S 1 as "S2023_S01" and S/2023 U 1 as "2023U1"."""
    return [str(int(t)) if t.isdigit() else t for t in re.findall(r"[a-z]+|\d+", name.lower())]


def fetch(command, center):
    params = {"format": "json", "COMMAND": f"'{command}'", "CENTER": f"'{center}'", "EPHEM_TYPE": "VECTORS",
              "TLIST": f"'{EPOCH}'", "OUT_UNITS": "KM-S", "REF_PLANE": "ECLIPTIC", "REF_SYSTEM": "ICRF",
              "VEC_TABLE": "2", "VEC_CORR": "NONE", "CSV_FORMAT": "YES", "OBJ_DATA": "NO", "MAKE_EPHEM": "YES"}
    url = f"{HORIZONS}?{urlencode(params)}"
    for attempt in range(3):
        try:
            with urlopen(url, timeout=120) as response:
                result = json.load(response)["result"]
            break
        except OSError:
            if attempt == 2:
                raise
            time.sleep(5)
    if "$$SOE" not in result:
        return None, None
    target = result.split("Target body name:")[1].split("{")[0].strip()
    row = [v.strip() for v in result.split("$$SOE")[1].split("$$EOE")[0].strip().splitlines()[0].split(",")]
    if float(row[0]) != EPOCH or "Ecliptic of J2000.0" not in result:
        raise ValueError(f"{command}: Horizons epoch/frame contract changed")
    return target, [float(v) for v in row[2:8]]


def identity_matches(target, name, code):
    """Horizons names a satellite "<name> (<code>)". A code that resolves to an
    asteroid instead ("75052 (1999 UM50)") must not be pinned as the moon."""
    label = target.split("(")[0].strip()
    inside = target[target.find("(") + 1:target.find(")")] if "(" in target else ""
    # Numbered asteroids read "4 Vesta (A807 FA)": drop the catalog number.
    unnumbered = re.sub(r"^\d+\s+", "", label)
    if normalized(name) in (normalized(label), normalized(unnumbered)) or tokens(label) == tokens(name):
        return True
    # A provisional satellite may drop its "S/" prefix, but then the
    # parenthesized code must be this moon's own code.
    return inside == str(code) and tokens(label) in (tokens(name)[1:], [str(code)])


# Mean orbit (a km, e) of the non-catalog moons, for the distance check below.
EXTRA_ORBITS = {4: (384400.0, 0.0549), 6: (9377.0, 0.0151), 7: (23460.0, 0.00033)}


def catalog_orbits():
    orbits = dict(EXTRA_ORBITS)
    for catalog, _ in CATALOGS:
        for moon in json.loads((ROOT / f"data/{catalog}_moons.json").read_text())["moons"]:
            orbits[moon["code"]] = (moon["a_km"], moon["eccentricity"])
    return orbits


def refresh(only=None):
    """Fetch every body, or with `only` re-fetch just those codes and keep
    the rest of the pinned snapshot."""
    bodies, undated = [], []
    if only:
        previous = json.loads(DATA.read_text())
        bodies = [b for b in previous["bodies"] if b["code"] not in only]
        undated = [u for u in previous["undated"] if u["code"] not in only]
    work = [(code, cmd, center, name) for code, cmd, center, name in EXTRA]
    for catalog, center in CATALOGS:
        for moon in json.loads((ROOT / f"data/{catalog}_moons.json").read_text())["moons"]:
            work.append((moon["code"], str(moon["code"]), f"@{center}", moon["name"]))
    if only:
        work = [item for item in work if item[0] in only]
    for code, command, center, name in work:
        target, state = fetch(command, center)
        if state is None or not identity_matches(target, name, code):
            undated.append({"code": code, "name": name, "horizons_target": target})
            print(f"undated: {name} ({code}) -> {target}", flush=True)
            continue
        bodies.append({"code": code, "name": name, "horizons_target": target, "center": center,
                       "position_km": state[:3], "velocity_km_s": state[3:]})
        time.sleep(0.2)
    # Keep the full-refresh order (EXTRA, then each catalog) after a partial one.
    full = [code for code, *_ in EXTRA] + [code for code in catalog_orbits() if code not in EXTRA_ORBITS]
    order = {code: index for index, code in enumerate(full)}
    bodies.sort(key=lambda b: order[b["code"]])
    snapshot = {"epoch": EPOCH, "frame": "J2000 ecliptic", "source": HORIZONS,
                "checked": datetime.now(timezone.utc).isoformat(), "bodies": bodies, "undated": undated}
    validate(snapshot)
    DATA.write_text(json.dumps(snapshot, indent=2, ensure_ascii=False) + "\n")


def require(condition, message):
    if not condition:
        raise ValueError(message)


def validate(snapshot):
    require(snapshot["epoch"] == EPOCH and snapshot["frame"] == "J2000 ecliptic", "epoch or frame changed")
    codes = [b["code"] for b in snapshot["bodies"]] + [u["code"] for u in snapshot["undated"]]
    require(len(codes) == len(set(codes)), "duplicate body in the scene snapshot")
    expected = {code for code, *_ in EXTRA}
    for catalog, _ in CATALOGS:
        expected |= {m["code"] for m in json.loads((ROOT / f"data/{catalog}_moons.json").read_text())["moons"]}
    require(set(codes) == expected, "the scene snapshot must cover every catalogued body exactly once")
    # Undated bodies are reviewed exceptions, not a silent fallback.
    require(len(snapshot["undated"]) <= 5, "too many undated bodies; review the Horizons refresh")
    for b in snapshot["bodies"]:
        require(identity_matches(b["horizons_target"], b["name"], b["code"]), f"{b['name']}: Horizons identity mismatch")
        require(len(b["position_km"]) == 3 and len(b["velocity_km_s"]) == 3, f"{b['name']}: malformed state")
    # A moon's state is relative to its primary, so its distance must lie near
    # its own orbit. The bounds are loose (osculating states of irregular moons
    # stray from mean elements: 0.76 to 1.18 times periapsis/apoapsis at this
    # epoch) but reject a wrong centre, which is off by orders of magnitude.
    orbits = catalog_orbits()
    for b in snapshot["bodies"]:
        if b["code"] not in orbits:
            continue
        a_km, e = orbits[b["code"]]
        r_km = math.hypot(*b["position_km"])
        require(0.5 * a_km * (1 - e) <= r_km <= 1.5 * a_km * (1 + e),
                f"{b['name']}: {r_km:.0f} km from its primary is far from its orbit; check the Horizons centre")


def generate(snapshot):
    lines = ["/* Generated by tools/scene_epoch.py from data/scene_epoch.json: Horizons",
             " * states at JD 2461200.5 TDB in simulation axes (x, y, z) = J2000 ecliptic",
             " * (X, Z, -Y), SI units. Moons are relative to their primary's centre; Vesta",
             " * and the Pluto, Didymos and Patroclus system barycenters are heliocentric. */"]
    for b in snapshot["bodies"]:
        p, v = b["position_km"], b["velocity_km_s"]
        # Proper rotation into simulation axes; see orbit_ecliptic_to_simulation().
        values = [x * 1000 for x in (p[0], p[2], -p[1], v[0], v[2], -v[1])]
        lines.append(f"    {{{b['code']}, {{" + ", ".join(format(x, ".17g") for x in values) + "}},")
    return "\n".join(lines) + "\n"


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--refresh", action="store_true")
    parser.add_argument("--only", type=int, nargs="+", metavar="CODE", help="with --refresh, re-fetch only these scene codes")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    if args.refresh:
        refresh(set(args.only) if args.only else None)
    snapshot = json.loads(DATA.read_text())
    validate(snapshot)
    output = generate(snapshot)
    if args.check:
        if not GENERATED.exists() or GENERATED.read_text() != output:
            raise SystemExit("stale scene epoch C data; run python3 tools/scene_epoch.py")
    else:
        GENERATED.write_text(output)
    print(f"Scene epoch OK: {len(snapshot['bodies'])} dated bodies, {len(snapshot['undated'])} undated, JD {EPOCH}")
