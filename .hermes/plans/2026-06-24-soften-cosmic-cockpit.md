# Soften cosmic cockpit visual pass

## Goal

Keep Jonathan's approved cosmic cockpit aesthetic, but lower the page intensity so it feels less huge and less toy-like.

## Changes

1. Reduce global type scale: smaller hero/display headings, less poster-style stacking, Nunito as the heading family instead of Fredoka.
2. Mute color saturation slightly while keeping the purple/blue/gold/cyan identity.
3. Slim borders, outlines, SVG strokes, shadows, and hover lift so panels feel illustrated rather than thick cartoon stickers.
4. Replace visible copy that overuses “cartoon”, “sticker”, “chunky”, and “cute” with “illustrated”, “friendly”, “cosmic cockpit”, and source-backed wording.
5. Apply the same quieter treatment to homepage, docs hub, and standalone WASM shell.

## Verification

Run `make web`, copy artifacts to `docs/public/wasm`, build Astro docs, run route/WASM checks, preview homepage/docs/WASM under `/solar-system-simulator/`, then commit/push/watch Pages and live smoke.
