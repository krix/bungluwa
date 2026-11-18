#include "../main.h"

EDITOR_SIZE(21, 21);
EDITOR_COLOR(100, 200, 255);

// buster.png: 42x21, 2 frames of 21x21. AS3 anim: [0,1] at speed 6
static anim_def_t *a_extra;

static void load(void) {
	image_t *sh = image("assets/sprites/busterextra.qoi");
	a_extra = anim_def(sh, vec2i(21, 21), 0.15, {0, 1});
}

static void init(entity_t *self) {
	self->size = vec2(21, 21);
	self->anim = anim(a_extra);
	self->physics = ENTITY_PHYSICS_ACTIVE;
	self->group = ENTITY_GROUP_PICKUP;
	self->check_against = ENTITY_GROUP_PLAYER;
	self->gravity = 1.0f;
	self->vel.x = 80;
}

static void touch(entity_t *self, entity_t *other) {
	if (other->type == ENTITY_TYPE_PLAYER) {
		game_collect_extra(self, other);
		entity_kill(self);
	}
}

entity_vtab_t entity_vtab_buster_extra = {
	.load = load,
	.init = init,
	.touch = touch,
};
