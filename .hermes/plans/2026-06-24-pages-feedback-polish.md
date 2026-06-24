# Pages feedback polish

## Goal

Apply Jonathan's feedback on the live Pages site:

1. Give docs pages the same top breathing room as other pages.
2. Stop docs manual/card overlap and clipping.
3. Replace the homepage embedded browser bundle with a dedicated WASM handoff.
4. Move static renderer notes out of the canvas and into the WASM page shell.
5. Add Google Analytics tag `G-WPC2ZGVDD0` like the other Pages repos.
6. Add a favicon using the homepage golden disc mark.

## Implementation

- Add docs layout top margin, matched sidebar/hero alignment, article grid gaps, and sidebar scroll bounds.
- Replace homepage iframe section with a source-backed runtime launch card.
- Add static scene/physics/renderer notes above the standalone WASM canvas.
- Reduce renderer overlay to live state only: elapsed days, focus, view/zoom, and basic controls.
- Add Astro `Analytics.astro`, wire it into `BaseLayout.astro`, and set the deploy workflow GA env.
- Generate `docs/public/favicon.ico` as a multi-size golden disc/sun mark matching the homepage brand.
- Strengthen docs/WASM route checks for the new structure, analytics tag, and favicon.

## Verification

- `make web`
- `python3 tools/check_wasm_artifacts.py build/web`
- `make test`
- `npm run build`
- `python3 ../tools/check_docs_routes.py dist`
- `git diff --check`
- Local preview route smoke for homepage, docs, docs/controls, WASM, and favicon.
- Browser QA for homepage, docs root, docs controls, standalone WASM, favicon, overflow, analytics markers, and docs filter behavior.
