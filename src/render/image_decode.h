#ifndef SOLAR_IMAGE_DECODE_H
#define SOLAR_IMAGE_DECODE_H

/* Decodes the renderer's bundled JPEG/PNG textures from memory (SPEC A65).
 * raylib ships with JPEG support compiled out, so the native and browser
 * builds both use the vendored stb_image decoder through this small,
 * raylib-free wrapper. Only the project's own pinned assets are decoded. */

#include <stdbool.h>
#include <stddef.h>

/* Largest accepted edge in pixels; the bundled maps are at most 4096 wide. */
#define RENDER_IMAGE_MAX_EDGE 8192

typedef struct DecodedImage {
    unsigned char *pixels; /* row-major, top row first, `channels` bytes per pixel */
    int width, height, channels; /* channels: 3 = RGB, 4 = RGBA */
} DecodedImage;

/* Returns false (and leaves *out empty) for unsupported, truncated or
 * oversized input. A successful result must be released with
 * render_free_image. */
bool render_decode_image(const unsigned char *bytes, size_t length, DecodedImage *out);
void render_free_image(DecodedImage *image);

#endif
