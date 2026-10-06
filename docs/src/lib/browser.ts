/** Small browser helpers shared by the simulator, comparison lab and catalog. */

/** Human-readable message for a status line: `Error: x` becomes `x`. */
export function errorMessage(error: unknown): string {
  return error instanceof Error ? error.message : String(error);
}

interface DownloadEnvironment {
  createObjectURL(blob: Blob): string;
  revokeObjectURL(url: string): void;
  createAnchor(): { href: string; download: string; click(): void };
  schedule(callback: () => void, delayMs: number): void;
}

const browserEnvironment = (): DownloadEnvironment => ({
  createObjectURL: blob => URL.createObjectURL(blob),
  revokeObjectURL: url => URL.revokeObjectURL(url),
  createAnchor: () => document.createElement('a'),
  schedule: (callback, delayMs) => { setTimeout(callback, delayMs); }
});

// Some browsers start reading an anchor download asynchronously after click();
// revoking the object URL immediately can cancel it. The texts are small, so
// keeping the blob alive briefly costs little.
const revokeDelayMs = 10_000;

/** Save generated text through a temporary object URL. */
export function downloadText(text: string, filename: string, type: string, environment: DownloadEnvironment = browserEnvironment()): void {
  const url = environment.createObjectURL(new Blob([text], { type }));
  const anchor = environment.createAnchor();
  anchor.href = url;
  anchor.download = filename;
  anchor.click();
  environment.schedule(() => environment.revokeObjectURL(url), revokeDelayMs);
}
