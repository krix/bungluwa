#include "../main.h"

EDITOR_IGNORE(true);

// Invisible entity placed at each crate position during ghost playback.
// Has no physics and no sprite, so it is completely transparent to the real
// player. The ghost manually resolves collision against these entities
// (ghost_resolve_crate_collisions in player.c).
//
// Mirrors the crumbling behaviour of crate.c: once the ghost has been
// standing on top long enough the ghost crate activates and moves exactly
// like the corresponding real crate would — so the ghost can ride it and
// eventually lose its footing at the same moment.

#define GHOST_CRATE_FALL_THRESHOLD 25
#define GHOST_CRATE_MAX_VEL_Y      200.0f
#define GHOST_CRATE_ACCEL          200.0f

static entity_t *find_ghost(void) {
	entity_list_t players = entities_by_type(ENTITY_TYPE_PLAYER);
	for (uint32_t i = 0; i < players.len; i++) {
		entity_t *p = entity_by_ref(players.entities[i]);
		if (p && p->player.is_ghost)
			return p;
	}
	return NULL;
}

static bool ghost_on_top(entity_t *self) {
	entity_t *ghost = find_ghost();
	if (!ghost) return false;
	if (ghost->vel.y < -50) return false;
	float pb = ghost->pos.y + ghost->size.y;
	float ct = self->pos.y;
	return (pb >= ct - 2 && pb <= ct + 4 &&
	        ghost->pos.x + ghost->size.x > self->pos.x + 1 &&
	        ghost->pos.x < self->pos.x + self->size.x - 1);
}

static void init(entity_t *self) {
	self->size = vec2(16, 16);
	self->physics = ENTITY_PHYSICS_NONE;
	self->group = ENTITY_GROUP_NONE;
	self->check_against = ENTITY_GROUP_NONE;
	self->gravity = 0;
	self->ghost_crate.fall_threshold = GHOST_CRATE_FALL_THRESHOLD;
	self->ghost_crate.crumbling = false;
}

// Called when the ghost crate hits a tile while falling — destroy it so the
// ghost loses support just like it would on a real crumbled crate.
static void collide(entity_t *self, vec2_t normal, trace_t *trace) {
	if (!self->ghost_crate.crumbling) return;
	if (!trace || !trace->tile) return;
	entity_kill(self);
}

static void update(entity_t *self) {
	if (self->ghost_crate.crumbling) {
		self->vel.y = clamp(self->vel.y, -GHOST_CRATE_MAX_VEL_Y, GHOST_CRATE_MAX_VEL_Y);
		entity_base_update(self);

		// Fallback: kill if scrolled off screen
		float screen_top = engine.viewport.y - 32;
		float screen_bot = engine.viewport.y + RENDER_HEIGHT + 32;
		if (self->pos.y < screen_top || self->pos.y > screen_bot)
			entity_kill(self);
		return;
	}

	if (ghost_on_top(self)) {
		if (self->ghost_crate.fall_threshold > 0)
			self->ghost_crate.fall_threshold--;

		if (self->ghost_crate.fall_threshold == 0) {
			self->ghost_crate.crumbling = true;
			self->gravity = 0;
			self->accel.y = (self->ghost_crate.crate_type == 0)
			                    ? GHOST_CRATE_ACCEL
			                    : -GHOST_CRATE_ACCEL;
			// Switch to WORLD physics so entity_base_update can move the crate
			// while still colliding with tiles (but not blocking the real player).
			self->physics = ENTITY_PHYSICS_WORLD;
		}
	}
}

entity_vtab_t entity_vtab_ghost_crate = {
	.init    = init,
	.update  = update,
	.collide = collide,
};
