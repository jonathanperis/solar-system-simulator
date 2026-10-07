import { implementedBodies, mainSceneBodies } from './bodies.ts';
import manifest from '../../public/catalog/manifest.json' with { type: 'json' };

/** Counts quoted in page copy, derived at build time from the same data the
 * atlas and catalog use, so prose cannot drift from the scene or snapshot. */
export const coreBodyCount = mainSceneBodies.length;
export const totalBodyCount = implementedBodies.length;
export const familyMoonCount = implementedBodies.filter(body => body.scene !== 'core').length;
const moonsOf = (parent: string) => implementedBodies.filter(body => body.parent === parent).length;
export const jovianMoonCount = moonsOf('Jupiter');
export const saturnianMoonCount = moonsOf('Saturn');
export const uranianMoonCount = moonsOf('Uranus');
export const neptunianMoonCount = moonsOf('Neptune');
export const plutonianMoonCount = moonsOf('Pluto');
export const smallBodyCatalogCount = manifest.count;
export const formatCount = (value: number): string => value.toLocaleString('en-US');
