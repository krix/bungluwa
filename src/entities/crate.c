#include "../main.h"

EDITOR_SIZE(16, 16);
EDITOR_COLOR(160, 120, 60);

// crate.png: 64x16, 4 frames of 16x16
// Frame 0: idle (down), 1: action (down), 2: idle (up), 3: action (up)
#define CRATE_FALL_THRESHOLD 25
#define CRATE_MAX_VEL_Y 200.0f
#define CRATE_ACCEL 200.0f

static anim_def_t *a_idle_down, *a_action_down, *a_idle_up, *a_action_up;

static void load(void) {
  image_t *sh = image("assets/sprites/crate.qoi");
  a_idle_down = anim_def(sh, vec2i(16, 16), 1.0, {0});
  a_action_down = anim_def(sh, vec2i(16, 16), 0.1, {0, 1});
  a_idle_up = anim_def(sh, vec2i(16, 16), 1.0, {2});
  a_action_up = anim_def(sh, vec2i(16, 16), 0.1, {2, 3});
}

static void init(entity_t *self) {
  self->size = vec2(16, 16);
  self->anim = anim(a_idle_down);
  self->physics = ENTITY_PHYSICS_FIXED;
  self->group = ENTITY_GROUP_BREAKABLE;
  self->crate.crate_type = 0;
  self->crate.fall_threshold = CRATE_FALL_THRESHOLD;
  self->crate.anim_offset = 0;
  self->crate.crumbling = false;
  self->check_against = ENTITY_GROUP_BREAKABLE;
}

static void settings(entity_t *self, json_t *def) {
  json_t *v;
  if ((v = json_value_for_key(def, "crate_type")))
    self->crate.crate_type = (uint8_t)json_number(v);
  self->anim = anim(self->crate.crate_type == 1 ? a_idle_up : a_idle_down);
}

static bool player_on_top(entity_t *self) {
  entity_t *p = entity_by_ref(g.player);
  if (!p || p->player.dead)
    return false;
  if (p->vel.y < -50)
    return false;
  float pb = p->pos.y + p->size.y;
  float ct = self->pos.y;
  if (pb >= ct - 2 && pb <= ct + 4 && p->pos.x + p->size.x > self->pos.x + 1 &&
      p->pos.x < self->pos.x + self->size.x - 1) {
    return true;
  }
  return false;
}

static void collide(entity_t *self, vec2_t normal, trace_t *trace) {
  if (!self->crate.crumbling)
    return;
  if (!trace || !trace->tile)
    return;

  vec2_t center = vec2(self->pos.x + 8, self->pos.y + 8);
  spawn_smoke(center, 6);
  entity_kill(self);
}
static void touch(entity_t *self, entity_t *other) {
  if (self->crate.crumbling && other->type == ENTITY_TYPE_CRATE) {
    if (self->crate.crumbling || other->crate.crumbling) {
      vec2_t center = vec2(self->pos.x + 8, self->pos.y + 8);
      spawn_smoke(center, 6);
      entity_kill(self);
    }
  }
}

static void update(entity_t *self) {
  if (self->crate.crumbling) {
    self->vel.y = clamp(self->vel.y, -CRATE_MAX_VEL_Y, CRATE_MAX_VEL_Y);
    entity_base_update(self);

    // Check if we are crushing the player against the ceiling
    if (self->crate.crate_type == 1 && player_on_top(self)) {
      entity_t *p = entity_by_ref(g.player);
      if (p) {
        trace_t t = trace(engine.collision_map, p->pos, vec2(0, -1), p->size);
        if (t.tile) {
          vec2_t center = vec2(self->pos.x + 8, self->pos.y + 8);
          spawn_smoke(center, 6);
          entity_kill(self);
          return;
        }
      }
    }

    // Kill if off-screen (fallback)
    float screen_top = engine.viewport.y - 32;
    float screen_bot = engine.viewport.y + RENDER_HEIGHT + 32;
    if (self->pos.y < screen_top || self->pos.y > screen_bot) {
      entity_kill(self);
    }
    return;
  }

  if (player_on_top(self)) {
    if (self->crate.anim_offset == 0) {
      self->anim =
          anim(self->crate.crate_type == 1 ? a_action_up : a_action_down);
    }
    self->crate.anim_offset = 20;

    if (self->crate.fall_threshold > 0) {
      self->crate.fall_threshold--;
    }

    // Color tinting as threshold decreases
    int used = CRATE_FALL_THRESHOLD - self->crate.fall_threshold;
    int tint = used * 8;
    if (tint > 200)
      tint = 200;
    if (self->crate.crate_type == 0) {
      self->anim.color = rgba(255, 255 - tint, 255 - tint, 255);
    } else {
      self->anim.color = rgba(255 - tint, 255, 255 - tint, 255);
    }

    if (self->crate.fall_threshold == 0) {
      self->crate.crumbling = true;
      // Stay FIXED so the crate remains solid and carries the player
      self->gravity = 0;
      self->accel.y =
          (self->crate.crate_type == 0) ? CRATE_ACCEL : -CRATE_ACCEL;
    }
  } else if (self->crate.anim_offset > 0) {
    self->crate.anim_offset--;
    if (self->crate.anim_offset == 0) {
      self->anim = anim(self->crate.crate_type == 1 ? a_idle_up : a_idle_down);
      self->anim.color = rgba_white();
    }
  }
}

entity_vtab_t entity_vtab_crate = {
    .load = load,
    .init = init,
    .settings = settings,
    .update = update,
    .collide = collide,
    .touch = touch,
};
