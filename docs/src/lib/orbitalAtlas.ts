export type ChartBody = { chartAngle: number };

export function normalizeDegrees(degrees: number): number {
  return ((degrees % 360) + 360) % 360;
}

export function nearestBodyIndex(bodies: ChartBody[], degrees: number): number {
  const target = normalizeDegrees(degrees);
  return bodies.reduce((nearestIndex, body, index) => {
    const nearestDistance = angularDistance(bodies[nearestIndex].chartAngle, target);
    return angularDistance(body.chartAngle, target) < nearestDistance ? index : nearestIndex;
  }, 0);
}

export function cycleIndex(index: number, direction: number, length: number): number {
  return ((index + direction) % length + length) % length;
}

function angularDistance(first: number, second: number): number {
  return Math.abs(((first - second + 540) % 360) - 180);
}

interface AnnouncerTimers { set(callback: () => void, delayMs: number): unknown; clear(handle: unknown): void }

/** One polite status region for the atlas. Discrete actions (click, keys)
 * announce immediately; continuous input (pointer drag, a dragged range thumb)
 * announces only after it settles, so assistive technology hears the final
 * selection once instead of every intermediate body. */
export function createSettledAnnouncer(write: (text: string) => void, settleMs = 400,
  timers: AnnouncerTimers = { set: (callback, delayMs) => setTimeout(callback, delayMs), clear: handle => clearTimeout(handle as number) }) {
  let pending: string | undefined, handle: unknown;
  const cancel = (): void => { if (handle !== undefined) timers.clear(handle); handle = undefined; };
  return {
    now(text: string): void { cancel(); pending = undefined; write(text); },
    /** Remember the latest text without announcing (input still in progress). */
    hold(text: string): void { cancel(); pending = text; },
    /** Announce the latest text once input has been idle for settleMs. */
    settle(text: string): void {
      cancel(); pending = text;
      handle = timers.set(() => { handle = undefined; if (pending !== undefined) write(pending); pending = undefined; }, settleMs);
    },
    /** Announce held text now (for example on pointerup). */
    flush(): void { cancel(); if (pending !== undefined) write(pending); pending = undefined; }
  };
}
