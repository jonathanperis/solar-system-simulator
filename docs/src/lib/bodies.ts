import jovianCatalog from '../../../data/jovian_moons.json' with { type: 'json' };

export type ImplementedBody = {
  slug: string;
  name: string;
  kind: 'Star' | 'Planet' | 'Moon' | 'Asteroid';
  parent: string;
  milestone: string;
  initialization: string;
  source: string;
  accent: 'solar' | 'cyan' | 'earth' | 'moon' | 'mars' | 'asteroid' | 'jupiter' | 'saturn' | 'uranus' | 'neptune';
  chart: {
    plate: 'heliocentric' | 'earth' | 'mars' | 'jupiter';
    angle: number;
    radius: number;
  };
  summary: string;
  group?: string;
};

/** Beginner copy uses the same identity and parent relationship as the catalog. */
export function bodyIntroduction(body: ImplementedBody): string {
  if (body.kind === 'Star') return 'The Sun is the central star of this model. Watch the planets travel around it; the core demonstration holds the Sun fixed.';
  if (body.kind === 'Moon') return `${body.name} orbits ${body.parent}. Explore its family to see a moon’s motion alongside its parent.`;
  return `${body.name} orbits the ${body.parent}. Watch its path, change your view, and compare its motion with other worlds.`;
}

export const implementedBodies: ImplementedBody[] = [
  {
    slug: 'sun',
    name: 'Sun',
    kind: 'Star',
    parent: 'None',
    milestone: 'Foundation',
    initialization: 'Fixed origin anchor for the current heliocentric baseline.',
    source: 'src/sim/solar_system.c',
    accent: 'solar',
    chart: { plate: 'heliocentric', angle: 0, radius: 0 },
    summary: 'Fixed origin anchor for the current heliocentric baseline.'
  },
  {
    slug: 'mercury',
    name: 'Mercury',
    kind: 'Planet',
    parent: 'Sun',
    milestone: 'Inner planet pass',
    initialization: 'Heliocentric perihelion position with vis-viva tangential speed.',
    source: 'src/sim/solar_system.c',
    accent: 'cyan',
    chart: { plate: 'heliocentric', angle: 324, radius: 21 },
    summary: 'Inner planet initialized at heliocentric perihelion.'
  },
  {
    slug: 'venus',
    name: 'Venus',
    kind: 'Planet',
    parent: 'Sun',
    milestone: 'Inner planet pass',
    initialization: 'Heliocentric perihelion position with vis-viva tangential speed.',
    source: 'src/sim/solar_system.c',
    accent: 'cyan',
    chart: { plate: 'heliocentric', angle: 52, radius: 35 },
    summary: 'Nearly circular inner orbit seeded at perihelion.'
  },
  {
    slug: 'earth',
    name: 'Earth',
    kind: 'Planet',
    parent: 'Sun',
    milestone: 'Earth pass',
    initialization: 'Heliocentric perihelion position on the +Z axis with vis-viva speed.',
    source: 'src/sim/solar_system.c',
    accent: 'earth',
    chart: { plate: 'heliocentric', angle: 152, radius: 51 },
    summary: 'Reference planet for the Earth-Moon relative system.'
  },
  {
    slug: 'moon',
    name: 'Moon',
    kind: 'Moon',
    parent: 'Earth',
    milestone: 'Earth Moon pass',
    initialization: 'Earth-relative perigee offset added to Earth absolute state.',
    source: 'src/sim/solar_system.c',
    accent: 'moon',
    chart: { plate: 'earth', angle: 37, radius: 38 },
    summary: 'Earth-relative perigee state, shown on the Earth plate.'
  },
  {
    slug: 'mars',
    name: 'Mars',
    kind: 'Planet',
    parent: 'Sun',
    milestone: 'Mars pass',
    initialization: 'Heliocentric perihelion position on the -Z axis with vis-viva speed.',
    source: 'src/sim/solar_system.c',
    accent: 'mars',
    chart: { plate: 'heliocentric', angle: 228, radius: 66 },
    summary: 'Outer inner-planet orbit seeded at perihelion.'
  },
  {
    slug: 'phobos',
    name: 'Phobos',
    kind: 'Moon',
    parent: 'Mars',
    milestone: 'Mars moons pass',
    initialization: 'Mars-relative periareion state added to Mars absolute state.',
    source: 'src/sim/solar_system.c',
    accent: 'mars',
    chart: { plate: 'mars', angle: 210, radius: 24 },
    summary: 'Inner Martian moon shown relative to Mars.'
  },
  {
    slug: 'deimos',
    name: 'Deimos',
    kind: 'Moon',
    parent: 'Mars',
    milestone: 'Mars moons pass',
    initialization: 'Mars-relative periareion state added to Mars absolute state.',
    source: 'src/sim/solar_system.c',
    accent: 'mars',
    chart: { plate: 'mars', angle: 33, radius: 47 },
    summary: 'Outer Martian moon shown relative to Mars.'
  },
  {
    slug: 'vesta',
    name: 'Vesta',
    kind: 'Asteroid',
    parent: 'Sun',
    milestone: 'Asteroid pass',
    initialization: 'Planar heliocentric perihelion position with vis-viva tangential speed.',
    source: 'src/sim/solar_system.c',
    accent: 'asteroid',
    chart: { plate: 'heliocentric', angle: 274, radius: 80 },
    summary: 'Main-belt asteroid, represented as a single sourced body.'
  },
  {
    slug: 'jupiter',
    name: 'Jupiter',
    kind: 'Planet',
    parent: 'Sun',
    milestone: 'Outer planet pass',
    initialization: 'Planar heliocentric perihelion position with vis-viva tangential speed.',
    source: 'src/sim/solar_system.c',
    accent: 'jupiter',
    chart: { plate: 'heliocentric', angle: 112, radius: 91 },
    summary: 'First gas giant, initialized at heliocentric perihelion.'
  },
  ...jovianCatalog.moons.map((moon, index): ImplementedBody => ({
    slug: moon.slug,
    name: moon.name,
    kind: 'Moon',
    parent: 'Jupiter',
    milestone: 'Complete Jovian moon catalog',
    group: moon.group,
    initialization: `${moon.frame}-frame mean elements converted to Jupiter-relative SI position and velocity.`,
    source: 'data/jovian_moons.json',
    accent: 'jupiter',
    chart: { plate: 'jupiter', angle: (index % 6) * 60 + 30, radius: 72 },
    summary: `${moon.group}. ${moon.inclination_deg > 90 ? 'Retrograde' : 'Prograde'} in the source frame. Mass: ${moon.mass_quality === 'unknown' ? 'unknown — test particle' : moon.mass_quality}. Radius: ${moon.radius_quality === 'unknown' ? 'unknown — marker only' : moon.radius_quality}.`
  })),
  {
    slug: 'saturn',
    name: 'Saturn',
    kind: 'Planet',
    parent: 'Sun',
    milestone: 'Saturn pass',
    initialization: 'Planar heliocentric perihelion position with vis-viva tangential speed.',
    source: 'src/sim/solar_system.c',
    accent: 'saturn',
    chart: { plate: 'heliocentric', angle: 196, radius: 97 },
    summary: 'Ringed gas giant initialized at heliocentric perihelion; rings are renderer-only.'
  },
  {slug:'uranus',name:'Uranus',kind:'Planet',parent:'Sun',milestone:'Uranus foundation',
    initialization:'Planar heliocentric perihelion with vis-viva speed.',source:'src/sim/solar_system.c',accent:'uranus',
    chart:{plate:'heliocentric',angle:270,radius:83},summary:'Ice giant with JPL-sourced mass, mean radius, and orbital elements.'},
  {slug:'neptune',name:'Neptune',kind:'Planet',parent:'Sun',milestone:'Neptune foundation',
    initialization:'Planar heliocentric perihelion with vis-viva speed.',source:'src/sim/solar_system.c',accent:'neptune',
    chart:{plate:'heliocentric',angle:330,radius:91},summary:'Outer giant included in every selected small-body experiment.'}
];

export const plannedBodies = [
  'complete Saturnian moons',
  'Uranian and Neptunian moon catalogs',
  'small-body satellite systems'
];

export const bodyFocusOrder = implementedBodies.map((body) => body.name);
