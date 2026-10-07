/* Native shader smoke test (macOS): renders Earth offscreen with the real
 * GLSL 330 shader strings, sphere mesh and textures through a CGL OpenGL 3.2+
 * core context (no window needed), then checks the pixels. CI's native job
 * only compiles the shader text; this proves what it actually draws.
 * Run with `make test-native-shaders` (macOS only). */
#define GL_SILENCE_DEPRECATION
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl3.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "render/render_resources.c" /* for the exact shader strings */

static GLuint compile(GLenum type, const char *src)
{
    GLuint s = glCreateShader(type); glShaderSource(s, 1, &src, NULL); glCompileShader(s);
    GLint ok; glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[2048]; glGetShaderInfoLog(s, 2048, NULL, log); fprintf(stderr, "%s\n", log); exit(1); }
    return s;
}

static GLuint texture_from(const char *path)
{
    FILE *f = fopen(path, "rb"); fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    unsigned char *bytes = malloc((size_t)n); fread(bytes, 1, (size_t)n, f); fclose(f);
    DecodedImage im; if (!render_decode_image(bytes, (size_t)n, &im)) { fprintf(stderr, "decode %s\n", path); exit(1); }
    GLuint t; glGenTextures(1, &t); glBindTexture(GL_TEXTURE_2D, t);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, im.channels == 4 ? GL_RGBA : GL_RGB, im.width, im.height, 0, im.channels == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, im.pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    render_free_image(&im); free(bytes);
    return t;
}

static double luma(const unsigned char *px, int w, int h, int x, int y)
{
    double sum = 0; int n = 0;
    for (int dy = -4; dy <= 4; ++dy) for (int dx = -4; dx <= 4; ++dx) {
        const unsigned char *p = &px[((h - 1 - (y + dy)) * w + (x + dx)) * 4];
        sum += 0.2126 * p[0] + 0.7152 * p[1] + 0.0722 * p[2]; ++n;
    }
    return sum / n;
}

/* Column-major 4x4 helpers. */
static void perspective(float *m, float fovy, float aspect, float n, float f)
{
    float t = 1.0f / tanf(fovy / 2); for (int i = 0; i < 16; ++i) m[i] = 0;
    m[0] = t / aspect; m[5] = t; m[10] = (f + n) / (n - f); m[11] = -1; m[14] = 2 * f * n / (n - f);
}

int main(void)
{
    CGLPixelFormatAttribute attributes[] = {kCGLPFAOpenGLProfile, (CGLPixelFormatAttribute)kCGLOGLPVersion_3_2_Core, 0};
    CGLPixelFormatObj pf; GLint npf; CGLContextObj ctx;
    if (CGLChoosePixelFormat(attributes, &pf, &npf) || !pf || CGLCreateContext(pf, NULL, &ctx)) { puts("no OpenGL 3.2 core context; skipping"); return 0; }
    CGLSetCurrentContext(ctx);
    const int W = 640, H = 640;
    GLuint fbo, color, depth; glGenFramebuffers(1, &fbo); glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glGenRenderbuffers(1, &color); glBindRenderbuffer(GL_RENDERBUFFER, color); glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, W, H);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color);
    glGenRenderbuffers(1, &depth); glBindRenderbuffer(GL_RENDERBUFFER, depth); glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, W, H);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);
    glViewport(0, 0, W, H); glClearColor(3/255.f, 5/255.f, 10/255.f, 1); glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); glEnable(GL_DEPTH_TEST); glEnable(GL_CULL_FACE);

    GLuint prog = glCreateProgram();
    glAttachShader(prog, compile(GL_VERTEX_SHADER, vertex_shader)); glAttachShader(prog, compile(GL_FRAGMENT_SHADER, fragment_shader));
    glBindAttribLocation(prog, 0, "vertexPosition"); glBindAttribLocation(prog, 1, "vertexTexCoord"); glBindAttribLocation(prog, 2, "vertexNormal");
    glLinkProgram(prog); glUseProgram(prog);

    int slices = 96, rings = 48; size_t nv = render_sphere_vertex_count(slices, rings), ni = render_sphere_index_count(slices, rings);
    float *pos = malloc(nv * 12), *nor = malloc(nv * 12), *uv = malloc(nv * 8); unsigned short *idx = malloc(ni * 2);
    render_build_sphere(slices, rings, pos, nor, uv, idx);
    GLuint vao, vb[4]; glGenVertexArrays(1, &vao); glBindVertexArray(vao); glGenBuffers(4, vb);
    glBindBuffer(GL_ARRAY_BUFFER, vb[0]); glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(nv * 12), pos, GL_STATIC_DRAW); glVertexAttribPointer(0, 3, GL_FLOAT, 0, 0, 0); glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, vb[1]); glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(nv * 8), uv, GL_STATIC_DRAW); glVertexAttribPointer(1, 2, GL_FLOAT, 0, 0, 0); glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, vb[2]); glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(nv * 12), nor, GL_STATIC_DRAW); glVertexAttribPointer(2, 3, GL_FLOAT, 0, 0, 0); glEnableVertexAttribArray(2);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vb[3]); glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(ni * 2), idx, GL_STATIC_DRAW);

    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, texture_from("assets/textures/earth_day.jpg"));
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, texture_from("assets/textures/earth_night.jpg"));
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, texture_from("assets/textures/saturn_ring.png"));
    glUniform1i(glGetUniformLocation(prog, "texture0"), 0); glUniform1i(glGetUniformLocation(prog, "texture1"), 1); glUniform1i(glGetUniformLocation(prog, "texture2"), 2);

    /* Earth at the origin, camera on +Z, Sun toward +X/+Z: a gibbous phase
     * with the terminator on the left. Model = identity (prime meridian +X). */
    float proj[16]; perspective(proj, 0.75f, 1.0f, 0.1f, 50.0f);
    float view[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,-3.2f,1};
    float mvp[16] = {0}; for (int c = 0; c < 4; ++c) for (int r = 0; r < 4; ++r) for (int k = 0; k < 4; ++k) mvp[c*4+r] += proj[k*4+r] * view[c*4+k];
    /* Rotate the body so longitude 30 E faces the camera (+Z). */
    float a = -(90.0f + 30.0f) * 3.14159265f / 180.0f, model[16] = {cosf(a),0,-sinf(a),0, 0,1,0,0, sinf(a),0,cosf(a),0, 0,0,0,1};
    float mvp2[16] = {0}; for (int c = 0; c < 4; ++c) for (int r = 0; r < 4; ++r) for (int k = 0; k < 4; ++k) mvp2[c*4+r] += mvp[k*4+r] * model[c*4+k];
    glUniformMatrix4fv(glGetUniformLocation(prog, "mvp"), 1, 0, mvp2);
    glUniformMatrix4fv(glGetUniformLocation(prog, "matModel"), 1, 0, model);
    glUniformMatrix4fv(glGetUniformLocation(prog, "matNormal"), 1, 0, model);
    glUniform4f(glGetUniformLocation(prog, "colDiffuse"), 1, 1, 1, 1);
    glUniform1f(glGetUniformLocation(prog, "mode"), 0);
    float lx = 0.80f, lz = 0.60f; glUniform3f(glGetUniformLocation(prog, "lightDir"), lx, 0.0f, lz);
    glUniform3f(glGetUniformLocation(prog, "viewPos"), 0, 0, 3.2f);
    RenderAtmosphere air = render_atmosphere_for_body(BODY_ID_EARTH);
    glUniform4f(glGetUniformLocation(prog, "atmosphere"), air.r, air.g, air.b, air.strength);
    glUniform1f(glGetUniformLocation(prog, "nightLights"), 1);
    glUniform1f(glGetUniformLocation(prog, "ringShadow"), 0);
    glDrawElements(GL_TRIANGLES, (GLsizei)ni, GL_UNSIGNED_SHORT, 0);

    unsigned char *px = malloc((size_t)W * H * 4); glReadPixels(0, 0, W, H, GL_RGBA, GL_UNSIGNED_BYTE, px);
    if (glGetError() != GL_NO_ERROR) { fputs("GL error during render\n", stderr); return 1; }
    /* Mean brightness of a small square around (x, y) in image coordinates
     * (y down); glReadPixels rows run bottom-up. */
    #define LUMA(x, y) luma(px, W, H, (x), (y))
    double background = LUMA(20, 20), day = LUMA(400, 330), night = LUMA(150, 330), glint = LUMA(453, 318);
    double africa_r = 0, africa_b = 0;
    for (int dy = -6; dy <= 6; ++dy) for (int dx = -6; dx <= 6; ++dx) {
        const unsigned char *p = &px[((H - 1 - (300 + dy)) * W + (290 + dx)) * 4];
        africa_r += p[0]; africa_b += p[2];
    }
    printf("GL %s | background %.0f day %.0f night %.0f glint %.0f\n", glGetString(GL_VERSION), background, day, night, glint);
    int failures = 0;
    #define EXPECT(cond, what) do { if (!(cond)) { fprintf(stderr, "FAIL: %s\n", what); ++failures; } } while (0)
    EXPECT(background < 20, "space behind the planet stays dark");
    EXPECT(day > night * 1.8, "the sunward side is clearly brighter than the far side (Lambert lighting)");
    EXPECT(glint > day, "the ocean sun glint is brighter than the surrounding sea");
    EXPECT(africa_r > africa_b, "30 E faces the camera: central Africa reads land-coloured, so the map is not mirrored");
    if (getenv("SOLAR_SHADER_PPM")) {
        FILE *out = fopen(getenv("SOLAR_SHADER_PPM"), "wb"); fprintf(out, "P6 %d %d 255\n", W, H);
        for (int y = H - 1; y >= 0; --y) for (int x = 0; x < W; ++x) fwrite(&px[(y * W + x) * 4], 1, 3, out);
        fclose(out);
    }
    if (!failures) puts("native shader smoke test passed");
    return failures ? 1 : 0;
}
