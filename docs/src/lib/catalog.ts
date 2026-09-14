import supplements from '../../../data/small_body_physical.json' with { type: 'json' };

export type CatalogRecord = [number,string,string,string,number|null,number|null,number|null,number|null,number|null,number|null,number|null,number|null,number|null,string|null];
export type IndexRow = [number,string,string,number|null,number|null,boolean];
export interface Asset { file: string; bytes: number; sha256: string; contentSha256: string }
export interface Shard { class: string; count: number; minId: number; maxId: number; index: Asset; data: Asset }
export interface Manifest {
  schema: number; count: number; mappable: number; unavailableOrbits: number;
  epoch: number; checked: string; source: string; sourceSha256: string;
  classes: Record<string,number>; classNames: Record<string,string>; shards: Shard[]; overview: Asset;
}
interface Physical { massKg: number; radiusM: number; massQuality: number; radiusQuality: number }
const physical = supplements.bodies as Record<string, Physical>;

export function physicalValues(r: CatalogRecord): Physical {
  return physical[String(r[0])] ?? {massKg: r[11] == null ? 0 : r[11]*1e9/6.67430e-11,
    radiusM: r[12] == null ? 0 : r[12]*500, massQuality: r[11] == null ? 2 : 3, radiusQuality: r[12] == null ? 2 : 3};
}

export function matchesIndex(r: IndexRow, query: string, qmax: number, size: string): boolean {
  const known = r[4] != null || physical[String(r[0])]?.radiusM != null;
  return (!query || `${r[0]} ${r[1]} ${r[2]}`.toLowerCase().includes(query.trim().toLowerCase()))
    && (qmax === Infinity || (r[3] != null && r[3] <= qmax))
    && (!size || (size === 'known' ? known : !known));
}

export function experimentText(records: CatalogRecord[], epoch: number): string {
  if (!records.length || records.length > 16) throw new Error('Select between 1 and 16 bodies.');
  const seen = new Set<number>();
  const lines = records.map(r => {
    if (seen.has(r[0])) throw new Error('Duplicate catalog identity.');
    seen.add(r[0]);
    if (!r.slice(4,11).every(v => typeof v === 'number' && Number.isFinite(v)) || r[5]! <= 0 || r[6]! < 0)
      throw new Error(`Orbit unavailable for ${r[1]}.`);
    const p = physicalValues(r);
    return [r[0],r[1].replace(/[\t\r\n]/g,' '),...r.slice(5,11),p.massKg,p.radiusM,p.massQuality,p.radiusQuality].join('\t');
  });
  return `SOLAR_EXPERIMENT_V1 ${epoch}\n${lines.join('\n')}\n`;
}

export async function fetchPacked<T>(base: string, asset: Asset, snapshot: string, signal?: AbortSignal): Promise<T> {
  const response = await fetch(`${base}${asset.file}?snapshot=${snapshot}`, {signal});
  if (!response.ok) throw new Error(`Catalog download failed (${response.status}). Retry the search.`);
  const received = await response.arrayBuffer(), bytes = new Uint8Array(received);
  const packed = bytes[0] === 0x1f && bytes[1] === 0x8b;
  const hash = Array.from(new Uint8Array(await crypto.subtle.digest('SHA-256',received)),b => b.toString(16).padStart(2,'0')).join('');
  if (hash !== (packed ? asset.sha256 : asset.contentSha256)) throw new Error('Catalog snapshot mismatch. Reload before continuing.');
  if (!packed) return await new Response(received).json() as T;
  const stream = new Blob([received]).stream().pipeThrough(new DecompressionStream('gzip'));
  return await new Response(stream).json() as T;
}

export async function loadRecord(base: string, manifest: Manifest, id: number, signal?: AbortSignal, group?: string): Promise<CatalogRecord> {
  // ID ranges can overlap across classes; inspect every candidate range.
  for (const shard of manifest.shards.filter(s => (!group || s.class===group) && id >= s.minId && id <= s.maxId)) {
    const rows = await fetchPacked<CatalogRecord[]>(base,shard.data,manifest.sourceSha256,signal);
    const record = rows.find(r => r[0] === id);
    if (record) return record;
  }
  throw new Error('Catalog identity not present in this snapshot.');
}
