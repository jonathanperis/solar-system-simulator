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
