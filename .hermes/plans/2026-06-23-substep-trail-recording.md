# Fix weird polygon orbit traces from long renderer frames

## Symptom
- User screenshot at ~559.95 elapsed days shows orbit traces as sparse straight-sided polygons/sharp bends instead of smooth trajectories.
- The physics state remained stable after the previous Martian moon plane fix, so this iteration targets app/trail sampling rather than orbital initial conditions.

## Root-cause hypothesis
- The app clamps physics into 300-second substeps for stability, but recorded motion trails only once per rendered frame.
- When a browser/WebAssembly frame is delayed or throttled, one visual frame can advance hours or days of simulation time. The trail then connects only the start/end positions for that long frame, producing long chords and polygon corners even though physics substeps were smooth.

## Fix plan
1. Add an app-level simulation stepping helper that advances the system in bounded physics substeps and records trail samples after every substep.
2. Cover the helper with a regression test proving a one-day frame records 288 trail samples at the existing 300-second max physics step, not one sparse endpoint.
3. Replace the `main.c` frame update path with the helper so native and web builds use the same trail cadence.
4. Verify with focused tests, full build/test suite, diff whitespace check, and headless raylib smoke.
