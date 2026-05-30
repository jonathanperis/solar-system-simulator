# Solar System Simulator Design Context

## Visual direction

Theme the public site as a **space observatory / mission-control lab** rather than an arcade cabinet. The experience should feel precise, dark, mathematical, and inviting: orbital gridlines, star fields, instrument panels, code readouts, and annotated diagrams.

## Palette

- Deep space: `#030712`, `#07111f`, `#0b1020`
- Observatory panels: `#111827`, `#172033`, `#1f2937`
- Star text: `#f8fafc`, `#cbd5e1`, `#94a3b8`
- Solar accent: `#facc15`, `#f97316`
- Scientific cyan: `#22d3ee`, `#38bdf8`
- Orbital violet: `#8b5cf6`, `#a78bfa`
- Mars / rock accent: `#fb923c`, `#b45309`

Use high contrast and subtle glow; avoid rainbow palettes that weaken the physics-lab identity.

## Typography

- Headings: a strong geometric sans-serif or system fallback with generous letter spacing for mission labels.
- Body: readable sans-serif, optimized for long technical notes.
- Code/math: monospace with clear `0`, `1`, and unit symbols.

## Components to plan for GitHub Pages

1. **Hero / Observatory deck**
   - Project name, physics-first promise, primary "Run Web Demo" action, secondary "Read the Code" action.
   - Small status chips: C11, raylib, SI units, velocity-Verlet, WebAssembly.

2. **WASM viewport**
   - Embedded canvas with boot status, normal/debug run buttons, and clear fallback errors.
   - Base-path-safe asset loading for GitHub Pages (`/solar-system-simulator/`).
   - Console/status rail that explains when `.js`, `.wasm`, and optional `.data` assets load.

3. **Physics notebook**
   - Explain constants, units, vectors, acceleration, vis-viva initialization, and integrator assumptions.
   - Link each concept to source files and tests.

4. **Body catalog / mission manifest**
   - One card per simulated body with mass, radius, initial state, and current milestone status.
   - Future bodies marked as planned, not implemented.

5. **Source atlas**
   - Docs pages organized by module: `src/sim`, `src/render`, `src/app`, `src/main.c`, tests, build pipeline.
   - Each page should explain responsibility, key APIs, units, and verification.

6. **Pipeline observatory**
   - Document native build, tests, WASM build, docs build, Pages deploy, and smoke checks.
   - Mirror the Super Mango separation between build/artifact workflow and Pages deployment.

## Interaction principles

- The site should teach while it runs: controls, equations, and source links should be near the canvas.
- Prefer cards, panels, and route matrices over long undifferentiated markdown pages.
- Make loading states explicit; a blank canvas is a bug.
- Keep reduced-motion users in mind: decorative star/parallax effects must not be required to understand content.

## Comment and documentation style

- Comments should answer: **what physical idea is this implementing? what units are expected? what approximation is being made?**
- Avoid line-by-line comments for obvious assignments.
- Public docs can be more explanatory than source comments; code comments should stay maintainable and close to tricky logic.
