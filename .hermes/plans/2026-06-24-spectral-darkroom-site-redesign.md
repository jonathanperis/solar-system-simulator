# Spectral darkroom website redesign

## Goal

Replace the current bright science-zine aesthetic with a completely different public-site direction that still protects the project principles: physics truth first, rendering as an explicit projection layer, source-backed docs, and a browser lab bench for the C/raylib WebAssembly artifact.

## Direction

Physical scene: a late-night orbital mechanics lab projects simulator state onto a black optical table through a spectrograph. The visitor sees traces, coordinate plates, specimen registers, and source references rather than corporate space wallpaper or playful paper stickers.

## Success criteria

- Homepage copy feels nerdish, scientific, and more experimental without inventing capabilities.
- The hero anchors on the phrase `Gravity as a measured signal.` and a spectral orbit darkroom narrative.
- Global site styling moves to a dark spectral instrument system with thin measured lines, OKLCH palette, and readable technical typography.
- The WASM demo, docs routes, body catalog, source atlas, and footer remain base-path safe.
- Smoke markers in `tools/check_docs_routes.py` match the new durable copy.
- Verify with docs build, route checker, diff whitespace check, local preview HTTP smokes, and visual browser inspection.

## Implementation steps

1. Update `DESIGN.md` and repo guidance to document the new spectral darkroom direction.
2. Rewrite homepage hero and evidence sections around instrument traces, SI units, and source-backed projection boundaries.
3. Retheme the orbit instrument component and global CSS for the whole Astro site.
4. Refresh smoke-check markers for the new homepage copy.
5. Build, preview under `/solar-system-simulator/`, visually inspect, then commit and push main.
