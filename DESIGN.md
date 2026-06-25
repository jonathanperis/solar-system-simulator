# Solar System Simulator Design Context

## Visual direction

Theme the public site as a **softened illustrated cosmic cockpit**: a bright, rounded space playground wrapped around the real C11 + raylib simulator, now lowered from toy-like weight into a calmer science-club interface. The art direction keeps illustrated planets, orbit cards, friendly astronauts, and purple/blue/gold/cyan color, but uses slimmer outlines, smaller type, quieter shadows, and fewer decal effects.

Physical scene: a curious learner opens a weekend science-club web toy on a laptop. It feels like an illustrated space lab, but every panel still points back to source files, SI units, tests, and the browser WebAssembly artifact.

## Palette

Use OKLCH tokens in CSS rather than raw hex for the site surface.

- Outer world: vibrant but softened purple-to-blue page gradient.
- Hero shell: deep violet space card with softened cream outlines and compact shadows.
- Action accents: solar gold, moon orange, orbital cyan, mint green, star pink, and Mars red.
- Text: warm cream and pale lavender; never pure white.
- Lines: restrained illustrated outlines on primary cards and buttons, softer cream orbit rings in the background.

This is a full-palette illustrated science identity, not the rejected technical-instrument direction and not a generic NASA dashboard.

## Typography

- Headings: friendly rounded sans, currently Nunito, sized large but no longer poster-scale.
- Body: friendly readable sans, currently Nunito, for explanatory notes.
- Measurements/code: JetBrains Mono for formulas, file paths, constants, and telemetry because those values are actual program facts.

## Components for GitHub Pages

1. **Hero / cosmic cockpit**
   - Project promise, “Launch simulator” action, docs/source actions, and status chips.
   - The hero should feel like a self-contained illustrated poster inside a rounded app window.

2. **Illustrated orbit playground**
   - Inline SVG illustration with clean Sun, planets, orbit paths, sparkles, and a tiny astronaut mechanic.
   - Must state that it is a playful map, not a physical scale poster.

3. **WASM viewport and standalone runtime**
   - Dedicated canvas page with boot status, full-demo link, control summary, denser starfield backdrop, brighter grid, closer default camera, solid Sun styling, styled bodies, fading trails, and frame-filled canvas layout.
   - The standalone `/wasm/solar-system-simulator.html` page should share the cosmic cockpit frame, keep static notes outside the canvas, show runtime diagnostics, expose controls, and link back to docs/source instead of looking like an unstyled generated shell.
   - Base-path-safe asset loading for GitHub Pages (`/solar-system-simulator/`).

4. **Docs manual structure**
   - `/docs/` should behave like a compact orbit manual: sticky sidebar, filterable route list, grouped manual cards, route counts, and a full index.
   - Keep Super Mango's service-manual information architecture idea, but translate it into the illustrated cosmic cockpit palette and physics wording.

5. **Physics boundary panels**
   - Pair the serious physics engine with the illustrated presentation layer.
   - Keep source paths and formulas visible so the design does not become decorative fluff.

6. **Planet field guide**
   - Implemented bodies appear as colorful, clickable rows with parent and initialization metadata.

7. **Source evidence ledger**
   - Link every major claim to docs or source paths.
   - Prefer proof rows and route matrices over generic feature claims.

## Interaction principles

- The site should feel clickable and joyful before it explains every detail.
- Illustrated visuals must invite exploration, then nearby copy must name the real physics boundary.
- Motion is light and floaty: stars twinkle, planets drift, astronaut floats, reduced-motion respected.
- Loading states stay explicit; a blank canvas is a bug.
- Preserve accessible contrast on saturated dark backgrounds.

## Comment and documentation style

- Comments should answer: **what physical idea is this implementing? what units are expected? what approximation is being made?**
- Avoid line-by-line comments for obvious assignments.
- Public docs can be more explanatory than source comments; code comments should stay maintainable and close to tricky logic.
