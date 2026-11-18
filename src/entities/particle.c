#include "../main.h"

EDITOR_IGNORE(true);

static void init(entity_t *self) {
	self->physics = ENTITY_PHYSICS_MOVE;
	self->gravity = 0.3f;
	self->particle.life_time = 0.5f;
	self->particle.fade_time = 0.5f;
	self->particle.base_color = rgba_white();
	self->particle.collide_world = false;
}

static void update(entity_t *self) {
	self->particle.life_time -= engine.tick;
	if (self->particle.life_time <= 0) {
		entity_kill(self);
		return;
	}

	// Apply collision mode (set after init by spawner)
	if (self->particle.collide_world && self->physics == ENTITY_PHYSICS_MOVE) {
		self->physics = ENTITY_PHYSICS_WORLD;
	}

	// Fade alpha based on base_color
	rgba_t c = self->particle.base_color;
	if (self->particle.fade_time > 0 && self->particle.life_time < self->particle.fade_time) {
		float alpha = self->particle.life_time / self->particle.fade_time;
		c.a = (uint8_t)(c.a * alpha);
	}
	self->anim.color = c;

	entity_base_update(self);
}

entity_vtab_t entity_vtab_particle = {
	.init = init,
	.update = update,
};
