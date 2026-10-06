import { ColumnarIndex, columnarRowLimit, fetchPacked, hitFromRow, indexFilter, loadRecord, numericIdentityCandidates, routeShards,
  type CatalogHit, type CatalogRecord, type IndexRow, type Manifest, type SearchFilter, type Shard } from './catalog.ts';

/*
 * Catalog search and record lookup run here, off the main thread.
 *
 * Search strategies, cheapest first:
 * 1. A built ColumnarIndex answers every query from memory.
 * 2. Digits-only queries (asteroid number / SPK-ID) fetch only the index
 *    shards whose manifest ID range contains the identity.
 * 3. Unfiltered browsing fetches only the shard that holds the requested page.
 * 4. Anything else needs the full index (~33 MB compressed). The worker
 *    replies `needsFullScan` until the page confirms the user accepted the
 *    download; the scan then builds the ColumnarIndex once for the session.
 *
 * A newer search supersedes older ones (their results are never published),
 * but it does not cancel an accepted full-index download: only an explicit
 * `cancel` message does, so changing a filter mid-download wastes nothing.
 */
const pageSize = 50;
const prefetch = 4;        // ordered parallel downloads during the full scan
const shardCacheSize = 3;  // small LRU for routed index shards and record data shards

type SearchMessage = SearchFilter & { type?: 'search'; base: string; manifest: Manifest; page: number; request: number; allowFullScan?: boolean };
type RecordMessage = { type: 'record'; base: string; manifest: Manifest; id: number; group: string; request: number };
type CancelMessage = { type: 'cancel' };

/** Least-recently-used promise cache keyed by snapshot and file name. */
class ShardCache<T> {
  private readonly entries = new Map<string, Promise<T>>();
  private readonly limit: number;
  constructor(limit: number) { this.limit = limit; }
  get(key: string, load: () => Promise<T>): Promise<T> {
    let entry = this.entries.get(key);
    if (entry) this.entries.delete(key);
    else {
      const loading = load();
      loading.catch(() => { if (this.entries.get(key) === loading) this.entries.delete(key); });
      entry = loading;
    }
    this.entries.set(key, entry);
    while (this.entries.size > this.limit) this.entries.delete(this.entries.keys().next().value!);
    return entry;
  }
}

const indexShards = new ShardCache<IndexRow[]>(shardCacheSize);
const dataShards = new ShardCache<CatalogRecord[]>(shardCacheSize);
let columnar: { snapshot: string; index: ColumnarIndex } | undefined;
let build: { snapshot: string; controller: AbortController; done: Promise<ColumnarIndex> } | undefined;
let searchGeneration = 0, recordGeneration = 0;
let waiting: { token: number; request: number } | undefined;  // search that receives scan progress

const post = (message: unknown): void => { self.postMessage(message); };
const failure = (error: unknown): string => error instanceof Error ? error.message : String(error);
const aborted = (error: unknown): boolean => error instanceof DOMException && error.name === 'AbortError';

const fetchIndex = (base: string, manifest: Manifest, shard: Shard): Promise<IndexRow[]> =>
  indexShards.get(`${manifest.sourceSha256}/${shard.index.file}`, () => fetchPacked<IndexRow[]>(base, shard.index, manifest.sourceSha256));

function startBuild(base: string, manifest: Manifest): Promise<ColumnarIndex> {
  if (build?.snapshot === manifest.sourceSha256) return build.done;
  build?.controller.abort();
  const controller = new AbortController();
  const totalBytes = manifest.shards.reduce((n, s) => n + s.index.bytes, 0);
  const megabytes = (bytes: number) => (bytes / 1e6).toFixed(1);
  const request = (i: number): Promise<IndexRow[]> => {
    const loading = fetchPacked<IndexRow[]>(base, manifest.shards[i].index, manifest.sourceSha256, controller.signal);
    loading.catch(() => undefined);  // awaited in order below; avoid unhandled rejections after a cancel
    return loading;
  };
  const done = (async () => {
    const declared = manifest.shards.reduce((n, s) => n + s.count, 0);
    if (declared > columnarRowLimit) throw new RangeError('Catalog index exceeds the in-memory cache bound.');
    const index = new ColumnarIndex(declared);
    const pending = manifest.shards.slice(0, prefetch).map((_, i) => request(i));
    let received = 0;
    for (let i = 0; i < manifest.shards.length; ++i) {
      controller.signal.throwIfAborted();
      const rows = await pending[i];
      if (i + prefetch < manifest.shards.length) pending.push(request(i + prefetch));
      pending[i] = Promise.resolve([]);  // release parsed rows promptly
      if (rows.length !== manifest.shards[i].count) throw new Error('Catalog shard does not match its manifest. Reload before continuing.');
      index.append(rows, manifest.shards[i].class);
      received += manifest.shards[i].index.bytes;
      if (waiting?.token === searchGeneration) post({ type: 'search', request: waiting.request, done: false,
        progress: `Downloading the catalog index: ${i + 1} of ${manifest.shards.length} files (${megabytes(received)} of ${megabytes(totalBytes)} MB)` });
    }
    index.finish();
    columnar = { snapshot: manifest.sourceSha256, index };
    return index;
  })();
  done.catch(() => undefined);
  void done.finally(() => { if (build?.done === done) build = undefined; }).catch(() => undefined);
  build = { snapshot: manifest.sourceSha256, controller, done };
  return done;
}

async function search(message: SearchMessage): Promise<void> {
  const token = ++searchGeneration;
  const { base, manifest, page, request } = message;
  const filter: SearchFilter = { query: message.query, qmax: message.qmax, size: message.size, group: message.group };
  const current = () => token === searchGeneration;
  const publish = (hits: CatalogHit[], total: number, source: string) => { if (current()) post({ type: 'search', request, hits, total, source, done: true }); };
  try {
    const ids = numericIdentityCandidates(filter.query);
    if (columnar?.snapshot === manifest.sourceSha256) {
      const result = columnar.index.search(filter, page, pageSize, ids);
      return publish(result.hits, result.total, 'memory');
    }
    if (ids) {
      const wanted = new Set(ids), accepts = indexFilter('', filter.qmax, filter.size), hits: CatalogHit[] = [];
      let total = 0;
      for (const shard of routeShards(manifest, ids, filter.group)) {
        const rows = await fetchIndex(base, manifest, shard);
        if (!current()) return;
        for (const row of rows) {
          if (!wanted.has(row[0]) || !accepts(row)) continue;
          if (total >= page * pageSize && hits.length < pageSize) hits.push(hitFromRow(row, shard.class));
          ++total;
        }
      }
      return publish(hits, total, 'identity');
    }
    const shards = manifest.shards.filter(s => !filter.group || s.class === filter.group);
    if (!filter.query.trim() && filter.qmax === Infinity && !filter.size) {
      // Unfiltered browsing: manifest counts locate the page without a scan.
      const hits: CatalogHit[] = [];
      let offset = page * pageSize;
      for (const shard of shards) {
        if (hits.length === pageSize) break;
        if (offset >= shard.count) { offset -= shard.count; continue; }
        const rows = await fetchIndex(base, manifest, shard);
        if (!current()) return;
        for (const row of rows.slice(offset, offset + pageSize - hits.length)) hits.push(hitFromRow(row, shard.class));
        offset = 0;
      }
      return publish(hits, shards.reduce((n, s) => n + s.count, 0), 'browse');
    }
    if (!message.allowFullScan && build?.snapshot !== manifest.sourceSha256) {
      const bytes = manifest.shards.reduce((n, s) => n + s.index.bytes, 0);
      if (current()) post({ type: 'search', request, needsFullScan: { bytes, files: manifest.shards.length }, done: true });
      return;
    }
    waiting = { token, request };
    const index = await startBuild(base, manifest);
    if (!current()) return;
    const result = index.search(filter, page, pageSize);
    publish(result.hits, result.total, 'memory');
  } catch (error) {
    if (!current()) return;
    post({ type: 'search', request, done: true, ...(aborted(error) ? { cancelled: true } : { error: failure(error) }) });
  }
}

async function lookup(message: RecordMessage): Promise<void> {
  const token = ++recordGeneration;
  const { base, manifest, id, group, request } = message;
  try {
    const record = await loadRecord(manifest, id, shard => dataShards.get(`${manifest.sourceSha256}/${shard.data.file}`,
      () => fetchPacked<CatalogRecord[]>(base, shard.data, manifest.sourceSha256)), group || undefined);
    if (token === recordGeneration) post({ type: 'record', request, record });
  } catch (error) {
    if (token === recordGeneration) post({ type: 'record', request, error: failure(error) });
  }
}

self.onmessage = event => {
  const message = event.data as SearchMessage | RecordMessage | CancelMessage;
  if (message.type === 'cancel') {
    // Explicit user cancel of the full-index download; a search waiting on it
    // reports `cancelled`. A later confirmed search can start it again.
    build?.controller.abort();
    build = undefined;
  } else if (message.type === 'record') void lookup(message);
  else void search(message);
};
