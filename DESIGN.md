# Solar System Simulator Design Context

Decided 2026-10-07 (SPEC A99–A105). Replaces the archival solar-chart atlas.

## Idea

The simulator is the product, so it is the homepage. The site has two
surfaces with different jobs:

- **Instrument** (`/`): the live C simulation, full viewport. People came to
  watch and steer orbits; nothing stands between them and the canvas.
- **Notebook** (`/learn/`, `/catalog/`, `/about/`): short, well-typeset pages
  that explain, list and source what the instrument does.

## What this site is not

The overhaul exists because the old site said too much. Avoid the patterns
that make a page read as generated:

- no hero banner, tagline or "features" section;
- no grids of cards, icons in circles, emoji, gradients, glass or glow;
- no marketing adjectives ("powerful", "seamless", "immersive");
- no paragraph of copy where a label, number or link does the job;
- no duplicated explanations: each fact lives on one page, others link to it.

Copy is plain and specific. Prefer "15 s fixed step, velocity-Verlet" to a
sentence about accuracy. Numbers carry units.

## Instrument (`/`)

- The canvas fills the viewport under a thin top bar: wordmark (links home),
  Learn, Catalog, About, Source.
- One bottom dock, left to right: play/pause, speed (1 h, 1 d, 5 d, 10 d,
  15 d per second), scene date, scene picker (Solar system, Jupiter, Saturn,
  Uranus, Neptune, Pluto, Didymos), body search.
- Selecting a body opens a compact inspector on the right (bottom sheet on
  phones): name, parent, distance, speed, period, mass, radius, data quality,
  and two actions (Frame system, Frame body).
- View settings (scale, trails, vectors, grid, labels) and keyboard help are
  small popovers from the dock. Lessons add a one-line strip over the canvas.
- Under 150 words of visible copy. Loading, errors and status changes are
  always visible and announced.

Look: near-black surround, hairline borders, one amber accent for the active
control and selection, mono labels in small caps, tabular numbers. Panels are
opaque, not translucent.

## Notebook pages

- Warm paper background, one column, about 65 characters per line.
- Serif headings, readable body text, mono for data, code and units.
- Data goes in tables with units in the header. Sources are footnote-style
  links next to the claim they support.
- In-page index at the top of long pages (About). No sidebars.
- Every body name links to the simulator at that body (`/?body=slug`).

## Palette and type

- Instrument: background `oklch(14% 0.01 260)`, panel `oklch(19% 0.012 260)`,
  hairline `oklch(32% 0.012 260)`, text `oklch(92% 0.01 90)`, accent amber
  `oklch(78% 0.13 75)`.
- Notebook: paper `oklch(96% 0.015 85)`, ink `oklch(24% 0.02 260)`, rule
  `oklch(84% 0.02 85)`, the same amber for links and focus.
- Type: Cormorant Garamond for notebook headings, the system UI font for
  notebook body text and instrument labels, JetBrains Mono for numbers, code
  and data tables. All fonts self-hosted.

## Interaction

- Keyboard first: every dock control is reachable by Tab; canvas shortcuts
  (Space, N, R, F, B, V, [ ]) work when the canvas has focus; `?` opens help.
- Visible focus rings in amber. Minimum target 24 px (44 px on touch).
- `prefers-reduced-motion` stops camera auto-rotation and UI transitions.
- Popovers and sheets close with Escape and return focus to their opener.

## Boundaries

- Runtime controls and physical readouts stay C-owned; the page never
  computes physics.
- Old URLs keep working through Astro redirect pages that preserve
  `?body=` and fragments.
- Static Astro output under `/solar-system-simulator/`, CSP without inline
  code, base-path-safe links.
