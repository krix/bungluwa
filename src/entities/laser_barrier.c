#include "../main.h"
#include <stdlib.h>

EDITOR_SIZE(16, 16);
EDITOR_COLOR(255, 0, 0);

// AS3: LaserBarrier is a ONE-WAY SOLID WALL, not a lethal enemy.
// It blocks movement from one direction only:
//   orient 0 (down):  blocks passage from below
//   orient 1 (up):    blocks passage from above
//   orient 2 (left):  blocks passage from left
//   orient 3 (right): blocks passage from right
// Always active. Rendered with additive blending + green TV noise force field.

// static anim_def_t *a_up, *a_down, *a_left, *a_right;

static void load(void) {
  // a_up    = anim_def(image("assets/sprites/laser_up.qoi"),    vec2i(16, 16),
  // 0.08, {0, 1, 2, 3, 2, 1}); a_down  =
  // anim_def(image("assets/sprites/laser_down.qoi"),  vec2i(16, 16), 0.08, {0,
  // 1, 2, 3, 2, 1}); a_left  = anim_def(image("assets/sprites/laser_left.qoi"),
  // vec2i(16, 16), 0.08, {0, 1, 2, 3, 2, 1}); a_right =
  // anim_def(image("assets/sprites/laser_right.qoi"), vec2i(16, 16), 0.08, {0,
  // 1, 2, 3, 2, 1});
}

/*static anim_def_t *laser_anim_for_dir(uint8_t dir) {
        switch (dir) {
                case 0: return a_down;
                case 1: return a_up;
                case 2: return a_left;
                case 3: return a_right;
                default: return a_down;
        }
}*/

static void init(entity_t *self) {
  self->size = vec2(16, 16);
  self->physics = ENTITY_PHYSICS_NONE;
  self->group = ENTITY_GROUP_NONE;
  self->check_against = ENTITY_GROUP_PLAYER;
  self->laser_barrier.orient = 0;
  self->laser_barrier.active = true;
  self->laser_barrier.timer = 0;
  // self->anim = anim(a_down);
  self->anim.color = rgba(255, 255, 255, 153);
}

static void settings(entity_t *self, json_t *def) {
  json_t *v;
  if ((v = json_value_for_key(def, "dir"))) {
    self->laser_barrier.orient = (uint8_t)json_number(v);
  }
  if ((v = json_value_for_key(def, "orient"))) {
    self->laser_barrier.orient = (uint8_t)json_number(v);
  }

  // self->anim = anim(laser_anim_for_dir(self->laser_barrier.orient));
  // self->anim.color = rgba(255, 255, 255, 153);
}

static void update(entity_t *self) { self->laser_barrier.timer += engine.tick; }

static void draw(entity_t *self, vec2_t viewport) {
  // Draw the laser sprite with additive blending
  render_set_blend_mode(RENDER_BLEND_LIGHTER);
  entity_base_draw(self, viewport);

  // Green TV noise force field with gradient in the blocking direction.
  // The field fades from transparent on the "passable" side to opaque on
  // the "blocking" side so the player can see which way is blocked.
  //   orient 0 (down):  blocks from below  → transparent at top, opaque at
  //   bottom orient 1 (up):    blocks from above  → transparent at bottom,
  //   opaque at top orient 2 (left):  blocks from left   → transparent at
  //   right, opaque at left orient 3 (right): blocks from right  → transparent
  //   at left, opaque at right
  vec2_t s = vec2_sub(self->pos, viewport);
  int sx = (int)s.x;
  int sy = (int)s.y;
  uint8_t orient = self->laser_barrier.orient;

  for (int py = 0; py < 16; py += 2) {
    for (int px = 0; px < 16; px += 2) {
      if ((rand() % 3) == 0) {
        // Compute gradient factor (0.0 = transparent, 1.0 = opaque)
        float t;
        switch (orient) {
        case 0:
          t = (float)py / 14.0f;
          break; // top→bottom
        case 1:
          t = 1.0f - (float)py / 14.0f;
          break; // bottom→top
        case 2:
          t = 1.0f - (float)px / 14.0f;
          break; // right→left
        case 3:
          t = (float)px / 14.0f;
          break; // left→right
        default:
          t = 1.0f;
          break;
        }
        uint8_t g = 80 + (rand() % 120);
        uint8_t a = (uint8_t)((40 + (rand() % 60)) * t);
        if (a < 4)
          continue;
        render_draw(vec2(sx + px, sy + py), vec2(2, 2), RENDER_NO_TEXTURE,
                    vec2(0, 0), vec2(0, 0), rgba(0, g, 0, a));
      }
    }
  }

  render_set_blend_mode(RENDER_BLEND_NORMAL);
}

static void touch(entity_t *self, entity_t *other) {
  if (other->type != ENTITY_TYPE_PLAYER)
    return;

  // One-way wall: allow passing through from the "passable" side, but block
  // from the "blocking" side. We check if the player's previous position
  // (computed from velocity) was already on the blocking side.
  float overlap_x =
      fminf(self->pos.x + self->size.x, other->pos.x + other->size.x) -
      fmaxf(self->pos.x, other->pos.x);
  float overlap_y =
      fminf(self->pos.y + self->size.y, other->pos.y + other->size.y) -
      fmaxf(self->pos.y, other->pos.y);

  float min_overlap = 4.0f;

  switch (self->laser_barrier.orient) {
  case 0: // blocks from below
    if (overlap_x > min_overlap && other->vel.y <= 0) {
      float old_top = other->pos.y - other->vel.y * engine.tick;
      if (old_top >= self->pos.y + self->size.y) {
        other->pos.y = self->pos.y + self->size.y;
        other->vel.y = 0;
      }
    }
    break;
  case 1: // blocks from above
    if (overlap_x > min_overlap && other->vel.y >= 0) {
      float old_bottom =
          (other->pos.y + other->size.y) - other->vel.y * engine.tick;
      if (old_bottom <= self->pos.y) {
        other->pos.y = self->pos.y - other->size.y;
        other->vel.y = 0;
        other->on_ground = true;
      }
    }
    break;
  case 2: // blocks from left
    if (overlap_y > min_overlap && other->vel.x >= 0) {
      float old_right =
          (other->pos.x + other->size.x) - other->vel.x * engine.tick;
      if (old_right <= self->pos.x) {
        other->pos.x = self->pos.x - other->size.x;
        other->vel.x = 0;
      }
    }
    break;
  case 3: // blocks from right
    if (overlap_y > min_overlap && other->vel.x <= 0) {
      float old_left = other->pos.x - other->vel.x * engine.tick;
      if (old_left >= self->pos.x + self->size.x) {
        other->pos.x = self->pos.x + self->size.x;
        other->vel.x = 0;
      }
    }
    break;
  }
}

entity_vtab_t entity_vtab_laser_barrier = {
    .load = load,
    .init = init,
    .settings = settings,
    .update = update,
    .draw = draw,
    .touch = touch,
};
