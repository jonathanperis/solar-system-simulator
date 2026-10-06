export function normalizeBase(rawBase: string): string {
  return rawBase.endsWith('/') ? rawBase : `${rawBase}/`;
}
