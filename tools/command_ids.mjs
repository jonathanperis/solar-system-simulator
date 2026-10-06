// Web buttons call the C command boundary with plain integers, so the C
// `SolarCommand` enum (src/main.c) and TypeScript `runtimeCommands`
// (docs/src/lib/simulator.ts) must agree exactly. Both are parsed from source
// text: importing simulator.ts would need a DOM.
import assert from 'node:assert/strict';

/** [NAME, value] pairs from `typedef enum SolarCommand { ... } SolarCommand;`. */
export function parseCCommands(source) {
  const body = source.match(/typedef enum SolarCommand\s*\{([^}]*)\}\s*SolarCommand;/);
  assert.ok(body, 'src/main.c must define typedef enum SolarCommand { ... } SolarCommand;');
  let nextValue = 0; // C enumerators count up from the previous explicit value.
  return body[1].replace(/\/\*[\s\S]*?\*\/|\/\/.*$/gm, '').split(',').map(entry => entry.trim()).filter(Boolean)
    .map(entry => {
      const [, name, value] = entry.match(/^SOLAR_COMMAND_([A-Z0-9_]+)(?:\s*=\s*(\d+))?$/) ?? [];
      assert.ok(name, `Unrecognized SolarCommand entry: ${entry}`);
      if (value !== undefined) nextValue = Number(value);
      return [name, nextValue++];
    });
}

/** [NAME, value] pairs from `export const runtimeCommands = { ... } as const`, camelCase → SNAKE. */
export function parseTsCommands(source) {
  const body = source.match(/export const runtimeCommands\s*=\s*\{([^}]*)\}\s*as const/);
  assert.ok(body, 'docs/src/lib/simulator.ts must export runtimeCommands = { ... } as const');
  return [...body[1].matchAll(/([A-Za-z_$][\w$]*)\s*:\s*(\d+)/g)]
    .map(([, name, value]) => [name.replace(/([a-z0-9])([A-Z])/g, '$1_$2').toUpperCase(), Number(value)]);
}

/** Throws unless both sides list the same names with the same integer values. */
export function assertCommandsAligned(cSource, tsSource) {
  const byValue = ([, a], [, b]) => a - b;
  const c = parseCCommands(cSource).sort(byValue);
  assert.deepEqual(parseTsCommands(tsSource).sort(byValue), c,
    'C SolarCommand values and TypeScript runtimeCommands must match name-for-name');
  return c.length;
}
