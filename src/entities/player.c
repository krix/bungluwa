#include "../main.h"

EDITOR_SIZE(8, 11);
EDITOR_RESIZE(false);
EDITOR_COLOR(96, 200, 255);

// Player constants matching AS3 Player.as
#define PLAYER_MAX_VEL_X 100.0f
#define PLAYER_MAX_VEL_Y 400.0f
#define PLAYER_GRAVITY 400.0f
#define PLAYER_RUN_ACCEL 500.0f // AS3: 300*(100*elapsed) ≈ 500 at 60fps
#define PLAYER_DRAG_X 400.0f    // AS3: drag.x = 400
#define PLAYER_MAX_POWER 100.0f
#define PLAYER_POWER_REGEN 140.0f // AS3: givePower(1.4) = 1.4*100*elapsed
#define PLAYER_POWER_DRAIN 120.0f // AS3: drainPower(1.4)
#define PLAYER_JUMP_MAX_T 0.30f   // max jump hold time

// bot.png: 39x13, 3 frames of 13x13.
// AS3 anims: idle=[0,1], walk=[0,1], jump=[2]
static anim_def_t *a_idle, *a_walk, *a_jump;

static void load(void) {
  image_t *sh = image("assets/sprites/player.qoi");
  a_idle = anim_def(sh, vec2i(13, 13), 1.0, {0, 1});
  a_walk = anim_def(sh, vec2i(13, 13), 0.1, {0, 1});
  a_jump = anim_def(sh, vec2i(13, 13), 1.0, {2});
}

static void init(entity_t *self) {
  self->size = vec2(8, 11);
  self->offset = vec2(3, 1);
  self->anim = anim(a_idle);
  self->physics = ENTITY_PHYSICS_ACTIVE;
  self->group = ENTITY_GROUP_PLAYER;
  self->check_against =
      ENTITY_GROUP_ENEMY | ENTITY_GROUP_PICKUP | ENTITY_GROUP_ITEM;
  self->gravity = 1.0f;
  self->friction = vec2(0, 0); // we handle drag manually (Flixel-style)
  self->player.power = PLAYER_MAX_POWER;
  self->player.can_jump = true;
  self->player.dead = false;
  self->player.death_timer = 0;
  self->player.jump_charge = 0;
  self->player.flip = false;
  self->player.extras = 0;

  // Only set g.player if it hasn't been set yet (the real player is spawned
  // first)
  if (!entity_by_ref(g.player)) {
    g.player = entity_ref(self);
  }
  engine.gravity = PLAYER_GRAVITY;

  if (self->player.is_ghost) {
    self->player.playback_index = 0;
    self->player.playback_tick_sum = 0;
    self->anim.color.a = 128; // Transparent ghost
    self->check_against = ENTITY_GROUP_NONE;
    self->group = ENTITY_GROUP_NONE;
    self->physics = ENTITY_PHYSICS_WORLD; // Only collide with tiles
    self->mass = 0.0f;                    // Ensure no impact on other entities
  }
}

static uint8_t player_get_input_mask(void) {
  uint8_t mask = 0;
  if (input_state(A_LEFT))
    mask |= (1 << A_LEFT);
  if (input_state(A_RIGHT))
    mask |= (1 << A_RIGHT);
  if (input_state(A_JUMP))
    mask |= (1 << A_JUMP);
  if (input_state(A_UP))
    mask |= (1 << A_UP);
  if (input_state(A_DOWN))
    mask |= (1 << A_DOWN);
  if (input_state(A_CONFIRM))
    mask |= (1 << A_CONFIRM);
  return mask;
}

// Resolve the ghost entity's overlap against all ghost-crate entities.
// Ghost crates have ENTITY_PHYSICS_NONE so they never block the real player;
// only the ghost uses this function to treat them as solid platforms.
static void ghost_resolve_crate_collisions(entity_t *self) {
  entity_list_t crates = entities_by_type(ENTITY_TYPE_GHOST_CRATE);
  for (uint32_t ci = 0; ci < crates.len; ci++) {
    entity_t *gc = entity_by_ref(crates.entities[ci]);
    if (!gc || !entity_is_touching(self, gc))
      continue;

    float overlap_x = self->pos.x < gc->pos.x
                          ? self->pos.x + self->size.x - gc->pos.x
                          : gc->pos.x + gc->size.x - self->pos.x;
    float overlap_y = self->pos.y < gc->pos.y
                          ? self->pos.y + self->size.y - gc->pos.y
                          : gc->pos.y + gc->size.y - self->pos.y;

    if (overlap_y > overlap_x) {
      // Separate on x-axis
      if (self->pos.x < gc->pos.x) {
        self->pos.x -= overlap_x;
      } else {
        self->pos.x += overlap_x;
      }
      self->vel.x = 0;
    } else {
      // Separate on y-axis
      if (self->pos.y < gc->pos.y) {
        // Ghost is above: push up and land on top of crate
        self->pos.y -= overlap_y;
        if (self->vel.y > 0) {
          self->vel.y = 0;
          self->on_ground = true;
        }
      } else {
        // Ghost is below: push down (hit from below)
        self->pos.y += overlap_y;
        if (self->vel.y < 0)
          self->vel.y = 0;
      }
    }
  }
}

// Give the ghost power for every ghost-bulb it is currently overlapping,
// then kill that ghost-bulb (mirrors game_collect_bulb's +20 power grant).
static void ghost_collect_bulbs(entity_t *self) {
  entity_list_t bulbs = entities_by_type(ENTITY_TYPE_GHOST_BULB);
  for (uint32_t i = 0; i < bulbs.len; i++) {
    entity_t *gb = entity_by_ref(bulbs.entities[i]);
    if (!gb || !entity_is_touching(self, gb))
      continue;
    self->player.power = fminf(PLAYER_MAX_POWER, self->player.power + 20.0f);
    entity_kill(gb);
  }
}

// If the ghost overlaps an exit door, despawn it — the ghost should not
// trigger game_player_exited() the way the real player does.
static void ghost_check_exit(entity_t *self) {
  entity_list_t exits = entities_by_type(ENTITY_TYPE_EXIT_DOOR);
  for (uint32_t i = 0; i < exits.len; i++) {
    entity_t *ex = entity_by_ref(exits.entities[i]);
    if (!ex || !entity_is_touching(self, ex))
      continue;
    entity_kill(self);
    return;
  }
}

static void update(entity_t *self) {
  // Death animation
  if (self->player.dead) {
    self->player.death_timer -= engine.tick;
    if (self->player.death_timer <= 0) {
      entity_kill(self);
      return;
    }
    entity_base_update(self);
    return;
  }

  if (input_pressed(A_KILL) && !self->player.is_ghost) {
    entity_damage(self, self, 1);
    return;
  }

  // Determine loop count and collect per-frame data.
  // For live play: 1 loop using current input + engine.tick.
  // For ghost: N loops using recorded input + recorded tick (matching JS).
  int loops = 0;
  int ghost_start_index = 0;

  if (self->player.is_ghost) {
    if (g.playback.count > 0) {
      ghost_start_index = self->player.playback_index;
      self->player.playback_tick_sum += engine.tick;

      // Count how many recorded frames fit within accumulated time
      float look_ahead_ticks = 0;
      int look_ahead = 0;
      while (self->player.playback_index + look_ahead < g.playback.count) {
        look_ahead_ticks += g.playback.frames[self->player.playback_index + look_ahead].tick;
        if (look_ahead_ticks > self->player.playback_tick_sum) break;
        look_ahead++;
        loops++;
        self->player.playback_tick_sum -= look_ahead_ticks;
        look_ahead_ticks = 0;
      }
    }
  } else {
    loops = 1;
  }

  float saved_tick = engine.tick;

  for (int loop_i = 0; loop_i < loops; loop_i++) {
    uint8_t mask = 0;

    if (self->player.is_ghost) {
      int fi = ghost_start_index + loop_i;
      mask = g.playback.frames[fi].input;
      engine.tick = g.playback.frames[fi].tick;
      self->player.playback_index = fi + 1;
    } else {
      mask = player_get_input_mask();
      ghost_record_frame(&g.recording, mask, engine.tick);
    }

    // Horizontal movement — Flixel-style: accel when input, drag when not
    bool moving = false;
    if (mask & (1 << A_LEFT)) {
      self->vel.x -= PLAYER_RUN_ACCEL * engine.tick;
      self->player.flip = true;
      moving = true;
    }
    if (mask & (1 << A_RIGHT)) {
      self->vel.x += PLAYER_RUN_ACCEL * engine.tick;
      self->player.flip = false;
      moving = true;
    }
    if (!moving) {
      float drag = PLAYER_DRAG_X * engine.tick;
      if (self->vel.x > 0) {
        self->vel.x = max(self->vel.x - drag, 0);
      } else if (self->vel.x < 0) {
        self->vel.x = min(self->vel.x + drag, 0);
      }
    }

    // Power regen on ground (not while jumping)
    if (self->on_ground && !(mask & (1 << A_JUMP))) {
      self->player.power =
          min(PLAYER_MAX_POWER,
              self->player.power + PLAYER_POWER_REGEN * engine.tick);
    }

    // Jump
    if ((mask & (1 << A_JUMP)) && self->player.can_jump &&
        self->player.power > 0) {
      if (self->player.jump_charge < PLAYER_JUMP_MAX_T) {
        if (self->player.jump_charge == 0) {
          if (!self->player.is_ghost) {
            sound_play(snd_jump);
          }
          g.total_jumps++;
        }
        self->player.jump_charge += engine.tick;
        float t = self->player.jump_charge;

        float base;
        if (t > 0.2f)
          base = 0.30f;
        else if (t > 0.1f)
          base = 0.24f;
        else
          base = 0.18f;

        self->vel.y = -PLAYER_GRAVITY * (base + self->player.power / 2000.0f);

        self->player.power =
            max(0, self->player.power - PLAYER_POWER_DRAIN * engine.tick);
      } else {
        self->player.can_jump = false;
      }
    }

    if (!(mask & (1 << A_JUMP))) {
      self->player.can_jump = true;
      self->player.jump_charge = 0;
    }

    if (self->on_ground && !(mask & (1 << A_JUMP))) {
      self->player.can_jump = true;
      self->player.jump_charge = 0;
    }

    // Clamp velocity
    self->vel.x = clamp(self->vel.x, -PLAYER_MAX_VEL_X, PLAYER_MAX_VEL_X);
    self->vel.y = clamp(self->vel.y, -PLAYER_MAX_VEL_Y, PLAYER_MAX_VEL_Y);

    self->accel = vec2(0, 0);
    entity_base_update(self);
  }

  // Restore engine tick
  engine.tick = saved_tick;

  // For the ghost: resolve collision against ghost crates so it doesn't fall
  // through platforms that the real player may have crumbled away.
  if (self->player.is_ghost) {
    ghost_resolve_crate_collisions(self);
    ghost_collect_bulbs(self);
    ghost_check_exit(self);
  }

  // Animation selection
  if (!self->on_ground) {
    if (self->anim.def != a_jump) {
      self->anim = anim(a_jump);
    }
  } else if (fabsf(self->vel.x) > 4) {
    if (self->anim.def != a_walk) {
      self->anim = anim(a_walk);
    }
  } else {
    if (self->anim.def != a_idle) {
      self->anim = anim(a_idle);
    }
  }

  self->anim.flip_x = self->player.flip;

  if (self->player.is_ghost) {
    self->anim.color.a = 128; // Less transparent
  }

  // Exhaust smoke from the robot's back
  self->player.exhaust_timer -= engine.tick;
  if (self->player.exhaust_timer <= 0 && !self->player.dead &&
      !self->player.is_ghost) {
    self->player.exhaust_timer = 0.04f;
    // Position: behind the robot (opposite of facing direction)
    float ex = self->player.flip
                   ? self->pos.x + 9 // facing left → exhaust on right
                   : self->pos.x;    // facing right → exhaust on left
    float ey = self->pos.y + 8;
    entity_t *p = entity_spawn(ENTITY_TYPE_PARTICLE, vec2(ex, ey));
    if (p) {
      p->anim = anim(anim_smoke);
      p->vel = vec2(rand_float(-10, 10), rand_float(-60, -30));
      p->physics = ENTITY_PHYSICS_MOVE;
      p->gravity = 0;
      p->particle.life_time = 0.3f;
      p->particle.fade_time = 0.3f;
      // Gradual darkening: white at full power, dark gray at 0
      uint8_t brightness =
          (uint8_t)(80 + (175.0f * self->player.power / PLAYER_MAX_POWER));
      p->particle.base_color = rgba(brightness, brightness, brightness, 255);
    }
  }
}

static void damage(entity_t *self, entity_t *other, float dmg) {
  if (self->player.dead || self->player.is_ghost)
    return;
  self->player.dead = true;
  self->player.death_timer = 1.5f;
  self->vel = vec2(0, 0);
  self->accel = vec2(0, 0);
  self->physics = ENTITY_PHYSICS_NONE;
  if (!self->player.is_ghost) {
    sound_play(snd_explode);
  }

  // Spawn gib particles (robot bursts into pieces)
  spawn_gibs(vec2(self->pos.x + 4, self->pos.y + 4));

  // Hide the player sprite during death
  self->anim.color = rgba(0, 0, 0, 0);

  engine.time_scale = 0.5;
}

static void kill(entity_t *self) {
  if (self->player.is_ghost)
    return;
  engine.time_scale = 1.0;
  game_player_died();
}

entity_vtab_t entity_vtab_player = {
    .load = load,
    .init = init,
    .update = update,
    .damage = damage,
    .kill = kill,
};
