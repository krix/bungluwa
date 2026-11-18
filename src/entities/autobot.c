#include "../main.h"

EDITOR_SIZE(8, 11);
EDITOR_COLOR(200, 50, 50);

#define AUTOBOT_SPEED 100.0f
#define AUTOBOT_EXHAUST_INTERVAL 0.04f

// badrobot.png: 39x13, 3 frames of 13x13
// Same sprite layout as player: idle=[0,1], walk=[0,1], jump=[2]
static anim_def_t *a_walk;
static float exhaust_timer;

static void load(void) {
	image_t *sh = image("assets/sprites/autobot.qoi");
	a_walk = anim_def(sh, vec2i(13, 13), 0.1, {0, 1});
}

static void init(entity_t *self) {
	self->size = vec2(8, 11);
	self->offset = vec2(3, 1);
	self->anim = anim(a_walk);
	self->physics = ENTITY_PHYSICS_ACTIVE;
	self->group = ENTITY_GROUP_ENEMY;
	self->check_against = ENTITY_GROUP_PLAYER;
	self->gravity = 1.0f;
	self->friction = vec2(0, 0);
	self->autobot.flip = false;
	self->autobot.speed = AUTOBOT_SPEED;
	self->vel.x = AUTOBOT_SPEED;
	exhaust_timer = 0;
}

static void settings(entity_t *self, json_t *def) {
	json_t *v;
	if ((v = json_value_for_key(def, "speed")))
		self->autobot.speed = (float)json_number(v);
	self->vel.x = self->autobot.speed;
}

static void update(entity_t *self) {
	entity_base_update(self);

	// Flip direction if we hit a wall (vel.x becomes 0 from collision)
	if (self->on_ground && fabsf(self->vel.x) < 1.0f) {
		self->autobot.flip = !self->autobot.flip;
		self->vel.x = self->autobot.flip ? -self->autobot.speed : self->autobot.speed;
	}

	// Edge detection: check if there's a tile below the robot in front of it
	if (self->on_ground && engine.collision_map) {
		float ahead_x = self->autobot.flip ? (self->pos.x - 1) : (self->pos.x + self->size.x + 1);
		int tile_below = map_tile_at_px(engine.collision_map, vec2(ahead_x, self->pos.y + self->size.y + 1));
		if (tile_below == 0) {
			self->autobot.flip = !self->autobot.flip;
			self->vel.x = self->autobot.flip ? -self->autobot.speed : self->autobot.speed;
		}
	}

	self->anim.flip_x = self->autobot.flip;

	// Exhaust smoke (same as player but always white)
	exhaust_timer -= engine.tick;
	if (exhaust_timer <= 0) {
		exhaust_timer = AUTOBOT_EXHAUST_INTERVAL;
		float ex = self->autobot.flip
			? self->pos.x + 9
			: self->pos.x;
		float ey = self->pos.y + 8;
		entity_t *p = entity_spawn(ENTITY_TYPE_PARTICLE, vec2(ex, ey));
		if (p) {
			p->anim = anim(anim_smoke);
			p->vel = vec2(rand_float(-10, 10), rand_float(-60, -30));
			p->physics = ENTITY_PHYSICS_MOVE;
			p->gravity = 0;
			p->particle.life_time = 0.3f;
			p->particle.fade_time = 0.3f;
		}
	}
}

static void collide(entity_t *self, vec2_t normal, trace_t *trace) {
	// Flip on horizontal wall collision
	if (normal.x != 0) {
		self->autobot.flip = (normal.x > 0);
		self->vel.x = self->autobot.flip ? -self->autobot.speed : self->autobot.speed;
	}
}

static void touch(entity_t *self, entity_t *other) {
	if (other->type == ENTITY_TYPE_PLAYER) {
		entity_damage(other, self, 1);
	}
}

entity_vtab_t entity_vtab_autobot = {
	.load = load,
	.init = init,
	.settings = settings,
	.update = update,
	.collide = collide,
	.touch = touch,
};
