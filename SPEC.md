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

C8: public site follows `DESIGN.md`. The 2026-10-07 overhaul (A99–A105) replaces the archival-atlas direction with a simulator-first instrument and lab-notebook pages.

C9: the renderer follows the explicit 2026-10-07 cinematic task (A64–A68); further visual overhauls need their own explicit task. The June beauty-pass rollback (B5) stays the cautionary baseline: change rendering in verified, screenshot-checked increments and never alter the viewport/canvas contract (V12).

C10: ⊥ ECS, scene format, asset manager, general shader stack, ephemeris loader before concrete need. Exception (A64–A66): one lighting shader pair (GLSL 330 native / GLSL 100 WebGL) and a fixed, attributed texture set.

C-catalog: regenerated small-body shards ⊥ committed to `main` history; next refresh moves shard delivery to hash-pinned release assets first. `small_body_catalog.py --generate` refuses to replace the tracked catalog without `--replace-tracked-catalog`.

## §I

I.cli: `make` → native app

I.lab: `make headless` → `build/solar-lab`; scene/duration/dt/sample/integrator/initial-speed options stream reproducible SI CSV; `--scene` accepts lessons and the astronomy scenes (`core`, `jupiter-system`, `saturn-system`, `uranus-system`, `neptune-system`, `pluto-system`, `didymos-system`, `patroclus-system`); `--catalog [SCENE]` exposes the C body manifest of the main or a planet-system scene; session CSV carries `contact_sphere_crossed`, and comparison CSV carries `contact_sphere_crossed_a/b`.

I.export: native E and web Advanced → Download measurements (CSV) use the same C `solar-lab-v1` CSV writer as headless series, including revision, configuration, physical-data quality, ticks and SI measurements.

I.test: `make test` → all C tests

I.run: `make run` → raylib app

I.web: `make web` → checked JS + WASM; Astro owns the runtime document and loader integration

I.dist: `make dist-wasm` → WASM zip

I.docs: `make docs-check` → generated route checks


I.sim: `SolarSystem`, `Body`, `solar_system_create_*`, `solar_system_step`

I.app: body trails, stable orbit camera, bounded simulation stepping

I.render: illustrative | real-scale transforms + raylib drawing; lighting shader pair, attributed textures (lazy-loaded on the web), IAU spin orientation, adaptive grid and faded trails are presentation only; Saturn rings are presentation-only geometry

I.controls: native `Tab` | `C` focus; web `C` focus and browser-native `Tab`; `V` scale; wheel zoom; native `K` cycles the astronomy scenes and `L` the guided lessons; the web dock's Scene picker loads the main or a planet-system scene, the Find sheet searches bodies in the active scene, and `?body=` links to a small moon load its planet-system scene first

I.inspection: native shortcuts and accessible web buttons share C-owned playback and selection; web readouts use live C physical state. Space pauses, N steps, R resets, A toggles camera rotation, F frames the selected system, B frames only the selected body; native 1–9 and 0 select the first ten catalog bodies and brackets change speed. Search/group selection reaches the full catalog.

I.pages: `/` (simulator), `/learn/`, `/learn/compare/`, `/catalog/`, `/catalog/small-bodies/`, `/about/`, `/404.html`; retired URLs (`/simulator/`, `/compare/`, `/body-catalog/`, `/small-bodies/`, `/physics/`, `/source-atlas/`, `/pipeline/`, `/docs/…`) and `/wasm/solar-system-simulator.html` are noindex Astro redirect pages that keep the query and fragment

I.ci: `Build` runs native tests, WASM validation, and docs/dependency checks; the informative macOS build and shader render runs in its own `macOS` workflow so it never gates a deploy (B55). `Deploy Pages` consumes the exact successful artifact and commit from a same-repository `main` push or manual run. `CodeQL` scans C/C++, JavaScript/TypeScript, and Actions on PRs, main pushes, weekly schedules, and manual dispatch.

## §R

R1|4 Vesta class|numbered Main-belt Asteroid|https://ssd-api.jpl.nasa.gov/sbdb.api?sstr=4%20Vesta&phys-par=1&full-prec=1
R2|4 Vesta orbit|a=`2.361365965127599 AU`; e=`0.09020374382834395`; i=`7.143925545058711 deg`|https://ssd-api.jpl.nasa.gov/sbdb.api?sstr=4%20Vesta&phys-par=1&full-prec=1
R3|4 Vesta physical|GM=`17.2882844 km^3/s^2`; effective diameter=`522.77 km`|https://ssd-api.jpl.nasa.gov/sbdb.api?sstr=4%20Vesta&phys-par=1&full-prec=1
R4|target rationale|Vesta second-most-massive main-belt body; Ceres dwarf planet|https://science.nasa.gov/solar-system/asteroids/4-vesta/
R5|Jupiter physical|planet-only GM=`126686531.900 km^3/s^2` (mass = GM/G); mean radius=`69911 km`; checked 2026-10-07|https://ssd.jpl.nasa.gov/api/horizons.api?format=text&COMMAND='599'&OBJ_DATA='YES'&MAKE_EPHEM='NO' ; https://ssd.jpl.nasa.gov/planets/phys_par.html
R6|Jupiter orbit|J2000 a=`5.20288700 AU`; e=`0.04838624`; i=`1.30439695 deg`|https://ssd.jpl.nasa.gov/planets/approx_pos.html
R7|Jovian inventory|115 recognized satellites, including provisional designations; checked 2026-09-10|https://science.nasa.gov/jupiter/moons/
R8|Jovian mean elements|115 entries, J2000 epoch, Laplace/ecliptic reference frames; shape/orientation baseline, not ephemerides; checked 2026-09-10|https://ssd.jpl.nasa.gov/sats/elem/
R9|Satellite physical data|available GM and mean-radius values with source/quality metadata; absent values remain unknown; checked 2026-09-10|https://ssd.jpl.nasa.gov/sats/phys_par/
R10|Saturn physical|planet-only GM=`37931206.234 km^3/s^2` (mass = GM/G); mean radius=`58232 km`; checked 2026-10-07|https://ssd.jpl.nasa.gov/api/horizons.api?format=text&COMMAND='699'&OBJ_DATA='YES'&MAKE_EPHEM='NO' ; https://ssd.jpl.nasa.gov/planets/phys_par.html
R11|Saturn orbit|J2000 a=`9.53667594 AU`; e=`0.05386179`; i=`2.48599187 deg`|https://ssd.jpl.nasa.gov/planets/approx_pos.html
R12|Saturn rings|ring system extent is roughly `282000 km`; axial tilt=`26.73 deg`; rings are modeled only as presentation geometry|https://science.nasa.gov/saturn/facts/
R13|Sandboxed browser CI|Playwright supports Ubuntu 22.04; Ubuntu 23.10+ AppArmor restrictions can prevent downloaded Chromium from starting its user-namespace sandbox. Pin the docs/browser job to supported 22.04 while retaining sandbox/TLS checks; checked 2026-09-16|https://playwright.dev/docs/intro#system-requirements ; https://chromium.googlesource.com/chromium/src/+/main/docs/security/apparmor-userns-restrictions.md
R14|Simulator textures|Solar System Scope 2k maps (Sun, planets, Moon, Saturn ring alpha) and 8k Milky Way, CC BY 4.0, re-encoded 2026-10-07; attribution in site footer and assets/textures/README.md|https://www.solarsystemscope.com/textures/
R15|Spin orientation|IAU WGCCRE 2015 rotation elements (pole alpha0/delta0, prime meridian W(d)); periodic terms omitted except Neptune's N|Archinal et al. 2018, Celest Mech Dyn Astr 130:22, https://doi.org/10.1007/s10569-017-9805-5
R16|Image decoder|stb_image v2.30 (MIT / public domain) pinned at nothings/stb 2c980bb, SHA-256 594c2fe3…; raylib builds with JPEG disabled|https://github.com/nothings/stb
R17|Uranus/Neptune physical|planet-only GM=`5793950.6103` and `6835099.97 km^3/s^2` (mass = GM/G); checked 2026-10-07|https://ssd.jpl.nasa.gov/api/horizons.api?format=text&COMMAND='799'&OBJ_DATA='YES'&MAKE_EPHEM='NO' ; https://ssd.jpl.nasa.gov/api/horizons.api?format=text&COMMAND='899'&OBJ_DATA='YES'&MAKE_EPHEM='NO'
R18|Giant-planet satellite mean elements|JPL table checked 2026-10-07: Saturn 291 rows (24 Laplace-frame SAT441/SAT415, 267 ecliptic SAT455–SAT459; 234 provisional; 210 retrograde; max e=0.909; all epoch 2000-01-01.5 TDB); Uranus 30 rows for 29 moons (Puck appears in both URA182 "equatorial" and URA184 Laplace), epochs 2000-01-01.5/2020-01-01.0/2025-01-01.0, the equatorial rows carry no pole columns and the Laplace rows give pole RA/Dec 77.3°/15.2° with tilt 180°; Neptune 16 rows, epochs 2000-01-01.5/2020-01-01.0, Laplace (Triton i=157.3°) and ecliptic|https://ssd.jpl.nasa.gov/sats/elem/
R19|Giant-planet satellite physical data|GM values: Saturn 16, Uranus 5, Neptune 8 satellites (Jupiter 9); others have no GM and become test particles under V25; checked 2026-10-07|https://ssd.jpl.nasa.gov/sats/phys_par/
R20|Recognized moon counts|NASA, checked 2026-10-07: Saturn page states both 274 (summary) and 293 "as of August 2026" (body); Uranus 29 "as of August 2026"; Neptune 16. Counts are context, not the simulated inventory|https://science.nasa.gov/saturn/moons/ ; https://science.nasa.gov/uranus/moons/ ; https://science.nasa.gov/neptune/moons/
R21|Pluto system|PLU060 equatorial elements for Charon, Styx, Nix, Kerberos and Hydra (epoch 2000-01-01.5); GM Charon 106.1±0.3, Nix 0.0015±0.0005, Hydra 0.0020±0.0003, Kerberos and Styx upper limits only; Pluto planet GM 869.326±0.4 km^3/s^2, radius 1188.3 km (Horizons 999, Brozovic & Jacobson 2024); orbit from SBDB 134340 solution 1; IAU WGCCRE 2015 positive pole RA 132.993°, Dec −6.163°; checked 2026-10-07|https://ssd.jpl.nasa.gov/sats/elem/ ; https://ssd.jpl.nasa.gov/sats/phys_par/ ; https://ssd.jpl.nasa.gov/api/horizons.api?format=text&COMMAND='999'&OBJ_DATA='YES'&MAKE_EPHEM='NO'
R22|Didymos system|Horizons DART s547 reconstruction: Dimorphos (120065803) osculating ecliptic J2000 elements about the Didymos primary (920065803) at 2024-01-01 TDB (a=1.1734 km, e=0.0279, i=170.72°), GM_Dimo ~3.0268e-10 and GM_Didy ~3.51278e-8 km^3/s^2 (approximate, no sigma), triaxial radii; system orbit from SBDB 65803 solution 240; checked 2026-10-07|https://ssd.jpl.nasa.gov/api/horizons.api?format=text&COMMAND='120065803'&OBJ_DATA='YES'&MAKE_EPHEM='NO' ; https://ssd-api.jpl.nasa.gov/sbdb.api?sstr=65803
R23|Other asteroid satellites|JPL's satellite tables list no asteroid moons. Horizons serves exactly two asteroid-satellite ephemerides: Didymos (DART reconstruction, R22) and Patroclus (R24); probes of the NAIF satellite codes of 18 other binaries (Kalliope, Sylvia, Eugenia, Ida, Dinkinesh, Eurybates, Elektra, Camilla, Kleopatra, Daphne, Minerva, Hermione, Dionysus, Polymele, …) return no body. The SBDB API's `sat=1` lists their satellites with literature orbits (e.g. Sylvia's Romulus and Remus, Fang et al. 2012; Eugenia's Petit-Prince, Marchis et al. 2010; both epoch 2004): mean elements without a current phase, not an ephemeris this project can pin. Corrected 2026-10-08 (the 2026-10-07 check missed Patroclus)|https://ssd.jpl.nasa.gov/api/horizons.api?format=text&COMMAND='120000617'&OBJ_DATA='YES'&MAKE_EPHEM='NO' ; https://ssd-api.jpl.nasa.gov/sbdb.api?sstr=87&sat=1
R24|Patroclus system|Horizons asteroid-satellite solution JPL#82 (solution date 2023-06-01; source tnosat_v001_20000617_jpl082): primary 920000617 GM 0.0740606 km^3/s^2, RAD 56.5 km; Menoetius 120000617 GM 0.020917 km^3/s^2, RAD 52 km (no uncertainties: estimates); osculating elements about the primary are unavailable ("required masses not defined"), so the catalog converts the 2024-01-01 TDB state about GM_P + GM_M (a = 692.5 km, e = 0.006, i = 152.5° to the ecliptic, P = 4.30 d; published ~4.28 d); system orbit from SBDB 617 solution 87 (epoch JD 2461200.5); epoch states from Horizons `617;` and `120000617 @920000617`; checked 2026-10-08|https://ssd.jpl.nasa.gov/api/horizons.api?format=text&COMMAND='920000617'&OBJ_DATA='YES'&MAKE_EPHEM='NO' ; https://ssd-api.jpl.nasa.gov/sbdb.api?sstr=617

## §V

V1: `src/sim/**` ∉ raylib headers, `Vector3`, draw/window APIs.

V2: position=m; mass=kg; time=s; velocity=m/s; acceleration=m/s²; simulation vectors=double.

V3: gravity = `G * source_mass / distance^3 * displacement`; self | zero-distance contribution=0.

V4: core/catalog stepping = velocity-Verlet kick-drift-kick; isolated learning presets may explicitly select Euler. Fixed bodies contribute gravity but never move.

V5: the main scene (`core`) holds the large bodies in this order: Sun, Mercury, Venus, Earth, Moon, Mars, Phobos, Deimos, Vesta, Jupiter, Io, Europa, Ganymede, Callisto, Saturn, Mimas, Enceladus, Tethys, Dione, Rhea, Titan, Iapetus, Uranus, Ariel, Umbriel, Titania, Oberon, Miranda, Neptune, Triton, Pluto, Charon (32 bodies; indices 0–9 unchanged since the Jupiter milestone). Every other moon lives only in its primary's family scene: the Sun, Mercury–Neptune (indices 1–8), Pluto, Didymos or Patroclus at 9 when that is the primary, then the complete catalog, major moons first (Jupiter 124, Saturn 300, Uranus 38, Neptune 25, Pluto 15, Didymos 11, Patroclus 11 bodies). Moons outside a binary companion (more than 1% of the primary's mass: Charon) start around the pair's barycenter with its total mass. Body IDs are stable JPL/NAIF codes; outer planets use NAIF center IDs 699/799/899. The array holds the largest scene (300). The complete small-body catalog is separate from the active scenes; selected experiments use Sun/eight planets plus at most 16 selected records at one source epoch.

V6: astronomy scenes start at JD 2461200.5 TDB (A92): each family barycenter (parent plus known-mass direct moons; massless test particles weigh nothing) takes its Horizons system-barycenter state and each dated moon its Horizons parent-relative state; undated moons keep mean elements. Guided lessons start from the legacy `*_at_perihelion` states with vis-viva speeds. Every massive planet's mass is GM / CODATA 2018 G: DE440 values for the Sun through Mars, planet-only JPL Horizons values for Jupiter–Neptune (R5, R10, R17); system GMs, which include moons, are never used. Scene planets with moons carry J2 (A94); lesson bodies are point masses.

V7: simulation axes are the proper rotation (x, y, z) = ecliptic (X, Z, −Y); +Y is ecliptic north and prograde angular momentum points to +Y, so prograde orbits draw counterclockwise from above. Lesson perihelion states stay in the X/Z plane; dated scene states carry their real inclinations. Mean-element moons convert their source frames through `orbit.c`, the only conic solver, before the parent's absolute state is added.

V8: app accumulates frame-scaled time and advances only in fixed physics steps (15 seconds for core/catalog; explicitly configured for lessons); unconsumed time remains in the app clock. Per-frame work is bounded to keep controls responsive. Once pending work is drained, equal accumulated time produces the same state regardless of frame partitioning; requested and achieved speeds are distinguished. Hidden/minimized wall time is excluded, and so is any single frame delta above `SOLAR_APP_STALL_FRAME_SECONDS` (1 s) or a negative/non-finite one: sleep and debugger stalls are discarded like a background resume. Frames at or below 1 s are never dropped.

V9: trails retain full-run temporal coverage and the current endpoint within 1025 visible points/body. History starts at a 300-second simulation-time cadence, rounded up to whole configured lesson ticks; each compaction doubles both historical spacing and future sampling cadence. All bodies share sample times for parent-relative rendering. Resolution coarsens uniformly during long runs; this is a history approximation, not a stored ephemeris or complete precomputed orbit.

V10: illustrative transforms affect render output only; asteroid radius=`0.03` render units; real-scale uses same physical scale for positions + known radii with no radius clamp. Unknown-radius wire markers are the explicitly labeled render-only exception described by V25. Saturn's ring lines and their framing extent are renderer-only and never replace its physical mean radius. Lighting, textures, atmosphere, clouds, glow, backdrop, faded grid/trails and IAU spin orientation are presentation only (A64–A68): they read SI state and never write it, and CSV/inspector values are identical with or without textures.

V11: camera focus covers ∀ bodies; wheel changes clamped distance only; pitch preserved.

V12: web `InitWindow()` dimensions derive from served `.canvas-wrap` before WebGL creation; canvas fills frame.

V13: ∀ Pages links/assets base-path-safe under `/solar-system-simulator/`. Self-hosted fonts and the page CSP use base-path-safe URLs; pages make no third-party requests except analytics under V17.

V14: Pages deploy only after successful native tests and WASM validation from this repository's `main` branch, triggered by a push or explicit manual run. Fork/PR-origin code must never execute with deployment write permissions; a matching branch name alone is not trusted origin.

V15: each new-body milestone updates constants, initialization, tests, renderer visibility, app/catalog/docs, route checks, verification.

V16: public claims trace to source/tests; checked claims match implementation; loading/runtime failure always visible, never blank unexplained canvas. Recoverable runtime stderr is logged, never fatal; only abort, non-zero exit, script load failure, missing WebGL or context loss disable the runtime.

V17: local/fork docs builds emit no analytics; deployed Pages build emits configured analytics only; public analytics disclosure exists. The page CSP allows Google Analytics hosts only in builds with `PUBLIC_GA_ID`.

V18: keyboard focus always visible; runtime status announces changes; interactive content uses valid semantic HTML. Web form controls retain Tab, arrow, Home/End, type-ahead, and native button activation despite Emscripten/GLFW window-level keyboard listeners; simulator shortcuts act only with canvas focus. Visible text is at least 12 px and no page scrolls sideways at 320 px; the simulator canvas is `role=application` with a described keyboard model.

V19: checked WASM begins `\0asm\1\0\0\0`; docs checker resolves all internal routes/assets under configured base path. Every generated HTML document is rendered by an Astro page: the route checker requires the Astro generator marker on each, a leading CSP meta without unsafe-inline/unsafe-eval and without wildcard sources other than Google's documented GA4 subdomains in `connect-src`/`img-src` (analytics builds only), no inline script/style/handlers, and no HTML, sitemap or robots file in `docs/public`; the sitemap is an Astro endpoint.

V20: retired with the homepage atlas (A105). Body names, parents and scenes on the catalog still derive from `implementedBodies`.

V21: retired with the homepage atlas (A105). Simulator sheets are native dialogs: `Escape` closes them and focus returns to the opening button.

V22: 100-day isolated Phobos/Deimos numerical checks compare orbital phase against an analytical Kepler solution (less than 1 degree error); the main scene compares the app step with half-sized reference steps over 100 days and each family scene over 20 days (less than 1% parent-relative position discrepancy).

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

V36: a comparison side or headless run contains at most `SOLAR_LAB_MAX_TICKS` = 10⁹ fixed ticks. Experiment and descriptor files are read by one bounded reader that rejects oversized or NUL-containing input. Headless `--output` accepts only a missing path or an existing regular file and replaces it atomically after a complete run (new files 0600, existing mode kept); a failed run leaves the previous file or none. Native snapshot export creates the next free `solar-snapshot-NNN.csv` exclusively and never replaces an existing file.

V37: catalog search cost is proportional to the query: digits-only queries route by manifest ID range; the full-index download is stated and needs user confirmation; one confirmed scan builds a bounded in-memory columnar index (≤2.5 M rows, about 60 MB) reused for the session; record lookup runs in the worker with an LRU of ≤3 data shards; SHA-256 verification is mandatory. Fetch URLs come only from manifest file names matching the pinned shard pattern, resolved inside the site's own `catalog/` directory; the worker accepts only dedicated-worker or same-origin messages that pass strict structural validation.

V38: browser session data is untrusted: a prepared experiment is validated and its UTF-8 text kept below `SOLAR_EXPERIMENT_TEXT_BYTES` before `ccall`.

V39: lesson speed factors are at least `lesson_minimum_velocity_factor` (two-body closest approach ≥ sum of radii, rounded up to 0.01) and at most 2; native, web and headless inputs respect the C limit. Orbital lessons and catalog experiments run a conservative swept check: once any step's straight-line drift crosses a contact sphere (possibly a coarse-step artifact), analytical lesson errors and the maximum phase error are withheld and CSV/inspector show `contact_sphere_crossed`. Scene capacity is compile-time checked and appends are bounded. Render-scale policy lives in `src/render/`.

V40: catalog experiments start the eight planets from Horizons system barycenters at JD 2461200.5 TDB (199/299 for moonless Mercury/Venus, 3–8 otherwise) with DE440 system GMs, because their moons are absent; `tools/planet_epoch.py --check` enforces the IDs and targets. Experiment rows are bounded (mass ≤ 1e23 kg, radius ≤ 2,400 km, q ≤ 1,000 AU). Shipped WebAssembly carries no debug sections (`WEB_CFLAGS`, artifact checker), every emcc rule uses the required C flags, and the browser runtime links without Asyncify.

## §A — Security PR integration, 2026-09-17

A39|PR #8 preserves the checked Pages artifact, source/checksum and stale-run guards, sandboxed browser lane and current SDK while adding trusted-origin/event restrictions, job-scoped deployment permissions, immutable raylib revisions, nonpersistent checkout credentials, dependency auditing and CodeQL security/quality coverage|trusted/untrusted event matrix, workflow structure checks, Build and CodeQL CI

Decision: Jonathan authorized merging PR #8 after PR #9 reached main, then explicitly approved rebasing its branch onto main, resolving conflicts, and pushing with an exact force-with-lease. Preserve the additional CodeQL security-and-quality commit observed at the branch head. CodeQL's manual C build includes the newly added headless entry point. The root specification remains tracked and .specs/ remains ignored.

## §A — Documentation currency audit, 2026-09-17

id|criterion|verify
A40|README, authored site routes, shared source/milestone metadata and maintainer context describe the current scene, lessons, trail policy, source provenance and checked-artifact delivery; build instructions include all runtime companions|source-to-claim review, offline catalogs, native CLI examples, Astro tests/type-check/build and generated route checks
A41|The sitemap lists every canonical generated page, including the small-body atlas; route validation detects missing or stale entries|focused validator regression test and `make docs-check`
A42|GitHub About has an accurate description, live Pages homepage and relevant topics, with remote values read back after the authorized gh update|`gh repo view` and public build provenance

Decision: Jonathan requested a repository-wide documentation audit, an evidence-backed enhancement/update plan and a GitHub About enhancement through gh, then approved proceeding with “go”. This iteration updates documentation and its sitemap verification on a feature branch. The existing root specification remains authoritative and tracked; .specs/ remains ignored.

Audit baseline: `eb6abd9ac7034103b309b38bdbe3c28a47a134cd`. On 2026-09-17, the public `wasm/build-info.json` reports this same revision and its Build, CodeQL and Deploy Pages runs are successful. Review covers all tracked Markdown, all 16 canonical Astro pages plus the compatibility redirect, shared page copy/navigation, source catalogs, Make/package commands and workflow ownership.

Finding|Evidence|Update
Outdated project context|PRODUCT.md still lists nine bodies; DESIGN.md omits the Jupiter plate|Align current capabilities and source boundaries with the 128-body core, pinned atlas and learning lab
Incomplete browser build recipe|README manually copies only the main JS/WASM pair; Makefile packages five runtime files plus build-info.json|Use the validated docs-assets entrypoint and document preview/contributor checks
Legacy generated HTML collision|An ignored local public/wasm HTML file shadows the Astro-owned redirect and fails the existing route check|Document relocation outside public assets when upgrading an older checkout; preserve the observed file as local audit evidence
Obsolete delivery summary|pipeline route says Deploy Pages rebuilds Astro; the workflow publishes the exact checked tree|Describe native/WASM/docs gates, trusted deployment and CodeQL accurately
Overgeneralized physics/trails|physics summary treats every scene as periapsis Verlet; rendering summary excludes moving stars|Distinguish core, Jovian mean elements, source-epoch experiments and lessons; explain synchronized moving-star history and lesson cadence
Incomplete discovery/ownership|sitemap omits small-bodies; source map omits CLI, comparison and small-body data ownership|Complete sitemap with a regression gate and update navigation/source metadata
Empty GitHub About|description/homepage/topics were empty|Set concise physics-learning scope, canonical Pages URL and technology/domain topics

### Dependency update amendment

A43|Repository dependencies and CI toolchain versions use current stable compatible releases, with unsupported upgrades explained rather than forced through peer constraints|registry/release/tag evidence, clean locked npm install/audit, docs tests/type-check/build, immutable action revision comparison
A44|Emscripten 6.0.9 builds all runtime modules; the visual runtime links raylib 6.0 compiled with the same SDK, while conic and comparison modules remain raylib-free; native/WASM parity, artifact checks and the authorized local browser suite preserve existing behavior|native tests, fresh WASM builds, C/WASM comparison, generated-route and browser checks

Decision: Jonathan requested “update everything to the newest version”, selected “Docs + dependencies”, and authorized repository-local dependency installation and verification. He separately selected “Run local browser tests” for the built loopback site, isolated contexts and a task-local pinned Chromium download if needed. Astronomy source snapshots retain their reviewed versions.

Release evidence checked 2026-09-17: npm latest Astro is 7.3.3; @astrojs/check is 0.9.10 with TypeScript peer range `^5.0.0 || ^6.0.0`; TypeScript latest is 7.0.2, so 6.0.3 remains the newest supported compiler for the existing checker. Playwright 1.63.0 is current. GitHub release/tag checks confirm raylib 6.0 and every existing action SHA already resolve to the latest stable releases. Node 26.9.0 is Current and 24.21.0 is LTS; CI moves to Node 26 while the supported minimum stays 24. Emscripten 6.0.9 is the latest tagged SDK; 6.0 raises generated runtime minimums to Chrome 85, Firefox 79 and Safari 14.1. Sources: npm package metadata; https://nodejs.org/en/about/previous-releases ; https://github.com/emscripten-core/emscripten/releases/tag/6.0.9 ; https://github.com/emscripten-core/emscripten/releases/tag/6.0.0 ; immutable GitHub release refs.

## §A — Beginner-first website UX, 2026-09-18

id|criterion|verify
A45|The simulator presents the live scene and compact playback/recovery controls before configuration; object, view, lesson and advanced controls remain accessible on desktop/mobile with clear focus and close behavior|Astro checks and local desktop/mobile browser journeys, existing C bridge tests
A46|Explore/Learn/Experiments/Reference navigation preserves every existing route; homepage and searchable/grouped core catalog hand off the selected object to the C-populated simulator; zero-match filtering distinguishes retained selection|Node and browser selection/search/navigation tests, generated route checks
A47|Small-body result selection visibly opens its details and experiment action with return focus, including loading/error states; basket feedback remains visible and catalog work stays bounded|local catalog browser journey and existing catalog tests
A48|Comparison questions and valid defaults precede optional configuration; rejected form values receive field-specific guidance while C remains authoritative and previous results are retained; help supports task terms such as pause|focused comparison tests and browser validation/help journeys
A49|First-orbit learning and web/touch controls precede numerical/developer reference, with accurate labels and progressive disclosure across the site; all eleven audit findings are addressed and verified|source/copy review, Astro/route checks, browser evidence and regression scan

Decision: Jonathan approved the full 2026-09-18 UX audit with “go and work on everything”. Preserve the archival visual direction, C-owned physics/validation, existing source provenance, fixed-step semantics, catalog bounds, base-path-safe routes and keyboard accessibility. Implement in the existing checkout on `feature/beginner-friendly-site-ux`. The tracked root specification remains authoritative; `.specs/` is ignored and no replacement specification is created. Audit evidence is in ignored `playwright-evidence/ux-audit-20260918/UX-AUDIT.md`. Commit/push and deployment are not part of this authorization.

UX interfaces: simulator panel dialogs use native modal focus/escape behavior; object links use `simulator/?body=<core-slug>` and resolve against the current C body list after readiness. Unknown slugs leave the normal scene selection intact. Catalog details open immediately on selection and show loading before source resolution; closing does not mutate the experiment basket. Comparison form diagnostics explain C rejection rather than replacing C validation. Existing routes and catalog fragment IDs remain valid.

Verification authorization: Jonathan explicitly selected “Yes, run full local checks” for isolated local browser interaction, generated CSV/config downloads, repository fixture imports and local share links. No external submissions are involved. The pre-implementation review was GO with explicit checks for modal failure visibility, refreshed catalog focus restoration and revealing invalid fields inside collapsed settings.

Delivery amendment: Jonathan subsequently requested “open pr, evaluate, merge, deploy, report”. This authorizes committing and pushing the UX branch, opening its PR, evaluating the complete diff and checks, merging after verification, and publishing through the existing Build → Deploy Pages pipeline. Confirm the production revision matches the merged main commit. The earlier local-only delivery boundary is superseded by this request.

A50|The UX PR is reviewed against its complete diff, required checks pass before merge, and the existing Pages pipeline publishes the merged revision with public provenance verified|PR/check readback, Build and Deploy Pages results, public build-info and site verification

## §A — Codebase audit remediation, 2026-10-06

id|criterion|verify
A51|CI dependency gate is green: the docs lockfile has no moderate-or-worse advisories, build-only packages are devDependencies, Astro is current, TypeScript stays on the newest major supported by `@astrojs/check`, and Dependabot groups CodeQL action updates and skips unsupported TypeScript majors|clean `npm ci`/`npm audit --audit-level=moderate`, docs tests/check/build, Build CI
A52|Workflows cancel superseded runs, bound job time, share one raylib pin through a cached composite action, cache emsdk, and replay every `examples/*.solar` through native and WASM comparison|workflow review, Build/CodeQL CI, `tools/test_learning_wasm.mjs`
A53|The ecliptic→simulation mapping is a proper rotation `(x, y, z) = (X, Z, −Y)`; prograde orbits have +Y angular momentum and appear counterclockwise from ecliptic north (+Y); sourced retrograde moons keep −Y|RED handedness tests over core and experiment scenes, docs copy
A54|Lesson initial-speed choices never silently pass through the parent: trajectories whose periapsis is inside the parent radius are rejected or flagged, and published lesson errors are marked invalid after contact|lesson/session tests at former failing factors
A55|A single frame delta beyond the stall threshold (sleep, debugger) is discarded like a background resume; ordinary slow frames still keep pending time and the per-update step cap|`test_simulation_step`, `test_simulation_session`
A56|Environment `CFLAGS`/`CPPFLAGS` extend but never replace required C11, warning, include and `-ffp-contract=off` flags; C tests refuse to compile with `NDEBUG`|`tests/test_build_contract.py`
A57|Runtime hardening: camera yaw stays bounded; CSV export refuses symlinked or non-regular targets; total comparison/headless ticks are capped; native/headless share one input reader; comparisons measure the subject index; ignored return values are checked; C and TypeScript command IDs are cross-checked; oversized browser experiment text is rejected before C|focused C/Python/Node tests
A58|Simulation structure: scene capacity is compile-time checked with bounded appends; experiment rows are strict TSV; `orbit.c` is the only Kepler solver; render-scale policy lives outside `src/sim`; planetary masses derive from cited GM values with source-checking tests; parent–moon families start with the parent at the family barycenter offset so the family barycenter follows the intended heliocentric state|C tests, V1 scan, docs copy
A59|Web runtime: numeric catalog searches load only the owning shard; repeated searches reuse a compact worker cache; a full scan states its download cost first; record lookup runs in the worker; non-fatal Emscripten stderr never disables the runtime; atlas announcements are single and throttled; text and canvas semantics meet accessibility basics; CSP and self-hosted fonts remove avoidable third-party loads; counts derive from data; downloads are not revoked early|Node and browser tests, Astro check/build
A60|Every generated HTML document and the sitemap come from Astro sources; `docs/public` contains no HTML; route validation fails if a generated page lacks the Astro generator marker or a public HTML file appears|`tools/check_docs_routes.py` regression tests, built-site check
A61|Repository hygiene: interrupted-fetch packs are pruned, ad-hoc screenshots live in ignored evidence folders, generated-data validation never depends on `assert`, failed catalog generation leaves no publishable directories, and redistributed NASA/JPL data carries a usage note|`git count-objects`, tool tests, data docs
A62|A second full audit round finds no unresolved Medium-or-higher issue; any remaining Low/Info items are recorded with rationale|second-round audit report

Decision: Jonathan requested “work on everything you mentioned until we have it all done, then a second audit round”, adding that this is a C study project and that every page must be Astro. AGENTS.md authorizes delivering each verified iteration to `main`; the repository ruleset requires that delivery to be a PR with green required checks, resolved threads and a rebase merge. Google Analytics remains governed by V17 (deployed main only, disclosed); the CSP allow-lists it rather than changing analytics policy.

Catalog storage decision (C-catalog): the pinned 135 MB shard set stays in history at its current revision. A future `--refresh` must not commit regenerated gzip shards to `main`; it must first move shard delivery to hash-pinned release assets fetched by CI, because compressed shards do not delta-compress and each refresh would add the full set to every clone.

## §A — Renovate replaces Dependabot, 2026-10-07

id|criterion|verify
A63|Renovate is the only dependency updater: `.github/dependabot.yml` is removed and Dependabot security updates are disabled, while the dependency graph and passive Dependabot alerts stay enabled as the read-only feed Renovate's `vulnerabilityAlerts` consumes (OSV covers only direct dependencies); `renovate.json` keeps the weekly schedule, 3-day minimum release age, grouped docs npm updates, grouped CodeQL action, SHA-pinned actions and no TypeScript major beyond the `@astrojs/check` peer range; Renovate also opens OSV vulnerability PRs immediately and tracks the raylib tag+commit, Emscripten and Node pins; runner images stay fixed per R13|`renovate-config-validator --strict`, local `--dry-run=extract` listing the raylib/emsdk/node pins, repository settings readback

Decision: Jonathan asked to keep only `main`, then "I want dependabot uninstalled and only renovate on". When review showed that disabling alerts would hide transitive lockfile advisories from Renovate, he chose to keep passive alerts on; nothing but Renovate opens dependency PRs. All non-main branches were deleted after confirming every feature branch was merged; Renovate's onboarding PR #23 had been closed, so committing `renovate.json` onboards the installed app directly.

## §A — Cinematic renderer, 2026-10-07

id|criterion|verify
A64|Bodies are lit by the rendered Sun (framing turns the camera to the framed body's sunlit side, ~40° off the Sun line): soft day/night terminator, faint ambient, Earth city lights on the night side and a cloud layer, atmosphere rim glow for bodies with atmospheres; the Sun is emissive with limb darkening and an additive glow; scenes without a star light bodies from the camera|pure style/table tests, desktop/mobile screenshots
A65|Real maps (Solar System Scope, CC BY 4.0, attributed) texture the Sun, eight planets, the Moon, Saturn's rings and a Milky Way backdrop; a vendored, pinned `stb_image.h` (public domain/MIT) decodes JPEG/PNG for both builds because raylib ships with JPEG disabled; native loads `assets/textures/`, the browser fetches the same files lazily after the first frame; a missing or failed texture falls back to a lit body colour without stopping the simulation; untextured bodies (minor moons, Vesta) stay honest lit colours|decode tests, texture-inventory test, browser journey asserting every texture loads, fallback check
A66|Textured bodies are oriented by the IAU WGCCRE 2015 rotation models (pole α0/δ0 and prime meridian W(d), Archinal et al. 2018), converted from ICRF through the J2000 ecliptic into simulation axes; d counts TDB days since J2000 (catalog experiments start at their source epoch, synthetic scenes at J2000); Saturn's rings lie in its equatorial plane|orientation tests: obliquities, retrograde spin of Venus/Uranus, Earth prime meridian at J2000
A67|The reference grid fades with distance instead of aliasing into moiré, trails fade with age, edges use 4× multisampling, and the Milky Way backdrop is drawn behind everything without affecting depth; the scene stays interactive on desktop and mobile web|grid/trail alpha tests, screenshots, browser suite timing
A68|Physics, CSV, inspector, camera framing and the canvas/viewport contract are unchanged; render style math lives in raylib-free `src/render/scene_style.*` with tests|full C/WASM/browser suites, V12/V24 checks

Decision: Jonathan asked for "better graphics overall", then chose "Cinematic realism" and approved adding real planet textures (~3 MB, public-domain/CC-BY). This explicitly lifts C9 for this task; delivery follows the PR-only workflow.

## §A — Renderer polish and performance, 2026-10-07

id|criterion|verify
A69|Lighting happens in linear light (sRGB decode → Lambert → encode); Earth adds an ocean sun glint masked to water; bodies with air show a limb halo (additive shell peaking at the planet's limb); clouds fade at grazing angles; Saturn and its rings shadow each other analytically|screenshots, offscreen GLSL 330 compile/link, browser rendering journeys
A70|Trails draw after opaque bodies without depth writes and with additive blending, stop at each body's drawn surface, and are drawn only as finely as their on-screen extent needs; a per-frame cache resolves parents and positions once and is bit-identical to the uncached path|`test_renderer` equivalence test, `test_scene_style` clip/stride tests, profile + frame-rate measurements
A71|Framing turns the camera to the framed body's sunlit side; the reference grid can be hidden through a C command (native G, web View options)|`test_orbit_camera`, command-ID check, browser journey
A72|The full scene holds 60 fps on desktop and on a 4×-CPU-throttled phone emulation at default speed, and does not regress the 15 days/s stress case|recorded measurements against the live site
A73|In-canvas labels name the Sun, planets, the selected body and known-size moons of a zoomed family; overlapping labels yield by priority and occluded bodies stay unlabeled; a C command toggles them (native H, web View options); the OFL font subset is embedded at build time|`test_scene_style` label tests, command-ID check, browser journey, screenshots, frame-rate measurements

Decision: Jonathan set the goal "keep working and iterating on our simulation to have a beautiful and optimized and good result". Each step is screenshot-verified and frame-rate measured against the live site before it is kept.

## §A — Open-item closure, 2026-10-07

id|criterion|verify
A74|Jupiter–Neptune masses are planet-only JPL GM / CODATA G like the inner planets (V6); G*M reproduces each sourced GM|`test_solar_system`, `test_outer_planets`
A75|At every tested viewport (320×640 to 2560×1440) no homepage atlas marker or label overlaps another, the hero copy, the actions or a plate caption; the plate never grows past 900 px; captions that only repeated the plate tabs and bearing control are gone and the plate name sits above the disclaimer|Playwright overlap probe across 15 viewports and all four plates, screenshots, browser suite
A76|A macOS CI job builds natively against the pinned raylib, runs `make test`, and renders Earth offscreen through the real GLSL 330 shaders and maps (`make test-native-shaders`): dark background, lit day side brighter than the night side, ocean glint, land at 30°E facing the camera (map not mirrored)|macOS job log, local CGL run
A77|`npm run check` type-checks the Playwright specs and config (Node types are a docs devDependency); Renovate re-onboards from `renovate.json` and labels its PRs|`astro check` probe error, Renovate Dependency Dashboard issue

Decision: Jonathan asked to "work on everything open" before any further body expansion. These items were the Low/Info residuals recorded after the second audit and the renderer rounds; the macOS job is informative (not a required check) so a hosted-runner image change cannot block delivery. Saturn's ring plane already follows the IAU pole (A66); `SOLAR_SATURN_AXIAL_TILT_DEGREES` remains only as the NASA obliquity the orientation tests compare against.

## §A — Moon-system expansion plan, 2026-10-07

Plan review before the next body milestone, requested by Jonathan ("revisit the plan to guarantee that the plan itself is in a good shape before we keep expanding"). It replaces the one-line T18 and the README list with sourced, staged and testable work. Nothing below is implemented yet.

Findings:
- Inventory: the simulated set must be orbit-bearing JPL element rows (R18), one per NAIF code. NASA's counts (R20) disagree with each other and with JPL for Saturn (274 / 293 vs 291 rows), and Uranus's 30 JPL rows hold 29 moons because Puck is listed twice. A full giant-planet set is 291 + 29 + 16 = 336 moons; the scene would grow from 128 to 464 bodies.
- Frames and epochs: unlike the Jovian table (one epoch, ecliptic/Laplace), Uranus mixes three epochs and adds a planet-equatorial frame with no pole columns. JPL's Uranian Laplace pole (RA 77.3°, Dec +15.2°, tilt 180°) is the antipode of the IAU north pole (257.31°, −15.18°): the regular moons orbit in Uranus's spin sense about it. Neptune's Triton is a massive, retrograde moon.
- Point-mass model error: two-body periods from JPL mean `a` and planet-only GM differ from the JPL period column by at most 0.75 % in all four systems (Mimas +0.51 %, Pan +0.27 %, Cordelia +0.18 %, Naiad +0.13 %; Europa +0.75 % already ships). The difference is oblateness (J2), resonances and rounding: the model omits them. Docs must name it as model error, not numerical error.
- Throughput: gravity costs one pair per (body × massive body). The 128-body scene has 22 massive bodies and runs at 26 simulated days per wall second natively (benchmark, M-series, trails included). Measured with the real kernel on synthetic scenes, cost scales with that pair count: 179 bodies with 51 massive is 2.9× slower, and 464 with 51 is 7.8× slower (about 3 days/s natively, far less on a throttled phone). Putting every moon in the core scene would break the 15 days/s guarantee (A11, A17, A72).
- Storage: `SolarSystem` holds bodies by value (136 B each; 17 KB today, about 63 KB at 464). Session reset/trial paths and the headless runner hold full copies as locals, comparison state embeds several scenes, and the simulator's WASM stack is 256 KB. Trails cost 1,025 points per body (about 11 MB at 464). The web body selector, labels and the homepage atlas were sized for 115 Jovian moons.

id|criterion|verify
A78|One catalog tool serves every giant planet: `tools/satellite_catalog.py --system jupiter|saturn|uranus|neptune` writes `data/<adjective>_moons.json` and `src/sim/<adjective>_moons.inc` from pinned JPL snapshots; `--check` is offline; `--refresh` is explicit; Jovian values and order are unchanged (the generated rows append the JPL period and a `major` flag). Duplicate NAIF codes need an explicit ephemeris preference (Puck keeps URA184); provisional spellings (`S2025_U_1`, `S2023_U1`, `S2002_N5`) normalize to IAU form; no moon code collides with a planet center (599/699/799/899); `major` is re-derived as measured GM ≥ 2 km³/s² and must equal the reviewed set (Galilean moons; Mimas, Enceladus, Tethys, Dione, Rhea, Titan, Iapetus; Miranda, Ariel, Umbriel, Titania, Oberon; Triton)|`tests/test_satellite_catalog.py`, `--check` for all systems, Jovian value regression
A79|Every element set keeps its own source epoch and frame; phases are mutually consistent only within one ephemeris solution, and docs never present a mixed-solution family as a dated snapshot. Ecliptic, Laplace (table pole) and planet-equatorial frames convert through `orbit.c`; Uranus's equatorial pole is the antipode of the IAU north pole kept in `src/sim/constants.h` (never borrowed from `src/render`)|C tests: each converted orbit normal lies exactly its source inclination from its source pole; Uranus's equatorial (URA182) and Laplace (URA184) moons share a plane within 1° and orbit about the spin pole; Saturn's inner and major moons share a plane; Triton opposes Proteus by more than 150°
A80|Point-mass periods stay within 1 % of the JPL period column for every simulated moon, and the docs attribute the gap to the omitted J2/resonance physics|table-driven C test over all catalogs, docs copy check
A81|Scale prerequisites: the body array is sized for the largest scene (300, Saturn's family) rather than the main-scene count; the simulator's WASM stack is 1 MB (a 300-body scene is ~41 KB and scene loads hold a few copies); trails stay bounded per body; the selector, labels and atlas page or group families of up to 291 moons|static asserts, sanitizer run, browser journeys loading the Saturn scene, atlas paging journey
A82|Throughput budget per scene: the main scene keeps A11/A17/A72 with headroom; every scene reports achieved speed honestly at higher presets (V8) and never drops time or enlarges steps. The gravity kernel works on structure-of-arrays blocks and the Verlet loops use component arithmetic, both bit-identical to the previous code, and WebAssembly builds with `-msimd128`. Measured 2026-10-07 (native benchmark on an M-series Mac / Chrome desktop / Pixel 7 emulation at 4× CPU throttle, default 1 day/s): main 172 days/s / 60 / 59 fps (was 32 days/s with 128 bodies); Jupiter 58 / 60 / 59; Saturn 20 / 60 / 58.5 fps; Uranus and Neptune above 150 days/s. Each trail's on-screen extent is re-measured every eighth frame (staggered across bodies, and at once when its history shrinks), which lifted the 300-body Saturn scene from 36.7 to 58.5 fps on the throttled phone under the same machine load; trail detail stays tied to on-screen path length (B50) and may lag a camera move by up to eight frames|`benchmark_simulation --scene NAME`, Chrome CDP frame-rate probe, profile
A83|Saturn system: all 291 R18 moons with Saturn parent and NAIF codes, R19 GM where measured (estimated/unknown provenance otherwise, V25), Saturn family barycenter per V6; Saturn's inner (Pan) and major moons share one plane within 1° (Iapetus follows its own tilted Laplace plane); 20-day half-step discrepancy below 1 % for every body, including the Janus/Epimetheus co-orbitals and the Tethys/Dione trojans|catalog, frame, barycenter and convergence C tests
A84|Uranus system: 29 unique moons; the equatorial frame, three epochs, Puck de-duplication and the tilt-180° Laplace pole convert correctly; Uranus's regular moons sit near its 98°-obliquity equator|frame/epoch/de-duplication C tests
A85|Neptune system: 16 moons; Triton is massive and retrograde, Nereid (e=0.751) and the distant irregulars use the shared conic kernel|catalog and orbit-sense C tests
A86|Each system completes V15: inspector quality labels, illustrative/real-scale visibility (no borrowed textures, A65; Titan, Enceladus and Triton get flat display colours only), family framing on load, labels, grouped search by catalog group (major moons, small regular or inner moons, irregular moons), C/TypeScript parity for all five scenes, an atlas plate per giant planet with paged groups, docs, CSV catalog and route checks|full C/WASM/docs/browser suites

Decision, 2026-10-07 (Jonathan): "main moons from Saturn, Uranus and Neptune should be secondary items, and I want a separation on Jupiter moons as well … bring the secondary objects behind as well; large items should be on the main scene". This is the hybrid option extended to Jupiter. The main scene keeps the Sun, the eight planets, Vesta, the Earth and Mars systems (one and two moons) and the 17 major moons (A78); it shrinks from 128 to 30 bodies, all massive. Every other moon lives in its planet's family scene. A family scene holds the Sun, the eight planets and that planet's complete catalog with its major moons first. The rejected options were family scenes with an unchanged 128-body core, and everything in the core (which needs an 8× faster kernel).

Order: T73 catalog tool → T74 scene split (main scene, four family scenes, capacity/stack/selector/atlas work, throughput budget; this replaces the separate T75–T77 system PRs, since the family scenes ship together) → T78 scoping. Each is its own PR. C5 is read as one concept per milestone; A12 set the precedent of a whole moon system in one milestone.

## §A — Small-body satellite systems, 2026-10-07

T78 scoping. The only small-body satellite systems with a JPL product to pin are Pluto's (the JPL satellite tables, R21) and Didymos–Dimorphos (Horizons, R22); other asteroid moons have no JPL ephemeris (R23) and stay out of scope (C10: no ephemeris loader). Jonathan asked to plan and execute everything that remained ("do it … only stop after did planned all and executed all plans").

id|criterion|verify
A87|`satellite_catalog.py` pins `plutonian_moons.json` (5 moons, PLU060 equatorial frame) and `didymos_moons.json` (Dimorphos from Horizons s547, ecliptic J2000 elements about the primary). Upper-limit GMs (Kerberos, Styx) are unknown masses; Charon passes the major-moon rule; Pluto's equatorial pole is the IAU 2015 positive pole, which for dwarf planets is already the angular-momentum pole|`tests/test_satellite_catalog.py`, `--check`
A88|Pluto (dwarf planet, NAIF 999, Horizons planet-only GM) and Charon join the main scene (32 bodies); the Pluto–Charon barycenter takes Pluto's planar perihelion state and lies outside Pluto (~2,100 km off its centre). Pluto starts on ecliptic +Y, opposite Neptune|`test_solar_system`, `test_satellites` (Charon about the IAU pole, ~113° from ecliptic north: retrograde)
A89|Pluto's small moons start around the Pluto–Charon barycenter with the pair's mass and stay within their eccentricity plus 5% of their published orbit for 100 days of N-body integration. Their two-body period around the pair is 1.5–3.7% longer than JPL's, because JPL's mean a and P describe orbits in the binary's rotating field: model error, not integrator error|`test_satellites` circumbinary and period tests
A90|The Didymos system scene (11 bodies) holds Didymos at its SBDB planar perihelion on ecliptic +X (1.4 AU from Earth's start) and Dimorphos on its retrograde 11.8-hour orbit; both masses are estimates (V25); the scene converges within 1% over 20 days|`test_solar_system`, `test_satellites`, `test_simulation_step`
A91|Both systems complete V15: presets `pluto-system` and `didymos-system` (scene picker, native K, `solar-lab --scene/--catalog`), deep links for Nix, Dimorphos and the rest, atlas plates for Pluto and Didymos with Pluto on the heliocentric plate, a dwarf-planet render size and label rule, flat display colours, C/TypeScript parity for all seven scenes, docs and route checks|full C/WASM/docs/browser suites

## §A — Dated sky, oblate planets and new lessons (plan), 2026-10-07

Jonathan asked to "work on all" of the remaining options: real dated positions, tilted orbits, planet flattening, Pluto–Charon and DART lessons, Saturn at 60 fps on phones, and Renovate. A prototype that propagated JPL mean elements from their epochs to 2026 missed Horizons positions by up to 178° (Io 121°, Titan 170°, Phobos 178°): resonances and forced eccentricities make mean periods useless over decades. Dated positions therefore come from a pinned snapshot of Horizons state vectors at one epoch, not from propagated mean elements.

id|criterion|verify
A92|Every scene starts at the real configuration of JD 2461200.5 TDB (2026-06-09), the epoch catalog experiments already use: planetary-system barycenters from `data/planet_epoch.json`; every moon, Vesta, Pluto, Didymos and Dimorphos from a pinned Horizons vector snapshot (`data/scene_epoch.json`, refreshed only by `tools/scene_epoch.py --refresh`, checked offline). Orbits are therefore inclined as in reality. A moon Horizons does not serve under its own name (checked by name, e.g. Uranus's S/2025 U 1, whose code resolves to an asteroid) keeps its mean-element state and is listed as undated; V6 family barycenters still take the planets' states. The `*_at_perihelion` factories remain for lessons and tests|`tests/test_scene_epoch.py`, C tests comparing scene states with the pinned vectors, undated list
A93|The inspector and HUD show the scene's calendar date (epoch plus elapsed simulated time); docs replace "not the sky on a particular date" with "starts from the real sky of 2026-06-09, then follows this model"; the explicit model limits (point masses, no relativity, fixed Sun) stay|docs/route checks, browser journey reading the date
A94|Oblateness: Earth, Mars, Jupiter, Saturn, Uranus and Neptune carry J2 and an equatorial radius from the NASA Planetary Fact Sheets and an IAU pole (as `src/sim` constants). Each oblate body pulls its own moons with the J2 term and feels the equal and opposite reaction, so momentum stays conserved; bodies outside a planet's family see it as a point mass. J2 applies only in astronomy scenes; guided lessons stay point masses so their analytic Kepler references hold. A test particle's nodal precession matches −3/2 n J2 (R/a)² cos i within 2%, and the reaction keeps momentum conserved. (J2 shortens Mimas's period by ~0.13%; the rest of its 0.5% gap to JPL is resonance and mean-element definition, so no closer period claim is made)|`test_physics` precession and momentum tests, scene/lesson J2 test
A95|The inspector reports a two-body orbital period (from the specific energy around the parent) next to distance and speed|session test, browser journey
A96|Lesson `pluto-charon`: the isolated pair at its barycenter, which lies outside Pluto; lesson `dart`: Didymos and Dimorphos on the pre-impact orbit (Horizons s547 at 2022-09-01, started at periapsis), where the speed factor is the along-track change: 0.985 reproduces DART's ~2.6 mm/s slowdown and a ~32-minute shorter two-body period (12.34 → ~11.8 h; observed mean periods 11.92 → 11.37 h), read from the period readout|lesson tests, comparison descriptor support, browser journey
A97|The 300-body Saturn scene's frame rate on the 4×-throttled phone profile at default speed is measured and recorded with each physics change. Measured 2026-10-07 with the dated sky and J2: 56.3 fps (desktop 60; every other scene 59–60), with the WebAssembly build at `-O3` (+8%). The 59 fps target was not reached: the frame is physics-bound (about 7,500 pair evaluations × 96 steps per frame at 1 day/s), and blocking four sources per pass measured slower. A larger gain needs an error-bounded tidal treatment of distant sources for massless moons, a model change this plan does not make. Follow-up 2026-10-08: the tidal treatment was prototyped and rejected (its truncation bound exceeds 1e-6 of the parent's pull for 267 of Saturn's 275 massless moons, and after 100 days it moves Jovian irregulars ~1e-3 of their orbit, six orders above the step error, for no gain over the exact fixes). The redesign's larger phone canvas (412×426 against 380×268 CSS px) had cut Saturn to ~38 fps, and framing a single body to 18. Exact or presentation-only fixes: the J2 moon loop is inlined, bit-identical (+33% WebAssembly physics); unknown-radius wire markers reuse one unit-vertex table instead of raylib's per-vertex trig and matrix, and markers under 3 px radius (a blob, not a readable wireframe) are dots like any tiny body; trail segments wholly outside one side of the view pyramid are skipped; trail detail is one point per 4 on-screen pixels with a 12-point floor (a 4 px chord on a 20 px radius strays 0.1 px). Same-session probe: Saturn 35.6–38 → 54.6–56 fps (Pan framed: 18 → 50), main, Jupiter and Uranus 59.2; screenshots unchanged. Second follow-up 2026-10-08: the web build called raylib's SetTargetFPS(60) although requestAnimationFrame already paces it, so EndDrawing spun (Emscripten's nanosleep busy-waits) until raylib's own 1/60 s clock expired: every frame used the whole 16.7 ms of main-thread CPU, and the two clocks' drift dropped frames that fit (light scenes read 59.3, not 60). The web build now sets no target; the page gets state reports on every fourth frame (commands still report at once); grid pieces whose endpoint alphas round to zero or that lie wholly outside one side of the view are skipped (13k grid vertices a frame in the Saturn overview, as many as all trails). Main-thread CPU per frame from a Chrome trace (unthrottled; 4× throttling makes 4.2 ms the 60 fps budget): main scene 0.99 ms, Jupiter 2.5, Saturn 3.4–4.9 (minimum and median of five runs on a host at load average 15–22). The best run fits the budget. Result 2026-10-08, host load average under 5: the merged build (f819aca) holds 60.0 fps in the Saturn scene on the 4×-throttled Pixel 7 profile at 1 d/s, in three deep-link runs and one scene-picker run; the main and Jupiter scenes hold 60.0 and Uranus 59.8–60.0 (re-measured 2026-10-09; its earlier 59.2 predates the pacing fix). Saturn has little margin: a run while the host load rose to 7 read 57.2. Framing a single Saturnian moon (Frame body on Pan) reads 55.7: the target applies to the scene's default view, and that close-up stays the slowest view. Target met on a quiet host|CDP frame-rate probe recorded in the PR
A98|Renovate: the configuration is valid (dry run). The hosted app had repository access on 2026-10-06 (it opened onboarding PR #23, closed in favour of the committed `renovate.json`), but as of 2026-10-08 it has created no Dependency Dashboard issue or update PR. Its job logs (developer.mend.io) and app access (GitHub Settings → Applications → Renovate) are visible only to Jonathan, so diagnosing it stays an owner action, not a code task. Checked again 2026-10-09: `renovate.json` validates against Renovate 44.148.6 and reached `main` five minutes after onboarding PR #23 was closed (2026-10-07 01:20Z); many pushes since triggered no run. Deferred 2026-10-09 (Jonathan): outside the open-work pass; resume from the Mend job log or by adding a `RENOVATE_TOKEN` secret for a self-hosted workflow|owner action, deferred

## §A — Design overhaul, 2026-10-07

Jonathan: "plan a design overhaul … the current one has a ton of info and is not so good at UX … a very non AI based website, interactive, lean and according to the project scope". Findings: 16 page routes; the field guide alone has 8 pages and about 6,400 words; the homepage is a decorative illustrative atlas (its positions are explicitly not physical), so the actual simulator is one click away; the simulator page carries about 2,500 words of copy around the canvas; navigation splits "Explore/Learn/Experiments/Reference" across overlapping pages.

Principles:
- The simulator is the product, so it is the homepage. Everything else supports it.
- An instrument, not a brochure: controls are terse labels and numbers with units; explanations live one click away, never as paragraphs around the canvas.
- No generic AI-site patterns: no hero banner, no feature-card grids, no icon-in-circle rows, no gradients or glass, no marketing adjectives, no emoji. Real data, real numbers, plain typography.
- Lean: four routes, each with one job. Every old URL keeps working through an Astro redirect page that preserves `?body=` and fragments.

id|criterion|verify
A99|Routes: `/` simulator (full viewport), `/learn/` (lessons, including A/B comparison at `/learn/compare/`), `/catalog/` (every simulated body plus the small-body atlas at `/catalog/small-bodies/`), `/about/` (model, data sources, build and tests, credits on one indexed page). Old routes (`/simulator/`, `/compare/`, `/physics/`, `/body-catalog/`, `/small-bodies/`, `/source-atlas/`, `/pipeline/`, `/docs/…`, `/wasm/solar-system-simulator.html`) are Astro redirect pages that preserve query and fragment|route checks, redirect journey
A100|Simulator page: the canvas fills the viewport below a thin bar (wordmark, Learn, Catalog, About, source link). A sky clock at the top left of the canvas (scene date, or lesson elapsed time, with days since the start); one bottom dock: play/pause, restart, speed (1 h–15 d per second), scene picker (Solar system, Jupiter, Saturn, Uranus, Neptune, Pluto, Didymos, Patroclus), and Find/View/Keys/Data sheets (body search lives in Find). A compact inspector (parent, distance, speed, period, mass, radius with quality). View settings (scale, trails, vectors, grid, labels) and keyboard help are small popovers. Visible copy on the page stays under 150 words; status and errors remain announced|word-count check, browser journeys, screenshots at desktop and phone sizes
A101|Learn: each lesson is one row: a question, what to watch, and Run, which opens `/?lesson=NAME` with a lesson strip (method, step, speed factor, reset) over the canvas. Comparison keeps its C-backed charts and descriptors behind a question-first form|browser journeys
A102|Catalog: one searchable, family-filtered table of all simulated bodies (name, kind, parent, scene, group; mass and radius with quality stay in the simulator inspector) where each row opens the simulator at that body; the small-body atlas keeps its search, map and experiment basket with trimmed copy|browser journeys, Node tests
A103|About: one page with an in-page index: what the model is and is not, units and integrator, J2 and limits, data sources (one table), build and verification commands, credits and licenses. It replaces the 8 field-guide pages, Physics, Source atlas and Pipeline|route checks, link checks
A104|Visual system (DESIGN.md): the simulator is a dark instrument (canvas, hairline borders, one amber accent, mono labels and numbers); text pages are a lab notebook (warm paper, serif headings, readable 65-character measure, mono data tables, footnote-style source links). Accessibility stays: keyboard paths, visible focus, 12 px minimum text, reduced motion, valid semantics, CSP without inline code|screenshots, axe-style checks in browser journeys, CSP route checks
A105|The decorative homepage atlas and its plates are removed. Its tests are replaced by catalog and simulator journeys; V20/V21 retire with it|test inventory in the PR

Result (T84–T85): the four routes, 15 redirect pages and the instrument home shipped. Instrument copy measures 146 visible words at 1440×900. On phones the HUD, lesson strip, inspector and dock stack around the canvas instead of floating over it. The Pluto–Charon and DART lessons open framed on the whole pair. The route checker, `tests/test_artifact_checks.py` and 25 browser journeys pass, including the redirect journey, 320 px overflow checks and panel-overlap checks from a 375×548 phone to 1440×900 during a lesson. A Node test requires every CSS custom property in use to be defined.

Follow-up (2026-10-08, Jonathan: move the date out of the dock): the date leads the canvas as a sky clock with an amber days-since-start note; status reads Running/Paused; the duplicate selected-body line is screen-reader only, so the dock fits one row at desktop widths.

Order: T80 dated epoch scenes → T81 J2 and period readout → T82 lessons → T83 Saturn 60 fps → T84 redesign: routes, redirects and simulator home → T85 redesign: learn, catalog, about, removals. Each is its own PR.

## §A — Patroclus system, 2026-10-08

Jonathan asked to work on all open work behind the site's "What's next" note ("moons of other asteroids have no JPL ephemeris to pin, so they wait for one"). Re-checking R23 found one: Horizons serves the Patroclus–Menoetius binary trojan as solution JPL#82. It joins the scenes the way Didymos did (A87–A91).

id|criterion|verify
A106|`satellite_catalog.py --system patroclus` pins `patroclus_moons.json` (Menoetius, ecliptic J2000 elements converted from the Horizons state about GM_P + GM_M, round-trip exact) and refuses a new Horizons solution until reviewed; `scene_epoch.py` pins the Patroclus system barycenter (`617;`) and Menoetius about the primary. Preset `patroclus-system` (11 bodies) holds Patroclus at index 9 and Menoetius on its retrograde 4.3-day orbit; both masses are estimates (V25); the pair's barycenter, ~150 km from Patroclus's centre, lies outside Patroclus as Pluto–Charon's does; the planar perihelion factory places Patroclus 60° behind Jupiter like an L5 trojan; the scene converges within 1% over 20 days; scene picker, native K, `solar-lab --scene/--catalog`, the `?body=menoetius` deep link, catalog rows, flat display colours, C/TypeScript parity for all eight scenes, docs and route checks|`tests/test_satellite_catalog.py`, `test_solar_system`, `test_satellites`, `test_simulation_step`, `check_catalog.mjs`, browser scene journey

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
T18|x|superseded by the staged moon-system plan T73–T79 (inventory reconciled in R18–R20)|A78–A86
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
T49|x|refresh audited documentation, onboarding and source ownership; verify claims and generated site|A40,V16
T50|x|repair and guard sitemap completeness|A41,V13
T51|x|enhance and verify GitHub About metadata through gh|A42
T52|x|update supported docs dependency releases and Node CI version, install the lockfile and verify|A43
T53|x|update Emscripten and documentation, rebuild matching raylib/WASM, and verify native/browser contracts|A44
T54|x|reorganize simulator into scene-first controls and accessible object/view/lesson/advanced panels|A45,V18,V23,V24
T55|x|group navigation and improve homepage/core-catalog object discovery and simulator handoffs|A46,V13,V20,V21
T56|x|make small-body details, selection recovery and basket feedback visible|A47,V18,A21
T57|x|lead comparisons with questions, explain invalid fields, and publish beginner-first help|A48,A49,V31,V34
T58|x|verify all eleven UX findings, route/bridge contracts, desktop/mobile journeys and regression scan|A45,A46,A47,A48,A49

T59|x|restore green CI dependency gate, group CodeQL updates, share/cache the raylib pin, bound/cancel CI runs, harden Make flags and replay every example natively and in WASM|A51,A52,A56,B18,B19
T60|x|correct scene handedness, lesson contact policy, scene capacity, strict experiment parsing, single Kepler solver, render-policy placement, GM-derived masses and barycentric families|A53,A54,A58
T61|x|discard stalled frames, bound camera yaw, harden CSV targets and work limits, share input reading, fix comparison subject measurement and check command alignment|A55,A57
T62|x|make catalog search/lookup proportional to the query, keep non-fatal runtime warnings non-fatal, close accessibility/CSP/font/count/download findings and guarantee Astro ownership of every page|A59,A60
T63|x|prune local packs, move screenshots, harden data tools and document data usage|A61
T64|x|integrate all rounds, run complete verification, update docs, commit/push main and observe CI/Pages|A51–A61
T65|x|run the second audit round and resolve or record its findings|A62
T66|x|replace Dependabot with an equivalent-or-stricter Renovate configuration and disable Dependabot|A63
T67|x|add raylib-free scene style math (texture inventory, atmospheres, IAU orientation, sphere/ring meshes, grid/trail fades) with RED tests|A64,A66,A67,A68
T68|x|implement lighting shaders, textures, glow, backdrop, rings, clouds and fades in the raylib renderer with native texture loading|A64,A65,A66,A67
T69|x|lazy-load textures in the browser, document attribution and controls, verify screenshots and the full suite, deliver by PR|A65,A67,A68
T70|x|polish lighting (linear light, glint, halos, ring shadows), trail ordering/clipping/adaptive detail, sunlit framing, grid toggle, and verify 60 fps on throttled phones|A69,A70,A71,A72
T71|x|add decluttered, occlusion-aware body labels with an embedded OFL font and a toggle|A73
T72|x|close the recorded audit residuals: GM-derived giant masses, homepage atlas collisions, macOS native CI with a shader render test, type-checked browser specs, Renovate re-onboarding|A74,A75,A76,A77
T73|x|generalize the satellite catalog tool to every giant planet with offline checks, de-duplication, epoch/frame metadata and a Jovian byte-identity regression|A78,A79
T74|x|split the scenes per the 2026-10-07 decision: 30-body main scene, Jupiter/Saturn/Uranus/Neptune family scenes, capacity/stack/selector/atlas work and measured throughput|A81,A82,A83,A84,A85,A86,V15
T75|x|folded into T74 (Saturn family scene)|A83
T76|x|folded into T74 (Uranus family scene)|A84
T77|x|folded into T74 (Neptune family scene)|A85
T78|x|scope small-body satellite systems and add the Pluto and Didymos systems|A87,A88,A89,A90,A91,R21,R22,R23
T79|x|review the expansion plan against current sources, frames, epochs, model error, throughput and storage|A78–A86
T80|x|start every scene from the Horizons sky of 2026-06-09 with a pinned vector snapshot|A92,A93
T81|x|add J2 oblateness for Earth, Mars and the giants, and an orbital-period readout|A94,A95
T82|x|add the Pluto–Charon and DART lessons|A96
T83|x|measure and improve the Saturn scene on the throttled phone profile (56 fps recorded; 59 not reached, see A97)|A97
T84|x|redesign: four routes, redirects and the simulator as the homepage|A99,A100,A104
T85|x|redesign: Learn, Catalog and About pages; remove the atlas and field-guide pages|A101,A102,A103,A105
T86|x|correct R23 and add the Patroclus–Menoetius system from Horizons JPL#82|A106,R23,R24
T87|x|close A97: exact J2 inlining, trail/grid culling, marker dots, no web frame limiter; Saturn at 60 fps on the throttled phone profile|A97

Audit remediation verification, 2026-10-06: delivered through PRs #20, #21, #22, #24, #25 (round 1), #26 (main CI/analytics blocker found by round 2) and #27 (round 2), each rebase-merged after the required Build/CodeQL checks passed and every review thread was resolved. Production Pages served 81eb308 after #26 with CSP, Astro generator marker, generated sitemap and no robots.txt verified on the live site. Locally, round 2 passed `make clean && make test-sanitize`, `make && make test test-build test-cli test-validators`, catalog/command/epoch/Jovian/small-body checks, a fresh raylib WASM build with native/WASM replay of every example, `npm ci`/`npm audit`/37 Node tests/`astro check`/build, route checks, and 17/17 sandboxed Chrome journeys for both analytics-free and `PUBLIC_GA_ID` builds. The second audit's Medium findings (main CI red, experiment Earth-centre start, debug-info WASM, duplicate SPEC IDs, PR-only workflow docs, invisible catalog download) are closed. Remaining Low/Info items are recorded rather than changed: Jonathan later chose Renovate over Dependabot (A63) and asked for every non-main branch to be deleted; CI stays Linux-only and the browser lane stays on Ubuntu 22.04 per R13; browser specs are type-checked by Playwright, not `astro check` (adding `@types/node` was deferred); GA consent policy remains V17; the pre-existing desktop atlas Vesta/Uranus label overlap, Saturn's illustrative ring tilt and literal Jupiter–Neptune masses (V6 scopes GM-derived masses to Sun–Mars) are unchanged.

Verification: Build run https://github.com/jonathanperis/solar-system-simulator/actions/runs/35175509807 passed native tests/sanitizers, WASM packaging, complete catalog checks, docs validation and sandboxed browser tests for implementation commit 016f187. PR creation was denied by the credential; branch-plus-CI delivery follows Jonathan's explicit amendment above.

Security integration verification: Build https://github.com/jonathanperis/solar-system-simulator/actions/runs/35243274760 and CodeQL https://github.com/jonathanperis/solar-system-simulator/actions/runs/35243274774 passed for ee35fba. The local policy matrix passed eight trusted/untrusted event cases; runtime/docs sources match the verified learning-lab main.

Documentation/dependency audit verification, 2026-09-17: A40–A44 pass locally. All 478 repository/site links across eight Markdown files and 17 generated HTML documents resolve, including fragments. Offline checks confirm 128 core bodies, 11 presets total (10 guided presets plus the core preset), 115 Jovian moons and 1,564,244 small-body records. Native raylib 6.0 build, `make test test-build test-cli test-validators`, documented CLI runs, Node 26.9.0 docs tests (15), Astro check/build, sitemap/route checks, Emscripten 6.0.9 packaging, standalone conic and native/WASM comparison checks, and five isolated local Chrome browser tests pass. Clean locked npm installation and audit report zero vulnerabilities. GitHub About description/homepage/11 topics were updated through gh and read back. Read-only browser inspection confirms the revised build/pipeline copy; durable build-guide snapshot/screenshot is in ignored `playwright-evidence/documentation-audit-20260917/`. The old ignored standalone HTML is preserved at `build/documentation-audit/legacy-public-runtime.html`. No source snapshot refresh or remote branch delivery occurred; hosted CI/Pages verification for these uncommitted changes and a fresh sanitizer run are not claimed. Eleven unique external links outside repository/Pages targets were not exhaustively fetched. Regression scan: 12 callers checked, 9 assertions checked, 1 flagged/fixed (A44 wording now distinguishes raylib-linked and raylib-free modules).

Beginner-first UX verification, 2026-09-18: A45–A49 pass locally; all eleven audit findings are closed. The scene begins at y=231 on 1440×1000 desktop and y=349 on 390×844 mobile, with everyday controls in the same viewport. Sixteen Node tests, Astro check/build, generated routes, three artifact-validator tests, all nine sandboxed Chrome browser journeys, and `make test` pass. After the 16-page local visual/semantic tour found scrolling mobile Close controls, sticky dismissal was added and the three affected mobile tests passed; the final compact comparison heading passed its focused browser journey and route check. The full Ceres preparation/confirmation path produced 10 active bodies. Named CLI evidence and the itemized report are in ignored `playwright-evidence/ux-audit-20260918/IMPLEMENTATION.md`. Existing JS/WASM companions were used (reported revision `eb6abd9ac703-dirty`; tracked C source is unchanged since that base). Fresh WASM packaging, native-app packaging, sanitizers, physical-device/cross-browser checks and a full accessibility certification were not run. No commit, push or deployment occurred. Regression scan: 24 callers checked, 148 assertions checked, 5 flagged/fixed.

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
B18|2026-10-03|new advisories in build-time docs dependencies failed the strict `npm audit` gate on every PR, and a Dependabot TypeScript 7 bump violated the `@astrojs/check` peer range|refresh the lockfile, keep TypeScript 6, ignore unsupported majors and gate at moderate severity (A51)
B19|2026-10-06|extending native/WASM replay to every example exposed last-digit drift in the phobos lesson: clang on arm64 fused multiply-adds while WebAssembly does not|compile all C with `-ffp-contract=off` and replay every shipped example (A52, A56)
B20|2026-10-06|a visible-window stall (sleep, debugger) arrived as one huge frame delta, queuing ~3.7e10 s of capped catch-up at 15 days/s|discard frames above 1 s or non-finite like a background resume (A55, V8)
B21|2026-10-06|unbounded float camera yaw lost its per-frame increment after days of auto-rotation and froze|wrap yaw into [0, 2π) (A57)
B22|2026-10-06|CSV export reopened existing paths with O_TRUNC, truncating symlink targets, blocking on FIFOs and leaving partial files on failure|lstat-checked regular targets, sibling mkstemp + fsync + rename, exclusive numbered snapshots (A57, V36)
B23|2026-10-06|comparison speed/specific energy came from the inspector selection rather than the subject index, and unchecked returns could print an uninitialized descriptor|measure by subject index, fail closed on start/format failures (A57)
B24|2026-10-06|tick counts up to 2^53 let descriptors and --dt/--duration pairs run essentially forever|cap ticks per side at 10⁹ in `lab_ticks_for` (A57, V36)
B25|2026-10-06|Emscripten `printErr` called `fail()`, so recoverable stderr permanently disabled a working runtime|log stderr; fail only on abort, non-zero exit, load error, missing WebGL or context loss (A59, V16)
B26|2026-10-06|every catalog search re-fetched, re-hashed and re-parsed all 203 index shards (~33 MB) and object dialogs parsed a whole data shard on the main thread|identity routing, a confirmed one-time scan into a columnar cache, worker record lookup with LRU (A59, V37)
B27|2026-10-06|the legacy redirect and sitemap were hand-built strings/files outside Astro and nothing enforced Astro ownership of pages|Astro page plus filename integration, Astro sitemap endpoint, generator-marker and public-HTML checks (A60, V19)
B28|2026-10-06|the basket download revoked its object URL right after `click()`, which can cancel the download|shared helper with deferred revocation (A59)
B29|2026-10-06|catalog fetch URLs were built from manifest/message data and the worker accepted unvalidated messages (CodeQL js/client-side-request-forgery, js/missing-origin-check)|pinned-filename URL builder under `catalog/` and strict worker message validation (A59, V37)
B30|2026-10-06|ecliptic→simulation mapped (X,Z,Y), a reflection: prograde orbits had −Y angular momentum and drew clockwise from north|proper rotation (X,Z,−Y); per-body direction test over core, lessons and experiments (A53, V7)
B31|2026-10-06|one 0.1–2 factor range for every lesson let point-mass trajectories pass through the parent while still publishing reference errors|per-lesson analytic minimum plus swept contact flag that withholds errors (A54, V39)
B32|2026-10-06|moons were added around a parent that kept its own heliocentric velocity, so family barycenters drifted (~12 m/s Earth–Moon)|place the family barycenter on the intended state (A58, V6)
B33|2026-10-06|sscanf `\t` matched any whitespace and `%[^\t]` accepted newlines in experiment names; a second fixed-iteration Kepler solver lived in satellite.c|strict tab split with strtod end-pointer and control-byte rejection; satellites propagate through `orbit.c` (A58)
B34|2026-10-06|the new no-third-party browser test ran only against analytics-free PR builds, so it failed on every main push (main embeds PUBLIC_GA_ID) and blocked Pages deploys; CI browser runs also sent real page views|a shared Playwright fixture stubs analytics hosts for every test, the security test admits exactly the CSP-listed loader only in analytics builds, and PRs rerun it against a dummy-ID build (A51, V17)
B35|2026-10-06|catalog experiments started Earth at Horizons 399 although no Moon is present, carrying a 12.4 m/s lunar reflex wobble (B32 still live in experiments)|reviewed refresh to system barycenters with DE440 system GMs; `--check` enforces barycenter IDs/targets (A58, V40)
B36|2026-10-06|the swept contact flag was described as "bodies touched" although a coarse step's chord can cross the sphere with every sample outside; monitoring silently skipped scenes over 3 bodies and the maximum phase error survived contact|rename to `contact_sphere_crossed` with conservative wording, capacity-sized monitoring including experiments, NaN maximum phase after a crossing (A54, V39)
B37|2026-10-06|`orbit_closest_approach_m` derived e from sqrt(1+2Eh²/μ²), losing ~1e-8 for circles, and the resonant-angle test passed in a mirrored frame|eccentricity-vector form with pinned conic cases; signed resonant-angle test (A53, A54)
B38|2026-10-06|catalog download progress was tied to the search that started it, so a superseding search hid progress and Stop while index downloads continued; a cancel during the last file could still install the index|worker reports download status independently of searches and re-checks Stop after every awaited file and before installing (A59, V37)
B39|2026-10-06|every download-progress message rewrote the `role=status` region (~200 announcements)|non-live progress bar plus a throttled announcer (start, each 10% or 5 s, completion) (A59, V18)
B40|2026-10-06|an invalid record message got a search-typed error and a worker crash left record lookups pending, so the dialog could hang on Loading|errors echo the request type; worker errors reject pending lookups (A59)
B41|2026-10-06|the comparison status echoed the URL `revision` parameter verbatim|only commit-hash-shaped values are shown, otherwise "unrecognized revision" (A59, V38)
B42|2026-10-06|missing URLs had no Astro-rendered 404 page and the ownership check ignored scriptable SVG/XHTML/SHTML files|`src/pages/404.astro` (noindex, not in sitemap) and an allow-list-only rule for scriptable document types in `check_docs_routes.py` (A60, V19)
B43|2026-10-06|the web app's emcc compile inherited native CFLAGS (-O2 -g), shipping DWARF with absolute build paths and unminified glue (850 KB WASM), while lab/catalog modules skipped the C11/warning flags|`WEB_CFLAGS ?= -O2` with required flags on every emcc rule, `ALL_CFLAGS` instead of override, checker rejects debug sections; Asyncify dropped after a full browser run (A56, V40)
B44|2026-10-06|`test_input_file` hardcoded build/tests fixtures, so a clean `make test-sanitize` aborted|fixtures live beside the running binary (A56)
B45|2026-10-06|the atomic replace silently overwrote read-only destinations, an interrupted run left its temporary, empty or '/'-terminated `--output` reached the filesystem, and non-finite headless runs exited 0|write-access refusal, async-signal-safe cleanup with re-raise, early usage errors, shared finite-state check, physical bounds on experiment rows (A57, V36, V40)
B46|2026-10-06|`lab_advance` reported work remaining when nothing was configured, and native snapshot failures gave no reason|return 0 when unconfigured (the comparison loop stops on it); testable numbered snapshot creation with strerror and exhaustion messages (A57)
B47|2026-10-06|`src/lab_web.c` compiled only under emcc, so CodeQL never analyzed the browser descriptor boundary|native analysis build with an Emscripten stub in the CodeQL C/C++ job (A52)
B48|2026-10-07|trails wrote depth before planets were drawn, so faint old segments in front of a planet blocked it and showed as dark scratches; trails also ran through bodies to their centres|draw trails after opaque bodies without depth writes (additive) and clip them at each drawn surface (A70)
B49|2026-10-07|sub-pixel bodies used raylib's DrawPoint3D, which draws a 0.1-unit line along +Z, so in real scale moons became streaks thousands of pixels long|camera-facing dots sized in pixels via tested `render_world_units_per_pixel` (renderer audit)
B50|2026-10-07|adaptive trail detail used on-screen extent, so a moon that had looped many times kept only ~3 points per orbit|budget detail by on-screen path length from up to 128 samples (renderer audit, A70)
B51|2026-10-07|the browser texture loader wrote to address 0 when `_malloc` failed under memory growth, fetched maps one at a time, and could reject unhandled if the runtime aborted; a shader link failure silently fell back to raylib's default shader (unlit scene, white halos)|check the pointer, keep inventory calls inside try, fetch in parallel and decode in order; treat a default-shader id or missing uniforms as unavailable (renderer audit)
B52|2026-10-07|rings and clouds could cover trails in front of them, the planet-on-ring shadow vanished without the ring map, the ring-shadow lookup sampled a mipmap inside a branch, WebGL used mediump for small real-scale shadow maths, labels ignored off-screen occluders and slid off the top, and canvas resizes reset the camera angle|trails after rings/clouds, geometry-only planet shadow, unconditional sample, highp when available, in-front occluders and clamped labels, re-fits keep the angle (renderer audit)
B53|2026-10-07|homepage atlas: Uranus sat on Vesta's chart spot, Neptune and Vesta fell under the hero copy, and viewport-anchored captions collided with plate captions, actions and the bearing control at wide or mid widths|retune illustrative chart angles, drop duplicate captions, cap the plate size, move the bearing control and plate name (A75)
B54|2026-10-07|the scene snapshot asked Horizons for the Moon, Phobos and Deimos with bare centres "399"/"499", which Horizons reads as ground-observatory codes: Phobos and Deimos started ~3e8 km from Mars and the Moon was topocentric (~4,000 km off); a session test compared the scene with the same snapshot, so nothing failed. The barycentric-core lesson also starts from the dated sky but was labelled undated|centres "@399"/"@499", re-fetched with `scene_epoch.py --refresh --only 4 6 7`; the validator rejects any moon farther than 0.5–1.5× its own periapsis/apoapsis, and C tests bound Phobos and the Moon independently; one `lesson_starts_at_epoch`/`simulation_session_is_dated` predicate (PR #40 review)
B55|2026-10-09|the informative macOS job lived in the Build workflow, and Deploy Pages runs only after Build succeeds: a job queued on GitHub's macOS runner pool held the deploy of 89df1ad, and cancelling it skipped that deploy although every Linux job had passed|the macOS job moved to its own `macOS` workflow (same check name, still not required)
