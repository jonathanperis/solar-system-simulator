# Giant-planet satellite snapshots

`tools/satellite_catalog.py` maintains one snapshot per giant planet:
`jovian_moons.json` (115 moons, checked 2026-09-10), `saturnian_moons.json` (291),
`uranian_moons.json` (29), `neptunian_moons.json` (16), `plutonian_moons.json` (5)
and `didymos_moons.json` (1), all checked 2026-10-07, and `patroclus_moons.json`
(1), checked 2026-10-08.
The Jovian description below applies to all four. System-specific rules:

- Saturn: 24 regular moons use SAT441/SAT415 Laplace planes; 267 irregular moons
  use the ecliptic. NASA's Saturn page states both 274 and 293 recognized moons;
  the simulated set is JPL's 291 orbit-bearing rows.
- Uranus: three ephemeris solutions with epochs 2000-01-01.5 (URA182, major
  moons, "equatorial" frame), 2025-01-01.0 (URA184, inner moons, Laplace) and
  2020-01-01.0 (URA117, irregulars, ecliptic). JPL gives no pole for the
  equatorial frame; it is Uranus's spin pole, the antipode of the IAU north pole
  (RA 257.311°, Dec −15.175°), since Uranus spins retrograde. Puck appears in
  URA182 and URA184; the tool keeps URA184.
- Neptune: NEP097 Laplace-frame regular moons (Triton is retrograde) and
  NEP104/NEP105 ecliptic irregulars at epoch 2020-01-01.0. Nereid's GM is listed
  as zero (not determined), so its mass is unknown.
- New systems label a GM "measured" when its 1σ uncertainty is at most 10 % of
  the value and "estimated" otherwise; the Jovian rules are unchanged.
- Pluto: `plutonian_moons.json` (5 moons, PLU060, epoch 2000-01-01.5) uses an
  equatorial frame without pole columns. It is Charon's orbit plane, around the
  IAU 2015 positive pole (RA 132.993°, Dec −6.163°); for dwarf planets that
  pole already follows the right-hand rule, so no antipode is taken. Kerberos
  and Styx have only GM upper limits, so their masses are unknown.
- Didymos: `didymos_moons.json` holds Dimorphos from the JPL Horizons DART s547
  post-impact reconstruction: osculating ecliptic J2000 elements about the
  Didymos primary at 2024-01-01 TDB, the approximate GM (estimate) and the
  volume-equivalent radius of its triaxial shape. The primary's GM, radius and
  SBDB orbit are pinned in `src/sim/constants.h`.
- Patroclus: `patroclus_moons.json` holds Menoetius from the JPL Horizons
  asteroid-satellite solution JPL#82 (bodies 120000617 and 920000617). Horizons
  gives no osculating elements about the primary, so the tool converts the
  state vector at 2024-01-01 TDB into ecliptic J2000 elements about the pair's
  GM (Patroclus 0.0740606 plus Menoetius 0.020917 km³/s², the two-body relative
  orbit). Neither GM nor radius has a published uncertainty: both are
  estimates. The primary's values and the SBDB 617 orbit (solution 87) are
  pinned in `src/sim/constants.h`.
- `major` marks moons with a measured GM of at least 2 km³/s² (about 3×10¹⁹ kg):
  the Galilean moons, Saturn's seven rounded moons (Mimas, the smallest, has
  2.5), Uranus's five and Triton. The Jovian file implies it for the Galilean
  group. The tool re-derives the set from the data and rejects any change.

Phases are mutually consistent only within one ephemeris solution; the mixed
epochs are never presented as a dated snapshot.

## Scene epoch snapshot

`scene_epoch.json` pins the real sky every scene starts from: JPL Horizons
vectors at JD 2461200.5 TDB (2026-06-09) in the J2000 ecliptic. It holds each
catalogued moon relative to its primary's centre (Earth's Moon, Phobos and
Deimos under their legacy scene IDs 4, 6 and 7), and the heliocentric states of
Vesta and the Pluto, Didymos and Patroclus system barycenters; the planetary-system
barycenters come from `planet_epoch.json`. `tools/scene_epoch.py --refresh`
makes one serialized Horizons request per body and accepts a result only when
Horizons names the same body ("Io (501)", "S2023_S01" for S/2023 S 1); two
bodies are undated and keep mean elements: Daphnis (no Horizons data at the
epoch) and S/2025 U 1 (its code resolves to an asteroid). Centres are always
written `@399`, `@499`, `@699`…: a bare `499` is an observatory code, not
Mars. The validator rejects any moon whose distance lies outside 0.5× its
periapsis to 1.5× its apoapsis, which catches a wrong centre. `--refresh --only
CODE…` re-fetches single bodies. `--check` verifies the generated
`src/sim/scene_epoch.inc` offline.

Propagating mean elements to the epoch instead was tried and rejected: it
missed Horizons by up to 178° (resonances and forced eccentricities).

## Jovian satellite snapshot

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
rotates into J2000 ecliptic coordinates (obliquity 23°26′21.448″), then applies
the proper rotation ecliptic `(X,Y,Z)` → simulator `(X,Z,−Y)`: the ecliptic is the
simulator x/z plane, `+y` is ecliptic north, and prograde orbits keep `+y`
angular momentum. (Before 2026-10-06 the mapping `(X,Z,Y)` mirrored the scene.)
The legacy perihelion scene is not a simultaneous J2000 ephemeris. Jupiter's
absolute position and velocity are added only after the relative conversion.

Normal builds require no network. After an explicitly reviewed source update:

```sh
python3 tools/satellite_catalog.py --refresh --system jupiter
python3 tools/satellite_catalog.py --check
```

The importer deliberately rejects inventory count/epoch/frame changes until
reviewed. Generated `src/sim/*_moons.inc` files are committed and checked in CI.

## Provenance and precision policy

| Data group | Versioned evidence | Epoch / retrieval | Precision and uncertainty |
| --- | --- | --- | --- |
| Legacy Sun/inner planets/Earth-Mars moons | `src/sim/constants.h`, numeric initialization tests, NASA [fact sheets](https://nssdc.gsfc.nasa.gov/planetary/factsheet/) and JPL [satellite physical parameters](https://ssd.jpl.nasa.gov/sats/phys_par/) | Original retrieval date not recorded for radii/orbits; Sun, Mercury, Venus, Earth, Moon and Mars masses re-derived 2026-10-06 as DE440 GM / CODATA 2018 G ([astrodynamic parameters](https://ssd.jpl.nasa.gov/astro_par.html), [planetary physical parameters](https://ssd.jpl.nasa.gov/planets/phys_par.html)); Mars radius is the JPL 3389.5 km mean radius; demonstration has no common source epoch | GM-derived masses are tested against the cited GM within 1e-9; other decimal counts are not uncertainty claims |
| Vesta demonstration | Constants and tests pin selected values attributed to JPL SBDB solution 36 | Original retrieval date and solution epoch not recorded; the live API can change | GM-to-mass and diameter-to-radius conversions are explicit; original uncertainty is unavailable in this legacy subset |
| Jupiter through Neptune | JPL [physical parameters](https://ssd.jpl.nasa.gov/planets/phys_par.html) and [approximate elements](https://ssd.jpl.nasa.gov/planets/approx_pos.html), pinned in constants/tests | J2000 approximate elements; demonstration phase is deliberately synthetic | Preserve published significant digits; derived speeds do not add measured precision |
| Jovian moons | `jovian_moons.json` | Checked 2026-09-10; individual frame/epoch fields retained | GM/radius uncertainties and references retained when available; missing values remain unknown |
| Saturnian, Uranian and Neptunian moons | `saturnian_moons.json`, `uranian_moons.json`, `neptunian_moons.json` | Checked 2026-10-07; per-moon ephemeris, frame and epoch retained (2000, 2020 and 2025 epochs) | Same policy; GM quality from the published 1σ |
| Selected small-body experiments | `planet_epoch.json`, `small_body_physical.json`, compressed catalog manifest/shards | See [SMALL_BODIES.md](SMALL_BODIES.md); source records retain epochs, and experiments align to JD 2461200.5 TDB | Quality and snapshot hashes remain explicit; no invented density/albedo estimates |

The versioned constants make legacy runs reproducible, but do not reconstruct an original source response whose epoch or retrieval date was never saved. Refresh such data only with a reviewed source snapshot and uncertainty record. Guided lessons reuse physical masses/radii while explicitly changing orbital initial conditions; they are not additional astronomical measurements.

## Data usage and attribution

The MIT [`LICENSE`](../LICENSE) covers this repository's code and original
writing. It does not relicense third-party data. The files in `data/`,
`src/sim/*_moons.inc`, `src/sim/planet_epoch.inc` and the small-body
shards in `docs/public/catalog/` are transformed extracts of NASA and
JPL/Caltech Solar System Dynamics products (SBDB, satellite mean elements and
physical parameters, planetary constants) and NASA Science pages.

These sources publish their data for public use and ask that NASA/JPL be
credited. Their own pages, not this repository, set the terms. When reusing
these extracts:

- credit "NASA/JPL-Caltech Solar System Dynamics" and link the source URL
  recorded in each snapshot or manifest;
- keep the retrieval date, epoch and transformation notes above, so derived
  values are not presented as fresh source data;
- do not imply NASA or JPL endorsement of this project.

The repository records each source URL, retrieval date and content hash, so
every redistributed value can be traced to the response it came from.
