# Bungluwa Makefile — follows the high_biolab unity-build pattern.
# All engine .c files are compiled via src/high_impact.c (unity build).
# Game source files are compiled individually from src/.

CC ?= gcc
EMCC ?= emcc
UNAME_S := $(shell uname -s 2>/dev/null || echo Windows)
RENDER ?= GL
DEBUG ?= false

L_FLAGS ?= -lm
C_FLAGS ?= -std=gnu99 -Wall -Wstrict-prototypes -Wno-unused-variable -Wno-unused-function -Isrc

# All .c files under src/ (including high_impact.c unity build, scenes, entities)
GAME_SRC  := $(wildcard src/*.c) $(wildcard src/scenes/*.c) $(wildcard src/entities/*.c)
BUILD_DIR  = build
OBJ_DIR    = build/obj
BIN        = build/bungluwa

ASSETS_SRC_DIR  = assets
ASSETS_BUILD_DIR = $(BUILD_DIR)/$(ASSETS_SRC_DIR)

QOICONV = high_impact/tools/qoiconv
QOACONV = high_impact/tools/qoaconv

ifeq ($(DEBUG), true)
	C_FLAGS := $(C_FLAGS) -g
else
	C_FLAGS := $(C_FLAGS) -O3
endif


# Renderer
ifeq ($(RENDER), GL)
	C_FLAGS := $(C_FLAGS) -DRENDER_GL
else ifeq ($(RENDER), SOFTWARE)
	C_FLAGS := $(C_FLAGS) -DRENDER_SOFTWARE
else ifeq ($(RENDER), METAL)
	C_FLAGS := $(C_FLAGS) -DRENDER_METAL
else
	$(error Unknown RENDER backend)
endif


# macOS
ifeq ($(UNAME_S), Darwin)
	C_FLAGS += -x objective-c -D_THREAD_SAFE -w
	L_FLAGS += -framework Foundation -framework Cocoa -framework QuartzCore -framework AudioToolbox
	ifeq ($(RENDER), GL)
		L_FLAGS += -framework OpenGL
	else ifeq ($(RENDER), METAL)
		C_FLAGS += -fobjc-arc
		L_FLAGS += -framework Metal -framework MetalKit
	endif

# Linux
else ifeq ($(UNAME_S), Linux)
	ifeq ($(RENDER), GL)
		L_FLAGS += -lOpenGL
	endif
	L_FLAGS += -lX11 -lXcursor -pthread -lXi -ldl -lasound

# Windows (MSYS2/MinGW)
else ifeq ($(shell uname -o 2>/dev/null), Msys)
	ifeq ($(RENDER), GL)
		L_FLAGS += -lopengl32
	endif
	L_FLAGS += -pthread -lkernel32 -luser32 -lshell32 -lgdi32 -lole32

# Windows (non-MSYS, e.g. plain cmd)
else ifeq ($(OS), Windows_NT)
	ifeq ($(RENDER), GL)
		L_FLAGS += -lopengl32
	endif
	L_FLAGS += -lkernel32 -luser32 -lshell32 -lgdi32 -lole32
	BIN = build/bungluwa.exe
endif


# Default target
.DEFAULT_GOAL := sokol

# Sokol platform
SOKOL_OBJ  = $(patsubst %.c, $(OBJ_DIR)/sokol/%.o, $(GAME_SRC))
SOKOL_DEPS = $(patsubst %.c, $(OBJ_DIR)/sokol/%.d, $(GAME_SRC))

sokol: $(SOKOL_OBJ)
	@mkdir -p $(BUILD_DIR)
	$(CC) $^ -o $(BIN) $(L_FLAGS)

$(OBJ_DIR)/sokol/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(C_FLAGS) -DPLATFORM_SOKOL -MMD -MP -c $< -o $@

-include $(SOKOL_DEPS)


# Asset tools
qoiconv: $(QOICONV)
$(QOICONV): high_impact/libs/qoiconv.c high_impact/libs/stb_image.h high_impact/libs/stb_image_write.h high_impact/libs/qoi.h
	@mkdir -p $(@D)
	$(CC) -std=c99 -O3 high_impact/libs/qoiconv.c -o $(QOICONV)

qoaconv: $(QOACONV)
$(QOACONV): high_impact/libs/qoaconv.c high_impact/libs/qoa.h
	@mkdir -p $(@D)
	$(CC) -std=c99 -O3 high_impact/libs/qoaconv.c -o $(QOACONV) -lm


clean:
	rm -rf $(OBJ_DIR) $(BUILD_DIR)

.PHONY: clean sokol qoiconv qoaconv
