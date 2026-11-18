#ifndef HIGH_IMPACT_H
#define HIGH_IMPACT_H

// -----------------------------------------------------------------------------
// High Impact configuration
//
// These defines overwrite the defaults set by high_impact. Resource limits are
// tuned for this game. The engine headers are pulled in from the submodule.

#define ALLOC_SIZE (32 * 1024 * 1024)
#define ALLOC_TEMP_OBJECTS_MAX 8

#define WINDOW_TITLE "Bungluwa"
#define WINDOW_WIDTH (320 * 2)
#define WINDOW_HEIGHT (240 * 2)

#define GAME_VENDOR "bungluwa"
#define GAME_NAME "bungluwa"

#define RENDER_WIDTH 320
#define RENDER_HEIGHT 240
#define RENDER_RESIZE_MODE RENDER_RESIZE_NONE
#define RENDER_SCALE_MODE RENDER_SCALE_DISCRETE

#define ENTITIES_MAX 512
#define ENTITY_MAX_SIZE 32
#define ENTITY_MIN_BOUNCE_VELOCITY 10
#define ENTITY_SWEEP_AXIS y

#define SOUND_MAX_UNCOMPRESSED_SAMPLES (64 * 1024)
#define SOUND_MAX_SOURCES 64
#define SOUND_MAX_NODES 256

#define IMAGE_MAX_SOURCES 64

#define RENDER_ATLAS_SIZE 128
#define RENDER_ATLAS_GRID 16
#define RENDER_ATLAS_BORDER 0
#define RENDER_BUFFER_CAPACITY 2048
#define RENDER_TEXTURES_MAX 64

#define ENGINE_MAX_TICK 0.1
#define ENGINE_MAX_BACKGROUND_MAPS 4

#include "../high_impact/src/alloc.h"
#include "../high_impact/src/animation.h"
#include "../high_impact/src/camera.h"
#include "../high_impact/src/entity_def.h"
#include "../high_impact/src/font.h"
#include "../high_impact/src/engine.h"
#include "../high_impact/src/image.h"
#include "../high_impact/src/input.h"
#include "../high_impact/src/map.h"
#include "../high_impact/src/noise.h"
#include "../high_impact/src/platform.h"
#include "../high_impact/src/render.h"
#include "../high_impact/src/sound.h"
#include "../high_impact/src/trace.h"
#include "../high_impact/src/types.h"
#include "../high_impact/src/utils.h"

// Careful: before we can include "../high_impact/src/entity.h", we need to set
// up our entity types and call ENTITY_DEFINE().


// -----------------------------------------------------------------------------
// Entity types (X-macro)

#define ENTITY_TYPES(TYPE) \
	TYPE(ENTITY_TYPE_PLAYER,         player) \
	TYPE(ENTITY_TYPE_BULB,           bulb) \
	TYPE(ENTITY_TYPE_CRATE,          crate) \
	TYPE(ENTITY_TYPE_GHOST_CRATE,    ghost_crate) \
	TYPE(ENTITY_TYPE_GHOST_BULB,     ghost_bulb) \
	TYPE(ENTITY_TYPE_EXIT_DOOR,      exit_door) \
	TYPE(ENTITY_TYPE_SPIKES,         spikes) \
	TYPE(ENTITY_TYPE_SPIKEBALL,      spikeball) \
	TYPE(ENTITY_TYPE_AUTOBOT,        autobot) \
	TYPE(ENTITY_TYPE_LASER_BARRIER,  laser_barrier) \
	TYPE(ENTITY_TYPE_ALARMLIGHT,     alarmlight) \
	TYPE(ENTITY_TYPE_BUSTER_EXTRA,   buster_extra) \
	TYPE(ENTITY_TYPE_PARTICLE,       particle) \
	TYPE(ENTITY_TYPE_FLOATING_TEXT,  floating_text) \
	TYPE(ENTITY_TYPE_TWINKLE,        twinkle) \
	TYPE(ENTITY_TYPE_SPARKS,         sparks)


// -----------------------------------------------------------------------------
// Per-entity-type scratch data (union members inside entity_t)

ENTITY_DEFINE(
	union {
		struct {
			float jump_charge;
			bool can_jump;
			bool flip;
			float power;
			bool dead;
			float death_timer;
			int extras;
			float exhaust_timer;
			bool is_ghost;
			uint32_t playback_index;
			float playback_tick_sum;
		} player;

		struct {
			float spawn_y;
			float bob_t;
			bool collected;
		} bulb;

		struct {
			uint8_t crate_type; // 0=down, 1=up
			int fall_threshold; // frames until crumble (starts at 25)
			int anim_offset;    // visual shake timer
			bool crumbling;     // currently falling
		} crate;

		struct {
			uint8_t crate_type;  // 0=down, 1=up (mirrored from real crate)
			int     fall_threshold;
			bool    crumbling;
		} ghost_crate;

		struct {
			uint8_t dummy;
		} ghost_bulb;

		struct {
			bool open;
			bool used;
			float timer;
		} exit_door;

		struct {
			uint8_t dir; // 0=up, 1=down, 2=left, 3=right
		} spikes;

		struct {
			uint8_t dir; // 0=up, 1=down, 2=left, 3=right
			float speed;
		} spikeball;

		struct {
			bool flip;
			float speed;
		} autobot;

		struct {
			uint8_t orient; // 0=down, 1=up, 2=left, 3=right (AS3 convention)
			bool active;
			float timer;
			float on_time;
			float off_time;
		} laser_barrier;

		struct {
			bool triggered;
			float timer;
		} alarmlight;

		struct {
			uint8_t dummy;
		} buster_extra;

		struct {
			float life_time;
			float fade_time;
			rgba_t base_color;  // base color (default white, preserved during fade)
			bool collide_world; // if true, use ENTITY_PHYSICS_ACTIVE (collide with tiles)
		} particle;

		struct {
			char text[32];
			float life;
		} floating_text;

		struct {
			float life;
		} twinkle;

		struct {
			float timer;
		} sparks;
	};
)


// -----------------------------------------------------------------------------
// Entity messages

typedef enum {
	EM_INVALID,
	MSG_NONE = EM_INVALID,
	MSG_ALARM_TRIGGERED,
} entity_message_t;

// Now that we have all the prerequisites, we can include entity.h
#include "../high_impact/src/entity.h"

#endif
