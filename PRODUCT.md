# Solar System Simulator Product Context

## Product promise

Solar System Simulator is a learning-first, physics-first C11 + raylib project that makes orbital mechanics inspectable. The public site can be playful and cartoonish, but the product truth stays scientific: visuals invite exploration while source files, SI units, and tests explain the model.

## Primary audience

- Learners who want to understand how a solar system can be simulated from first principles.
- C programmers who want a small, readable example of separating simulation math from rendering.
- Future agents and maintainers expanding the project one celestial body at a time.

## Strategic principles

1. **Physics truth before visual polish**
   - Internal simulation state uses SI units, double precision, and raylib-independent data structures.
   - Rendering may be illustrative, but it must never mutate physical state.

2. **One body / one concept per milestone**
   - Expand the system incrementally: constants, initialization, tests, renderer visibility, docs, then commit.
   - Avoid jumping to ephemerides, textures, shaders, or catalogs before the current body is physically grounded.

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
- The current public-site voice is cartoon cosmic cockpit: friendly, adventurous, and science-club curious.
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
- Bodies: Sun, Mercury, Venus, Earth, Moon, Mars, Phobos, Deimos.
- Tests: C test binaries for vector math, physics, scene initialization, orbit camera, trails, and renderer helpers.
- Public site: Astro static GitHub Pages docs with a base-path-safe cartoon WebAssembly cockpit, Super-Mango-inspired orbit manual docs hub, source atlas, physics notes, body catalog, pipeline docs, footer credits, and route smoke checks.
