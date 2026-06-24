# WASM canvas fill and lighting fix

## Goal

Fix Jonathan's live WASM feedback:

1. The renderer light treatment looked odd: the Sun read like a dark bullseye and a glow looked displaced from the actual Sun.
2. The simulator did not appear to fill the full canvas frame served by the website.

## Root cause

- The Sun and planet pass used transparent 3D halo/rim spheres before opaque body cores. In raylib's depth-tested 3D draw order, those larger transparent shapes could obscure the core and make the Sun read dark.
- The scene also drew a gold-tinted enlarged copy of the entire render texture as a pseudo-bloom pass, which colored the whole viewport instead of only emissive objects.
- The WASM shell let Emscripten/raylib and CSS disagree about the canvas display size, so the container could be wider than the active render target.

## Fix

- Remove the off-axis screen-space glow fields and whole-scene pseudo-bloom.
- Render the Sun as a solid warm sphere with subtle wire rings instead of transparent depth-writing halo spheres.
- Keep planet highlights/rims, but draw opaque bodies first and make accents smaller.
- Let CSS fill the visible canvas frame while preserving raylib/Emscripten's stable WebGL backing buffer.
- Force the canvas element to fill the `.canvas-wrap` frame with CSS and a small runtime size lock.

## Verification

- Native build and C tests.
- WebAssembly build and artifact smoke check.
- Astro build and docs route smoke check.
- Local browser inspection of the WASM page: frame-filled canvas, no right strip, no dark bullseye Sun, no displaced glow.
