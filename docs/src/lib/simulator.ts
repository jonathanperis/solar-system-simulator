import { lessonOptions, sceneNames } from './lessonCatalog.ts';
import { implementedBodies } from './bodies.ts';
import { downloadText, errorMessage } from './browser.ts';
type Readout = Pick<HTMLElement, 'textContent'>;
interface RuntimeReadouts {
  status: Readout;
  controls: Readout;
  /** Shows the date; `data-seconds` carries the exact simulated seconds (whole 15 s ticks in scenes) for tools and tests. */
  elapsed: Readout & { dataset?: DOMStringMap };
  clockNote?: Readout;
  today?: Pick<HTMLElement, 'textContent' | 'hidden' | 'dataset'>;
  dateOpen?: Pick<HTMLElement, 'hidden'>;
  interval: Readout;
  parent: Readout;
  distance: Readout;
  speed: Readout;
  period?: Readout;
  fromEarth?: Pick<HTMLElement, 'textContent'>;
  fromEarthRow?: Pick<HTMLElement, 'hidden'>[];
  name?: Readout;
  mass: Readout;
  radius: Readout;
  camera: Readout;
  achieved: Readout;
  pending: Readout;
  scene: Readout;
  acceleration: Readout;
  specificEnergy: Readout;
  energy: Readout;
  energyChange: Readout;
  momentum: Readout;
  angularMomentum: Readout;
  integration: Readout;
  magnification: Readout;
  position: Readout;
  velocity: Readout;
}

interface RuntimeControls {
  panels: HTMLFieldSetElement[];
  /* Every state report (A141 waits on it for a linked instant). */
  onState?: (state: RuntimeState) => void;
  filterStatus: Readout;
  pause: HTMLButtonElement;
  step: HTMLButtonElement;
  /* A128: on/off controls are role="switch" buttons (aria-checked). */
  rotate: Switch;
  body: HTMLSelectElement;
  /* A130: live matches under the Find name box. */
  results?: HTMLElement;
  /* Speed slider over C's five presets (A120). */
  speed: HTMLInputElement;
  speedLabel?: Readout;
  /* Scale and trail-frame segmented buttons (A128). */
  scale?: HTMLButtonElement[];
  trailFrames?: HTMLButtonElement[];
  search: HTMLInputElement;
  group: HTMLSelectElement;
  lesson: HTMLSelectElement;
  scene: HTMLSelectElement;
  method: HTMLSelectElement;
  dt: HTMLInputElement;
  factor: HTMLInputElement;
  vectors: Switch;
  grid: Switch;
  labels: Switch;
  orbits: Switch;
  contact: HTMLButtonElement;
  follow?: Switch;
  tour?: HTMLButtonElement;
  belt?: Switch;
  constellations?: Switch;
  /* Called with the selected body's name whenever C's selection changes. */
  onSelection?: (name: string) => void;
}

interface LabState {
  lesson: number; method: number; dt: number; ticks: number; factor: number;
  acceleration: number; specificEnergy: number; energy: number; energyChange: number;
  isolated: boolean; momentum: number; angularMomentum: number; magnification: number;
  trailFrame: number; vectors: boolean; position: number[]; velocity: number[];
  contactMode: number;
  /* C's per-lesson lower speed bound and first contact time (0 = none). */
  minFactor?: number; contactSeconds?: number;
  /* Reference grid visibility (presentation only). */
  grid?: boolean;
  /* In-canvas body names (presentation only). */
  labels?: boolean;
  /* Osculating orbit guides (presentation only, SPEC A107). */
  orbits?: boolean;
  /* Follow camera and tour (SPEC A120), small-body cloud (A121). */
  follow?: boolean;
  touring?: boolean;
  belt?: boolean;
  constellations?: boolean;
}

export const lessonNames = lessonOptions.map(([, label]) => label);

/** Scenes and catalog experiments start at JD 2461200.5 TDB (2026-06-09,
 * SPEC A92). TDB runs ~69 s ahead of UTC; the minute shown is TDB. */
export const sceneEpochJd = 2461200.5;
export function sceneDate(elapsedSeconds: number): string {
  const ms = (sceneEpochJd - 2440587.5) * 86400000 + elapsedSeconds * 1000;
  return `${new Date(ms).toISOString().slice(0, 16).replace('T', ' ')} TDB`;
}
/** Scene seconds at a UTC instant (A133), as scene_epoch_seconds_at_unix_time
 * in C: TDB runs 69.184 s ahead of UTC. */
export function sceneSecondsAt(unixMs: number): number {
  return unixMs / 1000 - (sceneEpochJd - 2440587.5) * 86400 + 69.184;
}
/** Scene seconds at a TDB calendar instant typed as "YYYY-MM-DD" and
 * "HH:MM" (A134), or NaN if it is not a valid date in the runnable range:
 * the epoch through 2076-06-08 23:59 (the C command carries whole seconds as
 * an int). */
export const dateRunMaxSeconds = (Date.UTC(2076, 5, 8, 23, 59) - Date.UTC(2026, 5, 9)) / 1000;
export function sceneSecondsAtDate(date: string, time: string): number {
  const d = /^(\d{4})-(\d{2})-(\d{2})$/.exec(date), t = /^(\d{2}):(\d{2})/.exec(time || '00:00');
  if (!d || !t) return NaN;
  const ms = Date.UTC(Number(d[1]), Number(d[2]) - 1, Number(d[3]), Number(t[1]), Number(t[2]));
  const seconds = ms / 1000 - (sceneEpochJd - 2440587.5) * 86400;
  return seconds >= 0 && seconds <= dateRunMaxSeconds ? seconds : NaN;
}
/** True when the clock already shows `target`'s minute: the form works in
 * whole minutes, so a target up to a minute behind the clock is not a
 * reason to restart (Date opened right after Today, say). */
export function dateRunArrived(target: number, elapsed: number): boolean {
  return target <= elapsed && elapsed - target < 60;
}
/** A linked instant (A141): `?at=YYYY-MM-DD` or `?at=YYYY-MM-DDTHH:MM`, in
 * TDB like the clock; NaN when malformed or outside the runnable range. */
export function sceneSecondsAtLink(value: string | null): number {
  const match = /^(\d{4}-\d{2}-\d{2})(?:T(\d{2}:\d{2}))?$/.exec(value ?? '');
  return match ? sceneSecondsAtDate(match[1], match[2] ?? '00:00') : NaN;
}
/** The `at` value for a scene time, to the minute (A141). */
export function linkForSceneSeconds(seconds: number): string {
  return sceneDate(seconds).slice(0, 16).replace(' ', 'T');
}
/** How a run to `target` will go from `elapsed` at 15 simulated days per
 * second (A134): a restart first when the target is behind the clock. */
export function dateRunNote(target: number, elapsed: number): string {
  if (!Number.isFinite(target)) return 'Choose a date from 2026-06-09 to 2076-06-08.';
  if (dateRunArrived(target, elapsed)) return 'The clock is already there.';
  const behind = target < elapsed;
  const seconds = (behind ? target : target - elapsed) / (15 * 86400);
  const span = seconds < 1 ? 'under a second' : seconds < 90 ? `about ${Math.round(seconds)} s`
    : `about ${Math.round(seconds / 60)} min`;
  return behind ? `Restarts at 2026-06-09, then plays ${span}.` : `Plays forward ${span} at 15 d/s.`;
}
/** The note beside the sky clock: how far the run has come from its starting
 * sky, or that a lesson clock counts from the lesson start. */
export function clockNote(elapsedSeconds: number, dated: boolean, untilSeconds?: number | null): string {
  const days = elapsedSeconds / 86400;
  if (!dated) return 'since the lesson started';
  const note = `${days >= 0 ? '+' : ''}${days.toFixed(2)} days since 2026-06-09`;
  // During a Today or Date run (A142) the note names where it is heading.
  // No-break spaces and word joiners around its hyphens keep the arrow and
  // destination one unbreakable piece on a narrow phone.
  if (untilSeconds == null || untilSeconds <= elapsedSeconds) return note;
  const destination = sceneDate(untilSeconds).slice(0, untilSeconds % 86400 ? 16 : 10)
    .replace(' ', '\u00a0').replaceAll('-', '\u2060-\u2060');
  return `${note} →\u00a0${destination}`;
}
/* Inspector readouts (SPEC A113). C supplies SI values; these only choose how
 * to print them: digits grouped in threes with narrow no-break spaces (the SI
 * style), AU beside planetary distances, powers of ten as superscripts. A
 * no-break space binds each value to its unit, so a narrow inspector wraps
 * only at the " · " between two readings. */
const nbsp = '\u00a0';
const auMeters = 149597870700;
function grouped(value: number, decimals: number): string {
  const [whole, fraction] = value.toFixed(decimals).split('.');
  const digits = whole.replace(/\B(?=(\d{3})+(?!\d))/g, '\u202f');
  return fraction === undefined ? digits : `${digits}.${fraction}`;
}
export function formatDistance(meters: number): string {
  const km = meters / 1000;
  if (km >= 1e6) return `${grouped(km, 0)}${nbsp}km · ${(meters / auMeters).toFixed(4)}${nbsp}AU`;
  return km >= 100 ? `${grouped(km, 1)}${nbsp}km` : `${km.toFixed(3)}${nbsp}km`;
}
export function formatRadius(meters: number): string {
  const km = meters / 1000;
  return km >= 100 ? `${grouped(km, 1)}${nbsp}km` : `${km.toFixed(3)}${nbsp}km`;
}
export function formatSpeed(mps: number): string {
  if (mps >= 1000) return `${(mps / 1000).toFixed(4)}${nbsp}km/s`;
  return mps >= 1 ? `${mps.toFixed(2)}${nbsp}m/s` : `${mps.toFixed(3)}${nbsp}m/s`;
}
const superscript = '⁰¹²³⁴⁵⁶⁷⁸⁹';
export function formatMass(kg: number): string {
  if (!(kg > 0) || !Number.isFinite(kg)) return `${kg}${nbsp}kg`;
  let exponent = Math.floor(Math.log10(kg));
  let mantissa = (kg / 10 ** exponent).toFixed(4);
  if (mantissa.startsWith('10')) { exponent += 1; mantissa = (kg / 10 ** exponent).toFixed(4); }
  const power = String(exponent).replace('-', '⁻').replace(/\d/g, digit => superscript[Number(digit)]);
  return `${mantissa}${nbsp}×${nbsp}10${power}${nbsp}kg`;
}
/** From Earth (A136): the distance, in AU beyond a hundredth of one, and
 * how long light takes to cross it (c = 299 792 458 m/s, exact). */
export function formatFromEarth(meters: number): string {
  const distance = meters >= 0.01 * auMeters ? `${(meters / auMeters).toFixed(4)}${nbsp}AU`
    : `${grouped(meters / 1000, 0)}${nbsp}km`;
  // Round the total before splitting it, so no unit ever reads 60.
  const s = meters / 299792458, wholeS = Math.round(s), wholeMin = Math.round(s / 60);
  const light = s < 59.995 ? `${s.toFixed(2)}${nbsp}s`
    : wholeS < 3600 ? `${Math.floor(wholeS / 60)}${nbsp}min${nbsp}${wholeS % 60}${nbsp}s`
    : `${Math.floor(wholeMin / 60)}${nbsp}h${nbsp}${wholeMin % 60}${nbsp}min`;
  // Like the other readings (A113), it wraps only before the " · ".
  return `${distance} ·${nbsp}light${nbsp}${light}`;
}
export function formatPeriod(seconds: number): string {
  const days = seconds / 86400;
  if (days >= 730.5) return `${grouped(days, 2)}${nbsp}d · ${(days / 365.25).toFixed(2)}${nbsp}yr`;
  return days >= 1 ? `${days.toFixed(3)}${nbsp}d` : `${(seconds / 3600).toFixed(3)}${nbsp}h`;
}
/** Preset indices of the astronomy scenes (main and family), as C numbers them. */
export const sceneIndices = lessonOptions.flatMap(([name], index) => sceneNames.includes(name) ? [index] : []);
/** Preset index C uses for a body's home scene (0 = main scene). */
export const sceneIndexOf = (scene: string): number => lessonOptions.findIndex(([name]) => name === scene);
const collisionLesson = lessonOptions.findIndex(([name]) => name === 'collision');
const barycentricLesson = lessonOptions.findIndex(([name]) => name === 'barycentric-core');

interface RuntimeState {
  body: string; parent: string; view: string; cameraTarget: string; selected: number;
  paused: boolean; speedPreset: number; autoRotate: boolean;
  elapsedSeconds: number; intervalSeconds: number; trailsFailed: boolean; hasParent: boolean;
  distanceM: number; speedMps: number; massKg: number; radiusM: number; zoom: number;
  massQuality: number; radiusQuality: number; achievedTimeScale: number; pendingSeconds: number;
  shortTimescale: boolean;
  /* Two-body period around the parent (s, NaN when unbound) and whether the
   * scene starts from the dated 2026-06-09 sky. */
  periodS?: number; dated?: boolean;
  /* The elapsed time a Today run is playing to (A133), or null. */
  untilSeconds?: number | null;
  /* Distance from Earth's centre in metres (A136), null for Earth itself or
   * a scene without Earth. */
  earthM?: number | null;
}

// Matches SolarCommand in src/main.c. All actions execute in the C runtime.
export const runtimeCommands = { pause: 0, step: 1, reset: 2, speed: 3, select: 4, view: 5, zoom: 6, rotate: 7, frame: 8, frameBody: 9,
  trails: 10, vectors: 11, background: 12, contact: 13, grid: 14, labels: 15, orbits: 16,
  follow: 17, tour: 18, belt: 19, constellations: 20, runUntil: 21 } as const;

interface RuntimeBody { index: number; name: string; group: string }

export function filterRuntimeBodies(bodies: RuntimeBody[], search: string, group: string, selected: number): RuntimeBody[] {
  const query = search.trim().toLowerCase();
  // Filtering changes menu visibility only. Keep C's current selection reachable
  // even if it falls outside the filter, until the user selects a different body.
  return bodies.filter(body => body.index === selected ||
    ((!group || body.group === group) && body.name.toLowerCase().includes(query)));
}

export function runtimeBodyFilterStatus(bodies: RuntimeBody[], search: string, group: string, selected: number): string {
  const matches = filterRuntimeBodies(bodies, search, group, -1);
  const current = bodies.find(body => body.index === selected);
  const retained = current && !matches.includes(current) ? ` ${current.name} remains selected.` : '';
  return `${matches.length ? `${matches.length} matching ${matches.length === 1 ? 'body' : 'bodies'}.` : 'No matching bodies.'}${retained}`;
}

/** Mirrors SOLAR_EXPERIMENT_TEXT_BYTES in src/sim/experiment.h: C rejects any
 * experiment whose UTF-8 text (excluding the terminator) reaches this size. */
export const experimentTextLimitBytes = 16384;

interface PreparedExperiment { text: string; snapshot: string; count: number }

/** Validate a prepared catalog experiment read from sessionStorage before any
 * of it reaches C. Session storage is writable by any script on this origin,
 * so its content is untrusted: the shape is checked here and the text
 * is bounded by the C parser limit. ccall('string') copies the argument onto
 * the 256 KiB WebAssembly stack, so an unbounded value could exhaust it before
 * C can reject it. C still performs the authoritative parse. */
export function parsePreparedExperiment(stored: string | null): PreparedExperiment {
  if (!stored) throw new Error('No prepared experiment. Choose bodies in the small-body atlas first.');
  let value: unknown;
  try { value = JSON.parse(stored); } catch { throw new Error('The prepared experiment could not be read. Prepare it again in the atlas.'); }
  const prepared = value as Partial<PreparedExperiment> | null;
  if (!prepared || typeof prepared.text !== 'string' || typeof prepared.snapshot !== 'string' || !/^[0-9a-f]{1,128}$/i.test(prepared.snapshot)
    || !Number.isInteger(prepared.count) || prepared.count! < 1 || prepared.count! > 16 || !prepared.text.startsWith('SOLAR_EXPERIMENT_V1 '))
    throw new Error('Invalid prepared experiment. Return to the atlas.');
  if (new TextEncoder().encode(prepared.text).length >= experimentTextLimitBytes)
    throw new Error(`Prepared experiment is too large (limit ${experimentTextLimitBytes.toLocaleString('en-US')} bytes). Prepare it again in the atlas.`);
  return { text: prepared.text, snapshot: prepared.snapshot, count: prepared.count! };
}

/** An on/off control: a role="switch" button with aria-checked (A128). */
export type Switch = Pick<HTMLElement, 'setAttribute' | 'getAttribute'>;
export function setSwitch(element: Switch | undefined, on: boolean): void {
  if (element && element.getAttribute('aria-checked') !== String(on)) element.setAttribute('aria-checked', String(on));
}
/** Mark the one pressed button of a segmented control. */
export function setSegment(buttons: HTMLButtonElement[] | undefined, pressed: (button: HTMLButtonElement) => boolean): void {
  for (const button of buttons ?? []) {
    const on = String(pressed(button));
    if (button.getAttribute('aria-pressed') !== on) button.setAttribute('aria-pressed', on);
  }
}

function setText(element: Readout, text: string): void {
  if (element.textContent !== text) element.textContent = text;
}

/** GLFW globally cancels Tab and Backspace. Stop only those listener calls;
 * other keys must reach native controls, while C gates shortcuts by focus. */
export function routeSimulatorKeyboard(event: Pick<KeyboardEvent, 'key' | 'stopImmediatePropagation' | 'preventDefault'>, canvasFocused: boolean): void {
  if (event.key === 'Tab' || event.key === 'Backspace') event.stopImmediatePropagation();
  // Space pauses and the arrows hop between bodies (A120): neither may scroll the page.
  else if (canvasFocused && (event.key === ' ' || event.key.startsWith('Arrow'))) event.preventDefault();
}

/** Find's live matches (A130): names that start with the query, then names
 * that contain it, at most eight, within the chosen group. */
export function findMatches(bodies: readonly RuntimeBody[], query: string, group: string): RuntimeBody[] {
  const q = query.trim().toLowerCase();
  if (!q) return [];
  const pool = bodies.filter(body => !group || body.group === group);
  const starts = pool.filter(body => body.name.toLowerCase().startsWith(q));
  const contains = pool.filter(body => !body.name.toLowerCase().startsWith(q) && body.name.toLowerCase().includes(q));
  return [...starts, ...contains].slice(0, 8);
}

/** Readable trail sample spacing for the Data sheet. */
export function formatSpacing(seconds: number): string {
  // Short spacings (contact lessons step in tenths of a second) keep three
  // significant figures rather than rounding to "0 s".
  if (seconds < 60) return `${Number(seconds.toPrecision(3))}\u00a0s between trail samples`;
  return seconds < 3600 ? `${Math.round(seconds)}\u00a0s between trail samples` : `${(seconds / 3600).toFixed(2)}\u00a0h between trail samples`;
}

/** Compact and spoken labels of C's speed presets (A120). */
export function speedLabels(shortTimescale: boolean): { short: string[]; spoken: string[] } {
  return shortTimescale
    ? { short: ['1 s/s', '5 s/s', '10 s/s', '25 s/s', '50 s/s'],
      spoken: ['1 second / second', '5 seconds / second', '10 seconds / second', '25 seconds / second', '50 seconds / second'] }
    : { short: ['1 h/s', '1 d/s', '5 d/s', '10 d/s', '15 d/s'],
      spoken: ['1 hour / second', '1 day / second', '5 days / second', '10 days / second', '15 days / second'] };
}

export function moveSelectByKey(select: Pick<HTMLSelectElement, 'selectedIndex' | 'options' | 'dispatchEvent'>, key: string): boolean {
  const enabled = Array.from(select.options, (option, index) => option.disabled ? -1 : index).filter(index => index >= 0);
  if (!enabled.length) return false;
  let next = select.selectedIndex;
  if (key === 'Home') next = enabled[0];
  else if (key === 'End') next = enabled.at(-1)!;
  else if (key === 'ArrowDown') next = enabled.find(index => index > next) ?? next;
  else if (key === 'ArrowUp') next = enabled.findLast(index => index < next) ?? next;
  else return false;
  if (next !== select.selectedIndex) {
    select.selectedIndex = next;
    select.dispatchEvent(new Event('change', { bubbles: true }));
  }
  return true;
}

/** Emscripten calls this boundary; all physics remains inside the C runtime. */
type TextureRuntime = {
  ccall?: (name: string, result: string | null, types: string[], args: unknown[]) => unknown;
  _malloc?: (bytes: number) => number;
  _free?: (pointer: number) => void;
  _solar_web_texture_count?: () => number;
  _solar_web_load_texture?: (slot: number, pointer: number, length: number) => number;
  _solar_web_load_belt?: (pointer: number, length: number) => number;
  HEAPU8?: Uint8Array;
};

/** Fetch the small-body sample (SPEC A121) and hand it to C. It is optional:
 * a failure leaves the cloud off and is reported on the canvas. */
export async function loadRuntimeBelt(runtime: TextureRuntime, canvas: HTMLCanvasElement, artifactUrl: URL): Promise<void> {
  if (!runtime._malloc || !runtime._free || !runtime._solar_web_load_belt) return;
  try {
    const response = await fetch(new URL('../belt/small_bodies.bin', artifactUrl));
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    const bytes = new Uint8Array(await response.arrayBuffer());
    const pointer = runtime._malloc(bytes.length);
    if (!pointer) throw new Error('out of WebAssembly memory');
    try {
      runtime.HEAPU8!.set(bytes, pointer);
      canvas.dataset.belt = runtime._solar_web_load_belt(pointer, bytes.length) ? 'loaded' : 'invalid';
    } finally {
      runtime._free(pointer);
    }
  } catch (error) {
    canvas.dataset.belt = 'unavailable';
    console.warn(`Small-body cloud unavailable (${errorMessage(error)}).`);
  }
}

/**
 * Fetch the planet/backdrop maps after the first frame so the scene starts at
 * once with lit colours and sharpens as each texture arrives (SPEC A65). C owns
 * the inventory (file names and order); this only moves bytes. A failed file is
 * logged and skipped: the body keeps its lit-colour fallback. Textures live in
 * the site's textures/ folder beside wasm/, so the URL is base-path safe.
 */
/** Texture slots fetched before the rest (A131): the 14 Solar System Scope
 * maps that come first in the C inventory (Sun, planets, Moon, rings, sky). */
export const firstWaveTextures = 14;

export async function loadRuntimeTextures(runtime: TextureRuntime, canvas: HTMLCanvasElement, artifactUrl: URL,
  afterFirstWave?: () => Promise<void>): Promise<void> {
  // The callback (the belt) runs exactly once on every path: between the
  // waves normally, or at the end when texture setup gave up early.
  let started = false;
  const between = (): Promise<void> => {
    if (started || !afterFirstWave) return Promise.resolve();
    started = true;
    return afterFirstWave().catch(() => undefined);
  };
  await loadTexturesInWaves(runtime, canvas, artifactUrl, between);
  await between();
}

async function loadTexturesInWaves(runtime: TextureRuntime, canvas: HTMLCanvasElement, artifactUrl: URL,
  between: () => Promise<void>): Promise<void> {
  let count = 0;
  const files: string[] = [];
  try {
    count = runtime._solar_web_texture_count?.() ?? 0;
    for (let slot = 0; slot < count; slot++) files.push(String(runtime.ccall?.('solar_web_texture_file', 'string', ['number'], [slot]) ?? ''));
  } catch (error) {
    console.warn(`Texture inventory unavailable (${errorMessage(error)}); keeping lit-colour fallbacks.`);
    return;
  }
  if (!count || !runtime._malloc || !runtime._free || !runtime._solar_web_load_texture) return;
  const folder = new URL('../textures/', artifactUrl);
  let loaded = 0;
  canvas.dataset.textures = `0/${count}`;
  // Download in two waves (A131): the Sun, planet, ring and sky maps first,
  // which every view shows, then the moon maps, so on a slow connection the
  // planets are not starved by maps of moons too small to see yet. Within a
  // wave the network overlaps; maps go to C one at a time in inventory order
  // so decoded pixels never pile up in WASM.
  const fetchMap = async (file: string): Promise<Uint8Array> => {
    if (!/^[a-z_]+\.(jpg|png)$/.test(file)) throw new Error(`unexpected texture name ${JSON.stringify(file)}`);
    const response = await fetch(new URL(file, folder));
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    return new Uint8Array(await response.arrayBuffer());
  };
  const firstWave = files.slice(0, Math.min(firstWaveTextures, files.length)).map(fetchMap);
  // The overview shows the asteroid belt long before any moon is big enough
  // to show its map, so the belt comes between the waves: the moon maps
  // start once it has arrived (or failed).
  const settled = Promise.allSettled(firstWave).then(between);
  const downloads = [...firstWave, ...files.slice(firstWave.length).map(file => settled.then(() => fetchMap(file)))];
  downloads.forEach(download => download.catch(() => undefined)); // reported below, in order
  for (let slot = 0; slot < count; slot++) {
    const file = files[slot];
    try {
      const bytes = await downloads[slot];
      const pointer = runtime._malloc(bytes.length);
      // With memory growth enabled, malloc reports exhaustion by returning 0
      // instead of aborting; writing there would overwrite static data.
      if (!pointer) throw new Error('out of WebAssembly memory');
      try {
        // Read HEAPU8 after malloc: memory growth replaces the heap view.
        runtime.HEAPU8!.set(bytes, pointer);
        if (runtime._solar_web_load_texture(slot, pointer, bytes.length)) loaded++;
        else console.warn(`Texture ${file} could not be decoded; keeping the lit-colour fallback.`);
      } finally {
        runtime._free(pointer);
      }
    } catch (error) {
      console.warn(`Texture ${file || slot} unavailable (${errorMessage(error)}); keeping the lit-colour fallback.`);
    }
    canvas.dataset.textures = `${loaded}/${count}`;
  }
}

/** A body requested by `?body=slug`, with the preset index of its home scene. */
export interface RequestedBody { name: string; scene: number }

/** A guided lesson requested by `?lesson=name` (Learn page links). */
export interface RequestedLesson { index: number; dt: number }

export function createSimulatorModule(canvas: HTMLCanvasElement, readouts: RuntimeReadouts, artifactUrl: URL, controls: RuntimeControls, requestedBody?: RequestedBody, requestedLesson?: RequestedLesson) {
  let failed = false;
  let reportedBody = -1;
  let reportedSpeed = -1;
  let reportedConfig = '';
  let hasParent = false;
  let activeBodyCount = 0;
  let shortTimescale = false, lastForceTime = -Infinity, lastForceBody = -1;
  let initialSelectionApplied = false;
  // Name to select once the scene that holds it has published its body list.
  let pendingBody: string | undefined;
  const bodies: RuntimeBody[] = [];
  const groups = new Set<string>();
  const filterBodies = (): void => {
    const visible = filterRuntimeBodies(bodies, controls.search.value, controls.group.value, reportedBody);
    const options = visible.map(body => new Option(`${body.name} — ${body.group}`, String(body.index)));
    controls.body.replaceChildren(...options);
    controls.body.value = String(reportedBody);
    setText(controls.filterStatus, runtimeBodyFilterStatus(bodies, controls.search.value, controls.group.value, reportedBody));
    // A130: with a name typed, the best matches become one-click buttons
    // (names starting with the query first). The full list stays in Body.
    if (controls.results) {
      const query = controls.search.value.trim().toLowerCase();
      const matches = query ? findMatches(bodies, query, controls.group.value) : [];
      controls.results.replaceChildren(...matches.map(body => {
        const button = document.createElement('button');
        button.type = 'button';
        button.dataset.findIndex = String(body.index);
        const name = document.createElement('span');
        name.textContent = body.name;
        const group = document.createElement('span');
        group.className = 'find-group';
        group.textContent = body.group;
        button.append(name, group);
        return button;
      }));
      controls.results.hidden = matches.length === 0;
    }
  };
  const fail = (reason: unknown): void => {
    failed = true;
    controls.panels.forEach(panel => { panel.disabled = true; panel.closest<HTMLDialogElement>('dialog')?.close(); });
    setText(readouts.status, `Runtime error: ${errorMessage(reason)}`);
  };

  return {
    canvas,
    ccall: undefined as undefined | ((name:string, result:string|null, types:string[], args:unknown[])=>unknown),
    clearBodies(experiment:boolean, count:number) {
      activeBodyCount = count;
      bodies.length=0; groups.clear(); reportedBody=-1;
      controls.body.replaceChildren();
      controls.group.replaceChildren(new Option('All groups',''));
      controls.search.value='';
      const scene=canvas.closest('[data-simulator]')?.querySelector('[data-active-scene]');
      if(scene) scene.textContent=experiment?`${count} active bodies · catalog epoch JD 2461200.5 TDB`:`${count} active bodies · sky of 2026-06-09`;
    },
    _solar_web_command: undefined as ((command: number, value: number) => void) | undefined,
    _malloc: undefined as ((bytes: number) => number) | undefined,
    _free: undefined as ((pointer: number) => void) | undefined,
    _solar_web_texture_count: undefined as (() => number) | undefined,
    _solar_web_load_texture: undefined as ((slot: number, pointer: number, length: number) => number) | undefined,
    HEAPU8: undefined as Uint8Array | undefined,
    addBody(index: number, name: string, group: string) {
      bodies.push({ index, name, group });
      if (!groups.has(group)) {
        controls.group.add(new Option(group, group));
        groups.add(group);
      }
      controls.body.add(new Option(`${name} — ${group}`, String(index)));
    },
    filterBodies,
    /** Runtime index of a body in the active scene by name, or -1. */
    bodyIndex(name: string): number {
      return bodies.find(body => body.name === name)?.index ?? -1;
    },
    locateFile(path: string) {
      const url = new URL(path, artifactUrl);
      url.search = artifactUrl.search;
      return url.href;
    },
    print: (message: string) => console.log(message),
    // Emscripten routes recoverable warnings through stderr (for example
    // "wasm streaming compile failed... falling back to ArrayBuffer
    // instantiation" or WebGL notices). Log them only: real failures arrive
    // through onAbort, onExit, script.onerror, WebGL unavailability or context
    // loss, and an early fail() here would freeze a working runtime.
    printErr: (message: string) => { console.warn(message); },
    setStatus(message: string) {
      if (!failed && message) setText(readouts.status, message);
    },
    onAbort: fail,
    onExit(status: number) {
      if (status !== 0) fail(`The simulator stopped (exit status ${status}). Reload to restart.`);
    },
    forceSample: { time: 0, rows: [] as { name: string; magnitude: number; fraction: number; vector: number[] }[] },
    reportForces(sample: { time: number; rows: { name: string; magnitude: number; fraction: number; vector: number[] }[] }) {
      if (failed) return;
      const now = performance.now();
      if (now - lastForceTime < 250 && lastForceBody === reportedBody) return;
      lastForceTime = now; lastForceBody = reportedBody;
      const root = canvas.closest('[data-simulator]')!;
      root.querySelector('[data-forces-time]')!.textContent = `${Number(sample.time.toPrecision(12))} simulated seconds`;
      const body = root.querySelector('[data-runtime-forces]')!;
      body.replaceChildren(...sample.rows.map(force => {
        const row = document.createElement('tr');
        for (const text of [force.name, `${force.magnitude.toExponential(4)} m/s²`, `${(force.fraction*100).toFixed(3)}%`,
          force.vector.map(value => value.toExponential(4)).join(', ')]) {
          const cell = document.createElement('td'); cell.textContent = text; row.append(cell);
        }
        return row;
      }));
      if (!sample.rows.length) {
        const row = document.createElement('tr'), cell = document.createElement('td');
        cell.colSpan = 4; cell.textContent = 'No other known-mass gravitational sources.'; row.append(cell); body.append(row);
      }
    },
    downloadCsv(text: string) {
      downloadText(text, 'solar-snapshot.csv', 'text/csv;charset=utf-8');
    },
    reportLabState(state: LabState) {
      if (failed) return;
      const config = `${state.lesson}:${state.method}:${state.dt}:${state.factor}`;
      if (config !== reportedConfig) {
        // Family scenes are chosen in the scene picker, not the lesson list.
        if (!sceneIndices.includes(state.lesson) || state.lesson === 0) controls.lesson.value = String(state.lesson);
        controls.scene.value = sceneIndices.includes(state.lesson) ? String(state.lesson) : '';
        // The lesson strip shows only while a guided lesson runs.
        const strip = canvas.closest?.('[data-simulator]')?.querySelector<HTMLElement>('[data-lesson-strip]');
        if (strip) strip.hidden = state.lesson < 0 || sceneIndices.includes(state.lesson);
        controls.method.value = String(state.method);
        controls.dt.value = String(state.dt);
        controls.factor.value = String(state.factor);
        reportedConfig = config;
      }
      if (Number.isFinite(state.minFactor) && Number(controls.lesson.value) === state.lesson) controls.factor.min = String(state.minFactor);
      const contact = state.contactSeconds ? ` · Contact sphere crossed at ${Number(state.contactSeconds.toPrecision(12))} s: a step's straight-line drift passed within the bodies' radii (may be a coarse-step artifact); lesson errors are withheld` : '';
      setText(readouts.scene, `${activeBodyCount} active bodies · ${lessonNames[state.lesson] ?? 'Catalog epoch JD 2461200.5 TDB'}${contact}`);
      setText(controls.step, `Step +${state.dt} s`);
      setSegment(controls.trailFrames, button => Number(button.dataset.trailFrame) === (state.trailFrame ? 1 : 0));
      setSwitch(controls.vectors, Boolean(state.vectors));
      setSwitch(controls.grid, state.grid !== false);
      setSwitch(controls.labels, state.labels !== false);
      setSwitch(controls.orbits, state.orbits !== false);
      setSwitch(controls.constellations, Boolean(state.constellations));
      setSwitch(controls.belt, state.belt !== false);
      setSwitch(controls.follow, Boolean(state.follow));
      if (controls.tour) {
        setText(controls.tour, state.touring ? 'Stop tour' : 'Tour');
        controls.tour.setAttribute('aria-pressed', String(Boolean(state.touring)));
      }
      controls.contact.hidden = state.lesson !== collisionLesson;
      setText(controls.contact, `Contact: ${state.contactMode === 2 ? 'Inelastic merge' : 'Elastic bounce'} (restart)`);
      setText(readouts.acceleration, hasParent ? `${state.acceleration.toExponential(6)} m/s²` : 'N/A — no parent');
      setText(readouts.specificEnergy, hasParent ? `${state.specificEnergy.toExponential(6)} J/kg (two-body diagnostic)` : 'N/A — no parent');
      setText(readouts.energy, `${state.energy.toExponential(6)} J`);
      setText(readouts.energyChange, `${state.energyChange.toExponential(3)} · ΔE / (K₀ + |U₀|)`);
      setText(readouts.momentum, `${state.momentum.toExponential(6)} kg·m/s · ${state.isolated ? 'isolated system' : 'fixed-body constraint; not conserved'}`);
      setText(readouts.angularMomentum, `${state.angularMomentum.toExponential(6)} kg·m²/s`);
      setText(readouts.integration, `${state.method ? 'Euler (teaching comparison)' : 'Velocity-Verlet'} · ${state.dt} s · ${state.ticks} ticks`);
      setText(readouts.magnification, state.magnification > 0 ? `${state.magnification.toPrecision(5)}× physical radius` : 'Unknown radius — marker only');
      setText(readouts.position, `${state.position.map(value => value.toExponential(6)).join(', ')} m`);
      setText(readouts.velocity, `${state.velocity.map(value => value.toExponential(6)).join(', ')} m/s`);
    },
    reportState(state: RuntimeState) {
      if (failed) return;
      if (shortTimescale !== state.shortTimescale) {
        shortTimescale = state.shortTimescale;
        reportedSpeed = -1; // relabel the slider below
      }
      hasParent = state.hasParent;
      controls.panels.forEach(panel => { panel.disabled = false; });
      controls.onState?.(state);
      controls.step.disabled = !state.paused;
      setText(controls.pause, state.paused ? 'Resume' : 'Pause');
      setSegment(controls.scale, button => button.dataset.scale === state.view);
      setSwitch(controls.rotate, state.autoRotate);
      // A native select may expose a tentative arrow-key value before change.
      // Sync only when C state changes, not on every animation-frame report.
      if (reportedBody !== state.selected) {
        reportedBody = state.selected;
        filterBodies();
        controls.onSelection?.(state.body);
      }
      if (reportedSpeed !== state.speedPreset) {
        controls.speed.value = String(state.speedPreset);
        reportedSpeed = state.speedPreset;
        const labels = speedLabels(shortTimescale);
        if (controls.speedLabel) setText(controls.speedLabel, labels.short[state.speedPreset] ?? '');
        controls.speed.setAttribute?.('aria-valuetext', labels.spoken[state.speedPreset] ?? '');
      }
      setText(readouts.status, state.trailsFailed ? 'Trail recording paused: memory unavailable.'
        : state.paused ? 'Paused' : 'Running');
      setText(readouts.controls, `Selected body: ${state.body}; view: ${state.view}.`);
      setText(readouts.elapsed, state.dated ? sceneDate(state.elapsedSeconds)
        : `${(state.elapsedSeconds / 86400).toFixed(4)} d (${Number(state.elapsedSeconds.toPrecision(12))} s)`);
      if (readouts.elapsed.dataset) readouts.elapsed.dataset.seconds = String(state.elapsedSeconds);
      if (readouts.clockNote) setText(readouts.clockNote, clockNote(state.elapsedSeconds, Boolean(state.dated), state.untilSeconds));
      if (readouts.today) {
        // Today plays dated scenes forward to the present; while it runs the
        // same button stops it.
        const running = state.untilSeconds != null;
        readouts.today.hidden = !state.dated;
        if (readouts.dateOpen) readouts.dateOpen.hidden = !state.dated;
        setText(readouts.today, running ? 'Stop' : 'Today');
        readouts.today.dataset.running = running ? 'true' : 'false';
      }
      if (readouts.name) setText(readouts.name, state.body);
      if (readouts.fromEarth) {
        const known = state.earthM != null && Number.isFinite(state.earthM);
        readouts.fromEarthRow?.forEach(element => { element.hidden = !known; });
        if (known) setText(readouts.fromEarth, formatFromEarth(state.earthM!));
      }
      if (readouts.period) setText(readouts.period, !state.hasParent ? 'N/A — no parent'
        : Number.isFinite(state.periodS) ? formatPeriod(state.periodS!) : 'Unbound (no closed orbit)');
      setText(readouts.interval, formatSpacing(state.intervalSeconds));
      setText(readouts.parent, state.parent);
      setText(readouts.distance, state.hasParent ? formatDistance(state.distanceM) : 'N/A — no parent');
      setText(readouts.speed, state.hasParent ? formatSpeed(state.speedMps) : 'N/A — no parent');
      setText(readouts.mass, state.massQuality === 2 ? 'Unknown (test particle)'
        : `${formatMass(state.massKg)}${state.massQuality === 1 ? ' (estimated)' : state.massQuality === 3 ? ' (published; quality unclassified)' : ''}`);
      setText(readouts.radius, state.radiusQuality === 2 ? 'Unknown (marker only)'
        : `${formatRadius(state.radiusM)}${state.radiusQuality === 1 ? ' (estimated)' : state.radiusQuality === 3 ? ' (published; quality unclassified)' : ''}`);
      setText(readouts.camera, `${state.cameraTarget} · ${state.zoom.toPrecision(4)} render units`);
      setText(readouts.achieved, shortTimescale ? `${(state.paused ? 0 : state.achievedTimeScale).toFixed(2)} seconds / second`
        : `${(state.paused ? 0 : state.achievedTimeScale / 86400).toFixed(2)} days / second`);
      setText(readouts.pending, `${(state.pendingSeconds / 86400).toFixed(3)} days`);
      // Resolve a catalog link once, after C has published its actual list.
      // A small moon first loads its planet's family scene; the selection
      // waits for that scene's body list. Later lesson changes and resets keep
      // the runtime's own selection policy.
      if (!initialSelectionApplied) {
        initialSelectionApplied = true;
        void loadRuntimeTextures(this, canvas, artifactUrl, () => loadRuntimeBelt(this, canvas, artifactUrl));
        pendingBody = requestedBody?.name;
        if (requestedBody && requestedBody.scene > 0)
          this.ccall?.('solar_web_lesson', 'number', ['number', 'number', 'number', 'number'], [requestedBody.scene, 1, 0, 15]);
        else if (requestedLesson && requestedLesson.index > 0)
          this.ccall?.('solar_web_lesson', 'number', ['number', 'number', 'number', 'number'], [requestedLesson.index, 1, 0, requestedLesson.dt]);
      }
      if (pendingBody) {
        const target = bodies.find(body => body.name === pendingBody);
        if (target) {
          pendingBody = undefined;
          this._solar_web_command!(runtimeCommands.select, target.index);
          this._solar_web_command!(runtimeCommands.frame, 0);
        } else if (!requestedBody || requestedBody.scene === 0) pendingBody = undefined;
      }
    }
  };
}

declare global {
  interface Window {
    Module: ReturnType<typeof createSimulatorModule>;
  }
}

export function mountSimulator(root: HTMLElement): void {
  const canvas = root.querySelector<HTMLCanvasElement>('canvas')!;
  const artifactUrl = new URL(root.dataset.runtimeSrc!, document.baseURI);
  const controls: RuntimeControls = {
    panels: [...root.querySelectorAll<HTMLFieldSetElement>('[data-runtime-panel]')],
    filterStatus: root.querySelector<HTMLElement>('[data-body-filter-status]')!,
    pause: root.querySelector<HTMLButtonElement>('[data-command="pause"]')!,
    step: root.querySelector<HTMLButtonElement>('[data-command="step"]')!,
    rotate: root.querySelector<HTMLButtonElement>('[data-runtime-rotate]')!,
    body: root.querySelector<HTMLSelectElement>('[data-runtime-body]')!,
    results: root.querySelector<HTMLElement>('[data-find-results]') ?? undefined,
    speed: root.querySelector<HTMLInputElement>('[data-runtime-speed]')!,
    speedLabel: root.querySelector<HTMLElement>('[data-speed-label]') ?? undefined,
    follow: root.querySelector<HTMLButtonElement>('[data-command="follow"]') ?? undefined,
    belt: root.querySelector<HTMLButtonElement>('[data-command="belt"]') ?? undefined,
    constellations: root.querySelector<HTMLButtonElement>('[data-command="constellations"]') ?? undefined,
    tour: root.querySelector<HTMLButtonElement>('[data-command="tour"]') ?? undefined,
    scale: [...root.querySelectorAll<HTMLButtonElement>('[data-scale]')],
    trailFrames: [...root.querySelectorAll<HTMLButtonElement>('[data-trail-frame]')],
    search: root.querySelector<HTMLInputElement>('[data-runtime-search]')!,
    group: root.querySelector<HTMLSelectElement>('[data-runtime-group]')!,
    lesson: root.querySelector<HTMLSelectElement>('[data-lesson]')!,
    scene: root.querySelector<HTMLSelectElement>('[data-runtime-scene]')!,
    method: root.querySelector<HTMLSelectElement>('[data-integrator]')!,
    dt: root.querySelector<HTMLInputElement>('[data-dt]')!,
    factor: root.querySelector<HTMLInputElement>('[data-velocity-factor]')!,
    vectors: root.querySelector<HTMLButtonElement>('[data-command="vectors"]')!,
    grid: root.querySelector<HTMLButtonElement>('[data-command="grid"]')!,
    labels: root.querySelector<HTMLButtonElement>('[data-command="labels"]')!,
    orbits: root.querySelector<HTMLButtonElement>('[data-command="orbits"]')!,
    contact: root.querySelector<HTMLButtonElement>('[data-command="contact"]')!
  };
  const runtime = createSimulatorModule(canvas, {
    status: root.querySelector<HTMLElement>('[data-runtime-status]')!,
    controls: root.querySelector<HTMLElement>('[data-runtime-controls]')!,
    elapsed: root.querySelector<HTMLElement>('[data-runtime-elapsed]')!,
    clockNote: root.querySelector<HTMLElement>('[data-clock-note]') ?? undefined,
    today: root.querySelector<HTMLButtonElement>('[data-today]') ?? undefined,
    dateOpen: root.querySelector<HTMLButtonElement>('[data-date-open]') ?? undefined,
    interval: root.querySelector<HTMLElement>('[data-runtime-interval]')!,
    parent: root.querySelector<HTMLElement>('[data-inspector-parent]')!,
    distance: root.querySelector<HTMLElement>('[data-inspector-distance]')!,
    speed: root.querySelector<HTMLElement>('[data-inspector-speed]')!,
    period: root.querySelector<HTMLElement>('[data-inspector-period]') ?? undefined,
    fromEarth: root.querySelector<HTMLElement>('[data-inspector-earth]') ?? undefined,
    fromEarthRow: [...root.querySelectorAll<HTMLElement>('[data-from-earth]')],
    name: root.querySelector<HTMLElement>('[data-inspector-name]') ?? undefined,
    mass: root.querySelector<HTMLElement>('[data-inspector-mass]')!,
    radius: root.querySelector<HTMLElement>('[data-inspector-radius]')!,
    camera: root.querySelector<HTMLElement>('[data-runtime-camera]')!,
    achieved: root.querySelector<HTMLElement>('[data-runtime-achieved]')!,
    pending: root.querySelector<HTMLElement>('[data-runtime-pending]')!,
    scene: root.querySelector<HTMLElement>('[data-active-scene]')!,
    acceleration: root.querySelector<HTMLElement>('[data-inspector-acceleration]')!,
    specificEnergy: root.querySelector<HTMLElement>('[data-inspector-specific-energy]')!,
    energy: root.querySelector<HTMLElement>('[data-energy]')!,
    energyChange: root.querySelector<HTMLElement>('[data-energy-change]')!,
    momentum: root.querySelector<HTMLElement>('[data-momentum]')!,
    angularMomentum: root.querySelector<HTMLElement>('[data-angular-momentum]')!,
    integration: root.querySelector<HTMLElement>('[data-integration]')!,
    magnification: root.querySelector<HTMLElement>('[data-magnification]')!,
    position: root.querySelector<HTMLElement>('[data-inspector-position]')!,
    velocity: root.querySelector<HTMLElement>('[data-inspector-velocity]')!
  }, artifactUrl, controls, (() => {
    const body = implementedBodies.find(item => item.slug === new URLSearchParams(location.search).get('body'));
    return body && { name: body.name, scene: sceneIndexOf(body.scene) };
  })(), (() => {
    const name = new URLSearchParams(location.search).get('lesson');
    const index = lessonOptions.findIndex(([lesson]) => lesson === name);
    return index > 0 && !sceneIndices.includes(index) ? { index, dt: name === 'collision' ? 0.1 : 15 } : undefined;
  })());

  const send = (command: keyof typeof runtimeCommands, value = 0): void => {
    if (!controls.panels[0].disabled) runtime._solar_web_command!(runtimeCommands[command], value);
  };
  // A120: the address follows the selection (?body=slug), so a copied link
  // shares the view and browser Back retraces the bodies visited. Lessons
  // and experiments keep their own addresses.
  const addressed = (): boolean => !new URLSearchParams(location.search).has('lesson')
    && !new URLSearchParams(location.search).has('experiment');
  let firstSelection = true, fromHistory = false;
  controls.onSelection = name => {
    const slug = implementedBodies.find(body => body.name === name)?.slug;
    if (!addressed() || !slug) return;
    const url = new URL(location.href);
    const requested = url.searchParams.get('body');
    // A deep link opens on the Sun for a frame before its body is selected:
    // leave the link's own entry alone until its body arrives.
    if (firstSelection && requested && requested !== slug && implementedBodies.some(body => body.slug === requested)) return;
    const unaddressedStart = firstSelection && !requested;
    if (!unaddressedStart) url.searchParams.set('body', slug);
    const depth: number = history.state?.solarDepth ?? 0;
    if (firstSelection || fromHistory || url.href === location.href) history.replaceState({ solarBody: slug, solarDepth: depth }, '', url);
    else history.pushState({ solarBody: slug, solarDepth: depth + 1 }, '', url);
    firstSelection = false;
    fromHistory = false;
  };
  window.addEventListener('popstate', () => {
    if (!addressed()) return;
    const slug = new URLSearchParams(location.search).get('body') ?? 'sun';
    const body = implementedBodies.find(item => item.slug === slug);
    const index = body ? runtime.bodyIndex(body.name) : -1;
    // A body of another scene: reload, and the deep link opens that scene.
    if (index < 0) { location.reload(); return; }
    fromHistory = true;
    send('select', index);
    send(index === 0 ? 'frame' : 'frameBody');
  });
  root.querySelectorAll<HTMLButtonElement>('[data-command]').forEach(button => {
    button.addEventListener('click', () => send(button.dataset.command as keyof typeof runtimeCommands, Number(button.dataset.value ?? 0)));
  });
  root.querySelector<HTMLButtonElement>('[data-today]')?.addEventListener('click', event => {
    const button = event.currentTarget as HTMLButtonElement;
    if (button.dataset.running === 'true') send('pause');
    else send('runUntil', Math.floor(sceneSecondsAt(Date.now())));
  });
  // Go to date (A134): opens on the present; Go runs to the chosen instant.
  const dateForm = root.querySelector<HTMLFormElement>('[data-date-form]');
  if (dateForm) {
    const dateInput = dateForm.querySelector<HTMLInputElement>('[data-date-input]')!;
    const timeInput = dateForm.querySelector<HTMLInputElement>('[data-time-input]')!;
    const note = dateForm.querySelector<HTMLElement>('[data-date-note]')!;
    const elapsed = (): number => Number(root.querySelector<HTMLElement>('[data-runtime-elapsed]')?.dataset.seconds ?? 0);
    const target = (): number => sceneSecondsAtDate(dateInput.value, timeInput.value);
    const refresh = (): void => { note.textContent = dateRunNote(target(), elapsed()); };
    root.querySelector<HTMLButtonElement>('[data-date-open]')?.addEventListener('click', () => {
      const [day, minute] = sceneDate(sceneSecondsAt(Date.now())).split(' ');
      dateInput.value = day; timeInput.value = minute;
      refresh();
    });
    dateInput.addEventListener('input', refresh);
    timeInput.addEventListener('input', refresh);
    dateForm.querySelector<HTMLButtonElement>('[data-date-copy]')?.addEventListener('click', async () => {
      const seconds = target();
      if (!Number.isFinite(seconds)) { refresh(); return; }
      const url = new URL(location.href);
      url.searchParams.set('at', linkForSceneSeconds(seconds));
      try { await navigator.clipboard.writeText(url.href); note.textContent = 'Link copied: it opens here and plays to this instant.'; }
      catch { note.textContent = url.href; }
    });
    dateForm.addEventListener('submit', event => {
      event.preventDefault();
      const seconds = target();
      if (!Number.isFinite(seconds)) { refresh(); dateInput.focus(); return; }
      if (!dateRunArrived(seconds, elapsed())) send('runUntil', Math.round(seconds));
      dateForm.closest('dialog')?.close();
    });
  }
  // A linked instant (A141): once C reports a dated scene with the linked
  // body selected (a family scene loads first, which would reset a run), play
  // to it like Date's Go, then drop `at` so the address never claims a stale
  // instant. Copy link in the Date panel writes a new one.
  const linkedAt = sceneSecondsAtLink(new URLSearchParams(location.search).get('at'));
  if (Number.isFinite(linkedAt) && addressed()) {
    const wanted = implementedBodies.find(body => body.slug === new URLSearchParams(location.search).get('body'))?.name;
    controls.onState = state => {
      if (!state.dated || (wanted && state.body !== wanted)) return;
      controls.onState = undefined;
      if (!dateRunArrived(linkedAt, state.elapsedSeconds)) send('runUntil', Math.round(linkedAt));
      const url = new URL(location.href);
      url.searchParams.delete('at');
      history.replaceState(history.state, '', url);
    };
  }
  controls.body.addEventListener('change', () => send('select', Number(controls.body.value)));
  controls.speed.addEventListener('input', () => send('speed', Number(controls.speed.value)));
  controls.search.addEventListener('input', runtime.filterBodies);
  // A130: picking a match selects it, flies there and closes Find.
  const findPanel = root.querySelector<HTMLDialogElement>('#find-panel');
  const goTo = (index: number): void => {
    send('select', index);
    send('frameBody');
    findPanel?.close();
  };
  controls.results?.addEventListener('click', event => {
    const button = (event.target as HTMLElement).closest<HTMLButtonElement>('[data-find-index]');
    if (button) goTo(Number(button.dataset.findIndex));
  });
  controls.search.addEventListener('keydown', event => {
    if (event.key !== 'Enter') return;
    const first = controls.results?.querySelector<HTMLButtonElement>('[data-find-index]');
    if (!first) return;
    event.preventDefault();
    goTo(Number(first.dataset.findIndex));
  });
  controls.group.addEventListener('change', runtime.filterBodies);
  root.querySelector('[data-clear-filters]')!.addEventListener('click', () => {
    controls.search.value = ''; controls.group.value = ''; runtime.filterBodies(); controls.search.focus();
  });
  root.querySelector('[data-whole-system]')!.addEventListener('click', () => { send('select', 0); send('frame'); });
  root.querySelectorAll<HTMLButtonElement>('[data-open-panel]').forEach(button => {
    const panel = root.querySelector<HTMLDialogElement>(`#${button.dataset.openPanel}`)!;
    if (!panel.classList.contains('popover')) {
      button.addEventListener('click', () => panel.showModal());
      return;
    }
    // A128: popover panels are non-modal so the scene stays visible and
    // usable; the opener toggles them and Escape closes them (non-modal
    // dialogs get no cancel event), returning focus to the opener.
    button.setAttribute('aria-expanded', 'false');
    button.addEventListener('click', () => {
      if (panel.open) panel.close();
      else { panel.show(); panel.querySelector<HTMLElement>('input, button:not([data-close-panel])')?.focus(); }
      button.setAttribute('aria-expanded', String(panel.open));
    });
    // However it closes (Close, Escape, the opener), focus left inside the
    // panel goes back to the opener: non-modal dialogs do not restore it.
    panel.addEventListener('close', () => {
      button.setAttribute('aria-expanded', 'false');
      const focused = document.activeElement;
      if (!focused || focused === document.body || panel.contains(focused)) button.focus();
    });
    panel.addEventListener('keydown', event => {
      if (event.key !== 'Escape') return;
      event.preventDefault();
      panel.close();
    });
  });
  root.querySelectorAll<HTMLElement>('[data-close-panel]').forEach(button => {
    button.addEventListener('click', () => {
      button.closest('dialog')!.close();
      const diagnostics = root.querySelector('#diagnostics-heading')?.closest('details');
      if (button.hasAttribute('data-open-diagnostics') && diagnostics) diagnostics.open = true;
    });
  });
  const sceneStatus = root.querySelector<HTMLElement>('[data-scene-status]')!;
  let sceneStatusTimer = 0;
  // A132: choosing a scene loads it; there is no separate Load button.
  // Keyboard browsing (arrows fire change on every step) waits until the
  // keys rest, so passing over a scene does not reload it.
  const loadScene = (): void => {
    if (!controls.scene.value) return;
    const accepted = runtime.ccall?.('solar_web_lesson', 'number', ['number', 'number', 'number', 'number'],
      [Number(controls.scene.value), 1, 0, 15]);
    // The note fades after a few seconds instead of lingering over the view.
    window.clearTimeout(sceneStatusTimer);
    sceneStatusTimer = window.setTimeout(() => { sceneStatus.textContent = ''; }, 6000);
    sceneStatus.textContent = accepted ? `${controls.scene.selectedOptions[0].text} loaded from its initial state.`
      : 'The scene could not be loaded.';
  };
  let sceneKeyAt = -Infinity, sceneLoadTimer = 0;
  controls.scene.addEventListener('keydown', () => { sceneKeyAt = performance.now(); });
  controls.scene.addEventListener('change', () => {
    window.clearTimeout(sceneLoadTimer);
    if (performance.now() - sceneKeyAt < 700) sceneLoadTimer = window.setTimeout(loadScene, 700);
    else loadScene();
  });
  for (const select of [controls.body, controls.group, controls.scene, controls.lesson, controls.method]) {
    select.addEventListener('keydown', event => {
      if (moveSelectByKey(select, event.key)) {
        event.preventDefault();
        event.stopPropagation();
      }
    });
  }
  // Segmented controls send C's toggle only when the other option is chosen.
  controls.scale?.forEach(button => button.addEventListener('click', () => {
    if (button.getAttribute('aria-pressed') !== 'true') send('view');
  }));
  controls.trailFrames?.forEach(button => button.addEventListener('click', () => {
    if (button.getAttribute('aria-pressed') !== 'true') send('trails');
  }));

  document.addEventListener('visibilitychange', () => send('background', document.hidden ? 1 : 0));
  const setLessonDefaults = () => {
    // C owns each lesson's lowest starting speed (slower orbits would pass
    // through the parent body); mirror it as the input's native minimum.
    const minimum = runtime.ccall?.('solar_web_minimum_factor', 'number', ['number'], [Number(controls.lesson.value)]);
    if (typeof minimum === 'number' && Number.isFinite(minimum)) controls.factor.min = String(minimum);
    controls.dt.value = Number(controls.lesson.value) === collisionLesson ? '0.1' : '15';
    if (controls.lesson.value === '0') {
      controls.method.value = '0'; controls.dt.value = '15'; controls.factor.value = '1';
    }
    if ([collisionLesson, barycentricLesson].includes(Number(controls.lesson.value))) controls.factor.value = '1';
  };
  controls.lesson.addEventListener('change', setLessonDefaults);
  const lessonStatus = root.querySelector<HTMLElement>('[data-lesson-status]')!;
  const lessonInputs = [controls.lesson, controls.method, controls.dt, controls.factor];
  const clearLessonValidity = () => lessonInputs.forEach(input => input.setCustomValidity(''));
  for (const input of lessonInputs) {
    input.addEventListener('input', clearLessonValidity);
    input.addEventListener('change', clearLessonValidity);
  }
  const loadLesson = () => {
    clearLessonValidity();
    const settings = controls.dt.closest('details');
    if (settings && (!controls.dt.checkValidity() || !controls.factor.checkValidity())) settings.open = true;
    if (controls.factor.validity.rangeUnderflow) {
      const message = `Below ${controls.factor.min}× this starting orbit would pass through the parent body; point-mass gravity has no surface. Use at least ${controls.factor.min}.`;
      lessonStatus.textContent = message;
      controls.factor.setCustomValidity(message);
    }
    if (!controls.dt.reportValidity() || !controls.factor.reportValidity()) return;
    const accepted = runtime.ccall?.('solar_web_lesson', 'number', ['number', 'number', 'number', 'number'],
      [Number(controls.lesson.value), Number(controls.factor.value), Number(controls.method.value), Number(controls.dt.value)]);
    lessonStatus.textContent = accepted ? 'Lesson loaded from its initial state. If paused, press Resume; Restart repeats this configuration.'
      : 'Configuration rejected. Core uses 15-second Verlet; core/barycentric-core require speed factor 1; collision steps are 0.01–0.25 s; planet scenes load from the Scene picker.';
    if (!accepted) {
      // Explain C's lesson-specific rejection after validation; ordinary bounds
      // are already handled by the numeric inputs. C still owns acceptance.
      const lesson = Number(controls.lesson.value);
      const rejectAt = (input: HTMLInputElement | HTMLSelectElement, message: string) => {
        const details = input.closest('details');
        if (details) details.open = true;
        lessonStatus.textContent = message;
        input.setCustomValidity(message);
        input.reportValidity();
      };
      if (lesson < 0) rejectAt(controls.lesson, 'Choose a lesson preset. Prepared catalog experiments start from the Data panel.');
      else if ([0, barycentricLesson].includes(lesson) && Number(controls.factor.value) !== 1)
        rejectAt(controls.factor, 'This preset keeps its starting speed unchanged. Use a multiplier of 1.');
      else if (lesson === 0 && Number(controls.method.value) !== 0)
        rejectAt(controls.method, 'The core demonstration uses Velocity-Verlet. Choose a lesson to compare calculation methods.');
      else if (lesson === 0 && Number(controls.dt.value) !== 15)
        rejectAt(controls.dt, 'The core demonstration uses a 15-second timestep. Choose a lesson to vary it.');
      else if (lesson === collisionLesson && Number(controls.dt.value) > .25)
        rejectAt(controls.dt, 'The contact lesson needs a timestep from 0.01 to 0.25 seconds. Try 0.1 seconds.');
    }
  };
  root.querySelector('[data-apply-lesson]')!.addEventListener('click', loadLesson);
  root.querySelectorAll<HTMLButtonElement>('[data-activity]').forEach(button => {
    button.addEventListener('click', () => {
      controls.lesson.value = button.dataset.activity!;
      controls.method.value = '0'; controls.factor.value = '1'; setLessonDefaults(); loadLesson();
    });
  });
  root.querySelector('[data-export]')!.addEventListener('click', () => {
    const accepted = runtime.ccall?.('solar_web_export', 'number', [], []);
    root.querySelector('[data-export-status]')!.textContent = accepted ? 'CSV snapshot downloaded with physical SI state and run metadata.' : 'Could not export the snapshot.';
  });

  const experimentStatus=root.querySelector<HTMLElement>('[data-experiment-status]')!;
  let prepared: PreparedExperiment|undefined;
  let experimentAction=0;
  if(new URLSearchParams(location.search).has('experiment')) {
    root.querySelector<HTMLDialogElement>('#advanced-panel')!.showModal();
    try {
      prepared=parsePreparedExperiment(sessionStorage.getItem('solar-catalog-experiment'));
      experimentStatus.textContent=`Prepared: ${prepared.count} catalog bodies plus Sun/eight planets. Press Start prepared experiment to replace the demonstration.`;
    } catch(error) {experimentStatus.textContent=errorMessage(error);prepared=undefined;}
  }
  root.querySelector('[data-start-experiment]')!.addEventListener('click', async()=>{
    const action=++experimentAction;
    if(!prepared){experimentStatus.textContent='Choose bodies in the small-body atlas first.';return;}
    const chosen=prepared;
    try {
      const response=await fetch(root.dataset.catalogManifest!,{cache:'no-cache'});
      if(!response.ok) throw new Error('Could not verify the catalog snapshot. Retry.');
      const manifest=await response.json();
      if(action!==experimentAction)return;
      if(manifest.sourceSha256!==chosen.snapshot || manifest.epoch!==2461200.5) throw new Error('Prepared catalog is stale. Prepare it again in the atlas.');
      const accepted=runtime.ccall?.('solar_web_experiment','number',['string'],[chosen.text]);
      if(!accepted) throw new Error('C rejected the experiment input. Return to the atlas to prepare it again.');
      experimentStatus.textContent=`Catalog experiment started with ${chosen.count+9} active bodies. Reset restores this same experiment.`;
    } catch(error) {if(action===experimentAction)experimentStatus.textContent=errorMessage(error);}
  });
  root.querySelector('[data-demo]')!.addEventListener('click',()=>{
    ++experimentAction;
    runtime.ccall?.('solar_web_demo',null,[],[]);
    experimentStatus.textContent='Main scene restored.';
  });

  // Configure the classic Emscripten module before its generated loader runs.
  // Both assets use the page revision so cached builds cannot mix their ABI.
  window.Module = runtime;
  const routeKeyboard = (event: KeyboardEvent): void => routeSimulatorKeyboard(event, document.activeElement === canvas);
  // A120: "/" opens body search from the view; Backspace on the view goes
  // back to the previously viewed body (browser history, below). Registered
  // before routeKeyboard, which stops Backspace from reaching GLFW.
  window.addEventListener('keydown', event => {
    if (document.activeElement !== canvas) return;
    if (event.key === '/') {
      event.preventDefault();
      event.stopImmediatePropagation();
      root.querySelector<HTMLDialogElement>('#find-panel')!.showModal();
      controls.search.focus();
    } else if (event.key === 'Backspace' && (history.state?.solarDepth ?? 0) > 0) {
      // Only within the simulator's own path: never back out of the site.
      event.preventDefault();
      history.back();
    }
  }, true);
  window.addEventListener('keydown', routeKeyboard, true);
  window.addEventListener('keypress', routeKeyboard, true);
  // raylib's web backend needs WebGL. Probe a detached canvas so the runtime's
  // own canvas keeps the context attributes Emscripten requests.
  const probe = document.createElement('canvas');
  if (!probe.getContext('webgl2') && !probe.getContext('webgl')) {
    runtime.onAbort('WebGL is unavailable in this browser, so the 3D simulator cannot start. The documentation and comparison lab remain available.');
    return;
  }
  const script = document.createElement('script');
  script.src = artifactUrl.href;
  script.onerror = () => runtime.onAbort('Could not download simulator JavaScript. Reload to retry.');
  canvas.addEventListener('webglcontextlost', () => runtime.onAbort('WebGL context lost. Reload to restart the simulation.'));
  document.body.append(script);
}
