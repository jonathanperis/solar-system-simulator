import supplements from '../../../data/small_body_physical.json' with { type: 'json' };

export type CatalogRecord = [number,string,string,string,number|null,number|null,number|null,number|null,number|null,number|null,number|null,number|null,number|null,string|null];
/** [SPK-ID, display name (includes the designation), designation, perihelion AU, diameter km, orbit available]. */
export type IndexRow = [number,string,string,number|null,number|null,boolean];
export interface Asset { file: string; bytes: number; sha256: string; contentSha256: string }
export interface Shard { class: string; count: number; minId: number; maxId: number; index: Asset; data: Asset }
export interface Manifest {
  schema: number; count: number; mappable: number; unavailableOrbits: number;
  epoch: number; checked: string; source: string; sourceSha256: string;
  classes: Record<string,number>; classNames: Record<string,string>; shards: Shard[]; overview: Asset;
}
/** One displayed search result; the UI needs no other index columns. */
export interface CatalogHit { id: number; name: string; perihelion: number | null; orbit: boolean; group: string }
export interface SearchFilter { query: string; qmax: number; size: string; group: string }
interface Physical { massKg: number; radiusM: number; massQuality: number; radiusQuality: number }
const physical = supplements.bodies as Record<string, Physical>;

export const integrityUnavailableMessage = 'This browser cannot verify catalog downloads here: SHA-256 checks need Web Crypto, which browsers only provide on HTTPS or localhost. Open the atlas over HTTPS or http://localhost to search.';

export function physicalValues(r: CatalogRecord): Physical {
  return physical[String(r[0])] ?? {massKg: r[11] == null ? 0 : r[11]*1e9/6.67430e-11,
    radiusM: r[12] == null ? 0 : r[12]*500, massQuality: r[11] == null ? 2 : 3, radiusQuality: r[12] == null ? 2 : 3};
}

const sizeKnown = (r: IndexRow): boolean => r[4] != null || physical[String(r[0])]?.radiusM != null;

/** Build a row predicate once per search: query normalization stays out of
 * the per-row loop. Text matches the display name, which already contains the
 * designation; numeric identities are handled by numericIdentityCandidates. */
export function indexFilter(query: string, qmax: number, size: string): (r: IndexRow) => boolean {
  const needle = query.trim().toLowerCase();
  return r => (!needle || r[1].toLowerCase().includes(needle) || (!r[1].includes(r[2]) && r[2].toLowerCase().includes(needle)))
    && (qmax === Infinity || (r[3] != null && r[3] <= qmax))
    && (!size || (size === 'known' ? sizeKnown(r) : !sizeKnown(r)));
}

export function matchesIndex(r: IndexRow, query: string, qmax: number, size: string): boolean {
  return indexFilter(query, qmax, size)(r);
}

// JPL SPK-IDs for numbered asteroids are 20,000,000 + the asteroid number;
// provisional-designation objects use other ranges (for example 5xxxxxxx).
const numberedSpkBase = 20_000_000;

/** Digits-only queries are identity lookups: an asteroid number or an SPK-ID.
 * They route to the shards whose [minId, maxId] contains a candidate instead
 * of scanning the full index. Returns undefined for text queries. */
export function numericIdentityCandidates(query: string): number[] | undefined {
  const text = query.trim();
  if (!/^\d{1,9}$/.test(text)) return undefined;
  const value = Number(text);
  const candidates = new Set<number>();
  if (value >= numberedSpkBase) candidates.add(value);
  if (value > 0 && value < numberedSpkBase) candidates.add(numberedSpkBase + value);
  return [...candidates];
}

/** Index shards that can contain any candidate identity. Ranges of different
 * classes overlap, so this can return one shard per class, but never the full set. */
export function routeShards(manifest: Manifest, ids: number[], group: string): Shard[] {
  return manifest.shards.filter(s => (!group || s.class === group) && ids.some(id => id >= s.minId && id <= s.maxId));
}

export function hitFromRow(r: IndexRow, group: string): CatalogHit {
  return { id: r[0], name: r[1], perihelion: r[3], orbit: r[5], group };
}

const flagSizeKnown = 1, flagOrbit = 2;
const upperA = 65, upperZ = 90;

/**
 * Columnar, read-only copy of the whole catalog index, built once after the
 * first full scan so later searches and filter changes run in memory without
 * refetching, rehashing or reparsing ~33 MB of compressed shards (~120 MB JSON).
 *
 * Memory trade-off for the 1.56 M-row snapshot (~57 MB in total):
 * - names: one lowercase string joined by '\n' (~25 MB as a one-byte string)
 *   searched with native indexOf; offsets in a Uint32Array (6 MB);
 * - display case: one bit per name character for ASCII uppercase (3 MB);
 *   names whose case cannot round-trip that way are kept verbatim in a Map;
 * - ids Uint32Array (6 MB), perihelion Float64Array with NaN for unknown
 *   (12.5 MB, exact comparisons), flags and class indexes Uint8Array (3 MB).
 * Per-row JS arrays for the same data would need several hundred MB. Builds
 * above columnarRowLimit are refused and the worker falls back to streaming.
 */
export const columnarRowLimit = 2_500_000;

export class ColumnarIndex {
  readonly ids: Uint32Array;
  readonly perihelion: Float64Array;
  readonly flags: Uint8Array;
  readonly groups: Uint8Array;
  readonly classes: string[] = [];
  private readonly starts: Uint32Array;
  private upper: Uint8Array;
  private chunks: string[] = [];
  private names = '';
  private readonly exactNames = new Map<number,string>();
  private readonly extraSearch = new Map<number,string>();
  private length = 0;
  private characters = 0;

  readonly capacity: number;

  constructor(capacity: number) {
    this.capacity = capacity;
    if (!Number.isInteger(capacity) || capacity < 0 || capacity > columnarRowLimit) throw new RangeError('Catalog index exceeds the in-memory cache bound.');
    this.ids = new Uint32Array(capacity);
    this.perihelion = new Float64Array(capacity);
    this.flags = new Uint8Array(capacity);
    this.groups = new Uint8Array(capacity);
    this.starts = new Uint32Array(capacity + 1);
    this.upper = new Uint8Array(Math.ceil(capacity * 24 / 8));
  }

  get count(): number { return this.length; }

  append(rows: IndexRow[], group: string): void {
    if (this.length + rows.length > this.capacity) throw new RangeError('Catalog index has more rows than its manifest declares.');
    let groupIndex = this.classes.indexOf(group);
    if (groupIndex < 0) { groupIndex = this.classes.push(group) - 1; if (groupIndex > 255) throw new RangeError('Too many orbital classes.'); }
    const lowered: string[] = [];
    for (const r of rows) {
      const i = this.length++;
      const lower = r[1].toLowerCase();
      this.ids[i] = r[0];
      this.perihelion[i] = r[3] ?? Number.NaN;
      this.flags[i] = (sizeKnown(r) ? flagSizeKnown : 0) | (r[5] ? flagOrbit : 0);
      this.groups[i] = groupIndex;
      this.starts[i] = this.characters;
      if (lower.length === r[1].length && /^[\x00-\x7f]*$/.test(r[1])) {
        this.ensureCaseBits(this.characters + lower.length);
        for (let c = 0; c < r[1].length; ++c) {
          const code = r[1].charCodeAt(c);
          if (code >= upperA && code <= upperZ) this.upper[(this.characters + c) >> 3] |= 1 << ((this.characters + c) & 7);
        }
      } else this.exactNames.set(i, r[1]);
      if (!r[1].includes(r[2])) this.extraSearch.set(i, r[2].toLowerCase());
      lowered.push(lower);
      this.characters += lower.length + 1;
    }
    this.starts[this.length] = this.characters;
    // Join per shard, then once at finish(): avoids holding 1.5 M small strings.
    this.chunks.push(lowered.join('\n') + '\n');
  }

  finish(): void {
    this.names = this.chunks.join('');
    this.chunks = [];
  }

  displayName(i: number): string {
    const exact = this.exactNames.get(i);
    if (exact !== undefined) return exact;
    const start = this.starts[i], end = this.starts[i + 1] - 1;
    let name = '';
    for (let c = start; c < end; ++c) {
      const char = this.names[c];
      name += this.upper[c >> 3] & (1 << (c & 7)) ? char.toUpperCase() : char;
    }
    return name;
  }

  hit(i: number): CatalogHit {
    const q = this.perihelion[i];
    return { id: this.ids[i], name: this.displayName(i), perihelion: Number.isNaN(q) ? null : q,
      orbit: (this.flags[i] & flagOrbit) !== 0, group: this.classes[this.groups[i]] };
  }

  /** Paginated search in catalog order. `ids` restricts to exact identities. */
  search(filter: SearchFilter, page: number, pageSize: number, ids?: number[]): { hits: CatalogHit[]; total: number } {
    const groupIndex = filter.group ? this.classes.indexOf(filter.group) : -1;
    if (filter.group && groupIndex < 0) return { hits: [], total: 0 };
    const { qmax, size } = filter;
    const first = page * pageSize, hits: CatalogHit[] = [];
    let total = 0;
    const accept = (i: number): void => {
      if (groupIndex >= 0 && this.groups[i] !== groupIndex) return;
      if (qmax !== Infinity && !(this.perihelion[i] <= qmax)) return;
      if (size && ((this.flags[i] & flagSizeKnown) !== 0) !== (size === 'known')) return;
      if (total >= first && hits.length < pageSize) hits.push(this.hit(i));
      ++total;
    };
    if (ids) {
      const wanted = new Set(ids);
      for (let i = 0; i < this.length; ++i) if (wanted.has(this.ids[i])) accept(i);
      return { hits, total };
    }
    const needle = filter.query.trim().toLowerCase().replace(/\n/g, ' ');
    if (!needle) {
      for (let i = 0; i < this.length; ++i) accept(i);
      return { hits, total };
    }
    // Rows matched through a designation that is not part of their name (none
    // in the current snapshot) are merged in catalog order.
    const extra = [...this.extraSearch].filter(([, text]) => text.includes(needle)).map(([i]) => i).sort((a, b) => a - b);
    let row = 0, next = 0;
    for (let at = this.names.indexOf(needle); at >= 0; at = this.names.indexOf(needle, this.starts[row + 1])) {
      while (this.starts[row + 1] <= at) ++row;
      while (next < extra.length && extra[next] < row) accept(extra[next++]);
      if (extra[next] === row) ++next;
      accept(row);
      if (row + 1 >= this.length) break;
    }
    while (next < extra.length) accept(extra[next++]);
    return { hits, total };
  }

  private ensureCaseBits(characters: number): void {
    if (characters <= this.upper.length * 8) return;
    const grown = new Uint8Array(Math.max(this.upper.length * 2, Math.ceil(characters / 8)));
    grown.set(this.upper);
    this.upper = grown;
  }
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
  // Integrity checks are mandatory; never fall back to unverified data.
  if (!globalThis.crypto?.subtle) throw new Error(integrityUnavailableMessage);
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

/** Find one record in the data shards for its class and ID range. `load` lets
 * the worker serve shards from its LRU cache. */
export async function loadRecord(manifest: Manifest, id: number, load: (shard: Shard) => Promise<CatalogRecord[]>, group?: string): Promise<CatalogRecord> {
  // ID ranges can overlap across classes; inspect every candidate range.
  for (const shard of manifest.shards.filter(s => (!group || s.class===group) && id >= s.minId && id <= s.maxId)) {
    const record = (await load(shard)).find(r => r[0] === id);
    if (record) return record;
  }
  throw new Error('Catalog identity not present in this snapshot.');
}
