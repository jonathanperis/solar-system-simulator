# Cartoon cosmic cockpit website redesign

## Goal

Replace the spectral orbit darkroom site with a completely different cartoon space direction based on Jonathan's new references: rounded app-window framing, saturated purple/blue background, thick cream/comic outlines, playful astronaut/planet illustration, and friendly planet cards.

## Success criteria

- Homepage no longer reads as spectral darkroom, scientific instrument wall, or prior visual system.
- First fold reads as a cartoon cosmic cockpit with a huge illustrated solar-system scene.
- Core principles remain visible: C11/raylib, SI units, source-backed physics, real WASM demo, current body catalog.
- Smoke-check markers use the new cartoon direction.
- Build, route check, preview, browser visual QA, CI, Pages deploy, and live smoke all pass.

## Implementation notes

- Rewrite `docs/src/pages/index.astro` copy around “Discover the solar system in motion.”
- Replace `OrbitInstrument.astro` with an inline cartoon SVG: chunky Sun, planets, orbit rings, sparkles, and astronaut mechanic.
- Replace `docs/public/styles/global.css` with a full OKLCH cartoon palette, rounded app shell, thick outlines, playful cards, and responsive behavior.
- Update `PRODUCT.md`, `DESIGN.md`, `AGENTS.md`, and `tools/check_docs_routes.py` so future edits preserve the cartoon cockpit direction.

## Verification checklist

- `npm run build`
- `python3 ../tools/check_docs_routes.py dist`
- `git diff --check`
- Local preview under `/solar-system-simulator/`
- Browser visual inspection and console check
- GitHub Actions Build and Deploy Pages for the pushed head SHA
- Live Pages marker smoke for root, docs, body catalog, CSS, and WASM
