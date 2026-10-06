# Small-body source contract

The canonical compact snapshot is `docs/public/catalog/manifest.json` and its
SHA-256-listed gzip files. It is intentionally separate from `implementedBodies`
and the active C scene. Full-precision source fields are retained in positional
arrays whose schema is declared by the manifest. Source GM is km³/s², diameter
is km, perihelion is AU, angles are degrees, and epochs are Julian days TDB.

`tools/small_body_catalog.py` obtains one bulk SBDB response rather than mutable
offset pages. It retains all asteroid-kind and CEN/TNO entries. It rejects
unexpected schema, duplicate identities, nonfinite fields and incomplete source
responses. Offline checks reconcile each record, index, class, density count and
checksum. The source cache in `build/` is disposable; normal builds use the
committed shards. The generated overview counts every usable orbit, using the C
conic kernel at JD 2461200.5. Projection is logarithmic, top-down and approximate.

The bulk API does not classify physical values as measured versus estimated.
Those readouts use `PHYSICAL_PUBLISHED`, with explicit unclassified quality.
Absent mass/radius fields stay unknown. The small, separately sourced
`small_body_physical.json` supplement covers the four distant official dwarf
planets; it does not infer masses from arbitrary density or albedo assumptions.

`planet_epoch.json` contains Sun-centered J2000 ecliptic geometric vectors from
Horizons at the same epoch for the eight planetary systems: Mercury (199) and
Venus (299), which have no moons, and the Earth–Moon, Mars, Jupiter, Saturn,
Uranus and Neptune system barycenters (3–8). Experiments contain no moons, so a
planet centre (399, 499, …) would carry an unbalanced moon wobble — about
12.4 m/s for Earth, drifting roughly 10⁸–10⁹ m per year. Each planet body
therefore stands for its whole system: it starts at the barycenter and uses the
DE440 system GM from <https://ssd.jpl.nasa.gov/astro_par.html> (Earth–Moon is
Earth plus Moon). The core scene, which models the moons, keeps planet-only
masses. `python3 tools/planet_epoch.py --check` refuses a snapshot whose IDs or
recorded Horizons targets are not these barycenters; the vectors were refreshed
on 2026-10-06 with each request URL recorded. `planet_epoch.inc` converts km to
m and maps source ecliptic `(X,Y,Z)` to simulation `(X,Z,−Y)`, the proper
rotation that keeps prograde orbits counterclockwise from ecliptic north.
Experiments use those initial vectors followed by the existing fixed-Sun
Newtonian model; moons are not additional active bodies there.

Native and web selected input uses `SOLAR_EXPERIMENT_V1 2461200.5`, then up to 16
tab-separated rows: SPK ID, name, q (AU), e, inclination, ascending node, argument
of perihelion (degrees), perihelion JD TDB, mass kg, radius m, mass quality, radius
quality. Quality codes are 0 measured, 1 estimated, 2 unknown, 3 published with
unclassified quality. Unknown physical values use zero in the force/marker model,
never a claimed physical measurement. A selected Vesta maps to its existing ID.
The C reader is strict: exactly twelve fields separated by single tab bytes, one
row per `\n` line (the last newline is optional), names of 1–95 bytes without
control characters, and numbers written as finite plain decimals (optional sign,
fraction and exponent; no spaces, hex floats, `inf`/`nan` or trailing text).

Sources:
- https://ssd-api.jpl.nasa.gov/doc/sbdb_query.html
- https://ssd-api.jpl.nasa.gov/doc/sbdb_filter.html
- https://ssd.jpl.nasa.gov/planets/phys_par.html
- https://ssd.jpl.nasa.gov/planets/approx_pos.html
- Exact Horizons requests in `planet_epoch.json`.

Data usage and attribution follow [README.md](README.md#data-usage-and-attribution): credit NASA/JPL-Caltech Solar System Dynamics and keep the recorded source URL, retrieval date and hashes with any reuse.
