# Simulator textures

Render-only surface and backdrop maps for the cinematic renderer (SPEC A65).
They change how bodies look, never their physical state.

## Source and license

All maps come from **Solar System Scope** (<https://www.solarsystemscope.com/textures/>),
distributed under the **Creative Commons Attribution 4.0 International** license
(<https://creativecommons.org/licenses/by/4.0/>). Credit line:

> Planet, Sun and Milky Way textures © Solar System Scope, CC BY 4.0.

The site footer carries the same attribution. Solar System Scope's maps are
artistic composites based on NASA data; they are not survey-grade cartography.

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
| `stars_milky_way.jpg` | `8k_stars_milky_way.jpg` | 4096 × 2048 | illustrative backdrop, not a sky ephemeris |

The inventory and load order are defined once in C (`render_texture_file` in
`src/render/scene_style.c`); `make test` checks that every listed file exists.
The native app reads this folder; `make docs-textures` copies it into the site,
whose browser runtime fetches the files after its first frame.
