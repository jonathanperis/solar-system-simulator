export type ImplementedBody = {
  slug: string;
  name: string;
  kind: 'Star' | 'Planet' | 'Moon' | 'Asteroid';
  parent: string;
  milestone: string;
  initialization: string;
  source: string;
  accent: 'solar' | 'cyan' | 'earth' | 'moon' | 'mars' | 'asteroid';
  chart: {
    plate: 'heliocentric' | 'earth' | 'mars';
    angle: number;
    radius: number;
  };
  summary: string;
};

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
  }
];

export const plannedBodies = [
  'Jupiter',
  'Galilean moons',
  'Saturn',
  'major Saturnian moons',
  'Uranus',
  'Neptune',
  'dwarf planets / Kuiper belt representatives'
];

export const bodyFocusOrder = implementedBodies.map((body) => body.name);
