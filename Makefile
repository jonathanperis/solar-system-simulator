CC ?= cc
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2 -g
CPPFLAGS ?= -Isrc
CPPFLAGS += -Ibuild
LDFLAGS ?=
LDLIBS ?= -lm

APP := build/solar-system-simulator
LAB := build/solar-lab
TEST_DIR := build/tests
WEB_DIR := build/web
WEB_APP := $(WEB_DIR)/solar-system-simulator.js
WEB_WASM := $(WEB_DIR)/solar-system-simulator.wasm
WEB_MANIFEST := $(WEB_DIR)/build-info.json
LAB_WEB_JS := $(WEB_DIR)/learning-lab.mjs
LAB_WEB_WASM := $(WEB_DIR)/learning-lab.wasm
WASM_ZIP := build/solar-system-simulator-wasm.zip

RAYLIB_LOCAL_PREFIX ?= $(HOME)/.local
RAYLIB_CFLAGS ?= $(shell pkg-config --cflags raylib 2>/dev/null || if [ -f "$(RAYLIB_LOCAL_PREFIX)/include/raylib.h" ]; then echo -I$(RAYLIB_LOCAL_PREFIX)/include; fi)
RAYLIB_LIBS ?= $(shell pkg-config --libs raylib 2>/dev/null || if [ -f "$(RAYLIB_LOCAL_PREFIX)/lib/libraylib.a" ]; then echo -L$(RAYLIB_LOCAL_PREFIX)/lib -lraylib -lm -ldl -lpthread; else echo -lraylib -lm -ldl -lpthread -lGL -lrt -lX11; fi)
RAYLIB_WEB_SRC ?= $(RAYLIB_LOCAL_PREFIX)/src/raylib/src
RAYLIB_WEB_LIB ?= $(RAYLIB_WEB_SRC)/libraylib.web.a
RAYLIB_WEB_CFLAGS ?= -I$(RAYLIB_WEB_SRC) -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2
RAYLIB_WEB_LDFLAGS ?= -s USE_GLFW=3 -s ALLOW_MEMORY_GROWTH=1 -s ASYNCIFY -s STACK_SIZE=262144 -s EXPORTED_RUNTIME_METHODS=ccall

SIM_SRCS := \
    src/sim/vec3d.c \
    src/sim/units.c \
    src/sim/body.c \
    src/sim/physics.c \
    src/sim/diagnostics.c \
    src/sim/lessons.c \
    src/sim/collisions.c \
    src/sim/satellite.c \
    src/sim/orbit.c \
    src/sim/experiment.c \
    src/sim/jovian_catalog.c \
    src/sim/solar_system.c

SESSION_SRCS := src/app/body_trails.c src/app/simulation_step.c src/app/simulation_session.c
LAB_SRCS := src/app/csv_export.c src/app/lab_config.c src/app/comparison.c $(SESSION_SRCS) $(SIM_SRCS)
APP_SRCS := src/main.c src/app/orbit_camera.c src/render/renderer.c $(LAB_SRCS)
APP_OBJS := $(APP_SRCS:%.c=build/%.o)

# Shared session/clock layouts must rebuild every native consumer after a header edit.
-include $(APP_OBJS:.o=.d)
.DEFAULT_GOAL := all

TEST_VEC3D := $(TEST_DIR)/test_vec3d
TEST_PHYSICS := $(TEST_DIR)/test_physics
TEST_SOLAR_SYSTEM := $(TEST_DIR)/test_solar_system
TEST_ORBIT_CAMERA := $(TEST_DIR)/test_orbit_camera
TEST_BODY_TRAILS := $(TEST_DIR)/test_body_trails
TEST_SIMULATION_STEP := $(TEST_DIR)/test_simulation_step
TEST_SIMULATION_SESSION := $(TEST_DIR)/test_simulation_session
TEST_RENDERER := $(TEST_DIR)/test_renderer
TEST_SATELLITES := $(TEST_DIR)/test_satellites
TEST_BINS := $(TEST_VEC3D) $(TEST_PHYSICS) $(TEST_SOLAR_SYSTEM) $(TEST_ORBIT_CAMERA) $(TEST_BODY_TRAILS) $(TEST_SIMULATION_STEP) $(TEST_SIMULATION_SESSION) $(TEST_RENDERER) $(TEST_SATELLITES) $(TEST_DIR)/test_orbit $(TEST_DIR)/test_outer_planets
TEST_BINS += $(TEST_DIR)/test_experiment
TEST_BINS += $(TEST_DIR)/test_learning_lab
TEST_BINS += $(TEST_DIR)/test_advanced_lessons
TEST_BINS += $(TEST_DIR)/test_comparison
HEADLESS_TEST_BINS = $(filter-out $(TEST_RENDERER),$(TEST_BINS))
SOURCE_HEADERS := $(wildcard src/app/*.h src/sim/*.h src/render/*.h src/sim/*.inc)

.PHONY: all run headless test test-binaries test-core test-sanitize test-build test-cli test-validators web raylib-web dist-wasm docs-assets docs-check clean FORCE

all: $(APP)

headless: $(LAB)

build/revision.h: FORCE
	@python3 tools/build_revision.py $@

$(LAB): src/headless.c $(LAB_SRCS) $(SOURCE_HEADERS) build/revision.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) $(LDLIBS) -o $@

test-cli: $(LAB)
	python3 tests/test_headless.py

test-validators:
	@mkdir -p build
	python3 tests/test_artifact_checks.py

run: $(APP)
	$(APP)

test: test-core $(TEST_RENDERER)
	$(TEST_RENDERER)

# Pure C/application contracts need no raylib headers, graphics libraries or window.
test-core: build/catalog-orbits.dylib $(HEADLESS_TEST_BINS)
	python3 tools/jovian_catalog.py --check
	python3 tools/planet_epoch.py --check
	python3 tests/test_small_body_catalog.py
	$(MAKE) test-binaries TEST_BINS="$(HEADLESS_TEST_BINS)"

test-binaries: $(TEST_BINS)
	@set -e; for test_bin in $(TEST_BINS); do \
		echo "Running $$test_bin"; \
		"$$test_bin"; \
	done

test-build:
	python3 tests/test_build_contract.py

test-sanitize:
	$(MAKE) test-binaries headless TEST_DIR=build/sanitized-tests LAB=build/solar-lab-sanitized CFLAGS='-std=c11 -Wall -Wextra -Wpedantic -Werror -O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer'
	SOLAR_LAB_BINARY=build/solar-lab-sanitized python3 tests/test_headless.py

web: $(WEB_MANIFEST)
	python3 tools/check_wasm_artifacts.py $(WEB_DIR)

$(WEB_MANIFEST): $(WEB_APP) $(WEB_WASM) $(WEB_DIR)/catalog-orbits.wasm $(LAB_WEB_JS) $(LAB_WEB_WASM) build/revision.h tools/write_wasm_manifest.py
	python3 tools/write_wasm_manifest.py $(WEB_DIR) build/revision.h

docs-assets: web
	python3 tools/prepare_wasm.py $(WEB_DIR) docs/public/wasm

raylib-web:
	$(MAKE) PLATFORM=PLATFORM_WEB -C $(RAYLIB_WEB_SRC)

dist-wasm: web
	@rm -f $(WASM_ZIP)
	python3 -m zipfile -c $(WASM_ZIP) $(WEB_APP) $(WEB_WASM) $(WEB_DIR)/catalog-orbits.wasm $(LAB_WEB_JS) $(LAB_WEB_WASM) $(WEB_MANIFEST)
	@echo "Created $(WASM_ZIP)"

docs-check:
	python3 tools/check_docs_routes.py docs/dist

$(WEB_APP): $(APP_SRCS) $(SOURCE_HEADERS) build/revision.h $(RAYLIB_WEB_LIB)
	@mkdir -p $(@D)
	emcc $(CPPFLAGS) $(CFLAGS) $(RAYLIB_WEB_CFLAGS) $(APP_SRCS) $(RAYLIB_WEB_LIB) $(RAYLIB_WEB_LDFLAGS) -o $@

# Emscripten produces both files in one link. Re-link if the companion binary
# was removed, even when the JS target itself is still up to date.
$(WEB_WASM): $(WEB_APP)
	@test -f $@ || { rm -f $(WEB_APP); $(MAKE) $(WEB_APP); }

$(RAYLIB_WEB_LIB):
	$(MAKE) PLATFORM=PLATFORM_WEB -C $(RAYLIB_WEB_SRC)

$(APP): $(APP_OBJS)
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) $^ $(RAYLIB_LIBS) -o $@

build/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(RAYLIB_CFLAGS) -MMD -MP -c $< -o $@

build/src/app/csv_export.o: build/revision.h

$(TEST_BINS): $(SOURCE_HEADERS)

$(TEST_VEC3D): tests/test_vec3d.c src/sim/vec3d.c src/sim/units.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) $(LDLIBS) -o $@

$(TEST_PHYSICS): tests/test_physics.c src/sim/vec3d.c src/sim/body.c src/sim/physics.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) $(LDLIBS) -o $@

$(TEST_SOLAR_SYSTEM): tests/test_solar_system.c $(SIM_SRCS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) $(LDLIBS) -o $@

$(TEST_SATELLITES): tests/test_satellites.c $(SIM_SRCS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) $(LDLIBS) -o $@

$(TEST_ORBIT_CAMERA): tests/test_orbit_camera.c src/app/orbit_camera.c src/app/orbit_camera.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_orbit_camera.c src/app/orbit_camera.c $(LDLIBS) -o $@

$(TEST_BODY_TRAILS): tests/test_body_trails.c src/app/body_trails.c src/app/body_trails.h $(SIM_SRCS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_body_trails.c src/app/body_trails.c $(SIM_SRCS) $(LDLIBS) -o $@

$(TEST_SIMULATION_STEP): tests/test_simulation_step.c src/app/simulation_step.c src/app/simulation_step.h src/app/body_trails.c src/app/body_trails.h $(SIM_SRCS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_simulation_step.c src/app/simulation_step.c src/app/body_trails.c $(SIM_SRCS) $(LDLIBS) -o $@

$(TEST_RENDERER): tests/test_renderer.c src/render/renderer.c src/render/renderer.h src/app/body_trails.c src/app/body_trails.h $(SIM_SRCS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(RAYLIB_CFLAGS) tests/test_renderer.c src/render/renderer.c src/app/body_trails.c $(SIM_SRCS) $(RAYLIB_LIBS) -o $@

$(TEST_SIMULATION_SESSION): tests/test_simulation_session.c src/app/simulation_session.c src/app/simulation_session.h src/app/simulation_step.c src/app/simulation_step.h src/app/body_trails.c src/app/body_trails.h $(SIM_SRCS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) tests/test_simulation_session.c src/app/simulation_session.c src/app/simulation_step.c src/app/body_trails.c $(SIM_SRCS) $(LDLIBS) -o $@

clean:
	rm -rf build

build/benchmark_simulation: tools/benchmark_simulation.c src/app/body_trails.c src/app/simulation_step.c $(SIM_SRCS) $(wildcard src/sim/*.h src/sim/*.inc src/app/*.h)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) $(LDLIBS) -o $@

$(TEST_DIR)/test_orbit: tests/test_orbit.c src/sim/orbit.c src/sim/vec3d.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) $(LDLIBS) -o $@

$(TEST_DIR)/test_outer_planets: tests/test_outer_planets.c $(SIM_SRCS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) $(LDLIBS) -o $@

$(TEST_DIR)/test_experiment: tests/test_experiment.c src/app/simulation_session.c src/app/simulation_step.c src/app/body_trails.c $(SIM_SRCS) $(wildcard src/app/*.h)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) $(LDLIBS) -o $@

build/catalog-orbits.dylib: src/sim/orbit.c src/sim/vec3d.c $(wildcard src/sim/*.h)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -shared -fPIC $(filter %.c,$^) -lm -o $@

$(WEB_DIR)/catalog-orbits.wasm: src/sim/orbit.c src/sim/vec3d.c $(wildcard src/sim/*.h)
	@mkdir -p $(@D)
	emcc $(CPPFLAGS) -O2 $(filter %.c,$^) -s STANDALONE_WASM --no-entry -Wl,--export=catalog_coordinate -Wl,--export=catalog_period_days -o $@

$(TEST_DIR)/test_learning_lab: tests/test_learning_lab.c $(SIM_SRCS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) $(LDLIBS) -o $@

$(TEST_DIR)/test_advanced_lessons: tests/test_advanced_lessons.c $(SESSION_SRCS) $(SIM_SRCS)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) $(LDLIBS) -o $@

$(TEST_DIR)/test_comparison: tests/test_comparison.c $(LAB_SRCS) build/revision.h
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(filter %.c,$^) $(LDLIBS) -o $@

$(LAB_WEB_JS): src/lab_web.c $(LAB_SRCS) $(SOURCE_HEADERS) build/revision.h
	@mkdir -p $(@D)
	emcc $(CPPFLAGS) -O2 -DPLATFORM_WEB $(filter %.c,$^) --no-entry -s MODULARIZE=1 -s EXPORT_ES6=1 -s EXPORT_NAME=createLearningLab -s ALLOW_MEMORY_GROWTH=1 -s STACK_SIZE=1048576 -s EXPORTED_RUNTIME_METHODS=ccall,UTF8ToString -o $@

$(LAB_WEB_WASM): $(LAB_WEB_JS)
	@test -f $@ || { rm -f $(LAB_WEB_JS); $(MAKE) $(LAB_WEB_JS); }
