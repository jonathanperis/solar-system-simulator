# SPEC

## §G

Physics-first C11/raylib solar-system simulator: inspectable SI orbital mechanics, deterministic tests, native + browser runtime, source-backed docs.

## §C

C1: simulator/runtime code C11 only.

C2: `src/sim/` raylib-independent; raylib ∈ `src/render/` | `src/main.c`.

C3: physical state uses double-precision SI units.

C4: fixed Sun heliocentric baseline until explicit barycentric task.

C5: expansion one body/concept per milestone.

C6: new behavior tests RED before implementation.

C7: GitHub Pages static output under `/solar-system-simulator/`; no SSR-only surface.

C8: public site follows archival solar-chart direction in `DESIGN.md`.

C9: current renderer baseline stays simple after beauty-pass rollback; no resurrection without explicit task.

C10: ⊥ ECS, scene format, asset manager, shader stack, ephemeris loader before concrete need.

## §I

I.cli: `make` → native app

I.test: `make test` → all C tests

I.run: `make run` → raylib app

I.web: `make web` → checked JS + WASM; Astro owns the runtime document and loader integration

I.dist: `make dist-wasm` → WASM zip

I.docs: `make docs-check` → generated route checks

I.atlas: homepage static SVG/DOM atlas; plate/body selection reachable by pointer, keyboard, touch

I.sim: `SolarSystem`, `Body`, `solar_system_create_*`, `solar_system_step`

I.app: body trails, stable orbit camera, bounded simulation stepping

I.render: illustrative | real-scale transforms + raylib drawing

I.controls: native `Tab` | `C` focus; web `C` focus and browser-native `Tab`; `V` scale; wheel zoom

I.inspection: native shortcuts and accessible web buttons share C-owned playback and selection; web readouts use live C physical state. Space pauses, N steps, R resets, A toggles camera rotation, F frames the selected system; native 1–9 selects a body and brackets change speed.

I.pages: `/`, `/docs/`, `/docs/architecture/`, `/docs/simulation-core/`, `/docs/rendering/`, `/docs/controls/`, `/docs/build-and-web/`, `/docs/roadmap/`, `/physics/`, `/body-catalog/`, `/source-atlas/`, `/pipeline/`, `/simulator/`; `/wasm/solar-system-simulator.html` redirects to `/simulator/`

I.ci: `Build` → native tests + WASM artifact; `Deploy Pages` consumes successful artifact

## §R

R1|4 Vesta class|numbered Main-belt Asteroid|https://ssd-api.jpl.nasa.gov/sbdb.api?sstr=4%20Vesta&phys-par=1&full-prec=1
R2|4 Vesta orbit|a=`2.361365965127599 AU`; e=`0.09020374382834395`; i=`7.143925545058711 deg`|https://ssd-api.jpl.nasa.gov/sbdb.api?sstr=4%20Vesta&phys-par=1&full-prec=1
R3|4 Vesta physical|GM=`17.2882844 km^3/s^2`; effective diameter=`522.77 km`|https://ssd-api.jpl.nasa.gov/sbdb.api?sstr=4%20Vesta&phys-par=1&full-prec=1
R4|target rationale|Vesta second-most-massive main-belt body; Ceres dwarf planet|https://science.nasa.gov/solar-system/asteroids/4-vesta/

## §V

V1: `src/sim/**` ∉ raylib headers, `Vector3`, draw/window APIs.

V2: position=m; mass=kg; time=s; velocity=m/s; acceleration=m/s²; simulation vectors=double.

V3: gravity = `G * source_mass / distance^3 * displacement`; self | zero-distance contribution=0.

V4: stepping = velocity-Verlet kick-drift-kick; fixed bodies contribute gravity but never move.

V5: shipped scene order = Sun, Mercury, Venus, Earth, Moon, Mars, Phobos, Deimos, Vesta; stable IDs + parents match catalog.

V6: planets + Vesta start heliocentric perihelion; Moon starts Earth-relative perigee; Phobos/Deimos start Mars-relative periareion; speeds use vis-viva.

V7: default no-inclination orbital motion ∈ X/Z plane; parent-relative Y position/velocity=0.

V8: app accumulates frame-scaled time and advances only in fixed 15-second physics steps; unconsumed time remains in the app clock. Equal accumulated time produces the same state regardless of frame partitioning.

V9: trails retain full-run temporal coverage and the current endpoint within 1025 visible points/body. History starts at a 300-second simulation-time cadence; each compaction doubles both historical spacing and future sampling cadence. All bodies share sample times for parent-relative rendering. Resolution coarsens uniformly during long runs; this is a history approximation, not a stored ephemeris or complete precomputed orbit.

V10: illustrative transforms affect render output only; asteroid radius=`0.03` render units; real-scale uses same physical scale for positions + radii with no radius clamp.

V11: camera focus covers ∀ bodies; wheel changes clamped distance only; pitch preserved.

V12: web `InitWindow()` dimensions derive from served `.canvas-wrap` before WebGL creation; canvas fills frame.

V13: ∀ Pages links/assets base-path-safe under `/solar-system-simulator/`.

V14: Pages deploy only after successful native tests + WASM artifact validation.

V15: each new-body milestone updates constants, initialization, tests, renderer visibility, app/catalog/docs, route checks, verification.

V16: public claims trace to source/tests; checked claims match implementation; loading/runtime failure always visible, never blank unexplained canvas.

V17: local/fork docs builds emit no analytics; deployed Pages build emits configured analytics only; public analytics disclosure exists.

V18: keyboard focus always visible; runtime status announces changes; interactive content uses valid semantic HTML. Web form controls retain Tab, arrow, Home/End, type-ahead, and native button activation despite Emscripten/GLFW window-level keyboard listeners; simulator shortcuts act only with canvas focus.

V19: checked WASM begins `\0asm\1\0\0\0`; docs checker resolves all internal routes/assets under configured base path.

V20: atlas visual scale/positions are explicitly illustrative; body names, parents, initialization, sources derive from `implementedBodies`; live motion claims link only to WASM runtime.

V21: atlas selection works without pointer; body controls expose selected state, drawer closes on `Escape`, focus returns to invoking body, reduced-motion suppresses ornamental motion.

V22: 100-day isolated Phobos/Deimos numerical checks compare orbital phase against an analytical Kepler solution (less than 1 degree error); the nine-body scene compares the app step with half-sized reference steps (less than 1% parent-relative position discrepancy).

V23: pause freezes simulation time and trails without accumulating paused wall time; single-step advances exactly 15 simulated seconds only while paused. Speed presets (1 hour, 1 day, 5 days per real second) change accumulated time, never the physics step. Reset restores initial physics, trails, and clock remainder while preserving selection, playback settings, and presentation settings.

V24: inspector distance and speed are relative to the identified parent in SI state, independent of render mode; parentless bodies show unavailable relative measurements. Framing is renderer-only: planet plus direct moons, a moon's parent plus siblings, or all bodies for the Sun; fit respects aspect ratio and physical/illustrative radii.

## §A — Runtime accuracy repair, 2026-09-09

id|criterion|verify
A1|At the reported 86.83-day duration, circular Mercury-like history retains distributed coverage, bounded chord error, synchronized parent/child samples, and the exact current endpoint; storage and drawing remain bounded|`test_body_trails`, `test_renderer`
A2|Fixed-step app clock is frame-partition independent; 100-day orbital phase and full-scene convergence satisfy V22|`test_simulation_step`
A3|Body colors are not covered by universal orange wireframes; the grid is subdued; SI state and render-only size policy remain separate|native/WASM build, renderer tests, headless visual inspection
A4|Astro runtime uses shared chrome, loads JS/WASM successfully under Pages base path, exposes focus/view state and visible load errors; existing HTML URL redirects successfully|docs route checker, WASM checker, Astro type-check, headless browser
A5|Source-backed docs describe fixed stepping, uniformly coarsened trails, and Astro runtime ownership; main is committed/pushed and Pages deployment is verified at its commit SHA|source scan, Build/Deploy Pages runs, public browser verification

Engineering decisions: Jonathan authorized all reported repairs and Astro integration on 2026-09-09, then explicitly confirmed `Main + Pages` deployment. The 15-second step and 100-day/one-degree target are engineering accuracy targets for this repair, not claims of ephemeris fidelity. Preserve the existing full-run history scope with honest uniformly decreasing resolution rather than silently switching to a recent-only trail.

## §A — Inspection and controls, 2026-09-10

id|criterion|verify
A6|Pause/resume, reset, speed presets and paused single-step preserve V8,V9,V23; camera rotation is independently switchable|session C tests, browser interactions
A7|Direct selection, view and zoom controls work through native shortcuts and touch/keyboard-accessible web controls; frame-system fits the selected family; Tab and form keys keep native browser behavior outside the canvas|camera/renderer C tests, web integration tests, desktop/mobile browser checks
A8|Live inspector shows selected name, parent, relative distance/speed, mass and physical radius from C with explicit units and unavailable parentless values; docs explain controls and reset semantics; verified main revision deploys to Pages|session/inspector C tests, web integration tests, build/route checks, CI and deployed revision

Decision: Jonathan approved the three proposed inspection enhancements with “go”, then authorized task-local `npm ci`, existing raylib 6.0 reuse, isolated headless local browser verification, and Main + Pages delivery. Jupiter, barycentric physics, and longer-period numerical work remain separate milestones.

## §T

id|status|task|cites
T1|x|build Sun foundation + SI physics|V1,V2,V3,V4,I.sim
T2|x|add Mercury milestone|V5,V6,V15
T3|x|add Venus + scale modes + focus|V5,V6,V10,V11,V15
T4|x|add Earth + stable camera zoom|V5,V6,V11,V15
T5|x|add Moon parent-relative milestone|V5,V6,V15
T6|x|add Mars milestone|V5,V6,V15
T7|x|add Phobos + Deimos + bounded substeps|V5,V6,V7,V8,V15
T8|x|consolidate body metadata; retire unused generated labels|V5,I.app
T9|x|ship native CI + WASM + Astro Pages pipeline|V12,V13,V14,I.web,I.ci
T10|x|upgrade static docs to Astro 7|C7,I.pages
T11|x|fix moon planes + substep trail sampling + bounded trail drawing|V7,V8,V9
T12|x|ship softened cockpit site + docs manual + WASM shell|C8,V13,V16
T13|x|revert renderer overhaul; retain responsive WASM frame|C9,V10,V12
T14|x|add 4 Vesta asteroid milestone: sourced constants, planar heliocentric perihelion state, nine-body scene, distinct render visibility, full docs/test surface|C5,C6,V5,V6,V7,V10,V15,V16
T15|.|add Jupiter milestone|C5,C6,V15
T16|.|add Galilean moons milestone|C5,C6,V15
T17|.|add Saturn milestone|C5,C6,V15
T18|.|add major Saturnian moons milestone|C5,C6,V15
T19|.|add Uranus milestone|C5,C6,V15
T20|.|add Neptune milestone|C5,C6,V15
T21|.|add dwarf-planet / Kuiper-belt representative milestone|C5,C6,V15
T22|x|bound full-run trail storage + draw cost; remove allocation abort path|V8,V9
T23|x|align README/site claims with runtime; remove dead label surface; add constants provenance|V16
T24|x|harden docs/WASM checks + premerge docs gate; add build provenance|V13,V14,V16,V19,I.docs,I.ci
T25|x|fix docs/runtime accessibility, metadata, deploy-only analytics disclosure|C7,C8,V17,V18
T26|x|refresh docs dependency tree; add Dependabot coverage|C7,V16,I.ci
T27|x|ship archival solar-chart atlas: full-screen accessible homepage plates, source-backed body drawer, engraved route system, matched WASM frame, route/check coverage|C7,C8,V13,V16,V18,V20,V21,I.atlas,I.pages,I.web
T28|x|repair trail retention and fixed-step accuracy using RED tests and 100-day analytical/convergence checks|A1,A2,V8,V9,V22
T29|x|subdue grid and remove orange wireframes; host runtime in Astro and preserve old links|A3,A4,I.web,I.pages,V10,V12,V13,V18
T30|x|update documentation, verify all affected boundaries, regression scan, commit/push main and deploy Pages|A5,V14,V16
T31|x|add C-owned playback, selection, physical inspector, and renderer-only system framing with RED tests|A6,A7,A8,V23,V24
T32|x|integrate accessible web controls, native shortcuts, state bridge, and source-backed documentation|A6,A7,A8,I.inspection
T33|~|verify native/WASM/docs/browser boundaries, regression scan, commit/push main, and verify Pages|A8,V14,V16

## §B

id|date|cause|fix
B1|2026-06-23|Phobos/Deimos relative velocity on Y → vertical trails|V7
B2|2026-06-23|trails sampled per render frame, not physics substep → polygon chords|V8
B3|2026-06-24|full trail history drawn without bound → long-run render cost|V9
B4|2026-06-25|CSS canvas frame ≠ hardcoded `InitWindow(1280,720)` → gutters|V12
B5|2026-06-25|renderer beauty pass broke expected scene/viewport → rollback|V10,V12
B6|2026-09-02|unbounded full-sample trails + draw cap too high → long-run memory/frame-cost growth|V9
B7|2026-09-02|public docs claimed easing, HUD labels, WASM magic check absent from code|V16,V19
B8|2026-09-02|WASM copy claimed unbounded trail history despite bounded decimation|V9,V16
B9|2026-09-02|atlas client selectors lacked typed DOM bindings|V21
B10|2026-09-09|repeatedly thinning old samples while recording new ones at full resolution erased early orbital curvature; endpoint-only straight-line tests missed it|V9,A1
B11|2026-09-09|one-orbit radius bounds did not detect long-run Phobos phase drift from a five-minute step|V8,V22,A2
B12|2026-09-10|broad window-capture filtering protected canvas shortcuts but prevented native select arrow/Home navigation; scope interception to GLFW-cancelled keys and give semantic selects an explicit tested navigation path|V18,A7
