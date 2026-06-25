# WASM long-run viewport investigation

## Report

Jonathan reported that after some time the live WASM page no longer looked correct. The screenshot showed:

- the page status still said `RUNNING PHYSICS SIMULATION`;
- HUD still rendered, with `Focus: Sun` and zoom around `7.6`;
- the active WebGL scene occupied a lower-left 1280x720-like rectangle;
- the larger served canvas frame had a dark top band and a dark right gutter.

## Root cause

The prior fix let CSS make the canvas element fill the `.canvas-wrap` frame, but the C/raylib app still started with `InitWindow(1280, 720)`. In raylib's web backend, the render size is initialized from that window size. When the served frame was wider/taller than 1280x720, drawing could remain bound to the original render dimensions inside the larger DOM frame.

A live resize loop was intentionally avoided because earlier browser QA showed that repeatedly resizing the WebGL context could lose the context.

## Fix

Initialize the web build from the served frame size before raylib creates the WebGL window:

- add `solar_web_initial_canvas_width()` and `solar_web_initial_canvas_height()` via `EM_JS`;
- read `.canvas-wrap.getBoundingClientRect()` before `InitWindow()`;
- pass those dimensions into `InitWindow()` for `PLATFORM_WEB`;
- keep native defaults at 1280x720;
- extend the WASM artifact checker so a hardcoded web `InitWindow(1280, 720)` cannot regress.

## Verification

- RED: `tools/check_wasm_artifacts.py` failed before the EM_JS sizing helpers were added.
- GREEN: `make clean && make && make test && make web` passed.
- Docs build and route checks passed.
- Local production-base preview showed canvas attribute size, WebGL drawing buffer size, CSS rect, and wrapper rect all matching.
- After waiting and wheel-zooming, the canvas stayed filled, WebGL context stayed alive, and Focus: Sun stayed visually centered.
