#include "../main.h"

EDITOR_SIZE(8, 8);
EDITOR_COLOR(255, 120, 0);

// spikeball.png: 10x10 single frame, collision 8x8 offset(1,1)
// AS3 SpikeBall: moves in one direction, bounces off walls/crates/spikes.
// Rotates visually. FlxSprite direction: UP=0, DOWN=1, LEFT=2, RIGHT=3
#define SB_SPEED 100.0f

static anim_def_t *a_ball;

static void load(void) {
	image_t *sh = image("assets/sprites/spikeball.qoi");
	a_ball = anim_def(sh, vec2i(10, 10), 1.0, {0});
	a_ball->pivot = vec2(5, 5); // rotate around center
}

static void apply_direction(entity_t *self) {
	// Flixel: LEFT=0, RIGHT=1, UP=2, DOWN=3
	switch (self->spikeball.dir) {
		case 0: self->vel = vec2(-SB_SPEED, 0); break; // LEFT
		case 1: self->vel = vec2( SB_SPEED, 0); break; // RIGHT
		case 2: self->vel = vec2(0, -SB_SPEED); break; // UP
		case 3: self->vel = vec2(0,  SB_SPEED); break; // DOWN
	}
}

static void init(entity_t *self) {
	self->size = vec2(8, 8);
	self->offset = vec2(1, 1);
	self->anim = anim(a_ball);
	self->physics = ENTITY_PHYSICS_ACTIVE;
	self->group = ENTITY_GROUP_ENEMY;
	self->check_against = ENTITY_GROUP_PLAYER;
	self->gravity = 0;
	self->spikeball.dir = 1; // DOWN default
	self->spikeball.speed = SB_SPEED;
}

static void settings(entity_t *self, json_t *def) {
	json_t *v;
	if ((v = json_value_for_key(def, "dir")))
		self->spikeball.dir = (uint8_t)json_number(v);
	apply_direction(self);
}

static void update(entity_t *self) {
	// Reapply velocity each frame (tile collisions zero it)
	apply_direction(self);

	// Visual rotation (AS3: angle += 5 degrees per frame at 60fps)
	self->anim.rotation += (5.0f * M_PI / 180.0f) * engine.tick * 60.0f;
	if (self->anim.rotation >= 2.0f * M_PI) self->anim.rotation -= 2.0f * M_PI;

	entity_base_update(self);
}

static void collide(entity_t *self, vec2_t normal, trace_t *trace) {
	// Flixel: LEFT=0, RIGHT=1, UP=2, DOWN=3
	if (normal.x > 0 && self->spikeball.dir == 0) {
		self->spikeball.dir = 1; // LEFT -> RIGHT
	} else if (normal.x < 0 && self->spikeball.dir == 1) {
		self->spikeball.dir = 0; // RIGHT -> LEFT
	} else if (normal.y > 0 && self->spikeball.dir == 2) {
		self->spikeball.dir = 3; // UP -> DOWN
	} else if (normal.y < 0 && self->spikeball.dir == 3) {
		self->spikeball.dir = 2; // DOWN -> UP
	}
}

static void touch(entity_t *self, entity_t *other) {
	if (other->type == ENTITY_TYPE_PLAYER) {
		entity_damage(other, self, 1);
	}
}

entity_vtab_t entity_vtab_spikeball = {
	.load = load,
	.init = init,
	.settings = settings,
	.update = update,
	.collide = collide,
	.touch = touch,
};
