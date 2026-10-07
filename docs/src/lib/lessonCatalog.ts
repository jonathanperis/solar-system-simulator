// Order matches C's LessonPreset (tools/check_catalog.mjs). The last four are
// family scenes, chosen from the simulator's scene picker, not lessons.
export const lessonOptions = [
  ['core', 'Main scene'], ['circular', 'Circular orbit'], ['eccentric', 'Eccentric orbit'],
  ['escape', 'Escape threshold'], ['earth-moon', 'Barycentric Earth–Moon'], ['inclined', 'Inclined orbit'],
  ['phobos', 'Phobos resolution'], ['barycentric-core', 'Moving-Sun barycentric core'],
  ['resonance', '3:2 resonance experiment'], ['encounter', 'Close Earth encounter'], ['collision', 'Head-on collisions'],
  ['jupiter-system', 'Jupiter system'], ['saturn-system', 'Saturn system'], ['uranus-system', 'Uranus system'],
  ['neptune-system', 'Neptune system']
] as const;

export type LessonName = (typeof lessonOptions)[number][0];
/** Astronomy scenes (fixed 15 s Verlet): the main scene and the family scenes. */
export const sceneNames: readonly LessonName[] = ['core', 'jupiter-system', 'saturn-system', 'uranus-system', 'neptune-system'];
/** Guided lessons offered in the Learn panel and on the comparison page. */
export const guidedLessons = lessonOptions.filter(([name]) => name === 'core' || !sceneNames.includes(name));

export const comparisonPresets: Record<string, string> = {
  core: 'SOLAR_LAB_V1 core 1 verlet 15 none verlet 15 none 3600 86400',
  circular: 'SOLAR_LAB_V1 circular 1 verlet 300 none verlet 150 none 3600 86400',
  eccentric: 'SOLAR_LAB_V1 eccentric 1 verlet 300 none euler 300 none 3600 864000',
  escape: 'SOLAR_LAB_V1 escape 1 verlet 300 none verlet 150 none 3600 864000',
  'earth-moon': 'SOLAR_LAB_V1 earth-moon 1 verlet 300 none verlet 150 none 3600 864000',
  inclined: 'SOLAR_LAB_V1 inclined 1 verlet 300 none verlet 150 none 3600 864000',
  phobos: 'SOLAR_LAB_V1 phobos 1 verlet 300 none verlet 15 none 21600 8640000',
  'barycentric-core': 'SOLAR_LAB_V1 barycentric-core 1 verlet 15 none verlet 15 none 3600 86400',
  resonance: 'SOLAR_LAB_V1 resonance 1 verlet 300 none verlet 150 none 86400 3153600000',
  encounter: 'SOLAR_LAB_V1 encounter 1 verlet 300 none verlet 15 none 1800 86400',
  collision: 'SOLAR_LAB_V1 collision 1 verlet 0.1 bounce verlet 0.1 merge 1 20'
};
