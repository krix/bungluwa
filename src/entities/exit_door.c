#include "../main.h"

EDITOR_SIZE(4, 4);
EDITOR_COLOR(100, 255, 100);

// AS3 Exit: 16x16 sprite, collision 4x4 offset(6,6)
// Always spawns particles from the constructor (no "open" gate).
// Touching it immediately triggers level complete (no bulb-count check).
static anim_def_t *a_exit;

static void load(void) {
	image_t *sh = image("assets/sprites/exit.qoi");
	a_exit = anim_def(sh, vec2i(16, 16), 1.0, {0});
}

static void init(entity_t *self) {
	// AS3: width=4, height=4, offset(6,6) — but for reliable touch detection
	// use the full sprite size. Offset 0 so sprite aligns with JSON position.
	self->size = vec2(16, 16);
	self->offset = vec2(0, 0);
	self->anim = anim(a_exit);
	self->physics = ENTITY_PHYSICS_NONE;
	self->group = ENTITY_GROUP_ITEM;
	self->check_against = ENTITY_GROUP_PLAYER;
	self->exit_door.open = true;  // always active
	self->exit_door.used = false;
}

static void update(entity_t *self) {
	// Continuously spawn exit particles (AS3: emitter starts in constructor)
	if (!self->exit_door.used) {
		self->exit_door.timer -= engine.tick;
		if (self->exit_door.timer <= 0) {
			self->exit_door.timer = 0.025f; // AS3: delay=0.025
			spawn_exit_particles(self);
		}
	}
}

static void touch(entity_t *self, entity_t *other) {
	if (!self->exit_door.used && other->type == ENTITY_TYPE_PLAYER) {
		self->exit_door.used = true;
		game_player_exited();
	}
}

entity_vtab_t entity_vtab_exit_door = {
	.load = load,
	.init = init,
	.update = update,
	.touch = touch,
};
