# Solar System Simulator Product Context

## Product promise

Solar System Simulator is a learning-first, physics-first C11 + raylib project that makes orbital mechanics inspectable. The app should feel like a small observatory instrument: the visuals exist to reveal Newtonian motion, not to hide the math behind cinematic effects.

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
   - Code, docs, and the future GitHub Pages site should explain why each formula, constant, and transform exists.
   - Comments should document intent, units, assumptions, and tradeoffs; they should not narrate obvious C syntax.

4. **Testable without a window**
   - `src/sim/` and `src/app/` helpers should remain testable by C test binaries.
   - raylib stays at the boundary in `src/render/` and `src/main.c`.

5. **Web demo as public lab bench**
   - GitHub Pages should host both documentation and a WebAssembly build so visitors can run the simulator in the browser.
   - The site should expose controls, physics notes, source links, and build provenance instead of being only a marketing page.

## Tone and voice

- Precise, patient, and educational.
- Use observatory / mission-control language sparingly when it helps orientation.
- Prefer grounded explanations over hype.
- Use source-backed facts for physical constants and build behavior.

## Anti-references

- Do not turn the simulator into a graphics-first space screensaver.
- Do not hide physics compromises behind vague wording.
- Do not create large frameworks or generic engines before the body-by-body roadmap needs them.
- Do not duplicate Super Mango code blindly; borrow the structure, CI patterns, and browser-hosting lessons while keeping this repo C11 + raylib + physics-first.

## Current verified state

- Native app: C11 + raylib, built by `Makefile`.
- Simulation: Newtonian N-body baseline with velocity-Verlet / kick-drift-kick stepping.
- Bodies: Sun, Mercury, Venus, Earth, Moon, Mars, Phobos, Deimos.
- Tests: C test binaries for vector math, physics, scene initialization, orbit camera, trails, and renderer helpers.
- No docs site, WebAssembly target, GitHub workflows, `AGENTS.md`, `PRODUCT.md`, or `DESIGN.md` existed before this planning pass.
