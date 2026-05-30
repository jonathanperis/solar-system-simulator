# Science-Themed Site Enhancement Implementation Plan

> **For Hermes:** Use subagent-driven-development skill to implement this plan task-by-task.

**Goal:** Upgrade the Solar System Simulator GitHub Pages site from a simple demo landing page into a polished science-themed observatory, with Jonathan's common website footprints: richer footer credits and a dedicated Astro docs section that explains the codebase.

**Architecture:** Keep the existing Astro static site and WASM pipeline. Add a stronger shared layout, science-lab visual system, source-backed content pages, reusable Astro components, and generated or data-driven documentation surfaces without changing simulation physics in this pass.

**Tech Stack:** Astro static site, HTML/CSS, C/raylib source references, Emscripten WASM artifact, GitHub Pages workflow, existing Makefile checks.

---

## Planning Progress Snapshot

**Planning state as of:** 2026-05-30 21:57:03 UTC  
**Repository:** `/opt/data/github/jonathanperis/solar-system-simulator`  
**Remote:** `https://github.com/jonathanperis/solar-system-simulator.git`  
**Branch/ref inspected:** `main @ 89e2a4a`  
**Working tree state before writing this plan:** clean and synced with `origin/main` (`0 0`)  
**Working tree state after writing this plan:** one new local plan file only

| Track | Planned | Evidence checked | Implementation status | Progress |
|---|---:|---:|---|---:|
| Repo and Pages orientation | yes | yes | complete for roadmap | 100% |
| Impeccable design direction | yes | yes | brief captured in this plan | 100% |
| Visual system enhancement | yes | current CSS and local browser QA inspected | implemented | 100% |
| Footer and common footprints | yes | generated HTML and browser QA inspected | implemented | 100% |
| Dedicated Astro docs section | yes | generated routes and browser QA inspected | implemented | 100% |
| Source-backed code explainer content | yes | source-map, docs pages, and route smoke checked | implemented | 100% |
| Validation and deployment gates | yes | local native/WASM/docs checks passing | implemented locally; CI/deploy pending for pushed SHA | 90% |

**Overall implementation progress:** approximately 95%. Local implementation, route checks, full validation, and browser QA are complete. Commit, push, CI/deploy watching, and live route smoke remain.

## Verified Current Context

- The live site exists at `https://jonathanperis.github.io/solar-system-simulator/` and currently has five top-level routes: Lab, Physics, Body catalog, Source atlas, Pipeline.
- The homepage is coherent but visually simple: text-heavy hero, one plain current-scene panel, an embedded WASM frame, three summary cards, and a minimal footer.
- The current visual system is dark space themed with star dots, cyan and solar accents, pill navigation, and card panels.
- Impeccable context files already exist:
  - `PRODUCT.md`: physics-first C11/raylib learning lab, web demo as public lab bench.
  - `DESIGN.md`: observatory / mission-control lab, physics notebook, body catalog, source atlas, pipeline observatory.
- Current footer is minimal: one build sentence plus a GitHub link.
- Current Astro layout is `docs/src/layouts/BaseLayout.astro` and global styling is `docs/public/styles/global.css`.
- Current code truth includes `src/sim/`, `src/render/`, `src/app/`, `src/main.c`, tests, Makefile, and workflows.

## Impeccable Shape Brief

### Feature Summary

Build a science-themed public observatory site for a physics-first solar-system simulator. The homepage should feel like a calibrated research instrument: readable, source-backed, and visually memorable without becoming a graphics-first space poster.

### Primary User Action

A visitor should quickly understand that the simulator is a C/raylib physics lab, then choose one of three paths: run the WASM demo, study the physics model, or inspect how the code is organized.

### Design Direction

- **Register:** Brand surface for a technical codebase, with developer-docs architecture-page treatment.
- **Color strategy:** Full palette, not restrained. Keep deep-space neutrals, solar amber, scientific cyan, orbital violet, and muted Mars rock as named roles. Convert the CSS palette to OKLCH tokens instead of raw hex where practical.
- **Theme scene sentence:** A learner opens the project at night on a large monitor, looking for a calm observatory console that explains orbital mechanics while the simulated bodies move.
- **Anchor references:** NASA mission control telemetry boards, museum science exhibit labels, astronomical atlas plates, and an instrument workbench rather than arcade neon.
- **Do not use:** gradient text, decorative glassmorphism, side-stripe cards, repeated identical icon cards, generic neon cyberpunk, or a hero-metric template.

### Scope

- **Fidelity:** Production-ready static site enhancement.
- **Breadth:** Whole Pages surface, with homepage, footer, and a dedicated `/docs/` section.
- **Interactivity:** Static Astro routes plus existing embedded WASM demo. Optional lightweight local UI behaviors only if they remain accessible and do not require client framework bloat.
- **Time intent:** Polish until it is shippable and live-verified.

### Layout Strategy

- Transform the homepage into an observatory deck:
  - hero copy plus a visual orbit instrument instead of only a text panel;
  - a visible telemetry rail with current bodies, integrator, units, controls, and artifact status;
  - a source-backed evidence ledger previewing physics, catalog, code, tests, and deploy pipeline.
- Move from generic card grid to varied scientific modules:
  - orbit schematic;
  - equation strip;
  - body manifest strip;
  - code atlas preview;
  - pipeline runbook strip.
- Create a proper docs hub at `/docs/`, separate from the current `source-atlas/` page, to match Jonathan's common website footprint of a dedicated Astro docs section explaining everything on the code.

### Key States

- Default page loaded with WASM embed present.
- WASM loading or blank canvas state must show an explicit fallback/status explanation in the surrounding page.
- Mobile layout must preserve CTA order, route discoverability, and readable docs navigation.
- Reduced-motion users must not lose any content.
- Route 404s for accidental helper pages must remain absent.

### Content Requirements

- Common footer credits:
  - creator credit for Jonathan Peris;
  - built with C, raylib, Emscripten, Astro, GitHub Pages;
  - source links and repo link;
  - physics disclaimer that the model is deterministic and educational, not ephemeris-accurate;
  - current milestone/body list or compact mission status;
  - optional cross-site language such as “designed as a learning lab.”
- Dedicated Astro docs section:
  - `/docs/` hub page;
  - `/docs/architecture/` explaining repo boundaries;
  - `/docs/simulation-core/` explaining vectors, bodies, gravity, integration, constants, and units;
  - `/docs/rendering/` explaining raylib boundary, scale conversion, illustrative vs real scale, labels, trails;
  - `/docs/controls/` explaining runtime controls and camera model;
  - `/docs/build-and-web/` explaining native, tests, WASM, artifact validation, and Pages deploy;
  - `/docs/roadmap/` explaining body-by-body expansion without overpromising.
- Homepage should preview the docs section without duplicating all details.
- Existing pages should either remain as top-level marketing/proof routes or become redirects/aliases in navigation. Prefer adding `/docs/` while preserving existing routes to avoid breaking links.

## Roadmap Priorities

### P0: Preserve working pipeline and repo scope

No simulation changes in this enhancement pass unless a site bug requires a small supporting fix. The main deliverable is the Astro Pages surface.

### P1: Upgrade the shared visual system

Create a stronger visual foundation using reusable Astro partials and CSS tokens.

Likely files:
- Modify: `docs/src/layouts/BaseLayout.astro`
- Modify: `docs/public/styles/global.css`
- Create: `docs/src/components/OrbitInstrument.astro`
- Create: `docs/src/components/TelemetryRail.astro`
- Create: `docs/src/components/EvidenceLedger.astro`
- Create: `docs/src/components/FooterCredits.astro`
- Create: `docs/src/lib/site.ts`

Acceptance criteria:
- Navigation remains base-path-safe through `import.meta.env.BASE_URL`.
- Footer credits appear on all routes.
- CSS avoids banned patterns from Impeccable.
- Page remains readable at 360px, 768px, and desktop widths.

### P2: Rebuild homepage as a science observatory

Use the existing route as the public lab deck.

Likely files:
- Modify: `docs/src/pages/index.astro`
- Modify: `docs/public/styles/global.css`
- Reuse components from P1.

Acceptance criteria:
- Hero includes a visual orbit instrument or schematic, not only text.
- Homepage shows concrete telemetry: bodies, integrator, SI units, C/raylib, WASM artifact, controls.
- WASM section includes surrounding explanation and fallback guidance so a blank canvas does not make the site feel broken.
- Links lead to demo, source, docs, physics, and body catalog.

### P3: Add dedicated Astro docs section

Create a structured docs area that explains the code like the user's other sites.

Likely files:
- Create: `docs/src/pages/docs/index.astro`
- Create: `docs/src/pages/docs/architecture.astro`
- Create: `docs/src/pages/docs/simulation-core.astro`
- Create: `docs/src/pages/docs/rendering.astro`
- Create: `docs/src/pages/docs/controls.astro`
- Create: `docs/src/pages/docs/build-and-web.astro`
- Create: `docs/src/pages/docs/roadmap.astro`
- Create: `docs/src/components/DocsShell.astro`
- Modify: `docs/src/layouts/BaseLayout.astro`

Acceptance criteria:
- `/docs/` is a discoverable nav item.
- Each docs page explains source paths, responsibilities, key files, and verification commands.
- Content is source-backed by current files, not aspirational.
- Existing pages are not orphaned. Either link them from docs or position them as overview/proof routes.

### P4: Add source-backed data layer for site facts

Centralize repeated facts so the homepage, footer, body catalog, and docs do not drift.

Likely files:
- Create: `docs/src/lib/site.ts`
- Create: `docs/src/lib/bodies.ts`
- Optionally create: `docs/src/lib/sourceMap.ts`
- Modify: `docs/src/pages/body-catalog.astro`
- Modify: docs components that currently hardcode the same list of bodies.

Acceptance criteria:
- Body names and current milestone labels appear from one data source in docs pages.
- Source paths and commands are reusable.
- `npm run build --prefix docs` succeeds with no accidental route generation.

### P5: Strengthen validation gates

Extend static checks around the new route set.

Likely files:
- Modify or create: `tools/check_docs_routes.py`
- Modify: `Makefile` if adding a docs route check target is appropriate.
- Modify: `.github/workflows/build.yml` if the new checker should run in CI.

Acceptance criteria:
- Generated HTML contains required markers for homepage, footer credits, docs hub, architecture docs, simulation-core docs, rendering docs, controls docs, build-and-web docs, and roadmap docs.
- Link smoke covers root, docs root, one docs child, WASM HTML/JS/WASM artifacts, and existing routes.
- The build fails if a common-footprint footer marker disappears.

## Bite-Sized Implementation Tasks

### Task 1: Re-verify current branch and clean tree

**Objective:** Ensure implementation starts from the correct repository state.

**Files:** none

**Steps:**
1. Run:
   ```bash
   git status --short --branch
   git remote -v
   git rev-list --left-right --count HEAD...origin/main
   ```
2. Expected: `main`, origin points to `jonathanperis/solar-system-simulator`, and divergence is `0 0` unless new upstream commits require `git pull --ff-only`.
3. If clean, continue. If dirty, stop and inspect before editing.

### Task 2: Create shared site facts

**Objective:** Centralize site URLs, credits, technology labels, body list, and source map.

**Files:**
- Create: `docs/src/lib/site.ts`
- Create: `docs/src/lib/bodies.ts`
- Create: `docs/src/lib/sourceMap.ts`

**Implementation notes:**
- Export `siteName`, `repoUrl`, `authorName`, `techStack`, `currentMilestone`, `implementedBodies`, and `sourceSections`.
- Keep facts plain and stable. Do not parse C source in this task.

**Verification:**
- Run `npm run build --prefix docs` after importing at least one value in a temporary or next task page.

### Task 3: Extract footer credits component

**Objective:** Replace the minimal footer with a reusable common-footprint footer.

**Files:**
- Create: `docs/src/components/FooterCredits.astro`
- Modify: `docs/src/layouts/BaseLayout.astro`
- Modify: `docs/public/styles/global.css`

**Content:**
- Credit Jonathan Peris.
- Link to GitHub source.
- Mention C, raylib, Emscripten, Astro, GitHub Pages.
- Include educational physics disclaimer.
- Include current milestone/body count.

**Verification:**
- Build docs.
- Check generated root HTML contains `Jonathan Peris`, `raylib`, `Emscripten`, `Astro`, and `not an ephemeris` or equivalent wording.

### Task 4: Add docs navigation model

**Objective:** Add `/docs/` to the primary nav and create a reusable docs shell.

**Files:**
- Modify: `docs/src/layouts/BaseLayout.astro`
- Create: `docs/src/components/DocsShell.astro`
- Modify: `docs/public/styles/global.css`

**Verification:**
- Build docs.
- Confirm generated links are base-path-safe: `/solar-system-simulator/docs/`, not `/docs/` in production output.

### Task 5: Create `/docs/` hub

**Objective:** Add a dedicated docs section page that explains how to read the project.

**Files:**
- Create: `docs/src/pages/docs/index.astro`

**Content:**
- Codebase map.
- “Start here” paths for learner, C maintainer, physics reviewer, and deployment reviewer.
- Links to architecture, simulation core, rendering, controls, build-and-web, roadmap.

**Verification:**
- Build docs.
- Check `docs/dist/docs/index.html` for docs hub markers and route links.

### Task 6: Create architecture docs page

**Objective:** Explain source boundaries and why physics is separate from rendering.

**Files:**
- Create: `docs/src/pages/docs/architecture.astro`

**Content:**
- `src/sim/`: raylib-independent physics.
- `src/app/`: testable app helpers.
- `src/render/`: raylib drawing boundary.
- `src/main.c`: native/web loop ownership.
- `tests/`: C test binaries.
- `docs/`, `web/`, `tools/`, `.github/workflows/`: public site and pipeline.

**Verification:**
- Generated HTML contains every source path above.

### Task 7: Create simulation-core docs page

**Objective:** Explain the physics model from source-backed facts.

**Files:**
- Create: `docs/src/pages/docs/simulation-core.astro`

**Content:**
- SI units.
- `Vec3d` double precision.
- Newtonian point-mass acceleration.
- velocity-Verlet / kick-drift-kick.
- body initialization and parent-relative moons.
- limitations: fixed Sun, no relativity, no ephemeris.

**Verification:**
- Generated HTML includes source path links or text for `src/sim/physics.c`, `src/sim/solar_system.c`, and `src/sim/vec3d.c`.

### Task 8: Create rendering docs page

**Objective:** Explain the raylib boundary and visual scale choices.

**Files:**
- Create: `docs/src/pages/docs/rendering.astro`

**Content:**
- render-scale conversion.
- illustrative vs real-scale mode.
- labels and body trails.
- grid scaling.
- why rendering never mutates physics state.

**Verification:**
- Generated HTML includes `src/render/renderer.c`, `src/app/body_labels.c`, and `src/app/body_trails.c`.

### Task 9: Create controls docs page

**Objective:** Document app controls, camera focus, and runtime modes.

**Files:**
- Create: `docs/src/pages/docs/controls.astro`

**Content:**
- `Tab` or `C` for focus cycle.
- `V` for illustrative vs real scale.
- mouse wheel zoom.
- stable orbit camera and pitch behavior.
- current body focus order.

**Verification:**
- Generated HTML includes key names and camera terminology.

### Task 10: Create build-and-web docs page

**Objective:** Explain native build, test, WASM build, artifact validation, and Pages deploy.

**Files:**
- Create: `docs/src/pages/docs/build-and-web.astro`

**Content:**
- `make`, `make test`, `make web`, `make dist-wasm`.
- `tools/check_wasm_artifacts.py`.
- Emscripten and `web/shell.html`.
- Build workflow and Deploy Pages workflow relationship.

**Verification:**
- Generated HTML includes all commands and workflow file names.

### Task 11: Create roadmap docs page

**Objective:** Explain the body-by-body expansion model without claiming unimplemented bodies are already simulated.

**Files:**
- Create: `docs/src/pages/docs/roadmap.astro`

**Content:**
- Current implemented bodies.
- Future sequence: asteroid belt representatives, Jupiter, Galilean moons, Saturn, major Saturnian moons, Uranus, Neptune, dwarf planets / Kuiper belt representatives.
- Per-body acceptance template: constants, initialization, tests, rendering, docs, verification.

**Verification:**
- Generated HTML distinguishes current vs planned bodies.

### Task 12: Add visual orbit instrument component

**Objective:** Give the homepage a scientific visual centerpiece without relying on external images.

**Files:**
- Create: `docs/src/components/OrbitInstrument.astro`
- Modify: `docs/public/styles/global.css`
- Modify: `docs/src/pages/index.astro`

**Implementation notes:**
- Use semantic HTML plus CSS/SVG orbit rings.
- Show Sun, inner planets, Earth/Moon, Mars/Phobos/Deimos as schematic markers.
- Include a small caption that it is illustrative, not scale accurate.
- Respect `prefers-reduced-motion`.

**Verification:**
- Visual QA in browser confirms the hero is no longer text-only.
- Generated HTML includes the caption and body labels.

### Task 13: Add telemetry rail component

**Objective:** Make the site feel like a science instrument by surfacing real project state.

**Files:**
- Create: `docs/src/components/TelemetryRail.astro`
- Modify: `docs/src/pages/index.astro`
- Modify: `docs/public/styles/global.css`

**Telemetry items:**
- Current bodies.
- Integrator.
- Units.
- Build targets.
- Controls.
- Web artifact types.

**Verification:**
- Generated root HTML contains stable telemetry markers.

### Task 14: Add evidence ledger component

**Objective:** Replace plain card repetition with a source-backed ledger of proof paths.

**Files:**
- Create: `docs/src/components/EvidenceLedger.astro`
- Modify: `docs/src/pages/index.astro`
- Modify: `docs/public/styles/global.css`

**Ledger rows:**
- Physics model: source and tests.
- Body catalog: implemented data.
- Rendering boundary: source and docs.
- WASM pipeline: artifact checks.
- Astro docs: code explainer pages.

**Verification:**
- Root page links to both overview pages and `/docs/` detail pages.

### Task 15: Improve WASM embed presentation

**Objective:** Prevent the browser simulator area from feeling blank or unfinished when the canvas is white, loading, or delayed.

**Files:**
- Modify: `docs/src/pages/index.astro`
- Modify: `docs/public/styles/global.css`
- Optional modify: `web/shell.html` only if the shell needs clearer statuses.

**Implementation notes:**
- Add surrounding status copy and artifact links.
- Add fallback text below the iframe with direct “open full demo” link.
- Keep the iframe source base-path-safe.

**Verification:**
- Browser screenshot of the homepage shows an explicit simulator context even before the embedded canvas is visually useful.

### Task 16: Update existing overview routes to link into docs

**Objective:** Keep existing routes useful while making `/docs/` the deeper explainer section.

**Files:**
- Modify: `docs/src/pages/physics.astro`
- Modify: `docs/src/pages/body-catalog.astro`
- Modify: `docs/src/pages/source-atlas.astro`
- Modify: `docs/src/pages/pipeline.astro`

**Verification:**
- Existing routes still build.
- Each route links to at least one corresponding `/docs/` page.

### Task 17: Add docs route smoke checker

**Objective:** Make common footprint and docs routes durable.

**Files:**
- Create: `tools/check_docs_routes.py`
- Optional modify: `Makefile`
- Optional modify: `.github/workflows/build.yml`

**Checker requirements:**
- Assert generated HTML exists for root, physics, body-catalog, source-atlas, pipeline, docs hub, and all docs child routes.
- Assert footer credit markers exist on representative pages.
- Assert WASM links still reference `wasm/solar-system-simulator.html`.

**Verification:**
- Run checker against `docs/dist` after `npm run build --prefix docs`.

### Task 18: Full local validation

**Objective:** Verify native, WASM, and docs remain healthy.

**Commands:**
```bash
git diff --check
make test
make web
make dist-wasm
python3 tools/check_wasm_artifacts.py build/web
rm -rf docs/public/wasm
mkdir -p docs/public/wasm
cp build/web/solar-system-simulator.* docs/public/wasm/
npm ci --prefix docs
npm run build --prefix docs
python3 tools/check_docs_routes.py docs/dist
```

**Expected:** all commands exit `0`.

### Task 19: Browser visual QA

**Objective:** Inspect the redesigned Pages surface like a visitor.

**Steps:**
1. Start a local preview with the production base path.
2. Navigate to root, `/docs/`, one docs child page, and the WASM page.
3. Use screenshots or browser vision to check:
   - science theme is visible, not only verbal;
   - footer credits are present;
   - docs nav is discoverable;
   - no horizontal overflow on mobile;
   - reduced-motion content still makes sense.
4. Stop the preview process.

### Task 20: Commit, push, and verify deployment

**Objective:** Ship the site enhancement to `main` and confirm it is live.

**Commands:**
```bash
git status --short
git add docs tools Makefile .github/workflows .hermes/plans/2026-05-30_215703-science-themed-site-enhancement.md
git commit -m "docs: enhance science themed pages site"
git push origin main
```

**Deployment verification:**
- Watch Build workflow for the pushed SHA.
- Watch Deploy Pages workflow for the same SHA after Build succeeds.
- Live-smoke:
  - `/`
  - `/docs/`
  - `/docs/architecture/`
  - `/docs/simulation-core/`
  - `/docs/rendering/`
  - `/docs/controls/`
  - `/docs/build-and-web/`
  - `/docs/roadmap/`
  - existing routes
  - WASM `.html`, `.js`, `.wasm`

## Validation Gates

Required before reporting implementation complete:

- `git diff --check`
- `make test`
- `make web`
- `make dist-wasm`
- `python3 tools/check_wasm_artifacts.py build/web`
- `npm ci --prefix docs`
- `npm run build --prefix docs`
- docs route smoke checker after it is added
- browser visual QA for root and `/docs/`
- GitHub Actions Build success
- Deploy Pages success
- live route smoke success

## Risks and Tradeoffs

- **Blank WASM canvas perception:** If the embedded simulator appears white in browser screenshots, the site needs visible surrounding context and possibly a shell/status fix. Do not hide the demo. Make the state explainable.
- **Overdesign risk:** Science theme should communicate physics and source evidence, not become decorative space noise.
- **Docs drift risk:** Repeated body lists and source paths should be centralized in `docs/src/lib/` where possible.
- **Route bloat risk:** `/docs/` should be structured but not a second complete wiki with duplicate content. Top-level pages can remain concise overviews.
- **Base path risk:** Every internal link must remain compatible with `/solar-system-simulator/` on GitHub Pages.

## Reporting Template for Future Progress Updates

```md
## Solar System Simulator Site Enhancement Progress

**Overall roadmap progress:** N%

| Lane | Status | Progress | Notes |
|---|---|---:|---|
| Visual system | ... | ... | ... |
| Footer credits | ... | ... | ... |
| Docs section | ... | ... | ... |
| Homepage observatory | ... | ... | ... |
| Validation/deploy | ... | ... | ... |

**Completed this pass:**
- ...

**Evidence:**
- command/result or URL

**Next recommended move:**
- ...
```

## Acceptance Criteria

- Site looks like a richer science observatory/workbench, not a simple text page with cards.
- Common footer credits appear across the site and include creator, stack, source, and educational physics disclaimer.
- Dedicated `/docs/` Astro section exists and explains the codebase from source-backed facts.
- Homepage previews simulation, telemetry, docs, source, and pipeline depth without becoming cluttered.
- Existing routes keep working.
- WASM demo remains available.
- Build, native tests, WASM checks, docs build, CI, Pages deploy, and live route smoke all pass.
