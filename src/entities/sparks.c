#include "../main.h"

EDITOR_SIZE(16, 16);
EDITOR_COLOR(255, 255, 100);

// Sparks: invisible emitter that produces small random burst explosions.
// Waits a random interval, then spawns a cluster of particles that fly
// outward in all directions and fade out. No collision with world.

static void init(entity_t *self) {
	self->size = vec2(16, 16);
	self->physics = ENTITY_PHYSICS_NONE;
	self->sparks.timer = rand_float(0.5f, 2.0f);
}

static void update(entity_t *self) {
	self->sparks.timer -= engine.tick;
	if (self->sparks.timer <= 0) {
		// Random delay until next burst
		self->sparks.timer = rand_float(1.0f, 4.0f);

		// Spawn a burst of 3-6 particles
		int count = 3 + (rand() % 4);
		for (int i = 0; i < count; i++) {
			float px = self->pos.x + 8;
			float py = self->pos.y + 8;
			entity_t *e = entity_spawn(ENTITY_TYPE_PARTICLE, vec2(px, py));
			if (e) {
				e->anim = anim(anim_sparks);
				e->anim.tile_offset = (uint16_t)(rand() % 5);
				e->vel = vec2(rand_float(-40, 40), rand_float(-60, 10));
				e->particle.life_time = 0.4f;
				e->particle.fade_time = 0.3f;
				e->particle.collide_world = false;
				e->gravity = 1.0f;
				e->size = vec2(2, 2);
			}
		}
	}
}

entity_vtab_t entity_vtab_sparks = {
	.init = init,
	.update = update,
};
