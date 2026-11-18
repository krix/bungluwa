#include "../main.h"

EDITOR_SIZE(16, 8);
EDITOR_COLOR(255, 80, 80);

// Each direction is a separate 16x16 PNG/QOI
static anim_def_t *a_up, *a_down, *a_left, *a_right;

static void load(void) {
  a_up =
      anim_def(image("assets/sprites/spikes_up.qoi"), vec2i(16, 16), 1.0, {0});
  a_down = anim_def(image("assets/sprites/spikes_down.qoi"), vec2i(16, 16), 1.0,
                    {0});
  a_left = anim_def(image("assets/sprites/spikes_left.qoi"), vec2i(16, 16), 1.0,
                    {0});
  a_right = anim_def(image("assets/sprites/spikes_right.qoi"), vec2i(16, 16),
                     1.0, {0});
}

static void init(entity_t *self) {
  self->size = vec2(16, 16);
  self->physics = ENTITY_PHYSICS_NONE;
  self->group = ENTITY_GROUP_ENEMY;
  self->check_against = ENTITY_GROUP_PLAYER;
  self->spikes.dir = 0;
  self->anim = anim(a_up);
}

static void settings(entity_t *self, json_t *def) {
  json_t *dir = json_value_for_key(def, "dir");
  if (dir) {
    self->spikes.dir = (uint8_t)json_number(dir);
  }

  // Select correct sprite and collision shape for direction
  switch (self->spikes.dir) {
  case 0: // Up: 10px height from bottom
    self->anim = anim(a_up);
    self->size = vec2(16, 10);
    self->offset = vec2(0, 6);
    self->pos.y += 6;
    break;
  case 1: // Down: 10px height from top
    self->anim = anim(a_down);
    self->size = vec2(16, 10);
    self->offset = vec2(0, 0);
    break;
  case 2: // Left: 10px width from right
    self->anim = anim(a_left);
    self->size = vec2(10, 16);
    self->offset = vec2(6, 0);
    self->pos.x += 6;
    break;
  case 3: // Right: 10px width from left
    self->anim = anim(a_right);
    self->size = vec2(10, 16);
    self->offset = vec2(0, 0);
    break;
  }
}

static void touch(entity_t *self, entity_t *other) {
  if (other->type == ENTITY_TYPE_PLAYER) {
    entity_damage(other, self, 1);
  }
}

entity_vtab_t entity_vtab_spikes = {
    .load = load,
    .init = init,
    .settings = settings,
    .touch = touch,
};
