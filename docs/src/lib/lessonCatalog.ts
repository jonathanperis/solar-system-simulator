export const lessonOptions = [
  ['core', 'Core demonstration'], ['circular', 'Circular orbit'], ['eccentric', 'Eccentric orbit'],
  ['escape', 'Escape threshold'], ['earth-moon', 'Barycentric Earth–Moon'], ['inclined', 'Inclined orbit'],
  ['phobos', 'Phobos resolution'], ['barycentric-core', 'Moving-Sun barycentric core'],
  ['resonance', '3:2 resonance experiment'], ['encounter', 'Close Earth encounter'], ['collision', 'Head-on collisions']
] as const;

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
