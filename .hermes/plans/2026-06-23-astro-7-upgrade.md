# Astro 7 Static Pages Upgrade Implementation Plan

> **For Hermes:** Use subagent-driven-development skill to implement this plan task-by-task.

**Goal:** Upgrade the simulator's GitHub Pages docs surface from Astro 6.4 to Astro 7 and adopt only the Astro 7 features that fit this repository's static Pages deployment.

**Architecture:** The repo has a C/raylib simulator plus a static Astro docs site under `docs/`. The docs site is built in the Pages workflow after the WebAssembly artifact is produced by the `Build` workflow. Astro 7's dependency-level improvements apply directly; server-only routing/cache features do not fit the current `output: 'static'` GitHub Pages model.

**Tech Stack:** Astro 7, Vite 8/Rolldown via Astro, Node 24 in GitHub Actions, npm lockfile, static GitHub Pages.

---

## Progress Snapshot

| Area | Evidence checked | Plan | Implementation | Validation |
| --- | --- | --- | --- | --- |
| Repo scope | `/opt/data/github/jonathanperis/solar-system-simulator`, `origin=https://github.com/jonathanperis/solar-system-simulator.git`, branch `main` clean before work | Use PR branch `chore/astro-7-upgrade` | Done | `git status` reviewed before edits |
| Existing Astro surface | `docs/package.json`, `docs/astro.config.mjs`, `.github/workflows/deploy-pages.yml` | Upgrade docs package only | Done | Baseline `npm run build` passed before upgrade |
| Astro 7 feature fit | Astro 7 release notes + static Pages workflow | Adopt dependency/runtime upgrade and background dev lifecycle scripts; skip SSR-only routing/cache features | Done | Build, route, and asset checks planned below |
| GitHub Pages runtime | Deploy workflow already uses Node 24 + `npm ci` + `npm run build` | Keep workflow as-is | Done | Workflow file inspected |

Overall progress: this plan records the scoped upgrade and feature triage. No simulator physics/rendering code is changed by the Astro upgrade.

---

## Feature Fit Decisions

### Adopt now

1. **Astro 7 dependency upgrade**
   - Upgrade `docs` from `astro ^6.4.2` to `astro ^7.0.2`.
   - Refresh `docs/package-lock.json` so npm CI resolves the Astro 7 toolchain.
   - Benefit: Rust compiler, Vite 8/Rolldown path, queued rendering defaults, and stricter template validation.

2. **Node 24 Pages runtime**
   - Keep `.github/workflows/deploy-pages.yml` on `actions/setup-node` `node-version: 24`.
   - This already satisfies Astro 7's `node >=22.12.0` requirement and avoids Bun's older embedded Node runtime pitfalls.

3. **Agent-friendly background dev lifecycle**
   - Add docs package scripts for `astro dev --background`, `astro dev status`, `astro dev logs`, and `astro dev stop`.
   - Benefit: future Hermes lanes can run and cleanly inspect/stop the Astro dev server without orphaning foreground preview processes.

### Skip for this repo unless hosting changes

1. **`src/fetch.ts` advanced routing**
   - The site is static (`output: 'static'`) and published through GitHub Pages. There is no SSR request pipeline for custom fetch handlers.

2. **Route caching / CDN cache providers**
   - Cache APIs are useful for SSR/adapters or supported CDN runtimes, not static pre-rendered Pages artifacts.

3. **Explicit Sätteri Markdown processor wiring**
   - This repo's docs pages are `.astro`, not Markdown/MDX content routes. Astro 7's default Markdown improvements are sufficient; adding Markdown processor dependencies/config would be dead surface.

---

## Implementation Tasks

### Task 1: Baseline the current docs and simulator gates

**Files:** none

**Steps:**
1. Run `npm run build` from `docs/` on Astro 6.4 and confirm the current site builds.
2. Run `make test` from the repo root to confirm the simulator test suite is healthy before touching docs dependencies.

**Expected:** current build and C test suite pass.

### Task 2: Upgrade docs dependencies

**Files:**
- Modify: `docs/package.json`
- Modify: `docs/package-lock.json`

**Steps:**
1. Run `npm install astro@^7.0.2 typescript@^6.0.3` from `docs/`.
2. Confirm `docs/package.json` now declares Astro 7 and TypeScript 6-compatible ranges.
3. Confirm `docs/package-lock.json` changed and resolves Astro 7 packages.

**Expected:** npm install completes without peer dependency errors.

### Task 3: Add background dev scripts

**Files:**
- Modify: `docs/package.json`

**Steps:**
1. Add scripts:
   - `dev:background`: `astro dev --background --host 127.0.0.1`
   - `dev:status`: `astro dev status`
   - `dev:logs`: `astro dev logs`
   - `dev:stop`: `astro dev stop`
2. Do not alter production `build` or `preview` commands.

**Expected:** future local/agent docs development can start, inspect, log, and stop the Astro dev server cleanly.

### Task 4: Verify Astro 7 static output

**Files:** none

**Steps:**
1. Run `npm run build` from `docs/`.
2. Run `python3 tools/check_docs_routes.py docs/dist` from repo root.
3. Check generated `_astro` references point at existing files under `docs/dist/_astro`.
4. Run `npm run dev:background`, then `npm run dev:status`, `npm run dev:logs`, and `npm run dev:stop` from `docs/`.

**Expected:** Astro 7 builds all 12 routes, route smoke passes, generated CSS/JS asset references exist, and the background dev lifecycle works.

### Task 5: Verify repository gates and prepare PR

**Files:**
- `.hermes/plans/2026-06-23-astro-7-upgrade.md`
- `docs/package.json`
- `docs/package-lock.json`

**Steps:**
1. Run `make test` from repo root.
2. Run `git diff --check`.
3. Commit the plan and docs dependency upgrade.
4. Push branch `chore/astro-7-upgrade`.
5. Open a PR against `main` with the local verification evidence and feature-fit decisions.

**Expected:** branch is pushed and PR is ready for review/CI.
