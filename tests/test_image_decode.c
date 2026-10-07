#include "require_assert.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "render/image_decode.h"

static unsigned char *read_file(const char *path, size_t *length)
{
    FILE *file = fopen(path, "rb");
    assert(file);
    unsigned char *data = malloc(1 << 16);
    assert(data);
    *length = fread(data, 1, 1 << 16, file);
    fclose(file);
    return data;
}

static void test_decodes_rgb_jpeg_and_rgba_png(void)
{
    size_t length;
    unsigned char *jpeg = read_file("tests/fixtures/texture_rgb.jpg", &length);
    DecodedImage image;
    assert(render_decode_image(jpeg, length, &image));
    assert(image.width == 8 && image.height == 4 && image.channels == 3);
    /* JPEG is lossy: the solid (200, 40, 20) fixture decodes within a few levels. */
    assert(abs(image.pixels[0] - 200) < 8 && abs(image.pixels[1] - 40) < 8 && abs(image.pixels[2] - 20) < 8);
    render_free_image(&image);
    assert(image.pixels == NULL);

    unsigned char *png = read_file("tests/fixtures/texture_rgba.png", &length);
    assert(render_decode_image(png, length, &image));
    assert(image.width == 4 && image.height == 2 && image.channels == 4);
    assert(image.pixels[0] == 10 && image.pixels[1] == 200 && image.pixels[2] == 30 && image.pixels[3] == 128);
    render_free_image(&image);

    /* Truncated input fails cleanly instead of reading past the buffer. */
    assert(!render_decode_image(jpeg, length / 3 > 40 ? 40 : length / 3, &image));
    assert(image.pixels == NULL);
    free(jpeg);
    free(png);
}

static void test_rejects_garbage_and_empty_input(void)
{
    DecodedImage image;
    const unsigned char garbage[] = "definitely not an image";
    assert(!render_decode_image(garbage, sizeof(garbage), &image));
    assert(!render_decode_image(NULL, 10, &image));
    assert(!render_decode_image(garbage, 0, &image));
    render_free_image(&image); /* harmless on an empty result */
}

int main(void)
{
    test_decodes_rgb_jpeg_and_rgba_png();
    test_rejects_garbage_and_empty_input();
    puts("test_image_decode passed");
    return 0;
}
