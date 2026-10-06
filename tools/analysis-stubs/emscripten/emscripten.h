#ifndef SOLAR_ANALYSIS_EMSCRIPTEN_STUB_H
#define SOLAR_ANALYSIS_EMSCRIPTEN_STUB_H

/* Native stand-in used only so static analysis (CodeQL) can compile
 * src/lab_web.c with the host compiler. The real header comes from the
 * Emscripten SDK; KEEPALIVE only keeps exports alive through emcc's linker. */
#define EMSCRIPTEN_KEEPALIVE

#endif
