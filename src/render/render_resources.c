#include "renderer.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../sim/constants.h"
#include "image_decode.h"
#include "render_scale.h"

/* One shader pair serves every body (SPEC A64, C10). `mode` selects the
 * lighting model: 0 lit surface, 1 emissive star, 2 cloud layer, 3 ring,
 * 4 sky backdrop. All vectors are in world space (origin-relative render
 * units), so a direction toward the Sun is the same for every fragment of a
 * body: the Sun is far away compared with any planet's radius.
 *
 * Desktop OpenGL 3.3 uses GLSL 330; the browser's WebGL 1 (OpenGL ES 2) only
 * accepts GLSL 100, which spells inputs/outputs and texture lookups
 * differently. The bodies of the two programs are otherwise identical. */
#if defined(PLATFORM_WEB)
#define SHADER_HEADER_VS "#version 100\n#define IN attribute\n#define OUT varying\n"
#define SHADER_HEADER_FS "#version 100\nprecision mediump float;\n#define IN varying\n#define TEX texture2D\n#define FRAG_COLOR gl_FragColor\n"
#define SHADER_OUTPUT ""
#else
#define SHADER_HEADER_VS "#version 330\n#define IN in\n#define OUT out\n"
#define SHADER_HEADER_FS "#version 330\n#define IN in\n#define TEX texture\n#define FRAG_COLOR finalColor\n"
#define SHADER_OUTPUT "out vec4 finalColor;\n"
#endif

static const char vertex_shader[] = SHADER_HEADER_VS
    "IN vec3 vertexPosition;\n"
    "IN vec2 vertexTexCoord;\n"
    "IN vec3 vertexNormal;\n"
    "uniform mat4 mvp;\n"
    "uniform mat4 matModel;\n"
    "uniform mat4 matNormal;\n"
    "OUT vec2 fragTexCoord;\n"
    "OUT vec3 fragNormal;\n"
    "OUT vec3 fragPosition;\n"
    "void main() {\n"
    "    fragTexCoord = vertexTexCoord;\n"
    "    fragNormal = normalize(vec3(matNormal * vec4(vertexNormal, 0.0)));\n"
    "    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));\n"
    "    gl_Position = mvp * vec4(vertexPosition, 1.0);\n"
    "}\n";

static const char fragment_shader[] = SHADER_HEADER_FS
    "IN vec2 fragTexCoord;\n"
    "IN vec3 fragNormal;\n"
    "IN vec3 fragPosition;\n"
    "uniform sampler2D texture0;\n"   /* surface map (white when absent) */
    "uniform sampler2D texture1;\n"   /* Earth's city lights */
    "uniform vec4 colDiffuse;\n"      /* tint: the body colour when untextured */
    "uniform float mode;\n"
    "uniform vec3 lightDir;\n"        /* unit vector from the body toward the Sun */
    "uniform vec3 viewPos;\n"
    "uniform vec4 atmosphere;\n"      /* rim tint (rgb) and strength (a) */
    "uniform float nightLights;\n"
    SHADER_OUTPUT
    "void main() {\n"
    "    vec4 surface = TEX(texture0, fragTexCoord) * colDiffuse;\n"
    "    vec3 n = normalize(fragNormal);\n"
    "    vec3 v = normalize(viewPos - fragPosition);\n"
    "    if (mode > 3.5) { FRAG_COLOR = vec4(surface.rgb * 1.7, 1.0); return; }\n"
    /* Star: emissive, darker toward the limb where we look through more of
     * the cooler outer photosphere (limb darkening). */
    "    if (mode > 0.5 && mode < 1.5) {\n"
    "        float mu = max(dot(n, v), 0.0);\n"
    "        FRAG_COLOR = vec4(surface.rgb * (0.55 + 0.6 * pow(mu, 0.5)), 1.0);\n"
    "        return;\n"
    "    }\n"
    /* Lambert's law: brightness follows the cosine of the Sun's angle. The
     * smoothstep wraps light slightly past 90 degrees for a soft terminator. */
    "    float ndl = dot(n, lightDir);\n"
    "    float light = smoothstep(-0.12, 1.0, ndl);\n"
    "    if (mode > 2.5) {\n" /* rings: thin, lit from either face */
    "        FRAG_COLOR = vec4(surface.rgb * (0.2 + 0.8 * abs(ndl)), surface.a);\n"
    "        return;\n"
    "    }\n"
    "    if (mode > 1.5) {\n" /* clouds: the map's brightness is its opacity */
    "        FRAG_COLOR = vec4(vec3(light), surface.r * (0.15 + 0.75 * light));\n"
    "        return;\n"
    "    }\n"
    "    /* A small ambient term stands in for starlight and scattered light so\n"
    "     * night sides keep their shape instead of vanishing into the sky. */\n"
    "    vec3 color = surface.rgb * (0.06 + 0.94 * light);\n"
    "    if (nightLights > 0.5) {\n"
    "        float night = 1.0 - smoothstep(-0.18, 0.06, ndl);\n"
    "        color += TEX(texture1, fragTexCoord).rgb * night * 1.4;\n"
    "    }\n"
    /* Atmosphere rim: grazing sight lines cross more air (Fresnel-like
     * falloff), lit mostly on the day side. */
    "    float rim = pow(1.0 - max(dot(n, v), 0.0), 3.0);\n"
    "    color += atmosphere.rgb * atmosphere.a * rim * smoothstep(-0.3, 0.45, ndl);\n"
    "    FRAG_COLOR = vec4(color, 1.0);\n"
    "}\n";

/* Copy generated geometry into raylib-owned buffers (UnloadMesh frees them
 * with RL_FREE) and upload it to the GPU once. */
static Mesh upload_mesh(size_t vertex_count, size_t index_count, const float *positions, const float *normals,
    const float *texcoords, const unsigned short *indices)
{
    Mesh mesh = {0};
    mesh.vertexCount = (int)vertex_count;
    mesh.triangleCount = (int)(index_count / 3);
    mesh.vertices = RL_MALLOC(vertex_count * 3 * sizeof(float));
    mesh.normals = RL_MALLOC(vertex_count * 3 * sizeof(float));
    mesh.texcoords = RL_MALLOC(vertex_count * 2 * sizeof(float));
    mesh.indices = RL_MALLOC(index_count * sizeof(unsigned short));
    if (!mesh.vertices || !mesh.normals || !mesh.texcoords || !mesh.indices) {
        RL_FREE(mesh.vertices); RL_FREE(mesh.normals); RL_FREE(mesh.texcoords); RL_FREE(mesh.indices);
        return (Mesh){0};
    }
    memcpy(mesh.vertices, positions, vertex_count * 3 * sizeof(float));
    memcpy(mesh.normals, normals, vertex_count * 3 * sizeof(float));
    memcpy(mesh.texcoords, texcoords, vertex_count * 2 * sizeof(float));
    memcpy(mesh.indices, indices, index_count * sizeof(unsigned short));
    UploadMesh(&mesh, false);
    return mesh;
}

static Mesh build_sphere(int slices, int rings)
{
    size_t vertices = render_sphere_vertex_count(slices, rings), indices = render_sphere_index_count(slices, rings);
    float *positions = malloc(vertices * 3 * sizeof(float)), *normals = malloc(vertices * 3 * sizeof(float));
    float *texcoords = malloc(vertices * 2 * sizeof(float));
    unsigned short *index = malloc(indices * sizeof(unsigned short));
    Mesh mesh = {0};
    if (positions && normals && texcoords && index && render_build_sphere(slices, rings, positions, normals, texcoords, index))
        mesh = upload_mesh(vertices, indices, positions, normals, texcoords, index);
    free(positions); free(normals); free(texcoords); free(index);
    return mesh;
}

static Mesh build_ring(int segments)
{
    size_t vertices = render_ring_vertex_count(segments), indices = render_ring_index_count(segments);
    float *positions = malloc(vertices * 3 * sizeof(float)), *normals = malloc(vertices * 3 * sizeof(float));
    float *texcoords = malloc(vertices * 2 * sizeof(float));
    unsigned short *index = malloc(indices * sizeof(unsigned short));
    Mesh mesh = {0};
    /* Unit annulus: the renderer scales it by Saturn's drawn radius. */
    float outer = (float)(SOLAR_SATURN_RING_OUTER_RADIUS_M / SOLAR_SATURN_RADIUS_M);
    if (positions && normals && texcoords && index &&
        render_build_ring(segments, (float)SOLAR_SATURN_RING_VISUAL_INNER_RATIO, outer, positions, normals, texcoords, index))
        mesh = upload_mesh(vertices, indices, positions, normals, texcoords, index);
    free(positions); free(normals); free(texcoords); free(index);
    return mesh;
}

/* A soft radial halo for the Sun, drawn with additive blending: each pixel's
 * colour is added to what is already on screen, so the glow brightens the
 * scene behind it without ever darkening it. */
static Texture2D build_glow(void)
{
    const int size = 128;
    Image image = GenImageColor(size, size, BLANK);
    Color *pixels = image.data;
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            double dx = (x + 0.5) / size * 2 - 1, dy = (y + 0.5) / size * 2 - 1;
            float i = render_glow_intensity(sqrt(dx * dx + dy * dy));
            pixels[y * size + x] = (Color){(unsigned char)(255 * i), (unsigned char)(196 * i), (unsigned char)(120 * i), 255};
        }
    }
    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
    return texture;
}

bool renderer_resources_init(RenderResources *resources)
{
    *resources = (RenderResources){0};
    resources->shader = LoadShaderFromMemory(vertex_shader, fragment_shader);
    if (!IsShaderValid(resources->shader)) return false;
    resources->loc_mode = GetShaderLocation(resources->shader, "mode");
    resources->loc_light_dir = GetShaderLocation(resources->shader, "lightDir");
    resources->loc_view_pos = GetShaderLocation(resources->shader, "viewPos");
    resources->loc_atmosphere = GetShaderLocation(resources->shader, "atmosphere");
    resources->loc_night_lights = GetShaderLocation(resources->shader, "nightLights");
    /* Planets get a smooth 96 x 48 sphere; dozens of small moons share a
     * cheaper 32 x 16 one so the 128-body scene stays light on phones. */
    resources->sphere_detailed = build_sphere(96, 48);
    resources->sphere_simple = build_sphere(32, 16);
    resources->ring = build_ring(192);
    Image white = GenImageColor(2, 2, WHITE);
    resources->white = LoadTextureFromImage(white);
    UnloadImage(white);
    resources->glow = build_glow();
    resources->material = LoadMaterialDefault();
    resources->material.shader = resources->shader;
    resources->ready = resources->sphere_detailed.vertexCount > 0 && resources->sphere_simple.vertexCount > 0 &&
        resources->ring.vertexCount > 0;
    return resources->ready;
}

void renderer_resources_unload(RenderResources *resources)
{
    for (int i = 0; i < RENDER_TEXTURE_COUNT; ++i) if (resources->texture_loaded[i]) UnloadTexture(resources->textures[i]);
    if (resources->sphere_detailed.vertexCount) UnloadMesh(resources->sphere_detailed);
    if (resources->sphere_simple.vertexCount) UnloadMesh(resources->sphere_simple);
    if (resources->ring.vertexCount) UnloadMesh(resources->ring);
    if (resources->white.id) UnloadTexture(resources->white);
    if (resources->glow.id) UnloadTexture(resources->glow);
    /* UnloadMaterial would also unload the shared shader and default texture;
     * release only the map array this material allocated. */
    RL_FREE(resources->material.maps);
    if (IsShaderValid(resources->shader)) UnloadShader(resources->shader);
    *resources = (RenderResources){0};
}

bool renderer_load_texture_memory(RenderResources *resources, RenderTextureSlot slot,
    const unsigned char *bytes, size_t length)
{
    if (slot < 0 || slot >= RENDER_TEXTURE_COUNT) return false;
    DecodedImage decoded;
    if (!render_decode_image(bytes, length, &decoded)) return false;
    Image image = {decoded.pixels, decoded.width, decoded.height, 1,
        decoded.channels == 4 ? PIXELFORMAT_UNCOMPRESSED_R8G8B8A8 : PIXELFORMAT_UNCOMPRESSED_R8G8B8};
    Texture2D texture = LoadTextureFromImage(image);
    render_free_image(&decoded);
    if (texture.id == 0) return false;
    /* Mipmaps keep distant planets from shimmering; every map is a power of
     * two so WebGL 1 can build them. Maps wrap in longitude; the ring strip
     * clamps so its inner and outer edges never bleed into each other. */
    GenTextureMipmaps(&texture);
    SetTextureFilter(texture, TEXTURE_FILTER_TRILINEAR);
    SetTextureWrap(texture, slot == RENDER_TEXTURE_SATURN_RING ? TEXTURE_WRAP_CLAMP : TEXTURE_WRAP_REPEAT);
    if (resources->texture_loaded[slot]) UnloadTexture(resources->textures[slot]);
    resources->textures[slot] = texture;
    resources->texture_loaded[slot] = true;
    return true;
}

int renderer_load_textures_from_directory(RenderResources *resources, const char *directory)
{
    int loaded = 0;
    for (int slot = 0; slot < RENDER_TEXTURE_COUNT; ++slot) {
        char path[1024];
        int written = snprintf(path, sizeof(path), "%s/%s", directory, render_texture_file((RenderTextureSlot)slot));
        if (written < 0 || (size_t)written >= sizeof(path) || !FileExists(path)) continue;
        int size = 0;
        unsigned char *bytes = LoadFileData(path, &size);
        if (bytes && size > 0 && renderer_load_texture_memory(resources, (RenderTextureSlot)slot, bytes, (size_t)size)) ++loaded;
        UnloadFileData(bytes);
    }
    return loaded;
}

int renderer_loaded_texture_count(const RenderResources *resources)
{
    int count = 0;
    for (int i = 0; i < RENDER_TEXTURE_COUNT; ++i) count += resources->texture_loaded[i];
    return count;
}
