#include "../main.h"

EDITOR_IGNORE(true);

// Invisible entity placed at each bulb position during ghost playback.
// Has no physics and no sprite, so it is completely transparent to the real
// player. The ghost player manually checks for overlap (ghost_collect_bulbs
// in player.c) and, upon touching one, receives +20 power — identical to
// the power the real player received when collecting that bulb during
// recording.

static void init(entity_t *self) {
	self->size = vec2(8, 8);
	self->physics = ENTITY_PHYSICS_NONE;
	self->group = ENTITY_GROUP_NONE;
	self->check_against = ENTITY_GROUP_NONE;
	self->gravity = 0;
}

entity_vtab_t entity_vtab_ghost_bulb = {
	.init = init,
};
