# Solar System Simulator Design Context

## Visual direction

Theme the public site as a **cartoon cosmic cockpit**: a bright, rounded, sticker-like space playground wrapped around the real C11 + raylib simulator. The art direction borrows from illustrated space landing pages: thick outlines, pill buttons, chunky display type, floating planet cards, friendly astronaut/planet drawings, and saturated purple, blue, gold, pink, and cyan color.

Physical scene: a curious learner opens a weekend science-club web toy on a laptop. It feels like an animated space poster, but every panel still points back to source files, SI units, tests, and the browser WebAssembly artifact.

## Palette

Use OKLCH tokens in CSS rather than raw hex for the site surface.

- Outer world: saturated purple-to-blue page gradient.
- Hero shell: deep violet space card with thick cream outlines and cartoon shadows.
- Action accents: solar gold, moon orange, orbital cyan, mint green, star pink, and Mars red.
- Text: warm cream and pale lavender; never pure white.
- Lines: bold comic outlines on primary cards and buttons, softer cream orbit rings in the background.

This is a full-palette cartoon science identity, not the rejected technical-instrument direction and not a generic NASA dashboard.

## Typography

- Headings: rounded display sans, currently Fredoka, sized very large with playful stacking.
- Body: friendly readable sans, currently Nunito, for explanatory notes.
- Measurements/code: JetBrains Mono for formulas, file paths, constants, and telemetry because those values are actual program facts.

## Components for GitHub Pages

1. **Hero / Cartoon cockpit**
   - Project promise, “Launch simulator” action, docs/source actions, and status stickers.
   - The hero should feel like a self-contained illustrated poster inside a rounded app window.

2. **Cartoon orbit playground**
   - Inline SVG illustration with chunky Sun, planets, orbit paths, sparkles, and a tiny astronaut mechanic.
   - Must state that it is a playful map, not a physical scale poster.

3. **WASM viewport**
   - Embedded canvas with boot status, full-demo link, and control summary.
   - Base-path-safe asset loading for GitHub Pages (`/solar-system-simulator/`).

4. **Physics boundary panels**
   - Pair the serious physics engine with the playful cartoon layer.
   - Keep source paths and formulas visible so the design does not become decorative fluff.

5. **Planet sticker book**
   - Implemented bodies appear as colorful, clickable cards with parent and initialization metadata.

6. **Source evidence ledger**
   - Link every major claim to docs or source paths.
   - Prefer proof rows and route matrices over generic feature claims.

## Interaction principles

- The site should feel clickable and joyful before it explains every detail.
- Cartoon visuals must invite exploration, then nearby copy must name the real physics boundary.
- Motion is light and floaty: stars twinkle, planets drift, astronaut floats, reduced-motion respected.
- Loading states stay explicit; a blank canvas is a bug.
- Preserve accessible contrast on saturated dark backgrounds.

## Comment and documentation style

- Comments should answer: **what physical idea is this implementing? what units are expected? what approximation is being made?**
- Avoid line-by-line comments for obvious assignments.
- Public docs can be more explanatory than source comments; code comments should stay maintainable and close to tricky logic.
