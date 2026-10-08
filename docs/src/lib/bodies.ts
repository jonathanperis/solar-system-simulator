import jovianCatalog from '../../../data/jovian_moons.json' with { type: 'json' };
import saturnianCatalog from '../../../data/saturnian_moons.json' with { type: 'json' };
import uranianCatalog from '../../../data/uranian_moons.json' with { type: 'json' };
import neptunianCatalog from '../../../data/neptunian_moons.json' with { type: 'json' };
import plutonianCatalog from '../../../data/plutonian_moons.json' with { type: 'json' };
import didymosCatalog from '../../../data/didymos_moons.json' with { type: 'json' };

/** Scenes mirror the C presets: `core` is the main scene of large bodies;
 * each family scene holds one giant planet's complete moon catalog. */
export type SceneName = 'core' | 'jupiter-system' | 'saturn-system' | 'uranus-system' | 'neptune-system'
  | 'pluto-system' | 'didymos-system';
export type FamilyPrimary = 'Jupiter' | 'Saturn' | 'Uranus' | 'Neptune' | 'Pluto' | 'Didymos';

export type ImplementedBody = {
  slug: string;
  name: string;
  kind: 'Star' | 'Planet' | 'Dwarf planet' | 'Moon' | 'Asteroid';
  parent: string;
  milestone: string;
  initialization: string;
  source: string;
  summary: string;
  group?: string;
  /** Where the body is simulated: the main scene, or (small moons only) its
   * planet's family scene. Major moons appear in both; `core` is listed. */
  scene: SceneName;
};

/** Beginner copy uses the same identity and parent relationship as the catalog. */
export function bodyIntroduction(body: ImplementedBody): string {
  if (body.kind === 'Star') return 'The Sun is the central star of this model. Watch the planets travel around it; the core demonstration holds the Sun fixed.';
  if (body.kind === 'Moon') return `${body.name} orbits ${body.parent}. Explore its family to see a moon’s motion alongside its parent.`;
  return `${body.name} orbits the ${body.parent}. Watch its path, change your view, and compare its motion with other worlds.`;
}

type CatalogMoon = (typeof jovianCatalog.moons)[number] & { major?: boolean };
type MoonFamily = { parent: string; scene: SceneName; adjective: string; milestone: string;
  source: string; moons: CatalogMoon[] };

/** The Jovian snapshot predates the `major` flag: its Galilean group is major. */
const isMajor = (moon: CatalogMoon): boolean => moon.major ?? moon.group === 'Galilean moons';

const moonFamilies: MoonFamily[] = [
  { parent: 'Jupiter', scene: 'jupiter-system', adjective: 'Jovian', milestone: 'Complete Jovian moon catalog',
    source: 'data/jovian_moons.json', moons: jovianCatalog.moons },
  { parent: 'Saturn', scene: 'saturn-system', adjective: 'Saturnian', milestone: 'Saturn system scene',
    source: 'data/saturnian_moons.json', moons: saturnianCatalog.moons as CatalogMoon[] },
  { parent: 'Uranus', scene: 'uranus-system', adjective: 'Uranian', milestone: 'Uranus system scene',
    source: 'data/uranian_moons.json', moons: uranianCatalog.moons as CatalogMoon[] },
  { parent: 'Neptune', scene: 'neptune-system', adjective: 'Neptunian', milestone: 'Neptune system scene',
    source: 'data/neptunian_moons.json', moons: neptunianCatalog.moons as CatalogMoon[] },
  { parent: 'Pluto', scene: 'pluto-system', adjective: 'Plutonian', milestone: 'Pluto system scene',
    source: 'data/plutonian_moons.json', moons: plutonianCatalog.moons as CatalogMoon[] },
  { parent: 'Didymos', scene: 'didymos-system', adjective: 'Didymos', milestone: 'Didymos system scene',
    source: 'data/didymos_moons.json', moons: didymosCatalog.moons as unknown as CatalogMoon[] }
];

/** A family's moons in C scene order: major moons first, each group in
 * catalog order (solar_system.c append_moons). */
const orderedMoons = (family: MoonFamily): CatalogMoon[] =>
  [...family.moons.filter(isMajor), ...family.moons.filter(moon => !isMajor(moon))];

function moonBody(family: MoonFamily, moon: CatalogMoon, index: number): ImplementedBody {
  const major = isMajor(moon);
  return {
    slug: moon.slug,
    name: moon.name,
    kind: 'Moon',
    parent: family.parent,
    milestone: major ? 'Main-scene major moon' : family.milestone,
    group: moon.group,
    initialization: `Horizons state relative to ${family.parent} on 2026-06-09; ${moon.frame}-frame JPL mean elements give its orbit data.`,
    source: family.source,
    summary: `${moon.group}. ${major ? `In the main scene and the ${family.parent} system scene.` : `In the ${family.parent} system scene.`} ${moon.inclination_deg > 90 ? 'Retrograde' : 'Prograde'} in the source frame. Mass: ${moon.mass_quality === 'unknown' ? 'unknown — test particle' : moon.mass_quality}. Radius: ${moon.radius_quality === 'unknown' ? 'unknown — marker only' : moon.radius_quality}.`,
    scene: major ? 'core' : family.scene
  };
}

const familyMoons = Object.fromEntries(moonFamilies.map(family =>
  [family.parent, orderedMoons(family).map((moon, index) => moonBody(family, moon, index))])) as Record<string, ImplementedBody[]>;
const majorMoonsOf = (parent: string) => familyMoons[parent].filter(body => body.scene === 'core');

type NamedBody = Omit<ImplementedBody, 'scene'>;
const named = (bodies: NamedBody[]): ImplementedBody[] => bodies.map(body => ({ ...body, scene: 'core' }));
const [sun, mercury, venus, earth, moon, mars, phobos, deimos, vesta, jupiter] = named([
  {
    slug: 'sun',
    name: 'Sun',
    kind: 'Star',
    parent: 'None',
    milestone: 'Foundation',
    initialization: 'Fixed at the origin of the heliocentric scenes.',
    source: 'src/sim/solar_system.c',
    summary: 'Fixed at the origin of the heliocentric scenes.'
  },
  {
    slug: 'mercury',
    name: 'Mercury',
    kind: 'Planet',
    parent: 'Sun',
    milestone: 'Inner planet pass',
    initialization: 'Horizons heliocentric state on 2026-06-09.',
    source: 'src/sim/solar_system.c',
    summary: 'The innermost planet.'
  },
  {
    slug: 'venus',
    name: 'Venus',
    kind: 'Planet',
    parent: 'Sun',
    milestone: 'Inner planet pass',
    initialization: 'Horizons heliocentric state on 2026-06-09.',
    source: 'src/sim/solar_system.c',
    summary: 'The nearly circular inner orbit.'
  },
  {
    slug: 'earth',
    name: 'Earth',
    kind: 'Planet',
    parent: 'Sun',
    milestone: 'Earth pass',
    initialization: 'Earth–Moon barycenter at its Horizons state on 2026-06-09; Earth sits opposite the Moon about it.',
    source: 'src/sim/solar_system.c',
    summary: 'Reference planet for the Earth-Moon relative system.'
  },
  {
    slug: 'moon',
    name: 'Moon',
    kind: 'Moon',
    parent: 'Earth',
    milestone: 'Earth Moon pass',
    initialization: 'Horizons state relative to Earth on 2026-06-09.',
    source: 'src/sim/solar_system.c',
    summary: 'Earth’s Moon, at its Horizons 2026-06-09 state relative to Earth.'
  },
  {
    slug: 'mars',
    name: 'Mars',
    kind: 'Planet',
    parent: 'Sun',
    milestone: 'Mars pass',
    initialization: 'Mars-system barycenter at its Horizons state on 2026-06-09.',
    source: 'src/sim/solar_system.c',
    summary: 'The outermost rocky planet, with two small moons.'
  },
  {
    slug: 'phobos',
    name: 'Phobos',
    kind: 'Moon',
    parent: 'Mars',
    milestone: 'Mars moons pass',
    initialization: 'Horizons state relative to Mars on 2026-06-09.',
    source: 'src/sim/solar_system.c',
    summary: 'Inner Martian moon shown relative to Mars.'
  },
  {
    slug: 'deimos',
    name: 'Deimos',
    kind: 'Moon',
    parent: 'Mars',
    milestone: 'Mars moons pass',
    initialization: 'Horizons state relative to Mars on 2026-06-09.',
    source: 'src/sim/solar_system.c',
    summary: 'Outer Martian moon shown relative to Mars.'
  },
  {
    slug: 'vesta',
    name: 'Vesta',
    kind: 'Asteroid',
    parent: 'Sun',
    milestone: 'Asteroid pass',
    initialization: 'Horizons heliocentric state on 2026-06-09.',
    source: 'src/sim/solar_system.c',
    summary: 'Main-belt asteroid, represented as a single sourced body.'
  },
  {
    slug: 'jupiter',
    name: 'Jupiter',
    kind: 'Planet',
    parent: 'Sun',
    milestone: 'Outer planet pass',
    initialization: 'Jovian-system barycenter at its Horizons state on 2026-06-09; Jupiter sits opposite its known-mass moons.',
    source: 'src/sim/solar_system.c',
    summary: 'The largest planet, with its four Galilean moons in the main scene.'
  },

]);
const [saturn, uranus, neptune] = named([
  {
    slug: 'saturn',
    name: 'Saturn',
    kind: 'Planet',
    parent: 'Sun',
    milestone: 'Saturn pass',
    initialization: 'Saturn-system barycenter at its Horizons state on 2026-06-09.',
    source: 'src/sim/solar_system.c',
    summary: 'Ringed gas giant; the rings are drawn only, not simulated.'
  },
  {slug:'uranus',name:'Uranus',kind:'Planet',parent:'Sun',milestone:'Uranus foundation',
    initialization:'Uranus-system barycenter at its Horizons state on 2026-06-09.',source:'src/sim/solar_system.c',summary:'Ice giant with JPL-sourced mass, mean radius, and orbital elements.'},
  {slug:'neptune',name:'Neptune',kind:'Planet',parent:'Sun',milestone:'Neptune foundation',
    initialization:'Neptune-system barycenter at its Horizons state on 2026-06-09.',source:'src/sim/solar_system.c',summary:'Outer giant included in every selected small-body experiment.'}
]);
const [pluto] = named([
  { slug: 'pluto', name: 'Pluto', kind: 'Dwarf planet', parent: 'Sun', milestone: 'Small-body satellite systems',
    initialization: 'Pluto–Charon barycenter at its Horizons state on 2026-06-09; Pluto circles a point outside its own surface.',
    source: 'src/sim/solar_system.c',
    summary: 'Dwarf planet with JPL Horizons GM; its 17° orbital inclination is not modeled.' }
]);
/** Didymos lives only in its family scene. */
const didymos: ImplementedBody = {
  slug: 'didymos', name: 'Didymos', kind: 'Asteroid', parent: 'Sun', milestone: 'Small-body satellite systems',
  group: 'Didymos system',
  initialization: 'Didymos-system barycenter at its Horizons state on 2026-06-09; masses are Horizons estimates.',
  source: 'src/sim/solar_system.c',
  summary: 'Near-Earth binary asteroid, the DART mission target, in the Didymos system scene.', scene: 'didymos-system'
};

/** The main scene, in the C order of solar_system_create_current(). */
export const mainSceneBodies: ImplementedBody[] = [sun, mercury, venus, earth, moon, mars, phobos, deimos, vesta, jupiter,
  ...majorMoonsOf('Jupiter'), saturn, ...majorMoonsOf('Saturn'), uranus, ...majorMoonsOf('Uranus'), neptune,
  ...majorMoonsOf('Neptune'), pluto, ...majorMoonsOf('Pluto')];

/** A family scene, in the C order of solar_system_create_family(): Pluto and
 * Didymos, which are not planets, follow the eight planets at index 9. */
export const familySceneBodies = (parent: FamilyPrimary): ImplementedBody[] =>
  [sun, mercury, venus, earth, mars, jupiter, saturn, uranus, neptune,
    ...(parent === 'Pluto' ? [pluto] : parent === 'Didymos' ? [didymos] : []), ...familyMoons[parent]];

/** Every simulated body once: the main scene, then each family's small
 * moons (and Didymos, the one primary outside the main scene). */
export const implementedBodies: ImplementedBody[] = [...mainSceneBodies,
  ...moonFamilies.flatMap(family => [...(family.parent === 'Didymos' ? [didymos] : []),
    ...familyMoons[family.parent].filter(body => body.scene !== 'core')])];

// Every planned system now has a scene (SPEC T78). Other asteroid satellites
// have no JPL source this project can pin (SPEC A91).
export const plannedBodies: string[] = [];

export const bodyFocusOrder = mainSceneBodies.map((body) => body.name);
