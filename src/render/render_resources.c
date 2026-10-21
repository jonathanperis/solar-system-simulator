#include "renderer.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <rlgl.h>
#include <string.h>

#include "../sim/constants.h"
#include "image_decode.h"
#include "render_scale.h"
/* JetBrains Mono Medium, ASCII subset (assets/fonts/, SIL OFL 1.1), embedded
 * by the build so labels never wait for a download. */
#include "label_font.inc"

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
/* WebGL 1 fragment shaders default to no float precision; use highp where the
 * GPU offers it (shadow and glint maths in small real-scale units needs it)
 * and fall back to mediump elsewhere. */
#define SHADER_HEADER_FS "#version 100\n#ifdef GL_FRAGMENT_PRECISION_HIGH\nprecision highp float;\n#else\nprecision mediump float;\n#endif\n#define IN varying\n#define TEX texture2D\n#define FRAG_COLOR gl_FragColor\n"
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

/* Lighting happens in linear light. Texture files store sRGB values (roughly
 * the square of the physical brightness, gamma 2.2), so the shader decodes
 * them, applies Lambert's cosine law, and re-encodes for the display. Doing the
 * maths on raw sRGB values would darken mid-tones and smear the terminator.
 * The ambient term is tiny in linear light (0.004 shows as ~8% grey) and
 * stands in for starlight so night sides keep their shape. */
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
    "uniform float nightLights;\n"    /* 1 = Earth: city lights and ocean glint */
    "uniform sampler2D texture2;\n"   /* Saturn's ring opacity strip, for ring shadows */
    "uniform float ringShadow;\n"     /* 1 = Saturn or its rings: cast/receive shadows */
    "uniform vec3 bodyCenter;\n"      /* Saturn's centre and drawn radius (world units) */
    "uniform float bodyRadius;\n"
    "uniform vec3 ringNormal;\n"      /* Saturn's pole: the ring plane's normal */
    "uniform vec2 ringRadii;\n"       /* ring inner/outer edge in Saturn radii */
    "uniform vec4 occluders[4];\n"    /* eclipse casters: centre (xyz) and drawn radius (w) */
    "uniform float occluderCount;\n"
    "uniform float sunRadius;\n"      /* the Sun's angular radius from this body, radians */
    SHADER_OUTPUT
    "vec3 toLinear(vec3 c) { return pow(c, vec3(2.2)); }\n"
    "vec3 toDisplay(vec3 c) { return pow(clamp(c, 0.0, 1.0), vec3(1.0 / 2.2)); }\n"
    "void main() {\n"
    "    vec4 surface = TEX(texture0, fragTexCoord) * colDiffuse;\n"
    "    vec3 n = normalize(fragNormal);\n"
    "    vec3 v = normalize(viewPos - fragPosition);\n"
    /* Atmospheric halo (additive shell 3.5% above the surface). mu is the
     * cosine between the shell normal and the eye. At the shell's own edge
     * (mu = 0) there is no air left, so the glow is zero; the planet's limb
     * sits at mu = sqrt(1 - (1/1.035)^2) ~ 0.26, where sight lines graze the
     * thickest air, so the glow peaks there and fades across the disc. */
    "    if (mode > 4.5) {\n"
    "        float mu = max(dot(n, v), 0.0);\n"
    "        float graze = smoothstep(0.0, 0.26, mu) * (1.0 - smoothstep(0.24, 0.75, mu));\n"
    "        float day = smoothstep(-0.25, 0.45, dot(n, lightDir));\n"
    "        FRAG_COLOR = vec4(atmosphere.rgb * atmosphere.a * graze * day * 1.6, 1.0);\n"
    "        return;\n"
    "    }\n"
    /* Sky backdrop (the real sky, A117): a contrast curve with a black point
     * just above the JPEG grain between stars, then a square, so the noise
     * goes black while stars and the Milky Way's band keep their light. */
    "    if (mode > 3.5) { vec3 sky = max(surface.rgb - 0.035, 0.0); FRAG_COLOR = vec4(min(sky * sky * 2.2 + sky * 0.55, vec3(1.0)), 1.0); return; }\n"
    /* Star: emissive, darker toward the limb where we look through more of
     * the cooler outer photosphere (limb darkening). */
    "    if (mode > 0.5 && mode < 1.5) {\n"
    /* The disc centre is hot enough to read near white; the limb keeps the
     * map's orange, as in white-light photographs of the photosphere. */
    "        float mu = max(dot(n, v), 0.0);\n"
    "        vec3 photosphere = surface.rgb * (0.7 + 0.75 * sqrt(mu));\n"
    "        FRAG_COLOR = vec4(min(mix(photosphere, vec3(1.0, 0.95, 0.82), 0.45 * mu * mu), vec3(1.0)), 1.0);\n"
    "        return;\n"
    "    }\n";

/* ISO C caps a string literal at 4095 characters, so main() continues in a
 * second literal; renderer_resources_init joins the two. */
static const char fragment_shader_lit[] =
    /* Lambert's law: brightness follows the cosine of the Sun's angle; a 5%
     * wrap stands in for light scattered just past the terminator. */
    "    vec3 albedo = toLinear(surface.rgb);\n"
    "    float ndl = dot(n, lightDir);\n"
    "    float lit = clamp((ndl + 0.05) / 1.05, 0.0, 1.0);\n"
    "    if (mode > 2.5) {\n" /* rings: thin ice and dust, lit from either face */
    /* Saturn's shadow on its rings: a ring point is dark when the ray toward
     * the Sun (P + t L, t > 0) passes closer to Saturn's centre than its
     * radius. b < 0 means the planet lies sunward of the point; b*b removed
     * from |d|^2 leaves the squared miss distance (Pythagoras). */
    "        float shade = 1.0;\n"
    "        if (ringShadow > 0.5) {\n"
    "            vec3 d = fragPosition - bodyCenter;\n"
    "            float b = dot(d, lightDir);\n"
    "            float miss = dot(d, d) - b * b;\n"
    "            float r2 = bodyRadius * bodyRadius;\n"
    "            if (b < 0.0) shade = smoothstep(r2 * 0.94, r2 * 1.02, miss);\n"
    "        }\n"
    /* From the unlit face the rings shine only by light passing through:
     * dense parts (high opacity, the B ring) go dark, sparse ones (the C ring,
     * the Cassini division) glow, as in Cassini's views from the night side. */
    "        float sunSide = step(0.0, dot(n, lightDir) * dot(n, v));\n"
    "        float through = mix(0.12 + 1.5 * (1.0 - surface.a), 1.0, sunSide);\n"
    "        FRAG_COLOR = vec4(toDisplay(albedo * (0.02 + 0.98 * abs(ndl)) * shade * through), surface.a);\n"
    "        return;\n"
    "    }\n"
    "    if (mode > 1.5) {\n" /* clouds: the map's brightness is its opacity */
    /* Fade at grazing angles: the shell sits 1.2% above the ground, and seen
     * edge-on at the limb it would otherwise draw a thin grey outline. */
    "        float edge = smoothstep(0.0, 0.3, dot(n, v));\n"
    "        FRAG_COLOR = vec4(toDisplay(vec3(0.004 + lit)), surface.r * (0.12 + 0.78 * smoothstep(-0.1, 0.3, ndl)) * edge);\n"
    "        return;\n"
    "    }\n"
    /* The rings' shadow on Saturn: follow the ray toward the Sun to the ring
     * plane (t from the plane equation) and read the ring strip's opacity at
     * that radius. Dense rings cast dark bands, the Cassini gap lets light by. */
    /* The ray is followed unconditionally (with guarded divisions) and the
     * strip sampled once: mipmapped lookups need screen-space derivatives,
     * which are undefined inside a per-pixel branch. Only applying the
     * shadow is conditional. Other bodies sample a white map harmlessly. */
    "    float facing = dot(lightDir, ringNormal);\n"
    "    float safeFacing = abs(facing) > 0.0001 ? facing : 0.0001;\n"
    "    float t = -dot(fragPosition - bodyCenter, ringNormal) / safeFacing;\n"
    "    float hitRadius = length(fragPosition + lightDir * t - bodyCenter) / max(bodyRadius, 0.000001);\n"
    "    float u = (hitRadius - ringRadii.x) / max(ringRadii.y - ringRadii.x, 0.000001);\n"
    "    float ringAlpha = TEX(texture2, vec2(clamp(u, 0.0, 1.0), 0.5)).a;\n"
    "    if (ringShadow > 0.5 && abs(facing) > 0.0001 && t > 0.0 && u > 0.0 && u < 1.0) lit *= 1.0 - 0.85 * ringAlpha;\n"
    /* Eclipses (SPEC A111): a moon's shadow on its planet, the planet's on
     * its moons. From this point the Sun is a disc of angular radius
     * sunRadius; each caster is a disc of angular radius R/t at angular
     * distance miss/t from it (t along the Sun direction). The light lost is
     * the overlap, as render_eclipse_fraction computes in C. */
    "    float sunlight = 1.0;\n"
    "    for (int k = 0; k < 4; ++k) {\n"
    "        if (float(k) >= occluderCount) break;\n"
    "        vec3 d = occluders[k].xyz - fragPosition;\n"
    "        float along = dot(d, lightDir);\n"
    "        if (along > 0.0) {\n"
    "            float theta = sqrt(max(dot(d, d) - along * along, 0.0)) / along;\n"
    "            float rho = occluders[k].w / along;\n"
    "            float inner = abs(rho - sunRadius);\n"
    "            float s = clamp((theta - inner) / max(rho + sunRadius - inner, 1e-12), 0.0, 1.0);\n"
    "            float area = min(rho * rho / max(sunRadius * sunRadius, 1e-24), 1.0);\n"
    "            sunlight *= 1.0 - (1.0 - s * s * (3.0 - 2.0 * s)) * area;\n"
    "        }\n"
    "    }\n"
    /* Direct sunlight, the ocean glint and the lit air of the rim all dim. */
    "    lit *= sunlight;\n"
    "    vec3 color = albedo * (0.004 + lit);\n"
    /* Dusk: sunlight grazing the terminator crosses a long path of air that
     * scatters away blue, so planets with air redden there. Gaussian band in
     * the Sun's elevation, scaled by the atmosphere's strength. */
    "    float dusk = ndl / 0.12;\n"
    "    color *= mix(vec3(1.0), vec3(1.45, 0.82, 0.55), exp(-dusk * dusk) * min(atmosphere.a * 1.2, 0.8));\n"
    /* Limb darkening for bodies with air: near the edge the sight line meets
     * the haze high up, above most of the reflecting cloud deck, so giant
     * planets look round rather than like flat discs. Airless bodies skip it
     * (strength 0); the full Moon really is nearly uniform. */
    "    float muView = max(dot(n, v), 0.0);\n"
    "    color *= mix(1.0, 0.35 + 0.65 * sqrt(muView), min(atmosphere.a * 1.6, 0.75));\n"
    "    if (nightLights > 0.5) {\n"
    /* City lights fade in across the terminator. */
    "        float night = 1.0 - smoothstep(-0.18, 0.06, ndl);\n"
    "        color += toLinear(TEX(texture1, fragTexCoord).rgb) * night * 2.0;\n"
    /* Sun glint: a Blinn-Phong highlight (half vector between Sun and eye)
     * masked to water, found where the day map is clearly bluer than it is
     * red or green. Land and ice stay matte. */
    "        float water = smoothstep(0.04, 0.16, surface.b - max(surface.r, surface.g));\n"
    "        vec3 h = normalize(lightDir + v);\n"
    "        float nh = max(dot(n, h), 0.0);\n"
    /* A sharp core plus a faint wide sheen, like sunlight on a rippled sea. */
    "        color += vec3(1.0, 0.93, 0.8) * (pow(nh, 300.0) * 1.2 + pow(nh, 24.0) * 0.05) * water * max(ndl, 0.0) * sunlight;\n"
    "    }\n"
    /* Atmosphere rim: grazing sight lines cross more air (Fresnel-like
     * falloff), lit mostly on the day side. */
    "    float rim = pow(1.0 - max(dot(n, v), 0.0), 3.0);\n"
    "    color += toLinear(atmosphere.rgb) * atmosphere.a * rim * smoothstep(-0.3, 0.45, ndl) * sunlight;\n"
    /* A soft shoulder instead of a hard clip: values above ~0.8 roll toward
     * white, so bright cloud tops, glints and city lights keep their detail. */
    "    color = mix(color, 1.0 - 0.04 / max(color - 0.6, vec3(0.2)), step(0.8, color));\n"
    "    FRAG_COLOR = vec4(toDisplay(color), 1.0);\n"
    "}\n";

/* Small-body cloud (SPEC A121). Every body is a quad of four vertices that
 * share its orbital elements; the vertex shader solves Kepler's equation for
 * the current day and moves the quad there, then spreads its corners a few
 * pixels on screen. It mirrors render_belt_position in scene_style.c exactly
 * (same Newton start, six steps, 3-1-3 rotation and simulation axes); the
 * CPU never touches the 48,000 bodies per frame.
 * Attributes: position = (corner x, corner y, kind), texcoord = (a [AU], e),
 * texcoord2 = (inclination, node), normal = (periapsis, M0, n [rad/day]). */
static const char belt_vertex_shader[] = SHADER_HEADER_VS
    "IN vec3 vertexPosition;\n"
    "IN vec2 vertexTexCoord;\n"
    "IN vec2 vertexTexCoord2;\n"
    "IN vec3 vertexNormal;\n"
    "uniform mat4 mvp;\n"
    "uniform float days;\n"      /* since the scene epoch */
    "uniform vec3 sunOffset;\n"  /* the Sun relative to the view origin, render units */
    "uniform float auUnits;\n"   /* render units per AU */
    "uniform vec2 viewport;\n"   /* pixels */
    "uniform float pointSize;\n" /* pixels */
    "uniform float focal;\n"     /* pixels per render unit at unit depth: viewport height / (2 tan(fovy/2)) */
    "OUT vec2 corner;\n"
    "OUT vec4 tint;\n"
    "void main() {\n"
    "    float a = vertexTexCoord.x, e = vertexTexCoord.y;\n"
    "    float m = mod(vertexNormal.y + vertexNormal.z * days, 6.2831853);\n"
    "    float E = m + e * sin(m);\n"
    "    for (int k = 0; k < 6; ++k) E -= (E - e * sin(E) - m) / (1.0 - e * cos(E));\n"
    "    float x = a * (cos(E) - e), y = a * sqrt(1.0 - e * e) * sin(E);\n"
    "    float cw = cos(vertexNormal.x), sw = sin(vertexNormal.x);\n"
    "    float ci = cos(vertexTexCoord2.x), si = sin(vertexTexCoord2.x);\n"
    "    float cn = cos(vertexTexCoord2.y), sn = sin(vertexTexCoord2.y);\n"
    "    vec3 ecl = vec3(x * (cw * cn - sw * sn * ci) - y * (sw * cn + cw * sn * ci),\n"
    "                    x * (cw * sn + sw * cn * ci) + y * (cw * cn * ci - sw * sn),\n"
    "                    x * (sw * si) + y * (cw * si));\n"
    "    vec3 world = sunOffset + vec3(ecl.x, ecl.z, -ecl.y) * auUnits;\n"
    "    vec4 clip = mvp * vec4(world, 1.0);\n"
    "    corner = vertexPosition.xy;\n"
    /* Density: a point fades as its own orbit shrinks on screen (a in AU times
     * pixels per AU at its depth), so 40,000 points piled into a few hundred
     * pixels around the Sun never blow out to white, while the wide Kuiper
     * belt stays visible from afar. */
    "    float orbitPixels = a * auUnits * focal / max(clip.w, 1e-6);\n"
    "    float density = smoothstep(25.0, 220.0, orbitPixels);\n"
    "    clip.xy += corner * pointSize / viewport * 2.0 * clip.w;\n"
    "    gl_Position = clip;\n"
    /* Muted hues by orbit class: belt stone, Trojan green, near-Earth red,
     * outer-system blue, Mars-crosser sand. */
    "    float kind = vertexPosition.z;\n"
    /* Quiet on purpose: the cloud is context behind the planets, never a
     * veil over them. Near-Earth objects crowd the inner system, so dimmest. */
    "    tint = kind < 0.5 ? vec4(0.62, 0.57, 0.48, 0.30) : kind < 1.5 ? vec4(0.45, 0.75, 0.5, 0.42)\n"
    "        : kind < 2.5 ? vec4(0.85, 0.5, 0.42, 0.26) : kind < 3.5 ? vec4(0.5, 0.65, 0.95, 0.5) : vec4(0.8, 0.62, 0.45, 0.24);\n"
    "    tint.a *= density;\n"
    "}\n";

static const char belt_fragment_shader[] = SHADER_HEADER_FS
    "IN vec2 corner;\n"
    "IN vec4 tint;\n"
    SHADER_OUTPUT
    "void main() {\n"
    "    float fade = 1.0 - smoothstep(0.35, 1.0, length(corner));\n"
    "    FRAG_COLOR = vec4(tint.rgb * tint.a * fade, 1.0);\n"
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

static Mesh build_ring(int segments, float inner, float outer)
{
    size_t vertices = render_ring_vertex_count(segments), indices = render_ring_index_count(segments);
    float *positions = malloc(vertices * 3 * sizeof(float)), *normals = malloc(vertices * 3 * sizeof(float));
    float *texcoords = malloc(vertices * 2 * sizeof(float));
    unsigned short *index = malloc(indices * sizeof(unsigned short));
    Mesh mesh = {0};
    /* Unit annulus in planet radii: the renderer scales it by the planet's drawn radius. */
    if (positions && normals && texcoords && index &&
        render_build_ring(segments, inner, outer, positions, normals, texcoords, index))
        mesh = upload_mesh(vertices, indices, positions, normals, texcoords, index);
    free(positions); free(normals); free(texcoords); free(index);
    return mesh;
}

/* A ring opacity strip rasterised in C (A122), four rows tall for the GPU. */
static Texture2D ring_strip_texture(const RenderRingSystem *system,
    void (*fill)(const RenderRingSystem *, unsigned char *), int width)
{
    static unsigned char strip[RENDER_RING_STRIP_WIDTH * 4 * 4];
    if (width > RENDER_RING_STRIP_WIDTH) return (Texture2D){0};
    fill(system, strip);
    for (int row = 1; row < 4; ++row) memcpy(strip + row * width * 4, strip, (size_t)width * 4);
    Image image = {strip, width, 4, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
    Texture2D texture = LoadTextureFromImage(image);
    if (texture.id) {
        GenTextureMipmaps(&texture);
        SetTextureFilter(texture, TEXTURE_FILTER_TRILINEAR);
        SetTextureWrap(texture, TEXTURE_WRAP_CLAMP);
    }
    return texture;
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
    static char fragment_source[sizeof(fragment_shader) + sizeof(fragment_shader_lit)];
    snprintf(fragment_source, sizeof(fragment_source), "%s%s", fragment_shader, fragment_shader_lit);
    resources->shader = LoadShaderFromMemory(vertex_shader, fragment_source);
    /* A compile failure yields id 0, but a link failure makes raylib quietly
     * substitute its default shader. Treat both as unavailable so the flat
     * colour fallback is used instead of an unlit, white-haloed scene. */
    if (!IsShaderValid(resources->shader) || resources->shader.id == rlGetShaderIdDefault()) return false;
    resources->loc_mode = GetShaderLocation(resources->shader, "mode");
    resources->loc_light_dir = GetShaderLocation(resources->shader, "lightDir");
    resources->loc_view_pos = GetShaderLocation(resources->shader, "viewPos");
    resources->loc_atmosphere = GetShaderLocation(resources->shader, "atmosphere");
    resources->loc_night_lights = GetShaderLocation(resources->shader, "nightLights");
    resources->loc_ring_shadow = GetShaderLocation(resources->shader, "ringShadow");
    resources->loc_body_center = GetShaderLocation(resources->shader, "bodyCenter");
    resources->loc_body_radius = GetShaderLocation(resources->shader, "bodyRadius");
    resources->loc_ring_normal = GetShaderLocation(resources->shader, "ringNormal");
    resources->loc_ring_radii = GetShaderLocation(resources->shader, "ringRadii");
    resources->loc_occluders = GetShaderLocation(resources->shader, "occluders");
    resources->loc_occluder_count = GetShaderLocation(resources->shader, "occluderCount");
    resources->loc_sun_radius = GetShaderLocation(resources->shader, "sunRadius");
    if (resources->loc_mode < 0 || resources->loc_light_dir < 0) {
        UnloadShader(resources->shader);
        resources->shader = (Shader){0};
        return false;
    }
    /* Planets get a smooth 96 x 48 sphere; dozens of small moons share a
     * cheaper 32 x 16 one so the 128-body scene stays light on phones. */
    resources->sphere_detailed = build_sphere(96, 48);
    resources->sphere_simple = build_sphere(32, 16);
    resources->ring = build_ring(192, (float)SOLAR_SATURN_RING_VISUAL_INNER_RATIO,
        (float)(SOLAR_SATURN_RING_OUTER_RADIUS_M / SOLAR_SATURN_RADIUS_M));
    /* Faint ring systems (A122, A126): meshes from each table's span,
     * opacity strips rasterised from the tables (no image files). */
    for (size_t k = 0; k < RENDER_RING_SYSTEM_COUNT; ++k) {
        const RenderRingSystem *system = render_ring_system_at(k);
        resources->ring_meshes[k] = build_ring(256, (float)(system->inner_m / system->planet_radius_m),
            (float)(system->outer_m / system->planet_radius_m));
        resources->ring_strips[k] = ring_strip_texture(system, render_ring_strip, RENDER_RING_STRIP_WIDTH);
        resources->ring_strips_illustrative[k] = ring_strip_texture(system, render_ring_strip_illustrative,
            RENDER_RING_ILLUSTRATIVE_WIDTH);
    }
    Image white = GenImageColor(2, 2, WHITE);
    resources->white = LoadTextureFromImage(white);
    UnloadImage(white);
    resources->glow = build_glow();
    /* Rasterize at 32 px once and draw scaled down with bilinear filtering,
     * which stays crisp at label sizes; raylib's built-in font is pixel art. */
    resources->label_font = LoadFontFromMemory(".ttf", label_font, label_font_size, 32, NULL, 95);
    resources->label_font_ready = resources->label_font.texture.id != 0;
    if (resources->label_font_ready) SetTextureFilter(resources->label_font.texture, TEXTURE_FILTER_BILINEAR);
    resources->belt_shader = LoadShaderFromMemory(belt_vertex_shader, belt_fragment_shader);
    if (IsShaderValid(resources->belt_shader) && resources->belt_shader.id != rlGetShaderIdDefault()) {
        resources->loc_belt_days = GetShaderLocation(resources->belt_shader, "days");
        resources->loc_belt_sun = GetShaderLocation(resources->belt_shader, "sunOffset");
        resources->loc_belt_au = GetShaderLocation(resources->belt_shader, "auUnits");
        resources->loc_belt_viewport = GetShaderLocation(resources->belt_shader, "viewport");
        resources->loc_belt_size = GetShaderLocation(resources->belt_shader, "pointSize");
        resources->loc_belt_focal = GetShaderLocation(resources->belt_shader, "focal");
        resources->belt_material = LoadMaterialDefault();
        resources->belt_material.shader = resources->belt_shader;
    } else {
        resources->belt_shader = (Shader){0};
    }
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
    for (size_t k = 0; k < RENDER_RING_SYSTEM_COUNT; ++k) {
        if (resources->ring_meshes[k].vertexCount) UnloadMesh(resources->ring_meshes[k]);
        if (resources->ring_strips[k].id) UnloadTexture(resources->ring_strips[k]);
        if (resources->ring_strips_illustrative[k].id) UnloadTexture(resources->ring_strips_illustrative[k]);
    }
    if (resources->white.id) UnloadTexture(resources->white);
    if (resources->glow.id) UnloadTexture(resources->glow);
    if (resources->label_font_ready) UnloadFont(resources->label_font);
    for (int k = 0; k < resources->belt_mesh_count; ++k) UnloadMesh(resources->belt_meshes[k]);
    if (resources->belt_shader.id) {
        RL_FREE(resources->belt_material.maps);
        UnloadShader(resources->belt_shader);
    }
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

bool renderer_load_belt_memory(RenderResources *resources, const unsigned char *bytes, size_t length)
{
    static RenderBeltBody bodies[RENDER_BELT_CAPACITY];
    if (!resources->belt_shader.id || resources->belt_mesh_count) return false;
    size_t count = render_belt_decode(bytes, length, bodies, RENDER_BELT_CAPACITY);
    if (!count) return false;
    const size_t per_mesh = 16383; /* 4 vertices each: indices stay below 65536 */
    const float corners[4][2] = {{-1, -1}, {1, -1}, {1, 1}, {-1, 1}};
    for (size_t first = 0; first < count && resources->belt_mesh_count < 4; first += per_mesh) {
        size_t n = count - first < per_mesh ? count - first : per_mesh;
        Mesh mesh = {0};
        mesh.vertexCount = (int)(n * 4);
        mesh.triangleCount = (int)(n * 2);
        mesh.vertices = RL_MALLOC(n * 4 * 3 * sizeof(float));
        mesh.texcoords = RL_MALLOC(n * 4 * 2 * sizeof(float));
        mesh.texcoords2 = RL_MALLOC(n * 4 * 2 * sizeof(float));
        mesh.normals = RL_MALLOC(n * 4 * 3 * sizeof(float));
        mesh.indices = RL_MALLOC(n * 6 * sizeof(unsigned short));
        if (!mesh.vertices || !mesh.texcoords || !mesh.texcoords2 || !mesh.normals || !mesh.indices) {
            RL_FREE(mesh.vertices); RL_FREE(mesh.texcoords); RL_FREE(mesh.texcoords2); RL_FREE(mesh.normals); RL_FREE(mesh.indices);
            break;
        }
        for (size_t k = 0; k < n; ++k) {
            const RenderBeltBody *b = &bodies[first + k];
            float mean_motion = (float)render_belt_mean_motion(b->a_au);
            for (int c = 0; c < 4; ++c) {
                size_t v = k * 4 + (size_t)c;
                mesh.vertices[v * 3] = corners[c][0]; mesh.vertices[v * 3 + 1] = corners[c][1]; mesh.vertices[v * 3 + 2] = b->kind;
                mesh.texcoords[v * 2] = b->a_au; mesh.texcoords[v * 2 + 1] = b->e;
                mesh.texcoords2[v * 2] = b->inclination; mesh.texcoords2[v * 2 + 1] = b->node;
                mesh.normals[v * 3] = b->periapsis; mesh.normals[v * 3 + 1] = b->mean_anomaly0; mesh.normals[v * 3 + 2] = mean_motion;
            }
            const unsigned short base = (unsigned short)(k * 4), order[6] = {0, 1, 2, 0, 2, 3};
            for (int t = 0; t < 6; ++t) mesh.indices[k * 6 + (size_t)t] = (unsigned short)(base + order[t]);
        }
        UploadMesh(&mesh, false);
        resources->belt_meshes[resources->belt_mesh_count++] = mesh;
    }
    resources->belt_count = count;
    return resources->belt_mesh_count > 0;
}
