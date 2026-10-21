# Simulator textures

Render-only surface and backdrop maps for the cinematic renderer (SPEC A65, A109).
They change how bodies look, never their physical state.

## Source and license

Sun, planet, Moon, ring and backdrop maps come from **Solar System Scope** (<https://www.solarsystemscope.com/textures/>),
distributed under the **Creative Commons Attribution 4.0 International** license
(<https://creativecommons.org/licenses/by/4.0/>). Credit line:

> Planet, Sun and Milky Way textures © Solar System Scope, CC BY 4.0.

The site footer carries the same attribution. Solar System Scope's maps are
artistic composites based on NASA data; they are not survey-grade cartography.

Moon and small-world maps (SPEC A109) come from **USGS Astrogeology**
global mosaics built from Voyager, Galileo, Cassini, New Horizons, Mars
Express and Dawn images. They are U.S. Government works in the public domain;
credit line: *Moon maps: USGS Astrogeology Science Center, NASA/JPL, ESA/DLR.*
`python3 tools/moon_maps.py --refresh` downloads the sources into the ignored
`build/moon-maps/` and regenerates the files below.

## Files and processing

Downloaded 2026-10-07 from the source page above, then re-encoded with Pillow
(Lanczos resampling, baseline JPEG, metadata stripped) to keep the browser
download near 2.9 MB in total. Every image is a power of two so WebGL 1 can
build mipmaps.

| File | Source file | Size | Notes |
| --- | --- | --- | --- |
| `sun.jpg` | `2k_sun.jpg` | 2048 × 1024 | emissive photosphere |
| `mercury.jpg` | `2k_mercury.jpg` | 2048 × 1024 | |
| `venus_atmosphere.jpg` | `2k_venus_atmosphere.jpg` | 2048 × 1024 | visible cloud deck, not the surface |
| `earth_day.jpg` | `2k_earth_daymap.jpg` | 2048 × 1024 | |
| `earth_night.jpg` | `2k_earth_nightmap.jpg` | 2048 × 1024 | city lights on the night side |
| `earth_clouds.jpg` | `2k_earth_clouds.jpg` | 2048 × 1024 | brightness used as cloud opacity |
| `moon.jpg` | `2k_moon.jpg` | 2048 × 1024 | |
| `mars.jpg` | `2k_mars.jpg` | 2048 × 1024 | |
| `jupiter.jpg` | `2k_jupiter.jpg` | 2048 × 1024 | |
| `saturn.jpg` | `2k_saturn.jpg` | 2048 × 1024 | |
| `saturn_ring.png` | `2k_saturn_ring_alpha.png` | 1024 × 64 | RGBA strip, inner edge left |
| `uranus.jpg` | `2k_uranus.jpg` | 1024 × 512 | nearly featureless; halved |
| `neptune.jpg` | `2k_neptune.jpg` | 2048 × 1024 | |
| `stars_milky_way.jpg` | `8k_stars_milky_way.jpg` | 4096 × 2048 | all-sky map in galactic coordinates (longitude increasing leftward, south pole up); `render_sky_orientation` turns it into the true sky (SPEC A117) |

USGS maps are reduced to 1024 × 512 by area averaging, recentred on
longitude 0 with east to the right, and saved as JPEG (quality 84).
Panchromatic maps are tinted so their mean equals an approximate true-colour
hue (the hue is presentation; the albedo pattern is measured), and unimaged
areas are filled with the mean surface colour.

| File | USGS source (`mosaic/` in the PDS Imaging bucket) | Colour |
| --- | --- | --- |
| `io.jpg` | `Io_GalileoSSI-Voyager_Global_Mosaic_ClrMerge_1km` | Galileo colour |
| `europa.jpg` | `Europa_Voyager_GalileoSSI_global_mosaic_500m` | tint 214 206 186 |
| `ganymede.jpg` | `Ganymede_Voyager_GalileoSSI_Global_ClrMosaic_1435m` | Voyager/Galileo colour |
| `callisto.jpg` | `Callisto_Voyager_GalileoSSI_global_mosaic_1km` | tint 150 138 122 |
| `enceladus.jpg` | `Enceladus_Cassini_mosaic_global_110m` | tint 238 241 244 |
| `tethys.jpg` | `Tethys_Cassini_mosaic_global_293m` | tint 226 224 220 |
| `dione.jpg` | `Dione_Cassini_Voyager_mosaic_global_154m` | tint 214 212 208 |
| `rhea.jpg` | `Rhea_Cassini_Voyager_mosaic_global_417m` | tint 206 202 196 |
| `iapetus.jpg` | `Iapetus_Cassini_Voyager_mosaic_global_783m` | tint 196 182 160 |
| `triton.jpg` | `Triton_Voyager2_ClrMosaic_GlobalFill_600m` | tint 220 202 192 (source has a green filter cast) |
| `pluto.jpg` | `Pluto_NewHorizons_Global_Mosaic_300m_Jul2017_8bit` | tint 222 188 150 |
| `charon.jpg` | `Charon_NewHorizons_Global_Mosaic_300m_Jul2017_8bit` | tint 176 170 164 |
| `phobos.jpg` | `Phobos_ME_SRC_Mosaic_Global_16ppd` | tint 132 118 104 |
| `vesta.jpg` | `Vesta_Dawn_FC_HAMO_Mosaic_Global_74ppd` | tint 176 168 158 |

Each mapped body has an IAU WGCCRE 2015 rotation model, so longitude 0 of a
synchronous moon faces its planet (checked by `make test`).

The inventory and load order are defined once in C (`render_texture_file` in
`src/render/scene_style.c`); `make test` checks that every listed file exists.
The native app reads this folder; `make docs-textures` copies it into the site,
whose browser runtime fetches the files after its first frame.
