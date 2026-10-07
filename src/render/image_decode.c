#include "image_decode.h"

#include <limits.h>
#include <stdlib.h>

/* Vendored stb_image (see src/render/third_party/README.md). STB_IMAGE_STATIC
 * gives every decoder function internal linkage, so this private copy cannot
 * collide with the stb_image that raylib compiles into itself. Only the two
 * formats the bundled textures use are compiled, and only from memory. */
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_JPEG
#define STBI_ONLY_PNG
#define STBI_NO_STDIO
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#define STBI_NO_FAILURE_STRINGS
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wsign-compare"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#pragma GCC diagnostic ignored "-Wsign-compare"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#pragma GCC diagnostic ignored "-Wimplicit-fallthrough"
#pragma GCC diagnostic ignored "-Wtype-limits"
#pragma GCC diagnostic ignored "-Wmisleading-indentation"
#endif
#include "third_party/stb_image.h"
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

bool render_decode_image(const unsigned char *bytes, size_t length, DecodedImage *out)
{
    *out = (DecodedImage){0};
    /* stb_image takes an int length; anything larger is not one of our maps. */
    if (!bytes || length == 0 || length > INT_MAX) return false;
    int width, height, channels;
    /* Read the header first so an oversized image is refused before stb
     * allocates width * height * channels bytes for it. */
    if (!stbi_info_from_memory(bytes, (int)length, &width, &height, &channels)) return false;
    if (width <= 0 || height <= 0 || width > RENDER_IMAGE_MAX_EDGE || height > RENDER_IMAGE_MAX_EDGE) return false;
    /* Keep alpha only where the file has it (ring opacity); JPEGs stay RGB. */
    int wanted = channels == 2 || channels == 4 ? 4 : 3;
    unsigned char *pixels = stbi_load_from_memory(bytes, (int)length, &width, &height, &channels, wanted);
    if (!pixels) return false;
    *out = (DecodedImage){pixels, width, height, wanted};
    return true;
}

void render_free_image(DecodedImage *image)
{
    if (image->pixels) stbi_image_free(image->pixels);
    *image = (DecodedImage){0};
}
