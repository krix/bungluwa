// Unity build: includes all high_impact source into one compilation unit.
// This speeds up compilation and lets the compiler optimize across modules.

#include "high_impact.h"

// Order matters: render.c before platform.c (OpenGL loader interference)
#include "../high_impact/src/alloc.c"
#include "../high_impact/src/animation.c"
#include "../high_impact/src/camera.c"
#include "../high_impact/src/engine.c"
#include "../high_impact/src/entity.c"
#include "../high_impact/src/font.c"
#include "../high_impact/src/image.c"
#include "../high_impact/src/input.c"
#include "../high_impact/src/map.c"
#include "../high_impact/src/noise.c"
#include "../high_impact/src/render.c"
#include "../high_impact/src/platform.c"
#include "../high_impact/src/sound.c"
#include "../high_impact/src/trace.c"
#include "../high_impact/src/utils.c"
