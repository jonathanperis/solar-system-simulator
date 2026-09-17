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

I.lab: `make headless` → `build/solar-lab`; scene/duration/dt/sample/integrator/initial-speed options stream reproducible SI CSV; `--catalog` exposes the C body manifest.

I.export: native E and web Export SI snapshot use the same C `solar-lab-v1` CSV writer as headless series, including revision, configuration, physical-data quality, ticks and SI measurements.

I.test: `make test` → all C tests

I.run: `make run` → raylib app

I.web: `make web` → checked JS + WASM; Astro owns the runtime document and loader integration

I.dist: `make dist-wasm` → WASM zip

I.docs: `make docs-check` → generated route checks

I.atlas: homepage static SVG/DOM atlas; plate/body selection reachable by pointer, keyboard, touch

I.sim: `SolarSystem`, `Body`, `solar_system_create_*`, `solar_system_step`

I.app: body trails, stable orbit camera, bounded simulation stepping

I.render: illustrative | real-scale transforms + raylib drawing; Saturn rings are presentation-only geometry

I.controls: native `Tab` | `C` focus; web `C` focus and browser-native `Tab`; `V` scale; wheel zoom

I.inspection: native shortcuts and accessible web buttons share C-owned playback and selection; web readouts use live C physical state. Space pauses, N steps, R resets, A toggles camera rotation, F frames the selected system, B frames only the selected body; native 1–9 and 0 select the first ten catalog bodies and brackets change speed. Search/group selection reaches the full catalog.

I.pages: `/`, `/docs/`, `/docs/architecture/`, `/docs/simulation-core/`, `/docs/rendering/`, `/docs/controls/`, `/docs/build-and-web/`, `/docs/roadmap/`, `/docs/experiments/`, `/physics/`, `/body-catalog/`, `/source-atlas/`, `/pipeline/`, `/simulator/`, `/compare/`; `/wasm/solar-system-simulator.html` redirects to `/simulator/`

I.ci: `Build` runs native tests, WASM validation, and docs/dependency checks. `Deploy Pages` consumes the exact successful artifact and commit from a same-repository `main` push or manual run. `CodeQL` scans C/C++, JavaScript/TypeScript, and Actions on PRs, main pushes, weekly schedules, and manual dispatch.

## §R

R1|4 Vesta class|numbered Main-belt Asteroid|https://ssd-api.jpl.nasa.gov/sbdb.api?sstr=4%20Vesta&phys-par=1&full-prec=1
R2|4 Vesta orbit|a=`2.361365965127599 AU`; e=`0.09020374382834395`; i=`7.143925545058711 deg`|https://ssd-api.jpl.nasa.gov/sbdb.api?sstr=4%20Vesta&phys-par=1&full-prec=1
R3|4 Vesta physical|GM=`17.2882844 km^3/s^2`; effective diameter=`522.77 km`|https://ssd-api.jpl.nasa.gov/sbdb.api?sstr=4%20Vesta&phys-par=1&full-prec=1
R4|target rationale|Vesta second-most-massive main-belt body; Ceres dwarf planet|https://science.nasa.gov/solar-system/asteroids/4-vesta/
R5|Jupiter physical|mass=`1898.125 × 10^24 kg`; mean radius=`69911 km`|https://ssd.jpl.nasa.gov/planets/phys_par.html
R6|Jupiter orbit|J2000 a=`5.20288700 AU`; e=`0.04838624`; i=`1.30439695 deg`|https://ssd.jpl.nasa.gov/planets/approx_pos.html
R7|Jovian inventory|115 recognized satellites, including provisional designations; checked 2026-09-10|https://science.nasa.gov/jupiter/moons/
R8|Jovian mean elements|115 entries, J2000 epoch, Laplace/ecliptic reference frames; shape/orientation baseline, not ephemerides; checked 2026-09-10|https://ssd.jpl.nasa.gov/sats/elem/
R9|Satellite physical data|available GM and mean-radius values with source/quality metadata; absent values remain unknown; checked 2026-09-10|https://ssd.jpl.nasa.gov/sats/phys_par/
R10|Saturn physical|mass=`568.317 × 10^24 kg`; mean radius=`58232 km`|https://ssd.jpl.nasa.gov/planets/phys_par.html
R11|Saturn orbit|J2000 a=`9.53667594 AU`; e=`0.05386179`; i=`2.48599187 deg`|https://ssd.jpl.nasa.gov/planets/approx_pos.html
R12|Saturn rings|ring system extent is roughly `282000 km`; axial tilt=`26.73 deg`; rings are modeled only as presentation geometry|https://science.nasa.gov/saturn/facts/
R13|Sandboxed browser CI|Playwright supports Ubuntu 22.04; Ubuntu 23.10+ AppArmor restrictions can prevent downloaded Chromium from starting its user-namespace sandbox. Pin the docs/browser job to supported 22.04 while retaining sandbox/TLS checks; checked 2026-09-16|https://playwright.dev/docs/intro#system-requirements ; https://chromium.googlesource.com/chromium/src/+/main/docs/security/apparmor-userns-restrictions.md

## §V

V1: `src/sim/**` ∉ raylib headers, `Vector3`, draw/window APIs.

V2: position=m; mass=kg; time=s; velocity=m/s; acceleration=m/s²; simulation vectors=double.

V3: gravity = `G * source_mass / distance^3 * displacement`; self | zero-distance contribution=0.

V4: core/catalog stepping = velocity-Verlet kick-drift-kick; isolated learning presets may explicitly select Euler. Fixed bodies contribute gravity but never move.

V5: core scene starts Sun through Jupiter, followed by the 115 Jovian moons, Saturn, Uranus and Neptune (128 bodies). Existing IDs/indices remain stable; outer planets use NAIF center IDs 699/799/899. The complete small-body catalog is separate from the active scene; selected experiments use Sun/eight planets plus at most 16 selected records at one source epoch.

V6: planets + Vesta start heliocentric perihelion; Moon starts Earth-relative perigee; Phobos/Deimos start Mars-relative periareion; speeds use vis-viva.

V7: legacy no-inclination initial states remain in X/Z. Jovian moon initial states preserve sourced inclination, orbital direction, eccentricity and phase; source frames convert into the common simulation frame before adding Jupiter's absolute position and velocity.

V8: app accumulates frame-scaled time and advances only in fixed physics steps (15 seconds for core/catalog; explicitly configured for lessons); unconsumed time remains in the app clock. Per-frame work is bounded to keep controls responsive. Once pending work is drained, equal accumulated time produces the same state regardless of frame partitioning; requested and achieved speeds are distinguished. Hidden/minimized wall time is excluded.

V9: trails retain full-run temporal coverage and the current endpoint within 1025 visible points/body. History starts at a 300-second simulation-time cadence, rounded up to whole configured lesson ticks; each compaction doubles both historical spacing and future sampling cadence. All bodies share sample times for parent-relative rendering. Resolution coarsens uniformly during long runs; this is a history approximation, not a stored ephemeris or complete precomputed orbit.

V10: illustrative transforms affect render output only; asteroid radius=`0.03` render units; real-scale uses same physical scale for positions + known radii with no radius clamp. Unknown-radius wire markers are the explicitly labeled render-only exception described by V25. Saturn's ring lines and their framing extent are renderer-only and never replace its physical mean radius.

V11: camera focus covers ∀ bodies; wheel changes clamped distance only; pitch preserved.

V12: web `InitWindow()` dimensions derive from served `.canvas-wrap` before WebGL creation; canvas fills frame.

V13: ∀ Pages links/assets base-path-safe under `/solar-system-simulator/`.

V14: Pages deploy only after successful native tests and WASM validation from this repository's `main` branch, triggered by a push or explicit manual run. Fork/PR-origin code must never execute with deployment write permissions; a matching branch name alone is not trusted origin.

V15: each new-body milestone updates constants, initialization, tests, renderer visibility, app/catalog/docs, route checks, verification.

V16: public claims trace to source/tests; checked claims match implementation; loading/runtime failure always visible, never blank unexplained canvas.

V17: local/fork docs builds emit no analytics; deployed Pages build emits configured analytics only; public analytics disclosure exists.

V18: keyboard focus always visible; runtime status announces changes; interactive content uses valid semantic HTML. Web form controls retain Tab, arrow, Home/End, type-ahead, and native button activation despite Emscripten/GLFW window-level keyboard listeners; simulator shortcuts act only with canvas focus.

V19: checked WASM begins `\0asm\1\0\0\0`; docs checker resolves all internal routes/assets under configured base path.

V20: atlas visual scale/positions are explicitly illustrative; body names, parents, initialization, sources derive from `implementedBodies`; live motion claims link only to WASM runtime.

V21: atlas selection works without pointer; body controls expose selected state, drawer closes on `Escape`, focus returns to invoking body, reduced-motion suppresses ornamental motion.

V22: 100-day isolated Phobos/Deimos numerical checks compare orbital phase against an analytical Kepler solution (less than 1 degree error); the default core scene compares the app step with half-sized reference steps (less than 1% parent-relative position discrepancy).

V23: pause freezes simulation time and trails without accumulating paused wall time; single-step advances one configured tick (15 seconds for core/catalog) only while paused. Speed presets (1 hour, 1 day, 5 days, 10 days, 15 days per real second) change accumulated time, never the physics step. Reset restores initial physics, trails, clock remainder, ticks, diagnostic baseline and achieved-speed measurement while preserving selection, playback/lesson settings, and presentation settings.

V24: inspector distance and speed are relative to the identified parent in SI state, independent of render mode; parentless bodies show unavailable relative measurements. Framing is renderer-only: planet plus direct moons, a moon's parent plus siblings, or all bodies for the Sun; fit respects aspect ratio, physical/illustrative radii, and Saturn's visible ring extent.

V25: mass and radius have explicit measured/estimated/unknown provenance. Unknown mass uses a zero-gravitational-mass test particle that feels known-source gravity without backreaction. Unknown physical values display as Unknown, never as measured zero; an unknown-radius marker is explicitly render-only in either view.

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

## §A — Jupiter milestone, 2026-09-10

id|criterion|verify
A9|Jupiter is the tenth body after Vesta with stable ID, Sun parent, JPL-sourced mass/radius/orbit, planar heliocentric perihelion state, and vis-viva speed; the fixed 15-second app step remains unchanged|solar-system and simulation-step C tests
A10|Jupiter is selectable by native `0`, cycle controls, and the C-populated web dropdown; inspector, trails, illustrative/real-scale rendering, family framing, atlas, catalog, controls, and source-backed docs expose the ten-body scene without changing existing indices|session/renderer C tests, docs tests/checks, native/WASM builds, browser verification

Decision: Jonathan approved the Jupiter plan on 2026-09-10 and authorized task-local `npm ci`, existing raylib reuse, isolated browser verification, direct commit/push to `main`, Pages deployment, and exact deployed-revision verification. Galilean moons, orbital inclinations, barycentric physics, and longer-period numerical work remain separate milestones.

## §A — Complete current-planet moons and faster playback, 2026-09-10

id|criterion|verify
A11|Five speed presets include 10 and 15 days/second; bounded playback retains pending time, pause/reset/step semantics and fixed-step determinism; achieved speed is visible|session/step tests and native/WASM throughput measurements
A12|All 115 Jovian moons are in a reproducible source-cited catalog and the 125-body scene, with stable identities, Jupiter parent, common-frame orbital initialization and explicit physical-data quality|catalog generation check, table-driven satellite/orbital/physics tests and full-scene convergence
A13|All bodies are reachable through grouped/searchable selection and Jupiter atlas groups; family and individual framing, bounded synchronized trails, inspector quality labels, docs and native/web controls match the C model|renderer/session/docs checks, authorized desktop/mobile browser verification
A14|Native/WASM accuracy and performance verification, regression scan and source-backed docs pass; approved main delivery has successful Build/Pages workflows and exact deployed revision|build/test/sanitizer/artifact checks, CI and public verification

Decision: Jonathan requested both faster presets and all moons of existing planets. On 2026-09-10 he selected “Yes, label approximations” for source-backed estimates and otherwise massless particles/Unknown readouts, then approved the complete 115-moon/125-body plan with “do it”. The plan includes scoped orbital geometry, native/WASM performance work, browser verification and release/deployed-revision verification. This expands T16 beyond the four Galilean moons. Task-local dependency setup and existing raylib reuse follow the approved delivery workflow.

## §A — Saturn milestone, 2026-09-10

id|criterion|verify
A15|Saturn is the 126th body at stable index 125 with explicit ID 699, Sun parent, JPL-sourced mass/radius/orbit, planar heliocentric perihelion state, and vis-viva speed; indices 0–124 and the fixed 15-second step remain unchanged|solar-system, satellite, and simulation-step C tests
A16|Saturn is searchable/selectable through native and C-populated web controls; inspector, reset, trails, color, physical/illustrative rendering, ring-aware framing, atlas, catalog, and source-backed docs match the C model|session/trail/renderer C tests, docs tests/checks, native/WASM builds
A17|Saturn's tilted rings are renderer-only; the 126-body scene remains below 1% half-step discrepancy and retains throughput above 15 simulated days/second|renderer/state tests, 100-day convergence, native throughput benchmark

Decision: Jonathan selected the Saturn-only milestone rather than combining Saturn with its moon system, then approved implementation with “do it” on 2026-09-10. He separately authorized task-local `npm ci` for the locked Astro dependencies. Commit, push, deployment, and browser interaction were not authorized. T18 now targets the complete Saturnian inventory; its source reconciliation remains separate.

## §A — Complete small-body atlas, 2026-09-14

id|criterion|verify
A18|The pinned catalog accounts for every asteroid-kind or CEN/TNO record, including provisional designations, without duplicates or silent missing-data omissions|full offline shard/index/density/checksum audit and importer tests
A19|Uranus and Neptune append at indices 126/127 with sourced mass/radius/orbit; prior IDs and core convergence remain valid|outer-planet, solar-system, satellite and full-scene tests
A20|The C conic kernel covers elliptic, near-parabolic, parabolic and hyperbolic orbits with independent reference/conservation proofs; WASM previews use that same kernel|C orbit tests and standalone WASM test
A21|Search/filter/pagination and density mapping expose all catalog records while bounding client memory and HTML/result size; source epochs/quality and active-versus-catalog counts remain explicit|full-catalog worker tests, docs/build/route checks, size audit
A22|Native/web explicit experiments share a bounded parser, source-epoch planetary states, owned names, identity/quality rules and deterministic reset; failed loads preserve the prior scene|experiment/session tests and artifact bridge checks
A23|Camera-relative double subtraction preserves distant local detail and grid/trail work remains bounded|renderer tests
A24|Native/WASM/docs verification, sanitizer and throughput checks, source audit and regression review pass|verification evidence

Decision: Jonathan selected all asteroids plus outer bodies, catalog plus selected simulation, and Uranus/Neptune foundations, then approved autonomous implementation. The 1,564,244-entry snapshot is source-accounted; 4,076 other comet records are outside the chosen scope. Catalog experiments use Sun/eight planets plus up to 16 selected bodies at JD 2461200.5 TDB. They remain a fixed-Sun point-mass approximation. Task-local locked Astro dependency installation and existing raylib archive reuse were explicitly authorized.

## §A — Audit completion and learning laboratory, 2026-09-16

id|criterion|verify
A25|Retain and verify the repairs already present on 1ad41c2: failure-propagating tests, header rebuilds, fixed stepping, bounded catch-up, uniform trails, live controls, contrast and visible runtime errors; close remaining build dependency and atlas bearing gaps|Make contract checks, existing C/JS suites, local browser interactions
A26|Small C-owned circular, eccentric, escape, barycentric Earth–Moon and inclined lessons expose a controlled initial-speed factor, Verlet/Euler comparison, physical diagnostics and reset; the core/catalog defaults retain their fixed 15-second Verlet policy|lesson/session tests, independent convergence and free-pair conservation checks
A27|A raylib-free C runner accepts scene, duration, timestep, sampling and integrator options; native/web snapshot export and headless series share a CSV contract with SI state, configuration and revision; invalid inputs fail visibly|CLI/CSV integration tests, native/WASM comparison
A28|Native/web render controls expose parent-relative history and optional physical velocity/acceleration directions without modifying SI state; inspector reports acceleration, energy change, timestep/ticks and visual magnification honestly|renderer/session tests, browser controls
A29|C and TypeScript core catalogs are cross-checked; artifact validators have focused failure tests; Build has read-only permissions and sanitizer verification; analytics build/check settings agree and Pages deploys the already checked artifact with stale-run protection|manifest comparison, Python checker tests, workflow inspection, native/WASM/docs checks
A30|A guided experiments route and contributor instructions explain prediction, configuration, measurement, numerical/model/render errors, ownership, provenance, native/WASM reproduction and tool prerequisites|docs checks, runnable documented examples
A31|Focused tests, complete C/docs/catalog/WASM verification, authorized local browser interaction, and regression scan pass; all audit items are accounted for as existing fixes, delivered changes, or explicit verification limitations|verification report and regression scan

Decision: Jonathan requested “work on everything” in the audit on 2026-09-16 and then authorized task-local locked dependency installation, raylib 6.0 build/reuse, and isolated local browser tests. Remote main has advanced from the audited 54a5fdc to 1ad41c2; implementation preserves its 128-body scene and pinned catalog and completes remaining audit work. Existing SPEC.md remains the authoritative tracked specification; .specs/ is already ignored. Newly discovered catalog roadmap items retain their existing milestone scope. Browser tests target only the task-local site and simulator controls/downloads; fault-path verification uses local test fixtures.

V26: lesson physics and diagnostics are C-owned. Euler is an explicitly labeled teaching comparison; core and source-epoch scenes continue using Verlet. Changing lesson parameters creates a fresh initial state, clock and diagnostic baseline; failed configuration leaves the prior experiment intact.

V27: exports contain physical SI coordinates, simulation ticks, step/method/scene parameters and source revision, never illustrative positions. CSV strings are escaped. Headless output streams with bounded memory and a fixed timestep; duration/sample boundaries must align to whole ticks. New native CSV files use explicit owner-read/write POSIX permissions independently of the process umask.

V28: total energy includes massive-body kinetic energy and each gravitational pair once. Massless tracers contribute no total energy/momentum. Momentum conservation is asserted only for free systems; parent-relative specific energy is labeled a two-body diagnostic.

V29: parent-relative history subtracts synchronized historical parent positions and anchors at the current parent; physics and stored history are immutable while rendering. Vector glyphs encode direction with an explicitly illustrative length.

## §A — Comparison school and advanced lessons, 2026-09-16

id|criterion|verify
A32|A C-owned comparison runs identical initial conditions with separate timestep/integrator/collision settings, reports only matched simulation checkpoints, stays responsive through bounded work, and exports both series with run configuration|comparison/config C tests, CLI replay, browser controls
A33|Live bounded plots show energy change, radius, speed, position discrepancy and analytical phase error where available; overlaid SI trajectories and accessible tabular values distinguish matched checkpoints from in-progress integration|comparison numerical tests, docs/browser tests
A34|A versioned bounded lesson descriptor saves/imports through files and shareable URLs; native CLI and browser use the same C validation and reset the same experiment; invalid configuration never replaces a valid run|config/CLI tests, URL/file browser round trips
A35|Guided challenges connect predictions, controlled runs and measured outcomes; force-contribution inspectors expose source acceleration vectors/magnitudes and clearly define percentages of summed magnitudes|force decomposition tests, challenge/UI tests
A36|Explicit new lessons provide a moving-Sun barycentric core, a perturbed 3:2 period-ratio experiment with resonant-angle diagnostics, a close Earth encounter, and head-on elastic/merging spheres; defaults and original core/catalog semantics remain intact|initial-state, conservation, collision and convergence tests
A37|Pinned project Playwright tests run against the built local site and in CI, covering comparison/configuration, export, mobile selection and visible failure states while retaining the browser sandbox and TLS validation|local automated browser suite and GitHub checks
A38|Native/WASM/docs checks, numerical evidence, regression review and source-backed learning docs pass; both local iterations are committed and pushed on the feature branch, and CI is observed|verification report and branch CI checks

Decision: Jonathan approved all six proposed enhancements and the four later physics topics with “do at all”. He then explicitly authorized pinned `@playwright/test` 1.63.0, isolated local Chrome and Chromium installation in CI, plus commit/push/PR creation and CI observation for both rounds. Merge/production deployment awaits review. Continue the existing task checkout/branch and preserve prior work.

Delivery amendment: after GitHub denied PR creation with the current credential, Jonathan explicitly selected “Finish branch and CI only”. The authorized delivery is the pushed feature branch with observed CI; no PR or merge is required.

I.compare: `/compare/` loads a separate C-only lab WASM module; `solar-lab --compare FILE` runs the same descriptor headlessly. Main simulator retains its raylib runtime and exposes the new presets and force inspector.

V30: A/B runs share initial physical state, compare the same identified subject at common checkpoint times, and never substitute mismatched integration times. Duration/sample spacing must align to each run's ticks. Incremental work and chart history are bounded; full CLI output streams. A missing merged subject is unavailable, never silently replaced by another body.

V31: configuration format `SOLAR_LAB_V1` contains a preset name, initial-speed factor, two methods/timesteps/contact policies, sample spacing and duration. It is a lesson descriptor, not an arbitrary scene format. Parse/validate before replacing a run. Export revision accompanies results; saved links identify their source revision without executing on navigation.

V32: collision response is opt-in for the two-sphere classroom preset only. Positive known masses/radii, head-on initial motion and a small fixed timestep bound prevent tunneling within this lesson's allowed speed factors. Elastic impulses conserve momentum/kinetic energy at contact; merging conserves mass and linear momentum, combines volume, and explicitly changes mechanical energy. No claim of general collision or internal-spin modeling.

V33: barycentric initialization unfixes and translates all core states by their mass-weighted position/velocity while preserving relative state. A period ratio alone is not called proven resonance: the experiment exposes the resonant angle and explains libration versus circulation. Close encounters remain a fixed-step accuracy experiment, not an adaptive solver.

V34: browser JavaScript formats/plots C measurements and descriptors; integration, collision response, force decomposition and analytical reference calculations remain in C. Force percentages divide each source magnitude by the sum of source magnitudes, not by the magnitude of the net vector.

V35: moving stars record synchronized history like other moving bodies; only fixed stars omit history. Barycentric parent-relative trails subtract historical Sun positions rather than an assumed fixed origin.

## §A — Security PR integration, 2026-09-17

A39|PR #8 preserves the checked Pages artifact, source/checksum and stale-run guards, sandboxed browser lane and current SDK while adding trusted-origin/event restrictions, job-scoped deployment permissions, immutable raylib revisions, nonpersistent checkout credentials, dependency auditing and CodeQL security/quality coverage|trusted/untrusted event matrix, workflow structure checks, Build and CodeQL CI

Decision: Jonathan authorized merging PR #8 after PR #9 reached main, then explicitly approved rebasing its branch onto main, resolving conflicts, and pushing with an exact force-with-lease. Preserve the additional CodeQL security-and-quality commit observed at the branch head. CodeQL's manual C build includes the newly added headless entry point. The root specification remains tracked and .specs/ remains ignored.

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
T15|x|add Jupiter milestone: sourced constants, planar heliocentric perihelion state, ten-body scene, selection/render/catalog/docs integration, verification and Pages delivery|A9,A10,C5,C6,V5,V6,V15
T16|x|add all 115 Jovian moons from a reproducible catalog, orbital geometry, data-quality model and five speed presets|A11,A12,V5,V7,V8,V23,V25
T17|x|add Saturn as the 126th body with sourced perihelion state, renderer-only rings, controls/catalog/docs integration, and verification|A15,A16,A17,C5,C6,V5,V6,V10,V15,V24
T18|.|add complete Saturnian moon catalog after reconciling the NASA/JPL inventory|C5,C6,V15
T19|x|add Uranus milestone|A19,C5,C6,V15
T20|x|add Neptune milestone|A19,C5,C6,V15
T21|x|add complete small-body atlas, conic geometry and selected source-epoch experiments|A18,A20,A21,A22,A23,A24,C5,C6,V15
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
T33|x|verify native/WASM/docs/browser boundaries, regression scan, commit/push main, and verify Pages|A8,V14,V16
T34|x|integrate full-catalog selection/atlas, individual framing, inspector quality and high-speed responsiveness|A11,A13,V9,V18,V20,V24,V25
T35|x|verify complete moon scene, performance, regressions and main/Pages delivery|A14,V14,V16,V22
T36|x|verify existing audit repairs and close build/test dependency gaps with headless and sanitizer entrypoints|A25,A29
T37|x|add C lesson presets, integrator comparison, scientific diagnostics and independent numerical proofs|A26,V26,V28
T38|x|add reproducible headless runner and shared native/web CSV export with provenance|A27,V27
T39|x|integrate lesson controls, diagnostics, parent-relative history and vector presentation in native/web runtime|A26,A28,V26,V29
T40|x|repair atlas bearing interaction, visible disclosure and remaining accessibility gaps; exercise loader failure states|A25,A31,V18,V21
T41|x|cross-check catalogs, harden validators and deliver checked Pages artifacts with consistent analytics policy|A29
T42|x|publish guided learning exercises, build/ownership/provenance documentation, and verify all affected boundaries|A30,A31
T43|x|implement descriptor validation, matched-checkpoint comparison, bounded C telemetry and force decomposition|A32,A33,A34,A35,V30,V31,V34
T44|x|add barycentric core, resonance, encounter and collision lessons with independent physical proofs|A36,V32,V33
T45|x|integrate comparison WASM/native CLI, live plots, trajectory overlays, save/share/import and guided challenges|A32,A33,A34,A35,V34
T46|x|add sandboxed pinned automated browser checks to local tooling and CI; update all learning/control/provenance docs|A37,A38
T47|x|run verification/regression review, commit and push both rounds, and observe branch CI|A38
T48|x|reconcile and verify PR #8's security controls and analysis coverage against the learning-lab pipeline|A39,V14

Verification: Build run https://github.com/jonathanperis/solar-system-simulator/actions/runs/35175509807 passed native tests/sanitizers, WASM packaging, complete catalog checks, docs validation and sandboxed browser tests for implementation commit 016f187. PR creation was denied by the credential; branch-plus-CI delivery follows Jonathan's explicit amendment above.

Security integration verification: Build https://github.com/jonathanperis/solar-system-simulator/actions/runs/35243274760 and CodeQL https://github.com/jonathanperis/solar-system-simulator/actions/runs/35243274774 passed for ee35fba. The local policy matrix passed eight trusted/untrusted event cases; runtime/docs sources match the verified learning-lab main.

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
B13|2026-09-14|browser fetch transparently decoded gzip responses before client hashing, so compressed-file hashes rejected valid catalog data|manifest schema 2 hashes both gzip and decoded JSON; clients verify the received form and decompress at most once
B14|2026-09-16|configurable lesson steps did not always divide the 300-second trail cadence, and fractional boundary roundoff could postpone a sample by a whole tick|align cadence to whole configured ticks and tolerate only floating-point boundary roundoff; session tests cover 200-second and 1.1-second steps
B15|2026-09-16|the old all-stars trail exclusion became invalid when the barycentric lesson released the Sun, corrupting parent-relative history|record and draw moving stars, omit only fixed stars, and verify the moving-parent transform
B16|2026-09-16|managed Chromium could not initialize its sandbox under the ubuntu-latest AppArmor user-namespace policy|pin the docs/browser lane to supported Ubuntu 22.04 and retain chromiumSandbox=true
B17|2026-09-17|a privileged workflow_run deployment checked success and branch name but not source ownership/event, allowing fork PR code to cross the deployment boundary|verify same repository, main branch, allowed event, successful build, and exact triggering commit/artifact under V14
