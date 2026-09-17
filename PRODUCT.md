# Solar System Simulator Product Context

## Product promise

Solar System Simulator is a learning-first, physics-first C11 + raylib project that makes orbital mechanics inspectable. The public site can be playful and illustrated, but the product truth stays scientific: visuals invite exploration while source files, SI units, and tests explain the model.

## Primary audience

- Learners who want to understand how a solar system can be simulated from first principles.
- C programmers who want a small, readable example of separating simulation math from rendering.
- Future agents and maintainers expanding the project one celestial body at a time.

## Strategic principles

1. **Physics truth before visual polish**
   - Internal simulation state uses SI units, double precision, and raylib-independent data structures.
   - Rendering may be illustrative, but it must never mutate physical state.

2. **One physical milestone at a time**
   - Expand incrementally: sourced constants or catalog inputs, initialization, tests, renderer visibility, docs, then verification.
   - Keep the complete source catalog separate from the bounded active scene. New physics and visual features need a concrete learning goal and evidence.

3. **Learning by inspection**
   - Code, docs, and the GitHub Pages site should explain why each formula, constant, and transform exists.
   - Comments should document intent, units, assumptions, and tradeoffs; they should not narrate obvious C syntax.

4. **Testable without a window**
   - `src/sim/` and `src/app/` helpers should remain testable by C test binaries.
   - raylib stays at the boundary in `src/render/` and `src/main.c`.

5. **Web demo as public lab bench**
   - GitHub Pages hosts both documentation and a WebAssembly build so visitors can run the simulator in the browser.
   - The site should expose controls, physics notes, source links, and build provenance instead of being only a marketing page.

## Tone and voice

- Precise, patient, educational, and scientifically playful.
- Public-site voice is archival, precise, and quietly playful: an illustrated solar chart that points learners to inspectable source and SI physics.
- Use playful labels when they clarify navigation, then ground claims in source-backed facts.
- Prefer grounded explanations over hype.

## Anti-references

- Do not turn the simulator into a graphics-first space screensaver.
- Do not hide physics compromises behind vague wording.
- Do not create large frameworks or generic engines before the body-by-body roadmap needs them.
- Do not duplicate Super Mango code blindly; borrow the structure, CI patterns, and browser-hosting lessons while keeping this repo C11 + raylib + physics-first.
- Do not use generic space stock, corporate galaxy fog, stale rejected visual language, or decorative sci-fi panels that do not explain source-backed behavior.

## Current verified state

- Native app: C11 + raylib, built by `Makefile`.
- Simulation: Newtonian N-body baseline with velocity-Verlet / kick-drift-kick stepping.
- Core scene: 128 bodies — the Sun, all eight planets, Vesta, Earth's Moon, Phobos, Deimos, and 115 Jovian moons. The default Sun is fixed; the explicit barycentric-core lesson releases it.
- Small-body atlas: 1,564,244 qualifying records in the pinned 2026-09-14 JPL snapshot, separate from the active scene. Selected experiments use epoch-aligned Sun/eight planets plus at most 16 objects.
- Learning lab: ten guided presets beyond the core, explicit Verlet/Euler comparisons, scientific diagnostics, force inspection, bounded A/B plots, save/share/import descriptors, and SI CSV export. The same C model runs natively, in WebAssembly, and through the raylib-free CLI.
- Verification: C physics/app/renderer tests, offline source-catalog checks, Python CLI/build/artifact contracts, Node integration tests, Astro type/build/route checks, sanitizers, and sandboxed browser tests in CI.
- Public site: Astro static GitHub Pages atlas with `/simulator/`, `/compare/`, `/small-bodies/`, field-guide docs, source atlas, physics notes, body catalog, and pipeline docs. Build produces the validated site; Deploy Pages publishes that exact artifact and revision.
