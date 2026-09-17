import { lessonOptions } from './lessonCatalog.ts';
type Readout = Pick<HTMLElement, 'textContent'>;
interface RuntimeReadouts {
  status: Readout;
  controls: Readout;
  elapsed: Readout;
  interval: Readout;
  parent: Readout;
  distance: Readout;
  speed: Readout;
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
  panel: HTMLFieldSetElement;
  pause: HTMLButtonElement;
  step: HTMLButtonElement;
  rotate: HTMLInputElement;
  body: HTMLSelectElement;
  speed: HTMLSelectElement;
  view: HTMLButtonElement;
  search: HTMLInputElement;
  group: HTMLSelectElement;
  lesson: HTMLSelectElement;
  method: HTMLSelectElement;
  dt: HTMLInputElement;
  factor: HTMLInputElement;
  trails: HTMLButtonElement;
  vectors: HTMLButtonElement;
  contact: HTMLButtonElement;
}

interface LabState {
  lesson: number; method: number; dt: number; ticks: number; factor: number;
  acceleration: number; specificEnergy: number; energy: number; energyChange: number;
  isolated: boolean; momentum: number; angularMomentum: number; magnification: number;
  trailFrame: number; vectors: boolean; position: number[]; velocity: number[];
  contactMode: number;
}

export const lessonNames = lessonOptions.map(([, label]) => label);
const collisionLesson = lessonOptions.findIndex(([name]) => name === 'collision');
const barycentricLesson = lessonOptions.findIndex(([name]) => name === 'barycentric-core');

interface RuntimeState {
  body: string; parent: string; view: string; cameraTarget: string; selected: number;
  paused: boolean; speedPreset: number; autoRotate: boolean;
  elapsedSeconds: number; intervalSeconds: number; trailsFailed: boolean; hasParent: boolean;
  distanceM: number; speedMps: number; massKg: number; radiusM: number; zoom: number;
  massQuality: number; radiusQuality: number; achievedTimeScale: number; pendingSeconds: number;
  shortTimescale: boolean;
}

// Matches SolarCommand in src/main.c. All actions execute in the C runtime.
export const runtimeCommands = { pause: 0, step: 1, reset: 2, speed: 3, select: 4, view: 5, zoom: 6, rotate: 7, frame: 8, frameBody: 9,
  trails: 10, vectors: 11, background: 12, contact: 13 } as const;

interface RuntimeBody { index: number; name: string; group: string }

export function filterRuntimeBodies(bodies: RuntimeBody[], search: string, group: string, selected: number): RuntimeBody[] {
  const query = search.trim().toLowerCase();
  // Filtering changes menu visibility only. Keep C's current selection reachable
  // even if it falls outside the filter, until the user selects a different body.
  return bodies.filter(body => body.index === selected ||
    ((!group || body.group === group) && body.name.toLowerCase().includes(query)));
}

function setText(element: Readout, text: string): void {
  if (element.textContent !== text) element.textContent = text;
}

/** GLFW globally cancels Tab and Backspace. Stop only those listener calls;
 * other keys must reach native controls, while C gates shortcuts by focus. */
export function routeSimulatorKeyboard(event: Pick<KeyboardEvent, 'key' | 'stopImmediatePropagation' | 'preventDefault'>, canvasFocused: boolean): void {
  if (event.key === 'Tab' || event.key === 'Backspace') event.stopImmediatePropagation();
  else if (canvasFocused && event.key === ' ') event.preventDefault();
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
export function createSimulatorModule(canvas: HTMLCanvasElement, readouts: RuntimeReadouts, artifactUrl: URL, controls: RuntimeControls) {
  let failed = false;
  let reportedBody = -1;
  let reportedSpeed = -1;
  let reportedConfig = '';
  let hasParent = false;
  let activeBodyCount = 0;
  let shortTimescale = false, lastForceTime = -Infinity, lastForceBody = -1;
  const bodies: RuntimeBody[] = [];
  const groups = new Set<string>();
  const filterBodies = (): void => {
    const visible = filterRuntimeBodies(bodies, controls.search.value, controls.group.value, reportedBody);
    const options = visible.map(body => new Option(`${body.name} — ${body.group}`, String(body.index)));
    controls.body.replaceChildren(...options);
    controls.body.value = String(reportedBody);
  };
  const fail = (reason: unknown): void => {
    failed = true;
    controls.panel.disabled = true;
    setText(readouts.status, `Runtime error: ${String(reason)}`);
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
      if(scene) scene.textContent=experiment?`${count} active bodies · catalog epoch JD 2461200.5 TDB`:`${count} active bodies · perihelion demonstration`;
    },
    _solar_web_command: undefined as ((command: number, value: number) => void) | undefined,
    addBody(index: number, name: string, group: string) {
      bodies.push({ index, name, group });
      if (!groups.has(group)) {
        controls.group.add(new Option(group, group));
        groups.add(group);
      }
      controls.body.add(new Option(`${name} — ${group}`, String(index)));
    },
    filterBodies,
    locateFile(path: string) {
      const url = new URL(path, artifactUrl);
      url.search = artifactUrl.search;
      return url.href;
    },
    print: (message: string) => console.log(message),
    printErr: (message: string) => { console.error(message); fail(message); },
    setStatus(message: string) {
      if (!failed && message) setText(readouts.status, message);
    },
    onAbort: fail,
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
      const url = URL.createObjectURL(new Blob([text], { type: 'text/csv;charset=utf-8' }));
      const link = document.createElement('a');
      link.href = url;
      link.download = 'solar-snapshot.csv';
      link.click();
      setTimeout(() => URL.revokeObjectURL(url), 0);
    },
    reportLabState(state: LabState) {
      if (failed) return;
      const config = `${state.lesson}:${state.method}:${state.dt}:${state.factor}`;
      if (config !== reportedConfig) {
        controls.lesson.value = String(state.lesson);
        controls.method.value = String(state.method);
        controls.dt.value = String(state.dt);
        controls.factor.value = String(state.factor);
        reportedConfig = config;
      }
      setText(readouts.scene, `${activeBodyCount} active bodies · ${lessonNames[state.lesson] ?? 'Catalog epoch JD 2461200.5 TDB'}`);
      setText(controls.step, `Step +${state.dt} s`);
      setText(controls.trails, `Trails: ${state.trailFrame ? 'Parent-relative' : 'Absolute'}`);
      setText(controls.vectors, `Vector directions: ${state.vectors ? 'On' : 'Off'}`);
      controls.vectors.setAttribute('aria-pressed', String(state.vectors));
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
        const labels = shortTimescale ? ['1 second / second', '5 seconds / second', '10 seconds / second', '25 seconds / second', '50 seconds / second']
          : ['1 hour / second', '1 day / second', '5 days / second', '10 days / second', '15 days / second'];
        [...controls.speed.options].forEach((option, index) => { option.textContent = labels[index]; });
      }
      hasParent = state.hasParent;
      controls.panel.disabled = false;
      controls.step.disabled = !state.paused;
      setText(controls.pause, state.paused ? 'Resume' : 'Pause');
      setText(controls.view, `View: ${state.view}`);
      controls.rotate.checked = state.autoRotate;
      // A native select may expose a tentative arrow-key value before change.
      // Sync only when C state changes, not on every animation-frame report.
      if (reportedBody !== state.selected) {
        reportedBody = state.selected;
        filterBodies();
      }
      if (reportedSpeed !== state.speedPreset) {
        controls.speed.value = String(state.speedPreset);
        reportedSpeed = state.speedPreset;
      }
      setText(readouts.status, state.trailsFailed ? 'Trail recording paused: memory unavailable.'
        : state.paused ? 'Simulation paused' : 'Running physics simulation');
      setText(readouts.controls, `Selected body: ${state.body}; view: ${state.view}.`);
      setText(readouts.elapsed, `${(state.elapsedSeconds / 86400).toFixed(5)} simulated days · ${Number(state.elapsedSeconds.toPrecision(12))} s`);
      setText(readouts.interval, `${(state.intervalSeconds / 3600).toFixed(2)} simulated hours`);
      setText(readouts.parent, state.parent);
      setText(readouts.distance, state.hasParent ? `${(state.distanceM / 1000).toFixed(3)} km` : 'N/A — no parent');
      setText(readouts.speed, state.hasParent ? `${(state.speedMps / 1000).toFixed(6)} km/s` : 'N/A — no parent');
      setText(readouts.mass, state.massQuality === 2 ? 'Unknown (test particle)'
        : `${state.massKg.toExponential(6)} kg${state.massQuality === 1 ? ' (estimated)' : state.massQuality === 3 ? ' (published; quality unclassified)' : ''}`);
      setText(readouts.radius, state.radiusQuality === 2 ? 'Unknown (marker only)'
        : `${(state.radiusM / 1000).toFixed(3)} km${state.radiusQuality === 1 ? ' (estimated)' : state.radiusQuality === 3 ? ' (published; quality unclassified)' : ''}`);
      setText(readouts.camera, `${state.cameraTarget} · ${state.zoom.toPrecision(4)} render units`);
      setText(readouts.achieved, shortTimescale ? `${(state.paused ? 0 : state.achievedTimeScale).toFixed(2)} seconds / second`
        : `${(state.paused ? 0 : state.achievedTimeScale / 86400).toFixed(2)} days / second`);
      setText(readouts.pending, `${(state.pendingSeconds / 86400).toFixed(3)} days`);
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
    panel: root.querySelector<HTMLFieldSetElement>('[data-runtime-panel]')!,
    pause: root.querySelector<HTMLButtonElement>('[data-command="pause"]')!,
    step: root.querySelector<HTMLButtonElement>('[data-command="step"]')!,
    rotate: root.querySelector<HTMLInputElement>('[data-runtime-rotate]')!,
    body: root.querySelector<HTMLSelectElement>('[data-runtime-body]')!,
    speed: root.querySelector<HTMLSelectElement>('[data-runtime-speed]')!,
    view: root.querySelector<HTMLButtonElement>('[data-command="view"]')!,
    search: root.querySelector<HTMLInputElement>('[data-runtime-search]')!,
    group: root.querySelector<HTMLSelectElement>('[data-runtime-group]')!,
    lesson: root.querySelector<HTMLSelectElement>('[data-lesson]')!,
    method: root.querySelector<HTMLSelectElement>('[data-integrator]')!,
    dt: root.querySelector<HTMLInputElement>('[data-dt]')!,
    factor: root.querySelector<HTMLInputElement>('[data-velocity-factor]')!,
    trails: root.querySelector<HTMLButtonElement>('[data-command="trails"]')!,
    vectors: root.querySelector<HTMLButtonElement>('[data-command="vectors"]')!,
    contact: root.querySelector<HTMLButtonElement>('[data-command="contact"]')!
  };
  const runtime = createSimulatorModule(canvas, {
    status: root.querySelector<HTMLElement>('[data-runtime-status]')!,
    controls: root.querySelector<HTMLElement>('[data-runtime-controls]')!,
    elapsed: root.querySelector<HTMLElement>('[data-runtime-elapsed]')!,
    interval: root.querySelector<HTMLElement>('[data-runtime-interval]')!,
    parent: root.querySelector<HTMLElement>('[data-inspector-parent]')!,
    distance: root.querySelector<HTMLElement>('[data-inspector-distance]')!,
    speed: root.querySelector<HTMLElement>('[data-inspector-speed]')!,
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
  }, artifactUrl, controls);

  const send = (command: keyof typeof runtimeCommands, value = 0): void => {
    if (!controls.panel.disabled) runtime._solar_web_command!(runtimeCommands[command], value);
  };
  root.querySelectorAll<HTMLButtonElement>('[data-command]').forEach(button => {
    button.addEventListener('click', () => send(button.dataset.command as keyof typeof runtimeCommands, Number(button.dataset.value ?? 0)));
  });
  controls.body.addEventListener('change', () => send('select', Number(controls.body.value)));
  controls.speed.addEventListener('change', () => send('speed', Number(controls.speed.value)));
  controls.search.addEventListener('input', runtime.filterBodies);
  controls.group.addEventListener('change', runtime.filterBodies);
  for (const select of [controls.body, controls.speed, controls.group, controls.lesson, controls.method]) {
    select.addEventListener('keydown', event => {
      if (moveSelectByKey(select, event.key)) {
        event.preventDefault();
        event.stopPropagation();
      }
    });
  }
  controls.rotate.addEventListener('change', () => send('rotate'));

  document.addEventListener('visibilitychange', () => send('background', document.hidden ? 1 : 0));
  controls.lesson.addEventListener('change', () => {
    controls.dt.value = Number(controls.lesson.value) === collisionLesson ? '0.1' : '15';
    if (controls.lesson.value === '0') {
      controls.method.value = '0'; controls.dt.value = '15'; controls.factor.value = '1';
    }
    if ([collisionLesson, barycentricLesson].includes(Number(controls.lesson.value))) controls.factor.value = '1';
  });
  const lessonStatus = root.querySelector<HTMLElement>('[data-lesson-status]')!;
  root.querySelector('[data-apply-lesson]')!.addEventListener('click', () => {
    if (!controls.dt.reportValidity() || !controls.factor.reportValidity()) return;
    const accepted = runtime.ccall?.('solar_web_lesson', 'number', ['number', 'number', 'number', 'number'],
      [Number(controls.lesson.value), Number(controls.factor.value), Number(controls.method.value), Number(controls.dt.value)]);
    lessonStatus.textContent = accepted ? 'Lesson started from its initial state. Reset repeats this configuration.'
      : 'Configuration rejected. Core uses 15-second Verlet; core/barycentric-core require speed factor 1; collision steps are 0.01–0.25 s; catalog scenes start through the atlas.';
  });
  root.querySelector('[data-export]')!.addEventListener('click', () => {
    const accepted = runtime.ccall?.('solar_web_export', 'number', [], []);
    lessonStatus.textContent = accepted ? 'CSV snapshot downloaded with physical SI state and run metadata.' : 'Could not export the snapshot.';
  });

  const experimentStatus=root.querySelector<HTMLElement>('[data-experiment-status]')!;
  let prepared: {text:string;snapshot:string;count:number}|undefined;
  let experimentAction=0;
  if(new URLSearchParams(location.search).has('experiment')) {
    try {
      const stored=sessionStorage.getItem('solar-catalog-experiment');
      if(!stored) throw new Error('No prepared experiment. Choose bodies in the small-body atlas first.');
      prepared=JSON.parse(stored);
      if(!prepared || typeof prepared.text!=='string' || !Number.isInteger(prepared.count) || prepared.count<1 || prepared.count>16)
        throw new Error('Invalid prepared experiment. Return to the atlas.');
      experimentStatus.textContent=`Prepared: ${prepared.count} catalog bodies plus Sun/eight planets. Press Start prepared experiment to replace the demonstration.`;
    } catch(error) {experimentStatus.textContent=String(error);prepared=undefined;}
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
    } catch(error) {if(action===experimentAction)experimentStatus.textContent=String(error);}
  });
  root.querySelector('[data-demo]')!.addEventListener('click',()=>{
    ++experimentAction;
    runtime.ccall?.('solar_web_demo',null,[],[]);
    experimentStatus.textContent='Core perihelion demonstration restored.';
  });

  // Configure the classic Emscripten module before its generated loader runs.
  // Both assets use the page revision so cached builds cannot mix their ABI.
  window.Module = runtime;
  const routeKeyboard = (event: KeyboardEvent): void => routeSimulatorKeyboard(event, document.activeElement === canvas);
  window.addEventListener('keydown', routeKeyboard, true);
  window.addEventListener('keypress', routeKeyboard, true);
  const script = document.createElement('script');
  script.src = artifactUrl.href;
  script.onerror = () => runtime.onAbort('Could not download simulator JavaScript. Reload to retry.');
  canvas.addEventListener('webglcontextlost', () => runtime.onAbort('WebGL context lost. Reload to restart the simulation.'));
  document.body.append(script);
}
