import { implementedBodies } from './bodies.ts';

/** Illustrative chart placement for the homepage atlas. These percentages are
 * presentation only: they index bodies on a readable plate and never describe
 * physical positions (V20). */
const chartPosition = (angle: number, radius: number): [number, number] => {
  const radians = (angle - 90) * Math.PI / 180;
  const distance = radius * 0.43;
  return [50 + Math.cos(radians) * distance, 50 + Math.sin(radians) * distance];
};

// The compact chart is an index, not a scaled orbit plot. Even angular spacing
// keeps all nine non-star heliocentric controls distinct at a 320px viewport.
const compactBodies = implementedBodies.filter(body => body.chart.plate === 'heliocentric' && body.slug !== 'sun');

export function compactAngle(slug: string, angle: number): number {
  const index = compactBodies.findIndex(body => body.slug === slug);
  return index < 0 ? angle : 20 + index * 360 / compactBodies.length;
}

/** Per-body custom properties as a static stylesheet. Emitting them as a CSS
 * file instead of one `style` attribute per body lets the page CSP forbid inline styles
 * (`style-src 'self'`) without hashes. */
export function atlasPositionCss(): string {
  return implementedBodies.map(body => {
    const [left, top] = chartPosition(body.chart.angle, body.chart.radius);
    const compact = compactBodies.some(item => item.slug === body.slug);
    const [compactLeft, compactTop] = chartPosition(compactAngle(body.slug, body.chart.angle), compact ? 85 : body.chart.radius);
    // Slugs are generated identifiers ([a-z0-9-]); CSS.escape is unavailable at build time.
    if (!/^[a-z0-9-]+$/.test(body.slug)) throw new Error(`Unsafe atlas slug: ${body.slug}`);
    return `.atlas-body[data-slug="${body.slug}"]{--chart-left:${left}%;--chart-top:${top}%;--compact-left:${compactLeft}%;--compact-top:${compactTop}%}`;
  }).join('\n') + '\n';
}
