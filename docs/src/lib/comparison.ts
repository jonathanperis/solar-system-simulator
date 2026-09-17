import { comparisonPresets } from './lessonCatalog.ts';

interface LabModule {
  ccall(name: string, result: string, types: string[], args: unknown[]): unknown;
  UTF8ToString(pointer: number): string;
  _lab_revision(): number;
  _lab_field_count(): number;
  _lab_field_name(index: number): number;
  _lab_advance(budget: number, oneCheckpoint: number): number;
  _lab_status(field: number): number;
  _lab_point_count(): number;
  _lab_point(index: number, side: number, field: number): number;
  _lab_subject(): number;
  _lab_force_count(side: number): number;
  _lab_force_name(side: number, index: number): number;
  _lab_force(side: number, index: number, field: number): number;
}
type PlotPoint = { x: number; y: number };

export function plotSegments(points: PlotPoint[], bounds: [number, number, number, number], wrap = false): string[] {
  const [xmin, xmax, ymin, ymax] = bounds;
  const segments: string[] = []; let current: string[] = []; let previous: number | undefined;
  const flush = () => { if (current.length) segments.push(current.join(' ')); current = []; };
  for (const point of points) {
    if (!Number.isFinite(point.x) || !Number.isFinite(point.y)) { flush(); previous = undefined; continue; }
    if (wrap && previous !== undefined && Math.abs(point.y - previous) > 180) flush();
    const x = 45 + 435 * (point.x - xmin) / (xmax - xmin || 1);
    const y = 175 - 150 * (point.y - ymin) / (ymax - ymin || 1);
    current.push(`${x.toFixed(2)},${y.toFixed(2)}`); previous = point.y;
  }
  flush(); return segments;
}

function download(text: string, filename: string, type: string): void {
  const url = URL.createObjectURL(new Blob([text], { type }));
  const anchor = document.createElement('a'); anchor.href = url; anchor.download = filename; anchor.click();
  setTimeout(() => URL.revokeObjectURL(url), 0);
}

const compact = (value: number) => Number.isFinite(value) ? value.toExponential(4) : 'Unavailable';

export async function mountComparison(root: HTMLElement): Promise<void> {
  const form = root.querySelector<HTMLFormElement>('form')!;
  const panel = root.querySelector<HTMLFieldSetElement>('fieldset')!;
  const status = root.querySelector<HTMLElement>('[data-lab-status]')!;
  const progress = root.querySelector<HTMLProgressElement>('progress')!;
  const input = (name: string) => form.elements.namedItem(name) as HTMLInputElement | HTMLSelectElement;
  const names = ['scene', 'factor', 'methodA', 'dtA', 'collisionA', 'methodB', 'dtB', 'collisionB', 'sample', 'duration'];
  let lab: LabModule, activeDefinition = '', running = false, stepping = false, stepStart = 0, frame = 0, lastDraw = 0;
  const fail = (error: unknown): void => { running = stepping = false; panel.disabled = true; status.textContent = `Comparison runtime unavailable: ${String(error)}`; };
  try {
    const url = new URL(root.dataset.labSrc!, document.baseURI);
    const { default: createLab } = await import(/* @vite-ignore */ url.href);
    lab = await createLab({ locateFile: (path: string) => { const file = new URL(path, url); file.search = url.search; return file.href; }, onAbort: fail });
  } catch (error) { fail(error); return; }
  const revision = lab.UTF8ToString(lab._lab_revision());
  root.querySelector('[data-lab-revision]')!.textContent = revision;
  const fieldNames = Array.from({ length: lab._lab_field_count() }, (_, i) => lab.UTF8ToString(lab._lab_field_name(i)));
  const normalize = (text: string): string => {
    if (text.includes('\0') || new TextEncoder().encode(text).length >= 512) throw new Error('Descriptor exceeds 511 bytes or contains a null character.');
    const normalized = lab.ccall('lab_normalize', 'string', ['string'], [text]);
    if (typeof normalized !== 'string' || !normalized) throw new Error('Invalid configuration. Sample spacing must be a whole multiple of both timesteps, duration a whole multiple of sample spacing, and collision steps 0.01–0.25 s with bounce/merge policies.');
    return normalized;
  };
  const readForm = (): string => normalize(`SOLAR_LAB_V1 ${names.map(name => input(name).value).join(' ')}`);
  const markPending = (): void => {
    const values = activeDefinition.split(/\s+/).slice(1);
    const changed = activeDefinition && names.some((name, i) => ['factor', 'dtA', 'dtB', 'sample', 'duration'].includes(name)
      ? Number(input(name).value) !== Number(values[i]) : input(name).value !== values[i]);
    root.querySelector<HTMLElement>('[data-pending-config]')!.hidden = !changed;
  };
  const fill = (definition: string): void => {
    const values = normalize(definition).trim().split(/\s+/).slice(1);
    names.forEach((name, index) => { input(name).value = ['factor', 'dtA', 'dtB', 'sample', 'duration'].includes(name)
      ? String(Number(values[index])) : values[index]; });
    markPending();
  };
  const setStatus = (text: string) => { if (status.textContent !== text) status.textContent = text; };

  const drawPlot = (svg: SVGSVGElement, series: PlotPoint[][], trajectory: boolean, wrap: boolean): void => {
    const xs = series.flat().map(p => p.x).filter(Number.isFinite), ys = series.flat().map(p => p.y).filter(Number.isFinite);
    svg.replaceChildren();
    const range = svg.closest('figure')!.querySelector('[data-plot-range]')!;
    if (!xs.length || !ys.length) { range.textContent = 'Unavailable for the current preset or subject.'; return; }
    let xmin = Math.min(...xs), xmax = Math.max(...xs), ymin = Math.min(...ys), ymax = Math.max(...ys);
    if (trajectory) {
      // Equal physical scale on X/Z: the plotting box is 435 by 150 CSS units.
      const xmid = (xmin+xmax)/2, ymid = (ymin+ymax)/2;
      const span = Math.max(xmax-xmin, (ymax-ymin)*435/150, 1e-12);
      xmin = xmid-span/2; xmax = xmid+span/2; ymin = ymid-span*150/435/2; ymax = ymid+span*150/435/2;
    } else if (ymin === ymax) { ymin -= 1; ymax += 1; }
    const node = (tag: string, attributes: Record<string, string>, text?: string) => {
      const element = document.createElementNS('http://www.w3.org/2000/svg', tag);
      Object.entries(attributes).forEach(([key, value]) => element.setAttribute(key, value));
      if (text) element.textContent = text;
      svg.append(element); return element;
    };
    node('path', { d: 'M45 25V175H480', fill: 'none', stroke: 'currentColor', 'stroke-opacity': '.4' });
    series.forEach((points, side) => plotSegments(points, [xmin, xmax, ymin, ymax], wrap).forEach(points => {
      node('polyline', { points, fill: 'none', stroke: side ? '#9a3b17' : '#0e5682', 'stroke-width': '2',
        'stroke-dasharray': side ? '6 3' : 'none', 'data-series': side ? 'B' : 'A' });
    }));
    range.textContent = `Y: ${compact(ymin)} … ${compact(ymax)} · X: ${compact(xmin)} … ${compact(xmax)}`;
  };

  const render = (): void => {
    const count = lab._lab_point_count(); if (!count) return;
    const points = Array.from({ length: count }, (_, index) => ({
      time: lab._lab_point(index, 0, -1), difference: lab._lab_point(index, 0, -2),
      values: [0, 1].map(side => Object.fromEntries(fieldNames.map((name, field) => [name, lab._lab_point(index, side, field)])))
    }));
    const latest = points.at(-1)!;
    root.querySelector('[data-displayed-definition]')!.textContent = activeDefinition;
    progress.value = lab._lab_status(3) / lab._lab_status(4);
    root.querySelector('[data-matched-time]')!.textContent = `${latest.time} s`;
    root.querySelector('[data-retention]')!.textContent = `${count} points; retaining every ${lab._lab_status(6)} checkpoint(s), plus the latest endpoint.`;
    root.querySelector('[data-subject]')!.textContent = lab.UTF8ToString(lab._lab_subject());
    const divisor = input('units').value === 'au' ? 149597870700 : 1;
    root.querySelectorAll<SVGSVGElement>('[data-plot]').forEach(svg => {
      const field = svg.dataset.plot!, trajectory = field === 'trajectory';
      const series = [0, 1].map(side => points.map(point => ({
        x: trajectory ? point.values[side].x_m / divisor : point.time,
        y: trajectory ? point.values[side].z_m / divisor : field === 'difference' ? point.difference : point.values[side][field]
      })));
      drawPlot(svg, field === 'difference' ? [series[0]] : series, trajectory, field === 'resonant_angle_deg');
    });
    root.querySelector('[data-trajectory-units]')!.textContent = `X/Z projection in ${divisor === 1 ? 'meters' : 'AU'}; equal axis scale. Parent-relative when a parent exists, otherwise inertial. Lines join recorded checkpoints; coarse sampling can obscure orbital loops.`;
    const table = root.querySelector<HTMLTableSectionElement>('[data-measurements]')!;
    table.replaceChildren(...fieldNames.map(name => {
      const row = document.createElement('tr');
      for (const value of [name, compact(latest.values[0][name]), compact(latest.values[1][name])]) {
        const cell = document.createElement('td'); cell.textContent = value; row.append(cell);
      }
      return row;
    }));
    for (const side of [0, 1]) {
      const body = root.querySelector<HTMLTableSectionElement>(`[data-force-side="${side}"]`)!;
      const total = lab._lab_force_count(side);
      body.replaceChildren(...Array.from({ length: Math.min(total, 6) }, (_, index) => {
        const row = document.createElement('tr');
        const values = [lab.UTF8ToString(lab._lab_force_name(side, index)), compact(lab._lab_force(side, index, 0)),
          `${(100 * lab._lab_force(side, index, 1)).toFixed(3)}%`,
          [2, 3, 4].map(field => compact(lab._lab_force(side, index, field))).join(', ')];
        for (const value of values) { const cell = document.createElement('td'); cell.textContent = value; row.append(cell); }
        return row;
      }));
      if (!total) {
        const row = document.createElement('tr'), cell = document.createElement('td'); cell.colSpan = 4;
        cell.textContent = latest.values[side].subject_present ? 'No other known-mass sources.' : 'Tracked subject merged; per-body force is unavailable.';
        row.append(cell); body.append(row);
      }
    }
    const feedback = root.querySelector<HTMLElement>('[data-challenge-feedback]')!;
    const scene = activeDefinition.split(/\s+/)[1];
    if (input('challenge').value === 'phase') {
      feedback.textContent = scene !== 'phobos' || Number(activeDefinition.split(/\s+/)[2]) !== 1 ? 'Load the Phobos challenge with initial speed factor 1 to measure its analytical phase error.'
        : latest.time < 8640000 ? 'Complete 100 simulated days before judging the phase budget.'
        : `Run A maximum sampled phase error: ${lab._lab_status(7).toFixed(6)}°. ${lab._lab_status(7) < 1 ? 'Budget met: below 1°.' : 'Budget exceeded: reduce A timestep and repeat.'} Run B: ${lab._lab_status(8).toFixed(6)}°.`;
    } else if (input('challenge').value === 'escape') {
      feedback.textContent = `Specific energy A: ${compact(latest.values[0].specific_energy_jpkg)} J/kg. Negative is bound, positive unbound, near zero marginal. Repeat the escape preset at factors 0.99 and 1.01.`;
    } else if (input('challenge').value === 'momentum') {
      feedback.textContent = `Linear momentum magnitude A: ${compact(latest.values[0].linear_momentum_kg_mps)} kg·m/s. Compare a moving-Sun or Earth–Moon run with the constrained core; interpret roundoff relative to the bodies' individual momenta.`;
    } else feedback.textContent = 'Pause, export CSV, change the trajectory display units, then export again. The physical data should be identical. The 3D simulator also supports this check across illustrative/real scale.';
  };

  const schedule = () => { if (!frame) frame = requestAnimationFrame(tick); };
  const tick = (now: number): void => {
    frame = 0; if (!running && !stepping) return;
    const state = lab._lab_advance(8192, stepping ? 1 : 0);
    if (state < 0) { running = stepping = false; setStatus('Numerical state became non-finite. Reduce the timestep; the last valid checkpoints are retained.'); }
    else if (state === 2) { running = stepping = false; setStatus('Comparison complete.'); }
    else if (stepping && lab._lab_status(3) > stepStart) { stepping = false; setStatus('Paused at the next matched checkpoint.'); }
    if (now-lastDraw >= 150 || (!running && !stepping)) { render(); lastDraw = now; }
    if (running || stepping) schedule();
  };
  form.addEventListener('submit', event => {
    event.preventDefault(); running = stepping = false;
    try {
      const definition = readForm();
      if (!lab.ccall('lab_start', 'number', ['string'], [definition])) throw new Error('C rejected configuration.');
      activeDefinition = definition.trim(); markPending(); running = true; setStatus('Computing matched checkpoints…'); render(); schedule();
    } catch (error) { setStatus(`${String(error)} Previous run retained and paused.`); }
  });
  root.querySelector('[data-pause-comparison]')!.addEventListener('click', () => { running = stepping = false; render(); setStatus('Comparison paused. Plots show the last matched checkpoint.'); });
  root.querySelector('[data-resume-comparison]')!.addEventListener('click', () => {
    if (lab._lab_status(0) && !lab._lab_status(1) && !lab._lab_status(2)) { running = true; setStatus('Computing matched checkpoints…'); schedule(); }
  });
  root.querySelector('[data-next-checkpoint]')!.addEventListener('click', () => {
    if (lab._lab_status(0) && !lab._lab_status(1) && !lab._lab_status(2)) { running = false; stepping = true; stepStart = lab._lab_status(3); schedule(); }
  });
  root.querySelector('[data-reset-comparison]')!.addEventListener('click', () => {
    if (!activeDefinition) return;
    running = stepping = false; lab.ccall('lab_start', 'number', ['string'], [activeDefinition]); render(); setStatus('Comparison reset and paused.');
  });
  input('scene').addEventListener('change', () => fill(comparisonPresets[input('scene').value]));
  form.addEventListener('input', markPending);
  form.addEventListener('change', markPending);
  input('units').addEventListener('change', render);
  root.querySelector('[data-export-comparison]')!.addEventListener('click', () => {
    const csv = lab.ccall('lab_export_csv', 'string', [], []);
    if (typeof csv === 'string' && csv) download(csv, 'solar-comparison.csv', 'text/csv');
    else setStatus('Start a comparison before exporting measurements.');
  });
  root.querySelector('[data-save-config]')!.addEventListener('click', () => {
    try { download(readForm(), 'experiment.solar', 'text/plain'); } catch (error) { setStatus(String(error)); }
  });
  root.querySelector('[data-share-config]')!.addEventListener('click', () => {
    try {
      const url = new URL(location.pathname, location.origin); url.searchParams.set('lab', readForm().trim()); url.searchParams.set('revision', revision);
      const share = root.querySelector<HTMLInputElement>('[data-share-link]')!; share.value = url.href; share.focus(); share.select();
      setStatus('Share link ready. Opening it loads parameters without starting a run.');
    } catch (error) { setStatus(String(error)); }
  });
  root.querySelector<HTMLInputElement>('[data-import-config]')!.addEventListener('change', async event => {
    const file = (event.target as HTMLInputElement).files?.[0]; if (!file) return;
    try {
      if (file.size >= 512) throw new Error('Configuration must be smaller than 512 bytes.');
      fill(await file.text()); setStatus('Configuration imported. Press Start comparison to run it.');
    } catch (error) { setStatus(`${String(error)} Previous configuration retained.`); }
  });
  root.querySelector('[data-load-challenge]')!.addEventListener('click', () => {
    const choice = input('challenge').value;
    fill(comparisonPresets[choice === 'phase' ? 'phobos' : choice === 'escape' ? 'escape' : choice === 'momentum' ? 'barycentric-core' : 'circular']);
    root.querySelector('[data-challenge-feedback]')!.textContent = choice === 'phase'
      ? 'Goal: complete 100 simulated days with initial speed factor 1 and maximum sampled phase error below 1°. Reduce run A timestep; keep the initial conditions fixed.'
      : choice === 'escape' ? 'Goal: run the escape preset at factors 0.99 and 1.01, then explain the specific-energy sign.'
      : choice === 'momentum' ? 'Goal: compare momentum behavior in the moving-Sun and constrained core presets. Explain the external constraint and floating-point scale.'
      : 'Goal: pause a completed run, export CSV, change display units, then verify the next CSV is identical.';
    setStatus('Challenge loaded. Write a prediction, then start the comparison.');
  });
  panel.disabled = false;
  const params = new URL(location.href).searchParams;
  try {
    fill(params.get('lab') ?? comparisonPresets.circular);
    setStatus(params.has('lab') ? `Shared configuration loaded; press Start comparison. Created revision: ${params.get('revision') ?? 'not recorded'}; running revision: ${revision}.` : 'Comparison lab ready. Choose parameters and start.');
  } catch (error) { fill(comparisonPresets.circular); setStatus(`Shared configuration rejected: ${String(error)}`); }
}
