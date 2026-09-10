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
