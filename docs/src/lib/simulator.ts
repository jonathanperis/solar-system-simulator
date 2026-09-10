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
}

interface RuntimeControls {
  panel: HTMLFieldSetElement;
  pause: HTMLButtonElement;
  step: HTMLButtonElement;
  rotate: HTMLInputElement;
  body: HTMLSelectElement;
  speed: HTMLSelectElement;
  view: HTMLButtonElement;
}

interface RuntimeState {
  body: string; parent: string; view: string; cameraTarget: string; selected: number;
  paused: boolean; speedPreset: number; autoRotate: boolean;
  elapsedSeconds: number; intervalSeconds: number; trailsFailed: boolean; hasParent: boolean;
  distanceM: number; speedMps: number; massKg: number; radiusM: number; zoom: number;
}

// Matches SolarCommand in src/main.c. All actions execute in the C runtime.
export const runtimeCommands = { pause: 0, step: 1, reset: 2, speed: 3, select: 4, view: 5, zoom: 6, rotate: 7, frame: 8 } as const;

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
  let next = select.selectedIndex;
  if (key === 'Home') next = 0;
  else if (key === 'End') next = select.options.length - 1;
  else if (key === 'ArrowDown') next = Math.min(next + 1, select.options.length - 1);
  else if (key === 'ArrowUp') next = Math.max(next - 1, 0);
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
  const fail = (reason: unknown): void => {
    failed = true;
    controls.panel.disabled = true;
    setText(readouts.status, `Runtime error: ${String(reason)}`);
  };

  return {
    canvas,
    _solar_web_command: undefined as ((command: number, value: number) => void) | undefined,
    addBody(index: number, name: string) {
      controls.body.add(new Option(name, String(index)));
    },
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
    reportState(state: RuntimeState) {
      if (failed) return;
      controls.panel.disabled = false;
      controls.step.disabled = !state.paused;
      setText(controls.pause, state.paused ? 'Resume' : 'Pause');
      setText(controls.view, `View: ${state.view}`);
      controls.rotate.checked = state.autoRotate;
      // A native select may expose a tentative arrow-key value before change.
      // Sync only when C state changes, not on every animation-frame report.
      if (reportedBody !== state.selected) {
        controls.body.value = String(state.selected);
        reportedBody = state.selected;
      }
      if (reportedSpeed !== state.speedPreset) {
        controls.speed.value = String(state.speedPreset);
        reportedSpeed = state.speedPreset;
      }
      setText(readouts.status, state.trailsFailed ? 'Trail recording paused: memory unavailable.'
        : state.paused ? 'Simulation paused' : 'Running physics simulation');
      setText(readouts.controls, `Selected body: ${state.body}; view: ${state.view}.`);
      setText(readouts.elapsed, `${(state.elapsedSeconds / 86400).toFixed(5)} simulated days · ${state.elapsedSeconds.toFixed(0)} s`);
      setText(readouts.interval, `${(state.intervalSeconds / 3600).toFixed(2)} simulated hours`);
      setText(readouts.parent, state.parent);
      setText(readouts.distance, state.hasParent ? `${(state.distanceM / 1000).toFixed(3)} km` : 'N/A — no parent');
      setText(readouts.speed, state.hasParent ? `${(state.speedMps / 1000).toFixed(6)} km/s` : 'N/A — no parent');
      setText(readouts.mass, `${state.massKg.toExponential(6)} kg`);
      setText(readouts.radius, `${(state.radiusM / 1000).toFixed(3)} km`);
      setText(readouts.camera, `${state.cameraTarget} · ${state.zoom.toPrecision(4)} render units`);
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
    view: root.querySelector<HTMLButtonElement>('[data-command="view"]')!
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
    camera: root.querySelector<HTMLElement>('[data-runtime-camera]')!
  }, artifactUrl, controls);

  const send = (command: keyof typeof runtimeCommands, value = 0): void => {
    if (!controls.panel.disabled) runtime._solar_web_command!(runtimeCommands[command], value);
  };
  root.querySelectorAll<HTMLButtonElement>('[data-command]').forEach(button => {
    button.addEventListener('click', () => send(button.dataset.command as keyof typeof runtimeCommands, Number(button.dataset.value ?? 0)));
  });
  controls.body.addEventListener('change', () => send('select', Number(controls.body.value)));
  controls.speed.addEventListener('change', () => send('speed', Number(controls.speed.value)));
  for (const select of [controls.body, controls.speed]) {
    select.addEventListener('keydown', event => {
      if (moveSelectByKey(select, event.key)) {
        event.preventDefault();
        event.stopPropagation();
      }
    });
  }
  controls.rotate.addEventListener('change', () => send('rotate'));

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
