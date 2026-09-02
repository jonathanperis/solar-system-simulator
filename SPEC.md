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

C8: public site follows softened illustrated cosmic cockpit direction in `DESIGN.md`.

C9: current renderer baseline stays simple after beauty-pass rollback; no resurrection without explicit task.

C10: ⊥ ECS, scene format, asset manager, shader stack, ephemeris loader before concrete need.

## §I

I.cli: `make` → native app

I.test: `make test` → all C tests

I.run: `make run` → raylib app

I.web: `make web` → checked HTML + JS + WASM

I.dist: `make dist-wasm` → WASM zip

I.docs: `make docs-check` → generated route checks

I.sim: `SolarSystem`, `Body`, `solar_system_create_*`, `solar_system_step`

I.app: body trails, stable orbit camera, bounded simulation stepping

I.render: illustrative | real-scale transforms + raylib drawing

I.controls: `Tab` | `C` focus; `V` scale; wheel zoom

I.pages: `/`, `/docs/`, `/docs/architecture/`, `/docs/simulation-core/`, `/docs/rendering/`, `/docs/controls/`, `/docs/build-and-web/`, `/docs/roadmap/`, `/physics/`, `/body-catalog/`, `/source-atlas/`, `/pipeline/`, `/wasm/solar-system-simulator.html`

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

V8: app physics step ≤300 simulated seconds; trail sample recorded after each substep.

V9: trails retain full-run visual span in bounded memory; at point cap older samples decimate; renderer draws ≤1024 trail segments/body.

V10: illustrative transforms affect render output only; asteroid radius=`0.03` render units; real-scale uses same physical scale for positions + radii with no radius clamp.

V11: camera focus covers ∀ bodies; wheel changes clamped distance only; pitch preserved.

V12: web `InitWindow()` dimensions derive from served `.canvas-wrap` before WebGL creation; canvas fills frame.

V13: ∀ Pages links/assets base-path-safe under `/solar-system-simulator/`.

V14: Pages deploy only after successful native tests + WASM artifact validation.

V15: each new-body milestone updates constants, initialization, tests, renderer visibility, app/catalog/docs, route checks, verification.

V16: public claims trace to source/tests; checked claims match implementation; loading/runtime failure always visible, never blank unexplained canvas.

V17: local/fork docs builds emit no analytics; deployed Pages build emits configured analytics only; public analytics disclosure exists.

V18: keyboard focus always visible; runtime status announces changes; interactive content uses valid semantic HTML.

V19: checked WASM begins `\0asm\1\0\0\0`; docs checker resolves all internal routes/assets under configured base path.

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

## §B

id|date|cause|fix
B1|2026-06-23|Phobos/Deimos relative velocity on Y → vertical trails|V7
B2|2026-06-23|trails sampled per render frame, not physics substep → polygon chords|V8
B3|2026-06-24|full trail history drawn without bound → long-run render cost|V9
B4|2026-06-25|CSS canvas frame ≠ hardcoded `InitWindow(1280,720)` → gutters|V12
B5|2026-06-25|renderer beauty pass broke expected scene/viewport → rollback|V10,V12
B6|2026-09-02|unbounded full-sample trails + draw cap too high → long-run memory/frame-cost growth|V9
B7|2026-09-02|public docs claimed easing, HUD labels, WASM magic check absent from code|V16,V19
