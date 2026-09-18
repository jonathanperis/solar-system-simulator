# Solar System Simulator Design Context

## Visual direction

Public site is an **archival solar chart**: cream paper, ink navy, brass rules, engraved orbit geometry, editorial serif type. It feels like a maintained astronomical atlas, not a cockpit, dashboard, or space screensaver.

Homepage is a full-screen interactive orrery. It indexes real core-body metadata with illustrative position and scale. Physical integration belongs to the C WebAssembly simulator and comparison lab; the separate small-body atlas uses C-computed approximate two-body previews.

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
   - Full-screen SVG/DOM chart with heliocentric, Earth-relative, Mars-relative, and grouped/paginated Jupiter-relative plates.
   - Each body is a semantic button and supports pointer, keyboard, touch, and no-JS catalog fallback.
   - Brass observation arm may select nearest body. It never claims physical orbital state.

2. **Liner-note drawer**
   - Lead with selected-body identity, a readable parent/orbit explanation, and a direct simulator link; disclose initialization, milestone and source under Model and sources.
   - Desktop side drawer; mobile bottom sheet with a persistent Close control.

3. **Field-guide routes**
   - Existing routes stay static and scrollable.
   - Docs are readable field sheets. Physics, source, bodies, and pipeline retain source-backed claims.
   - Primary navigation follows Explore, Learn, Experiments and Reference. The reference hub keeps all existing atlases and developer routes discoverable.
   - First-orbit and web/touch guidance precede numerical and developer reference. Page search includes task keywords and a useful no-results state.

4. **WASM runtime**
   - Archival frame and real loading/error state around unchanged raylib canvas.
   - Canvas remains dark renderer output; no page CSS mutates physics or renderer behavior.
   - Put the scene and playback/recovery controls first. Find an object, View options, Learn and Advanced open named native dialogs; mobile uses bottom sheets with visible dismissal and restored focus.
   - Core body links use `simulator/?body=<slug>` and resolve against C's ready body list once. Runtime controls and physical readouts remain C-owned.

5. **Learning and catalog surfaces**
   - Comparison plots and accessible tables present matched C measurements, with units, source revision, and explicit unavailable values.
   - The small-body atlas distinguishes catalog, density, result, and active-physics counts. Its logarithmic map and two-body previews are labeled approximations.
   - Small-body selection opens visible details immediately, including source-loading/error feedback. The basket is separate from selection; preparing it does not start the simulator automatically.
   - Comparison questions and valid defaults precede optional parameters; field-specific diagnostics explain C rejection without replacing its validation.

## Interaction principles

- Body selection: click/tap, `Left`/`Right`, previous/next controls.
- Plate selection: visible buttons; wheel changes plate only while atlas focused.
- `Escape` closes detail drawer and restores focus to invoking body.
- `prefers-reduced-motion` removes ornamental transitions/rotation.
- Empty canvas/loading state never unexplained.
- Decorative SVG is `aria-hidden`; interaction uses valid buttons and linked fallbacks.

## Boundaries

- Chart scale, positions, orbit geometry, and body pigments are illustrative; label this in UI.
- Core names, kinds, parents, initialization, milestone, and source derive from `docs/src/lib/bodies.ts` and its versioned Jovian input. Small-body metadata derives from the pinned catalog manifest/shards.
- SI state, integrator, renderer transforms, and runtime controls remain source-backed.
- Do not add Three.js, p5.js, GSAP, shaders, textures, or asset system for site atlas.
