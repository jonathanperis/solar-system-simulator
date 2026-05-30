CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2 -g
CPPFLAGS ?= -Isrc
LDFLAGS ?=
LDLIBS ?= -lm

APP := build/solar-system-simulator
TEST_DIR := build/tests
WEB_DIR := build/web
WEB_APP := $(WEB_DIR)/solar-system-simulator.html
WASM_ZIP := build/solar-system-simulator-wasm.zip

RAYLIB_LOCAL_PREFIX ?= $(HOME)/.local
RAYLIB_CFLAGS ?= $(shell pkg-config --cflags raylib 2>/dev/null || if [ -f "$(RAYLIB_LOCAL_PREFIX)/include/raylib.h" ]; then echo -I$(RAYLIB_LOCAL_PREFIX)/include; fi)
RAYLIB_LIBS ?= $(shell pkg-config --libs raylib 2>/dev/null || if [ -f "$(RAYLIB_LOCAL_PREFIX)/lib/libraylib.a" ]; then echo -L$(RAYLIB_LOCAL_PREFIX)/lib -lraylib -lm -ldl -lpthread; else echo -lraylib -lm -ldl -lpthread -lGL -lrt -lX11; fi)
RAYLIB_WEB_SRC ?= $(RAYLIB_LOCAL_PREFIX)/src/raylib/src
RAYLIB_WEB_LIB ?= $(RAYLIB_WEB_SRC)/libraylib.web.a
RAYLIB_WEB_CFLAGS ?= -I$(RAYLIB_WEB_SRC) -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2
RAYLIB_WEB_LDFLAGS ?= -s USE_GLFW=3 -s ALLOW_MEMORY_GROWTH=1 -s ASYNCIFY

SIM_SRCS := \
    src/sim/vec3d.c \
    src/sim/units.c \
    src/sim/body.c \
    src/sim/physics.c \
    src/sim/solar_system.c

APP_SRCS := src/main.c src/app/orbit_camera.c src/app/body_trails.c src/app/body_labels.c src/render/renderer.c $(SIM_SRCS)
APP_OBJS := $(APP_SRCS:%.c=build/%.o)

TEST_VEC3D := $(TEST_DIR)/test_vec3d
TEST_PHYSICS := $(TEST_DIR)/test_physics
TEST_SOLAR_SYSTEM := $(TEST_DIR)/test_solar_system
TEST_ORBIT_CAMERA := $(TEST_DIR)/test_orbit_camera
TEST_BODY_TRAILS := $(TEST_DIR)/test_body_trails
TEST_BODY_LABELS := $(TEST_DIR)/test_body_labels
TEST_RENDERER := $(TEST_DIR)/test_renderer
TEST_BINS := $(TEST_VEC3D) $(TEST_PHYSICS) $(TEST_SOLAR_SYSTEM) $(TEST_ORBIT_CAMERA) $(TEST_BODY_TRAILS) $(TEST_BODY_LABELS) $(TEST_RENDERER)

.PHONY: all run test web raylib-web dist-wasm clean

all: $(APP)

run: $(APP)
	$(APP)

test: $(TEST_BINS)
	@for test_bin in $(TEST_BINS); do \
		echo "Running $$test_bin"; \
		$$test_bin; \
	done

web: $(WEB_APP)
	python3 tools/check_wasm_artifacts.py $(WEB_DIR)

raylib-web:
	$(MAKE) PLATFORM=PLATFORM_WEB -C $(RAYLIB_WEB_SRC)

dist-wasm: web
	@rm -f $(WASM_ZIP)
	python3 -m zipfile -c $(WASM_ZIP) $(WEB_DIR)/solar-system-simulator.html $(WEB_DIR)/solar-system-simulator.js $(WEB_DIR)/solar-system-simulator.wasm
	@echo "Created $(WASM_ZIP)"

$(WEB_APP): $(APP_SRCS) web/shell.html $(RAYLIB_WEB_LIB)
	@mkdir -p $(@D)
	emcc $(CPPFLAGS) $(CFLAGS) $(RAYLIB_WEB_CFLAGS) $(APP_SRCS) $(RAYLIB_WEB_LIB) $(RAYLIB_WEB_LDFLAGS) --shell-file web/shell.html -o $@

$(RAYLIB_WEB_LIB):
	$(MAKE) PLATFORM=PLATFORM_WEB -C $(RAYLIB_WEB_SRC)

$(APP): $(APP_OBJS)
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) $^ $(RAYLIB_LIBS) -o $@

build/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(RAYLIB_CFLAGS) -c $< -o $@

$(TEST_VEC3D): tests/test_vec3d.c src/sim/vec3d.c src/sim/units.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDLIBS) -o $@

$(TEST_PHYSICS): tests/test_physics.c src/sim/vec3d.c src/sim/body.c src/sim/physics.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDLIBS) -o $@

$(TEST_SOLAR_SYSTEM): tests/test_solar_system.c $(SIM_SRCS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $^ $(LDLIBS) -o $@

$(TEST_ORBIT_CAMERA): tests/test_orbit_camera.c src/app/orbit_camera.c src/app/orbit_camera.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_orbit_camera.c src/app/orbit_camera.c $(LDLIBS) -o $@

$(TEST_BODY_TRAILS): tests/test_body_trails.c src/app/body_trails.c src/app/body_trails.h $(SIM_SRCS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_body_trails.c src/app/body_trails.c $(SIM_SRCS) $(LDLIBS) -o $@

$(TEST_BODY_LABELS): tests/test_body_labels.c src/app/body_labels.c src/app/body_labels.h $(SIM_SRCS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_body_labels.c src/app/body_labels.c $(SIM_SRCS) $(LDLIBS) -o $@

$(TEST_RENDERER): tests/test_renderer.c src/render/renderer.c src/render/renderer.h src/app/body_trails.c src/app/body_trails.h $(SIM_SRCS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(RAYLIB_CFLAGS) tests/test_renderer.c src/render/renderer.c src/app/body_trails.c $(SIM_SRCS) $(RAYLIB_LIBS) -o $@

clean:
	rm -rf build
