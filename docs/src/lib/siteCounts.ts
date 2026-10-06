import { implementedBodies } from './bodies.ts';
import manifest from '../../public/catalog/manifest.json' with { type: 'json' };

/** Counts quoted in page copy, derived at build time from the same data the
 * atlas and catalog use, so prose cannot drift from the scene or snapshot. */
export const coreBodyCount = implementedBodies.length;
export const jovianMoonCount = implementedBodies.filter(body => body.parent === 'Jupiter').length;
export const smallBodyCatalogCount = manifest.count;
export const formatCount = (value: number): string => value.toLocaleString('en-US');
