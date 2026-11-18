#include "../main.h"

EDITOR_SIZE(8, 8);
EDITOR_COLOR(255, 255, 100);

static anim_def_t *a_bulb;

static void load(void) {
	image_t *sh = image("assets/sprites/bulb.qoi");
	a_bulb = anim_def(sh, vec2i(8, 8), 0.15, {0, 1, 2, 3, 2, 1});
}

static void init(entity_t *self) {
	self->size = vec2(8, 8);
	self->anim = anim(a_bulb);
	self->physics = ENTITY_PHYSICS_NONE;
	self->group = ENTITY_GROUP_PICKUP;
	self->check_against = ENTITY_GROUP_PLAYER;
	self->bulb.spawn_y = self->pos.y;
	self->bulb.bob_t = rand_float(0, 2 * M_PI);
	self->bulb.collected = false;
}

static void update(entity_t *self) {
	// Bobbing motion
	self->bulb.bob_t += engine.tick * 4.0f;
	self->pos.y = self->bulb.spawn_y + sinf(self->bulb.bob_t) * 1.5f;
}

static void touch(entity_t *self, entity_t *other) {
	if (self->bulb.collected) return;
	if (other->type == ENTITY_TYPE_PLAYER) {
		self->bulb.collected = true;
		game_collect_bulb(self, other);
		entity_kill(self);
	}
}

entity_vtab_t entity_vtab_bulb = {
	.load = load,
	.init = init,
	.update = update,
	.touch = touch,
};
