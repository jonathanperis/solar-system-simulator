# Jovian satellite snapshot

`jovian_moons.json` is the versioned input for both C initialization and the
Astro catalog. It records all 115 Jupiter moons in JPL's 2026-09-10 mean-element
table, including provisional designations. NASA's August 2026 inventory agrees.

- Orbital elements: <https://ssd.jpl.nasa.gov/sats/elem/>
- GM, mean radius, uncertainty and references: <https://ssd.jpl.nasa.gov/sats/phys_par/>
- Inventory: <https://science.nasa.gov/jupiter/moons/>

The snapshot preserves source units (km, km³/s², degrees) and J2000 TDB epoch;
C converts to SI. Mass is GM / G. Published dynamical/size estimates are labeled
as such. Missing values remain null, rather than being invented from a density
or albedo assumption. The 106 moons without entries in the JPL physical table
are massless test particles with unknown radii in this model.

Mean elements describe representative orbital shape and orientation, not dated
ephemerides. Laplace-plane nodes are measured from the reference plane's node
on the ICRF equator; its pole is specified by ICRF RA/declination. The initializer
rotates into J2000 ecliptic coordinates (obliquity 23°26′21.448″), then maps
ecliptic `(x,y,z)` to simulator `(x,z,y)`, matching the existing X/Z orbit plane.
The legacy perihelion scene is not a simultaneous J2000 ephemeris. Jupiter's
absolute position and velocity are added only after the relative conversion.

Normal builds require no network. After an explicitly reviewed source update:

```sh
python3 tools/jovian_catalog.py --refresh
python3 tools/jovian_catalog.py --check
```

The importer deliberately rejects inventory count/epoch/frame changes until
reviewed. Generated `src/sim/jovian_moons.inc` is committed and checked in CI.

## Provenance and precision policy

| Data group | Versioned evidence | Epoch / retrieval | Precision and uncertainty |
| --- | --- | --- | --- |
| Legacy Sun/inner planets/Earth-Mars moons | `src/sim/constants.h`, numeric initialization tests, NASA [fact sheets](https://nssdc.gsfc.nasa.gov/planetary/factsheet/) and JPL [satellite physical parameters](https://ssd.jpl.nasa.gov/sats/phys_par/) | Original retrieval date not recorded; demonstration has no common source epoch | Preserve baseline values; decimal count is not an uncertainty claim |
| Vesta demonstration | Constants and tests pin selected values attributed to JPL SBDB solution 36 | Original retrieval date and solution epoch not recorded; the live API can change | GM-to-mass and diameter-to-radius conversions are explicit; original uncertainty is unavailable in this legacy subset |
| Jupiter through Neptune | JPL [physical parameters](https://ssd.jpl.nasa.gov/planets/phys_par.html) and [approximate elements](https://ssd.jpl.nasa.gov/planets/approx_pos.html), pinned in constants/tests | J2000 approximate elements; demonstration phase is deliberately synthetic | Preserve published significant digits; derived speeds do not add measured precision |
| Jovian moons | `jovian_moons.json` | Checked 2026-09-10; individual frame/epoch fields retained | GM/radius uncertainties and references retained when available; missing values remain unknown |
| Selected small-body experiments | `planet_epoch.json`, `small_body_physical.json`, compressed catalog manifest/shards | See [SMALL_BODIES.md](SMALL_BODIES.md); source records retain epochs, and experiments align to JD 2461200.5 TDB | Quality and snapshot hashes remain explicit; no invented density/albedo estimates |

The versioned constants make legacy runs reproducible, but do not reconstruct an original source response whose epoch or retrieval date was never saved. Refresh such data only with a reviewed source snapshot and uncertainty record. Guided lessons reuse physical masses/radii while explicitly changing orbital initial conditions; they are not additional astronomical measurements.
