#!/usr/bin/env python3
"""Refresh a reviewed JPL snapshot explicitly; normal builds use offline data.

--refresh fetches the two public source tables and writes the normalized JSON.
--check verifies the committed C include against that snapshot without network.
No masses or radii are inferred from assumed densities or albedos.
"""
import argparse
from datetime import date
from html.parser import HTMLParser
import json
from pathlib import Path
import re
from urllib.request import urlopen

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "data/jovian_moons.json"
GENERATED = ROOT / "src/sim/jovian_moons.inc"
ELEMENTS = "https://ssd.jpl.nasa.gov/sats/elem/"
PHYSICAL = "https://ssd.jpl.nasa.gov/sats/phys_par/"


class TableRows(HTMLParser):
    """JPL omits some </td> tags; a new cell must flush its predecessor."""
    def __init__(self):
        super().__init__()
        self.rows = []
        self.row = []
        self.cell = None

    def flush(self):
        if self.cell is not None:
            self.row.append(" ".join("".join(self.cell).split()))
            self.cell = None

    def handle_starttag(self, tag, attrs):
        if tag == "tr":
            self.flush()
            self.row = []
        elif tag in ("td", "th"):
            self.flush()
            self.cell = []

    def handle_data(self, text):
        if self.cell is not None:
            self.cell.append(text)

    def handle_endtag(self, tag):
        if tag in ("td", "th", "tr"):
            self.flush()
        if tag == "tr":
            self.rows.append(self.row)
            self.row = []


def fetch_rows(url):
    parser = TableRows()
    with urlopen(url, timeout=60) as response:
        parser.feed(response.read().decode("utf-8"))
    return parser.rows


def refresh():
    physical = {int(r[2]): r for r in fetch_rows(PHYSICAL) if len(r) == 12 and r[0] == "Jupiter"}
    moons = []
    for r in fetch_rows(ELEMENTS):
        if len(r) < 19 or r[1] != "Jupiter":
            continue
        code = int(r[3])
        name = re.sub(r"^S(\d{4})_J_(\d+)$", r"S/\1 J \2", r[2])
        # JPL's discovery table has the full IAU spellings; the elements table
        # misspells Megaclite and truncates Philophrosyne. Identity stays by code.
        name = {519: "Megaclite", 558: "Philophrosyne"}.get(code, name)
        p = physical.get(code)
        # The small inner moons' dynamical masses are model estimates. Himalia's
        # ground-based size is likewise an estimate; do not imply spacecraft data.
        mass_quality = "measured" if code in (501, 502, 503, 504, 505) else "estimated" if p else "unknown"
        radius_quality = "estimated" if code == 506 else "measured" if p else "unknown"
        group = "Galilean moons" if 501 <= code <= 504 else "Inner small moons" if code in (505, 514, 515, 516) else "Irregular moons"
        moons.append({
            "code": code, "name": name, "slug": re.sub(r"[^a-z0-9]+", "-", name.lower()).strip("-"),
            "group": group, "ephemeris": r[4], "frame": r[5], "epoch_tdb": r[6],
            "a_km": float(r[7]), "eccentricity": float(r[8]),
            "periapsis_deg": float(r[9]), "mean_anomaly_deg": float(r[10]),
            "inclination_deg": float(r[11]), "node_deg": float(r[12]),
            "period_days": float(r[13]),
            "pole_ra_deg": float(r[16]) if r[5] == "Laplace" else None,
            "pole_dec_deg": float(r[17]) if r[5] == "Laplace" else None,
            "element_reference": r[19] if len(r) > 19 else r[-1],
            "gm_km3_s2": float(p[3]) if p else None, "radius_km": float(p[6]) if p else None,
            "gm_sigma_km3_s2": float(p[4]) if p else None, "radius_sigma_km": float(p[7]) if p else None,
            "mass_quality": mass_quality, "radius_quality": radius_quality,
            "gm_reference": p[5] if p else None, "radius_reference": p[8] if p else None,
        })
    snapshot = {"schema": 1, "checked": date.today().isoformat(),
        "inventory_source": "https://science.nasa.gov/jupiter/moons/",
        "names_source": "https://ssd.jpl.nasa.gov/sats/discovery.html",
        "elements_source": ELEMENTS, "physical_source": PHYSICAL, "moons": moons}
    validate(snapshot)
    DATA.write_text(json.dumps(snapshot, indent=2) + "\n")


def validate(snapshot):
    moons = snapshot["moons"]
    assert snapshot["schema"] == 1 and len(moons) == 115, "review inventory changes explicitly"
    assert len({m["code"] for m in moons}) == len({m["slug"] for m in moons}) == 115
    assert [m["code"] for m in moons[:4]] == [501, 502, 503, 504]
    for m in moons:
        assert m["frame"] in ("Laplace", "ecliptic") and m["epoch_tdb"] == "2000-01-01.5"
        assert m["a_km"] > 0 and 0 <= m["eccentricity"] < 0.5 and 0 <= m["inclination_deg"] <= 180
        if m["frame"] == "Laplace":
            assert 0 <= m["pole_ra_deg"] < 360 and -90 <= m["pole_dec_deg"] <= 90
        else:
            assert m["pole_ra_deg"] is None and m["pole_dec_deg"] is None
        for value, quality in (("gm_km3_s2", "mass_quality"), ("radius_km", "radius_quality")):
            assert m[quality] in ("measured", "estimated", "unknown")
            assert (m[value] is None) == (m[quality] == "unknown")
            if m[value] is not None:
                assert m[value] > 0


def generate(snapshot):
    lines = ["/* Generated by tools/jovian_catalog.py from data/jovian_moons.json. */"]
    for m in snapshot["moons"]:
        fields = [str(m["code"]), json.dumps(m["name"]), json.dumps(m["group"])]
        fields += [str(m[key] or 0) for key in ("a_km", "eccentricity", "periapsis_deg", "mean_anomaly_deg", "inclination_deg", "node_deg", "pole_ra_deg", "pole_dec_deg", "gm_km3_s2", "radius_km")]
        fields += ["SATELLITE_FRAME_" + m["frame"].upper(), "PHYSICAL_" + m["mass_quality"].upper(), "PHYSICAL_" + m["radius_quality"].upper()]
        lines.append("    {" + ", ".join(fields) + "},")
    return "\n".join(lines) + "\n"


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--refresh", action="store_true")
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    if args.refresh:
        refresh()
    snapshot = json.loads(DATA.read_text())
    validate(snapshot)
    output = generate(snapshot)
    if args.check:
        if GENERATED.read_text() != output:
            raise SystemExit("stale Jovian C data; run python3 tools/jovian_catalog.py")
    else:
        GENERATED.write_text(output)
    print(f"Jovian catalog OK: {len(snapshot['moons'])} moons, snapshot {snapshot['checked']}")
