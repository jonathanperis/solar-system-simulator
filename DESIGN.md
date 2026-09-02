# Solar System Simulator Design Context

## Visual direction

Public site is an **archival solar chart**: cream paper, ink navy, brass rules, engraved orbit geometry, editorial serif type. It feels like a maintained astronomical atlas, not a cockpit, dashboard, or space screensaver.

Homepage is a full-screen interactive orrery. It indexes real catalog metadata with illustrative position and scale. Live motion belongs only to browser WASM runtime.

## Palette

- Paper: warm cream with faint grain and chart grid.
- Ink: near-black navy for text, rules, and orbit lines.
- Brass: selected body, controls, measurement marks, and action emphasis.
- Planet color: restrained mineral pigments; no neon or generic galaxy fog.
- Use OKLCH tokens in CSS.

## Typography

- Headings/body: editorial serif, Cormorant Garamond fallback Georgia.
- Measurements/code: JetBrains Mono fallback monospace.
- Labels: small caps or spaced mono only when information-dense.

## Components

1. **Orbital atlas**
   - Full-screen SVG/DOM chart with heliocentric, Earth-relative, Mars-relative plates.
   - Each body is a semantic button and supports pointer, keyboard, touch, and no-JS catalog fallback.
   - Brass observation arm may select nearest body. It never claims physical orbital state.

2. **Liner-note drawer**
   - Selected body: kind, parent, initialization, milestone, source path, catalog route.
   - Desktop side drawer; mobile bottom sheet.

3. **Field-guide routes**
   - Existing routes stay static and scrollable.
   - Docs are readable field sheets. Physics, source, bodies, and pipeline retain source-backed claims.

4. **WASM runtime**
   - Archival frame and real loading/error state around unchanged raylib canvas.
   - Canvas remains dark renderer output; no page CSS mutates physics or renderer behavior.

## Interaction principles

- Body selection: click/tap, `Left`/`Right`, previous/next controls.
- Plate selection: visible buttons; wheel changes plate only while atlas focused.
- `Escape` closes detail drawer and restores focus to invoking body.
- `prefers-reduced-motion` removes ornamental transitions/rotation.
- Empty canvas/loading state never unexplained.
- Decorative SVG is `aria-hidden`; interaction uses valid buttons and linked fallbacks.

## Boundaries

- Chart scale, positions, orbit geometry, and body pigments are illustrative; label this in UI.
- Names, kinds, parents, initialization, milestone, source derive from `docs/src/lib/bodies.ts`.
- SI state, integrator, renderer transforms, and runtime controls remain source-backed.
- Do not add Three.js, p5.js, GSAP, shaders, textures, or asset system for site atlas.
