#!/usr/bin/env python3
"""Refresh reviewed JPL satellite snapshots explicitly; normal builds use offline data.

One tool serves every giant planet (SPEC A78):

  --refresh            fetch the two public JPL tables and rewrite the JSON
                       snapshot of each selected system
  --check              verify the committed C includes against the snapshots
                       without network access
  --system NAME        jupiter, saturn, uranus, neptune, pluto, didymos or
                       patroclus (repeatable; default: all)

No masses or radii are inferred from assumed densities or albedos. Each moon
keeps the epoch and reference frame of its own ephemeris solution; phases are
mutually consistent only inside one solution (A79).
"""
import argparse
from datetime import date
from html.parser import HTMLParser
import json
import math
from pathlib import Path
import re
from urllib.request import urlopen

ROOT = Path(__file__).resolve().parents[1]
ELEMENTS = "https://ssd.jpl.nasa.gov/sats/elem/"
PHYSICAL = "https://ssd.jpl.nasa.gov/sats/phys_par/"
HORIZONS = "https://ssd.jpl.nasa.gov/api/horizons.api"

# IAU WGCCRE 2015 north pole of Uranus (ICRF, J2000). Uranus spins retrograde
# about it, so its regular moons orbit about the antipode; JPL's URA182
# "equatorial" rows carry no pole columns and are measured from that spin
# pole (their Ariel has i = 0). src/sim/constants.h holds the same constant
# and tests/test_satellites.c checks the two agree.
URANUS_IAU_POLE_RA_DEG = 257.311
URANUS_IAU_POLE_DEC_DEG = -15.175
URANUS_SPIN_POLE = (round((URANUS_IAU_POLE_RA_DEG + 180.0) % 360.0, 6), -URANUS_IAU_POLE_DEC_DEG)
# IAU WGCCRE 2015 pole of Pluto. For dwarf planets the report uses the
# right-hand (positive) pole, which is the angular-momentum pole of Pluto's
# spin and Charon's orbit, so JPL's PLU060 "equatorial" rows (Charon i = 0)
# use it directly; no antipode as for Uranus.
PLUTO_IAU_POLE = (132.993, -6.163)


def jovian_group(code, frame):
    if 501 <= code <= 504:
        return "Galilean moons"
    return "Inner small moons" if code in (505, 514, 515, 516) else "Irregular moons"


def regular_group(major_label, small_label):
    """Groups for systems whose regular moons share a Laplace/equatorial frame."""
    def group(code, frame, major):
        if major:
            return major_label
        return small_label if frame in ("Laplace", "equatorial") else "Irregular moons"
    return group


# Per-system policy. "major" lists the moons with a measured GM of at least
# MAJOR_GM_KM3_S2: the Galilean moons, Saturn's seven rounded moons (Mimas,
# the smallest, has 2.5 km^3/s^2; Hyperion 0.37 and Phoebe 0.55 fall below),
# Uranus's five and Triton (Proteus's 2.6 is an estimate with a larger
# uncertainty). Major moons belong to the main scene; every other moon only to
# its family scene. validate() re-derives the set from the data.
MAJOR_GM_KM3_S2 = 2.0
SYSTEMS = {
    "jupiter": {
        "planet": "Jupiter", "adjective": "Jovian", "count": 115,
        "epochs": {"2000-01-01.5"}, "frames": {"Laplace", "ecliptic"}, "max_e": 0.5,
        "major": {501, 502, 503, 504},
        "inventory_source": "https://science.nasa.gov/jupiter/moons/",
    },
    "saturn": {
        "planet": "Saturn", "adjective": "Saturnian", "count": 291,
        "epochs": {"2000-01-01.5"}, "frames": {"Laplace", "ecliptic"}, "max_e": 0.95,
        "major": {601, 602, 603, 604, 605, 606, 608},
        "inventory_source": "https://science.nasa.gov/saturn/moons/",
    },
    "uranus": {
        "planet": "Uranus", "adjective": "Uranian", "count": 29,
        "epochs": {"2000-01-01.5", "2020-01-01.0", "2025-01-01.0"},
        "frames": {"equatorial", "Laplace", "ecliptic"}, "max_e": 0.7,
        "major": {701, 702, 703, 704, 705},
        # Puck appears in the major-moon solution (URA182) and the inner-moon
        # solution (URA184). Keep the newer inner-moon solution, whose phases
        # are consistent with Puck's neighbours.
        "prefer": {715: "URA184"},
        "inventory_source": "https://science.nasa.gov/uranus/moons/",
    },
    "neptune": {
        "planet": "Neptune", "adjective": "Neptunian", "count": 16,
        "epochs": {"2000-01-01.5", "2020-01-01.0"}, "frames": {"Laplace", "ecliptic"}, "max_e": 0.8,
        "major": {801},
        "inventory_source": "https://science.nasa.gov/neptune/moons/",
    },
    # Small-body satellite systems (SPEC T78). Pluto's moons come from the same
    # JPL tables; Didymos's companion from the Horizons DART reconstruction and
    # Patroclus's from the Horizons asteroid-satellite solution JPL#82.
    "pluto": {
        "planet": "Pluto", "adjective": "Plutonian", "count": 5,
        "epochs": {"2000-01-01.5"}, "frames": {"equatorial"}, "max_e": 0.1,
        "major": {901}, "center": 999, "equatorial_pole": PLUTO_IAU_POLE,
        "inventory_source": "https://science.nasa.gov/dwarf-planets/pluto/moons/",
    },
    "didymos": {
        "planet": "Didymos", "adjective": "Didymos", "count": 1, "source": "horizons",
        "epochs": {"2024-01-01.0"}, "frames": {"ecliptic"}, "max_e": 0.1,
        "major": set(), "center": 920065803, "solution": "JPL s547",
        "inventory_source": "https://ssd.jpl.nasa.gov/api/horizons.api?format=text&COMMAND='120065803'&OBJ_DATA='YES'&MAKE_EPHEM='NO'",
    },
    "patroclus": {
        "planet": "Patroclus", "adjective": "Patroclus", "count": 1, "source": "horizons",
        "epochs": {"2024-01-01.0"}, "frames": {"ecliptic"}, "max_e": 0.1,
        "major": set(), "center": 920000617, "solution": "JPL#82",
        "inventory_source": "https://ssd.jpl.nasa.gov/api/horizons.api?format=text&COMMAND='120000617'&OBJ_DATA='YES'&MAKE_EPHEM='NO'",
    },
}
for _name, _center in (("jupiter", 599), ("saturn", 699), ("uranus", 799), ("neptune", 899)):
    SYSTEMS[_name]["center"] = _center
SYSTEMS["uranus"]["equatorial_pole"] = URANUS_SPIN_POLE
GROUPS = {
    "saturn": regular_group("Major moons", "Small regular moons"),
    "uranus": regular_group("Major moons", "Inner moons"),
    "neptune": regular_group("Major moon", "Inner moons"),
    "pluto": lambda code, frame, major: "Major moon" if major else "Small moons",
    "didymos": lambda code, frame, major: "Didymos system",
    "patroclus": lambda code, frame, major: "Patroclus system",
}


def paths(system):
    adjective = SYSTEMS[system]["adjective"].lower()
    return ROOT / f"data/{adjective}_moons.json", ROOT / f"src/sim/{adjective}_moons.inc"


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


def iau_name(raw):
    """JPL writes provisional designations as S2003_J_2, S2023_U1 or S2002_N5."""
    return re.sub(r"^S(\d{4})_([A-Z])_?(\d+)$", r"S/\1 \2 \3", raw)


def qualities(system, code, physical):
    if system == "jupiter":
        # The small inner moons' dynamical masses are model estimates. Himalia's
        # ground-based size is likewise an estimate; do not imply spacecraft data.
        mass = "measured" if code in (501, 502, 503, 504, 505) else "estimated" if physical else "unknown"
        radius = "estimated" if code == 506 else "measured" if physical else "unknown"
        return mass, radius
    if not physical:
        return "unknown", "unknown"
    if physical[3].startswith("<"):
        # Kerberos and Styx: only an upper limit, so the mass is unknown.
        return "unknown", "measured"
    gm, sigma = float(physical[3]), float(physical[4])
    # A zero GM (Nereid) means "not determined", never a massless body we know
    # of. A 1-sigma uncertainty above 10 % marks a model estimate.
    mass = "unknown" if gm <= 0 else "measured" if sigma <= 0.1 * gm else "estimated"
    return mass, "measured"


def moon_record(system, r, physical):
    config = SYSTEMS[system]
    code = int(r[3])
    name = iau_name(r[2])
    if system == "jupiter":
        # JPL's discovery table has the full IAU spellings; the elements table
        # misspells Megaclite and truncates Philophrosyne. Identity stays by code.
        name = {519: "Megaclite", 558: "Philophrosyne"}.get(code, name)
    p = physical.get(code)
    mass_quality, radius_quality = qualities(system, code, p)
    major = code in config["major"]
    group = jovian_group(code, r[5]) if system == "jupiter" else GROUPS[system](code, r[5], major)
    if r[5] == "Laplace":
        pole = (float(r[16]), float(r[17]))
    elif r[5] == "equatorial":
        pole = config["equatorial_pole"]
    else:
        pole = (None, None)
    gm = float(p[3]) if p and mass_quality != "unknown" else None
    record = {
        "code": code, "name": name, "slug": re.sub(r"[^a-z0-9]+", "-", name.lower()).strip("-"),
        "group": group, "ephemeris": r[4], "frame": r[5], "epoch_tdb": r[6],
        "a_km": float(r[7]), "eccentricity": float(r[8]),
        "periapsis_deg": float(r[9]), "mean_anomaly_deg": float(r[10]),
        "inclination_deg": float(r[11]), "node_deg": float(r[12]),
        "period_days": float(r[13]),
        "pole_ra_deg": pole[0], "pole_dec_deg": pole[1],
        "element_reference": r[19] if len(r) > 19 else r[-1],
        "gm_km3_s2": gm if mass_quality != "unknown" else None, "radius_km": float(p[6]) if p else None,
        "gm_sigma_km3_s2": float(p[4]) if p and mass_quality != "unknown" else None,
        "radius_sigma_km": float(p[7]) if p else None,
        "mass_quality": mass_quality, "radius_quality": radius_quality,
        "gm_reference": p[5] if p and mass_quality != "unknown" else None, "radius_reference": p[8] if p else None,
    }
    if system != "jupiter" or major:
        record["major"] = major
    return record


def horizons_text(**params):
    query = "&".join(f"{key}='{value}'" if key != "format" else f"{key}={value}" for key, value in params.items())
    with urlopen(f"{HORIZONS}?{query}", timeout=60) as response:
        return response.read().decode("utf-8")


def refresh_didymos():
    """Dimorphos relative to the Didymos primary (Horizons body 920065803) from
    the DART s547 post-impact reconstruction: osculating ecliptic J2000
    elements at 2024-01-01 TDB, and the published approximate GMs."""
    rows = horizons_text(format="text", COMMAND="120065803", CENTER="@920065803", MAKE_EPHEM="YES",
        EPHEM_TYPE="ELEMENTS", START_TIME="2024-01-01", STOP_TIME="2024-01-02", STEP_SIZE="1d",
        OUT_UNITS="KM-S", REF_PLANE="ECLIPTIC", REF_SYSTEM="ICRF", CSV_FORMAT="YES", OBJ_DATA="NO")
    first = rows.split("$$SOE")[1].split("$$EOE")[0].strip().splitlines()[0]
    f = [field.strip() for field in first.split(",")]
    # JDTDB, date, EC, QR, IN, OM, W, Tp, N, MA, TA, A, AD, PR
    info = horizons_text(format="text", COMMAND="120065803", OBJ_DATA="YES", MAKE_EPHEM="NO")
    # The label must name the solution Horizons actually served: a new
    # reconstruction changes the elements and needs review before it is pinned.
    solution = re.search(r"\((JPL s\d+) reconstruction\)", info)
    require(solution is not None, "didymos: Horizons no longer names its Dimorphos solution; review the source")
    require(solution.group(1) == SYSTEMS["didymos"]["solution"],
            f"didymos: Horizons now serves {solution.group(1)}; review it and update SYSTEMS before pinning")
    gm = float(re.search(r"GM_Dimo ~ ([0-9.E+-]+)", info).group(1))
    radii = [float(v) for v in re.search(r"Radii\s+~ \(([0-9.]+) x ([0-9.]+) x ([0-9.]+)\) km", info).groups()]
    # Volume-equivalent radius of the published triaxial shape.
    radius = round((radii[0] * radii[1] * radii[2]) ** (1 / 3), 5)
    moon = {
        "code": 120065803, "name": "Dimorphos", "slug": "dimorphos", "group": "Didymos system",
        "ephemeris": solution.group(1), "frame": "ecliptic", "epoch_tdb": "2024-01-01.0",
        "a_km": round(float(f[11]), 7), "eccentricity": round(float(f[2]), 7),
        "periapsis_deg": round(float(f[6]), 5), "mean_anomaly_deg": round(float(f[9]), 5),
        "inclination_deg": round(float(f[4]), 5), "node_deg": round(float(f[5]), 5),
        "period_days": round(float(f[13]) / 86400, 7),
        "pole_ra_deg": None, "pole_dec_deg": None, "element_reference": "Horizons 120065803 @920065803",
        "gm_km3_s2": gm, "radius_km": radius, "gm_sigma_km3_s2": None, "radius_sigma_km": None,
        "mass_quality": "estimated", "radius_quality": "estimated",
        "gm_reference": "Horizons GM_Dimo (approximate)", "radius_reference": "Horizons triaxial radii, volume-equivalent",
        "major": False,
    }
    snapshot = {"schema": 1, "checked": date.today().isoformat(), "inventory_source": SYSTEMS["didymos"]["inventory_source"],
        "names_source": "https://ssd.jpl.nasa.gov/api/horizons.api", "elements_source": HORIZONS, "physical_source": HORIZONS,
        "moons": [moon]}
    validate("didymos", snapshot)
    paths("didymos")[0].write_text(json.dumps(snapshot, indent=2, ensure_ascii=False) + "\n")


def elements_from_state(position_km, velocity_km_s, mu_km3_s2):
    """Osculating Keplerian elements (a km, e, i, node, argument of periapsis
    and mean anomaly in degrees) of a relative state about `mu`, in the frame of
    the state vectors. For a binary, mu = G(m1 + m2): the two-body relative
    orbit, the same mu src/sim/satellite.c uses to place the moon."""
    r, v = position_km, velocity_km_s
    cross = lambda a, b: (a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0])
    dot = lambda a, b: sum(x * y for x, y in zip(a, b))
    norm = lambda a: math.sqrt(dot(a, a))
    h = cross(r, v)
    node_vector = (-h[1], h[0], 0.0)
    rv = dot(r, v)
    speed2 = dot(v, v)
    radius = norm(r)
    e_vector = [((speed2 - mu_km3_s2 / radius) * r[k] - rv * v[k]) / mu_km3_s2 for k in range(3)]
    e = norm(e_vector)
    a = 1.0 / (2.0 / radius - speed2 / mu_km3_s2)
    # Rounding can push a unit-vector cosine a few ulps past +-1; clamp
    # before acos so a valid near-equatorial state cannot raise.
    clamp = lambda x: max(-1.0, min(1.0, x))
    inclination = math.degrees(math.acos(clamp(h[2] / norm(h))))
    node = math.degrees(math.atan2(node_vector[1], node_vector[0])) % 360.0
    periapsis = math.degrees(math.acos(clamp(dot(node_vector, e_vector) / (norm(node_vector) * e))))
    if e_vector[2] < 0:
        periapsis = 360.0 - periapsis
    true_anomaly = math.acos(clamp(dot(e_vector, r) / (e * radius)))
    if rv < 0:
        true_anomaly = 2 * math.pi - true_anomaly
    eccentric = 2 * math.atan2(math.sqrt(1 - e) * math.sin(true_anomaly / 2), math.sqrt(1 + e) * math.cos(true_anomaly / 2))
    mean = math.degrees(eccentric - e * math.sin(eccentric)) % 360.0
    return a, e, inclination, node, periapsis, mean


def horizons_gm_radius(info):
    """GM (km^3/s^2) and radius (km) from a Horizons asteroid physical block."""
    gm = re.search(r"GM=\s*([0-9.E+-]+)\s*km\^3/s\^2", info)
    radius = re.search(r"RAD=\s*([0-9.]+)", info)
    require(gm is not None and radius is not None, "Horizons physical block changed; review the source")
    return float(gm.group(1)), float(radius.group(1))


def refresh_patroclus():
    """Menoetius relative to the Patroclus primary (Horizons body 920000617)
    from the JPL#82 asteroid-satellite solution. Horizons gives no osculating
    elements about the primary (it has no centre mass for it), so the
    elements come from the state vector at 2024-01-01 TDB about the pair's
    GM, as for a two-body relative orbit."""
    config = SYSTEMS["patroclus"]
    info = horizons_text(format="text", COMMAND="120000617", OBJ_DATA="YES", MAKE_EPHEM="NO")
    primary = horizons_text(format="text", COMMAND="920000617", OBJ_DATA="YES", MAKE_EPHEM="NO")
    # The source line names the solution, e.g. "tnosat_v001_20000617_jpl082_...".
    solution = re.search(r"wrt/(JPL#\d+) system barycenter", info)
    require(solution is not None, "patroclus: Horizons no longer names its Menoetius solution; review the source")
    require(solution.group(1) == config["solution"],
            f"patroclus: Horizons now serves {solution.group(1)}; review it and update SYSTEMS before pinning")
    gm, radius = horizons_gm_radius(info)
    primary_gm, _ = horizons_gm_radius(primary)
    rows = horizons_text(format="text", COMMAND="120000617", CENTER="@920000617", MAKE_EPHEM="YES",
        EPHEM_TYPE="VECTORS", TLIST="2460310.5", OUT_UNITS="KM-S", REF_PLANE="ECLIPTIC", REF_SYSTEM="ICRF",
        VEC_TABLE="2", VEC_CORR="NONE", CSV_FORMAT="YES", OBJ_DATA="NO")
    row = [field.strip() for field in rows.split("$$SOE")[1].split("$$EOE")[0].strip().splitlines()[0].split(",")]
    require(float(row[0]) == 2460310.5 and "Ecliptic of J2000.0" in rows, "patroclus: Horizons epoch/frame contract changed")
    state = [float(v) for v in row[2:8]]
    mu = primary_gm + gm
    a, e, inclination, node, periapsis, mean = elements_from_state(state[:3], state[3:], mu)
    moon = {
        "code": 120000617, "name": "Menoetius", "slug": "menoetius", "group": "Patroclus system",
        "ephemeris": solution.group(1), "frame": "ecliptic", "epoch_tdb": "2024-01-01.0",
        "a_km": round(a, 4), "eccentricity": round(e, 7),
        "periapsis_deg": round(periapsis, 5), "mean_anomaly_deg": round(mean, 5),
        "inclination_deg": round(inclination, 5), "node_deg": round(node, 5),
        "period_days": round(2 * math.pi * math.sqrt(a ** 3 / mu) / 86400, 7),
        "pole_ra_deg": None, "pole_dec_deg": None, "element_reference": "Horizons 120000617 @920000617 state, mu = GM_primary + GM_satellite",
        "gm_km3_s2": gm, "radius_km": radius, "gm_sigma_km3_s2": None, "radius_sigma_km": None,
        "mass_quality": "estimated", "radius_quality": "estimated",
        "gm_reference": "Horizons 120000617 GM (no published uncertainty)", "radius_reference": "Horizons 120000617 RAD",
        "major": False,
    }
    snapshot = {"schema": 1, "checked": date.today().isoformat(), "inventory_source": config["inventory_source"],
        "names_source": "https://ssd-api.jpl.nasa.gov/sbdb.api?sstr=617&sat=1", "elements_source": HORIZONS,
        "physical_source": HORIZONS, "moons": [moon]}
    validate("patroclus", snapshot)
    paths("patroclus")[0].write_text(json.dumps(snapshot, indent=2, ensure_ascii=False) + "\n")


def refresh(system, element_rows, physical_rows):
    config = SYSTEMS[system]
    if config.get("source") == "horizons":
        (refresh_patroclus if system == "patroclus" else refresh_didymos)()
        return
    planet = config["planet"]
    physical = {int(r[2]): r for r in physical_rows if len(r) == 12 and r[0] == planet}
    rows = [r for r in element_rows if len(r) >= 19 and r[1] == planet]
    by_code = {}
    for r in rows:
        by_code.setdefault(int(r[3]), []).append(r)
    chosen = {}
    for code, candidates in by_code.items():
        preferred = config.get("prefer", {}).get(code)
        matches = [r for r in candidates if r[4] == preferred]
        require(len(candidates) == 1 or len(matches) == 1,
                f"{planet} {code}: duplicate rows need an explicit ephemeris preference")
        chosen[code] = matches[0] if matches else candidates[0]
    # Keep JPL's table order, which lists each solution's moons together.
    moons = [moon_record(system, r, physical) for r in rows if chosen[int(r[3])] is r]
    data_path, _ = paths(system)
    if system == "jupiter":
        # The Jovian snapshot predates this tool; preserve its reviewed date.
        checked = json.loads(data_path.read_text())["checked"] if data_path.exists() else date.today().isoformat()
    else:
        checked = date.today().isoformat()
    snapshot = {"schema": 1, "checked": checked,
        "inventory_source": config["inventory_source"],
        "names_source": "https://ssd.jpl.nasa.gov/sats/discovery.html",
        "elements_source": ELEMENTS, "physical_source": PHYSICAL, "moons": moons}
    validate(system, snapshot)
    data_path.write_text(json.dumps(snapshot, indent=2, ensure_ascii=False) + "\n")


def require(condition, message):
    """Fail validation explicitly; `assert` disappears under `python3 -O`."""
    if not condition:
        raise ValueError(message)


def is_major(m):
    return m.get("major", m["group"] == "Galilean moons")


def validate(system, snapshot):
    config = SYSTEMS[system]
    moons = snapshot["moons"]
    count = config["count"]
    require(snapshot["schema"] == 1 and len(moons) == count, f"{system}: review inventory changes explicitly")
    require(len({m["code"] for m in moons}) == len({m["slug"] for m in moons}) == count,
            f"{system}: moon codes and slugs must be unique")
    require({m["code"] for m in moons if is_major(m)} == config["major"], f"{system}: major moons changed")
    derived = {m["code"] for m in moons if m["mass_quality"] == "measured" and m["gm_km3_s2"] >= MAJOR_GM_KM3_S2}
    require(derived == config["major"], f"{system}: the measured-GM rule no longer yields the reviewed major moons")
    if system == "jupiter":
        require([m["code"] for m in moons[:4]] == [501, 502, 503, 504], "Galilean moons must lead the catalog")
    planet_center = config["center"]
    for m in moons:
        name = m.get("name", m.get("code"))
        require(m["code"] != planet_center, f"{name}: code collides with the planet center")
        require(m["frame"] in config["frames"] and m["epoch_tdb"] in config["epochs"],
                f"{name}: unexpected frame or epoch")
        require(m["a_km"] > 0 and 0 <= m["eccentricity"] < config["max_e"] and 0 <= m["inclination_deg"] <= 180,
                f"{name}: orbital elements out of range")
        if m["frame"] in ("Laplace", "equatorial"):
            require(0 <= m["pole_ra_deg"] < 360 and -90 <= m["pole_dec_deg"] <= 90,
                    f"{name}: reference-plane pole out of range")
        else:
            require(m["pole_ra_deg"] is None and m["pole_dec_deg"] is None,
                    f"{name}: ecliptic elements must not carry a pole")
        for value, quality in (("gm_km3_s2", "mass_quality"), ("radius_km", "radius_quality")):
            require(m[quality] in ("measured", "estimated", "unknown"), f"{name}: unknown {quality}")
            require((m[value] is None) == (m[quality] == "unknown"),
                    f"{name}: {value} presence must match {quality}")
            if m[value] is not None:
                require(m[value] > 0, f"{name}: {value} must be positive")
        require(m["mass_quality"] != "unknown" or not is_major(m), f"{name}: a major moon needs a mass")


def generate(system, snapshot):
    data_path, _ = paths(system)
    lines = [f"/* Generated by tools/satellite_catalog.py from data/{data_path.name}. */"]
    for m in snapshot["moons"]:
        fields = [str(m["code"]), json.dumps(m["name"]), json.dumps(m["group"])]
        fields += [str(m[key] or 0) for key in ("a_km", "eccentricity", "periapsis_deg", "mean_anomaly_deg", "inclination_deg", "node_deg", "pole_ra_deg", "pole_dec_deg", "gm_km3_s2", "radius_km", "period_days")]
        fields += ["SATELLITE_FRAME_" + m["frame"].upper(), "PHYSICAL_" + m["mass_quality"].upper(), "PHYSICAL_" + m["radius_quality"].upper()]
        fields += ["true" if is_major(m) else "false"]
        lines.append("    {" + ", ".join(fields) + "},")
    return "\n".join(lines) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--refresh", action="store_true")
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--system", action="append", choices=sorted(SYSTEMS))
    args = parser.parse_args()
    systems = args.system or list(SYSTEMS)
    if args.refresh:
        # Fetch the JPL satellite tables only when a table-backed system needs
        # them, so a Didymos- or Patroclus-only refresh does not depend on those requests.
        tables = any(SYSTEMS[system].get("source") != "horizons" for system in systems)
        element_rows, physical_rows = (fetch_rows(ELEMENTS), fetch_rows(PHYSICAL)) if tables else ([], [])
        for system in systems:
            refresh(system, element_rows, physical_rows)
    for system in systems:
        data_path, generated = paths(system)
        snapshot = json.loads(data_path.read_text())
        validate(system, snapshot)
        output = generate(system, snapshot)
        if args.check:
            if not generated.exists() or generated.read_text() != output:
                raise SystemExit(f"stale {system} C data; run python3 tools/satellite_catalog.py --system {system}")
        else:
            generated.write_text(output)
        print(f"{SYSTEMS[system]['adjective']} catalog OK: {len(snapshot['moons'])} moons, snapshot {snapshot['checked']}")


if __name__ == "__main__":
    main()
