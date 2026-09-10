type Readout = Pick<HTMLElement, 'textContent'>;
interface RuntimeReadouts {
  status: Readout;
  controls: Readout;
  elapsed: Readout;
  interval: Readout;
}

function setText(element: Readout, text: string): void {
  if (element.textContent !== text) element.textContent = text;
}

/** Run before Emscripten's window listener so GLFW cannot cancel browser Tab. */
export function preserveBrowserTabNavigation(event: Pick<KeyboardEvent, 'key' | 'stopImmediatePropagation'>): void {
  if (event.key === 'Tab') event.stopImmediatePropagation();
}

/** Emscripten calls this boundary; all physics remains inside the C runtime. */
export function createSimulatorModule(canvas: HTMLCanvasElement, readouts: RuntimeReadouts, artifactUrl: URL) {
  let failed = false;
  const fail = (reason: unknown): void => {
    failed = true;
    setText(readouts.status, `Runtime error: ${String(reason)}`);
  };

  return {
    canvas,
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
    reportState(body: string, view: string, days: number, intervalSeconds: number, trailsFailed: number) {
      if (failed) return;
      setText(readouts.status, trailsFailed ? 'Trail recording paused: memory unavailable.' : 'Running physics simulation');
      setText(readouts.controls, `Focused body: ${body}; view: ${view}.`);
      setText(readouts.elapsed, `${days.toFixed(2)} simulated days`);
      setText(readouts.interval, `${(intervalSeconds / 3600).toFixed(2)} simulated hours`);
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
  const runtime = createSimulatorModule(canvas, {
    status: root.querySelector<HTMLElement>('[data-runtime-status]')!,
    controls: root.querySelector<HTMLElement>('[data-runtime-controls]')!,
    elapsed: root.querySelector<HTMLElement>('[data-runtime-elapsed]')!,
    interval: root.querySelector<HTMLElement>('[data-runtime-interval]')!
  }, artifactUrl);

  // Configure the classic Emscripten module before its generated loader runs.
  // Both assets use the page revision so cached builds cannot mix their ABI.
  window.Module = runtime;
  window.addEventListener('keydown', preserveBrowserTabNavigation, true);
  const script = document.createElement('script');
  script.src = artifactUrl.href;
  script.onerror = () => runtime.onAbort('Could not download simulator JavaScript. Reload to retry.');
  canvas.addEventListener('webglcontextlost', () => runtime.onAbort('WebGL context lost. Reload to restart the simulation.'));
  document.body.append(script);
}
