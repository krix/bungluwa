#include "../main.h"

EDITOR_IGNORE(true);

static void init(entity_t *self) {
	self->physics = ENTITY_PHYSICS_NONE;
	self->twinkle.life = 0.4f;
	self->draw_order = 90;
	// anim set by spawn_twinkle()
}

static void update(entity_t *self) {
	self->twinkle.life -= engine.tick;
	if (self->twinkle.life <= 0 || anim_looped(&self->anim)) {
		entity_kill(self);
	}
}

entity_vtab_t entity_vtab_twinkle = {
	.init = init,
	.update = update,
};
