# Cartoon docs manual and WASM cockpit polish

## Goal

Keep the accepted cartoon cosmic cockpit aesthetic, then extend it to the two routes Jonathan called out:

- `/wasm/solar-system-simulator.html` should no longer look like a raw Emscripten shell. It should feel like a standalone cartoon cockpit runtime with status, controls, links, and source-backed context.
- `/docs/` should adopt the Super Mango docs structure: sticky manual sidebar, filterable route list, grouped manual cards, route counts, and a full index, translated into this simulator's physics/source vocabulary.

## Steps

1. Inspect live Super Mango docs structure and current Solar docs/WASM source.
2. Centralize docs route codes/groups in `docs/src/lib/site.ts`.
3. Rebuild `DocsShell` and `/docs/` as an orbit manual command center.
4. Restyle docs-specific CSS while preserving the current cartoon palette.
5. Replace `web/shell.html` with a cartoon runtime shell and update WASM artifact checks.
6. Build WASM/docs, preview under the GitHub Pages base path, visually verify docs and WASM, then commit/push/watch Pages.

## Acceptance checks

- Docs root contains `Trace every orbit wire without one giant scroll`, `Solar manual routes`, and `Every orbit manual page`.
- WASM shell contains `Cartoon cockpit runtime`, `Launch-ready C/raylib canvas`, and `Controls expose real simulator state`.
- `make web`, `npm run build`, route smoke checks, browser visual QA, and live Pages smoke pass.
