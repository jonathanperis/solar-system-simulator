# Revert renderer overhaul; keep WASM/page responsiveness

## Request

Jonathan reported that the renderer overhaul still was not working and asked to revert to the original state before the renderer overhaul, keeping only responsiveness between the renderer and webpage.

## Scope

Revert to the pre-overhaul renderer/app baseline from parent commit `47af292c` for:

- `src/render/renderer.c`
- `src/render/renderer.h`
- `src/app/orbit_camera.c`
- `src/app/orbit_camera.h`
- renderer/orbit-camera tests
- docs/copy that claimed beauty-pass renderer behavior

Keep only the webpage/canvas sizing bridge:

- `web/shell.html` keeps responsive `.canvas-wrap` and full-size `canvas.emscripten` CSS.
- `src/main.c` keeps web-only `EM_JS` helpers to read the served canvas frame size before `InitWindow()`.
- `tools/check_wasm_artifacts.py` checks that the web build does not return to a hardcoded 1280x720 `InitWindow()` path.

## Verification

Run native, tests, WASM, docs build, route checks, whitespace checks, local browser smoke, then push and watch GitHub Actions + Pages.
