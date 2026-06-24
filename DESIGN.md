# Solar System Simulator Design Context

## Visual direction

Theme the public site as a **spectral orbit darkroom**: a late-night scientific instrument wall where simulator state is projected as traces, specimen ledgers, coordinate plates, and source-backed field notes. The aesthetic is nerdish and experimental, but the information hierarchy stays disciplined. Physics remains the source of truth; the website makes the measurement boundary visible.

Physical scene: a learner and C programmer are in a dim lab at 1 a.m., reading orbital traces on a black optical table while the browser runs the actual WebAssembly artifact beside the source atlas.

## Palette

Use OKLCH tokens in CSS rather than raw hex for the site surface.

- Void neutrals: tinted blue-black, graphite, and low-glare slate.
- Trace colors: spectral cyan, comet green, solar amber, Mars vermilion, and violet ultraviolet.
- Supporting text: warm off-white and muted blue-gray, never pure white.
- Lines: translucent instrument grid strokes and measured outlines, not heavy comic borders.

This is a full-palette scientific instrument system, not the previous bright paper-collage palette and not generic neon cyberpunk.

## Typography

- Headings: compressed technical sans with sharp scale contrast, suitable for lab labels and instrument titles.
- Body: highly readable humanist sans for explanatory notes.
- Measurements/code: a deliberate mono face for formulas, file paths, constants, and telemetry, used because the surface is literally about measured program state.

## Components for GitHub Pages

1. **Hero / Spectral chamber**
   - Project name, physics-first promise, primary "Run orbit chamber" action, docs/source actions.
   - Status chips: C11 physics core, SI units, velocity-Verlet, 8 bodies, WebAssembly artifact.

2. **Orbit trace plate**
   - Abstract diagram of current implemented bodies as a measurement plate.
   - Must clearly state it is not to scale and that render projections do not mutate simulation state.

3. **WASM viewport**
   - Embedded canvas with boot status, full-demo link, and control summary.
   - Base-path-safe asset loading for GitHub Pages (`/solar-system-simulator/`).

4. **Physics boundary panels**
   - Pair simulation state with projection layer: meters/kilograms/seconds in `src/sim`, labels/trails/scale modes in renderer/app code.

5. **Body specimen register**
   - A compact table-like register for implemented bodies with source-backed parent and initialization metadata.

6. **Source evidence ledger**
   - Link every major claim to docs or source paths.
   - Prefer proof rows and route matrices over generic feature cards.

## Interaction principles

- The site should teach while it runs: controls, equations, and source links stay near the canvas.
- Decorative traces must support scientific reading, not hide content.
- Motion is subtle: trace glow or pulse only, with reduced-motion respected.
- Make loading states explicit; a blank canvas is a bug.
- Preserve accessible contrast on dark backgrounds.

## Comment and documentation style

- Comments should answer: **what physical idea is this implementing? what units are expected? what approximation is being made?**
- Avoid line-by-line comments for obvious assignments.
- Public docs can be more explanatory than source comments; code comments should stay maintainable and close to tricky logic.
