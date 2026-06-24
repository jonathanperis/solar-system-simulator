# Raylib renderer beauty pass

## Goal

Apply the renderer beautification sequence Jonathan approved, one step at a time, without changing simulation physics.

## Steps

1. Add RED tests for render style helpers, trail fading, deterministic starfield samples, and smoothed camera targeting.
2. Add a procedural starfield/backdrop at the raylib boundary.
3. Add solid Sun styling and body-specific surface styling.
4. Add sun-lit highlights as presentation-only geometry.
5. Fade trail segments by age while preserving full trail history.
6. Smooth camera focus transitions using the existing orbit camera helper layer.
7. Keep the render texture matched to the visible canvas frame.
8. Update README, docs, and runtime shell copy so the site explains the new visual pass honestly.
9. Verify native tests, WASM build, docs build, and browser runtime.

## Non-goals

- No physics constants or initial states change.
- No texture asset pack or shader-material stack yet.
- No outer planets or new bodies in this pass.
