export function normalizeBase(rawBase: string): string {
  return rawBase.endsWith('/') ? rawBase : `${rawBase}/`;
}

export function hrefWithBase(base: string, href: string): string {
  if (href.startsWith('http') || href.startsWith('#')) return href;
  return `${base}${href}`;
}
