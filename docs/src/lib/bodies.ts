export type ImplementedBody = {
  name: string;
  kind: 'Star' | 'Planet' | 'Moon';
  parent: string;
  milestone: string;
  initialization: string;
  source: string;
  accent: 'solar' | 'cyan' | 'earth' | 'moon' | 'mars';
};

export const implementedBodies: ImplementedBody[] = [
  {
    name: 'Sun',
    kind: 'Star',
    parent: 'None',
    milestone: 'Foundation',
    initialization: 'Fixed origin anchor for the current heliocentric baseline.',
    source: 'src/sim/solar_system.c',
    accent: 'solar'
  },
  {
    name: 'Mercury',
    kind: 'Planet',
    parent: 'Sun',
    milestone: 'Inner planet pass',
    initialization: 'Heliocentric perihelion position with vis-viva tangential speed.',
    source: 'src/sim/solar_system.c',
    accent: 'cyan'
  },
  {
    name: 'Venus',
    kind: 'Planet',
    parent: 'Sun',
    milestone: 'Inner planet pass',
    initialization: 'Heliocentric perihelion position with vis-viva tangential speed.',
    source: 'src/sim/solar_system.c',
    accent: 'cyan'
  },
  {
    name: 'Earth',
    kind: 'Planet',
    parent: 'Sun',
    milestone: 'Earth pass',
    initialization: 'Heliocentric perihelion position on the +Z axis with vis-viva speed.',
    source: 'src/sim/solar_system.c',
    accent: 'earth'
  },
  {
    name: 'Moon',
    kind: 'Moon',
    parent: 'Earth',
    milestone: 'Earth Moon pass',
    initialization: 'Earth-relative perigee offset added to Earth absolute state.',
    source: 'src/sim/solar_system.c',
    accent: 'moon'
  },
  {
    name: 'Mars',
    kind: 'Planet',
    parent: 'Sun',
    milestone: 'Mars pass',
    initialization: 'Heliocentric perihelion position on the -Z axis with vis-viva speed.',
    source: 'src/sim/solar_system.c',
    accent: 'mars'
  },
  {
    name: 'Phobos',
    kind: 'Moon',
    parent: 'Mars',
    milestone: 'Mars moons pass',
    initialization: 'Mars-relative periareion state added to Mars absolute state.',
    source: 'src/sim/solar_system.c',
    accent: 'mars'
  },
  {
    name: 'Deimos',
    kind: 'Moon',
    parent: 'Mars',
    milestone: 'Mars moons pass',
    initialization: 'Mars-relative periareion state added to Mars absolute state.',
    source: 'src/sim/solar_system.c',
    accent: 'mars'
  }
];

export const plannedBodies = [
  'asteroid belt representatives / major asteroids',
  'Jupiter',
  'Galilean moons',
  'Saturn',
  'major Saturnian moons',
  'Uranus',
  'Neptune',
  'dwarf planets / Kuiper belt representatives'
];

export const bodyFocusOrder = implementedBodies.map((body) => body.name);
