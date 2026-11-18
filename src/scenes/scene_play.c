#include "../main.h"

// The main gameplay scene — loads level, runs entities, HUD, camera, scoring.

static image_t *light_tex;
static image_t *bg_img;
static image_t *hud_bg;
static image_t *battery_back;
static image_t *battery_hull;
static bool paused;
static int pause_selection;
bool scene_play_game_over;
float scene_play_game_over_timer;
static int gameover_selection;
static bool time_warning_shown;
static rgba_t bg_tint; // per-level background tint color

static const char *gameover_messages[] = {
    "You don't have to collect all the bulbs...",
    "Get used to the controls!",
    "You should try to beat the highscore later...",
    "Never give up! You can do it! :)",
    "Oh no, not again",
    "Nevermind...",
    "Please don't die!",
    "Practice practice practice...",
    "Yes, it is hard. It is very hard!",
    "No, it is not time for a pause, yet :)",
    "You can't possibly have so much bad luck!",
    "It is not easy, no it really isn't!",
    "Learn to control your jumping power!"
};
int gameover_msg_idx = 0;

static void init(void) {
  // Reset per-level state
  g.level_time = 0;
  g.bulbs_collected = 0;
  g.score = 0;
  g.deaths = 0;
  g.combo_count = 0;
  g.combo_window = 0;
  g.alarm_time = 0;
  g.extras = 0;
  g.popup_count = 0;
  g.player = entity_ref_none();
  paused = false;
  pause_selection = 0;
  gameover_selection = 0;
  scene_play_game_over = false;
  scene_play_game_over_timer = 0;
  time_warning_shown = false;

  // Load the level (spawns entities and sets up maps)
  engine_load_level((char *)level_table[g.level_index].path);

  // Ghost initialization
  ghost_reset(&g.recording);
  ghost_load(g.level_index);

  // Find the player's spawn position to spawn the ghost there too
  entity_t *player_ent = entity_by_ref(g.player);
  if (player_ent && g.playback.count > 0) {
    entity_t *ghost = entity_spawn(ENTITY_TYPE_PLAYER, player_ent->pos);
    if (ghost) {
      ghost->player.is_ghost = true;

      // Re-apply ghost settings since they were skipped during init
      // (is_ghost was false during entity_spawn -> init)
      ghost->player.playback_index = 0;
      ghost->player.playback_tick_sum = 0;
      ghost->anim.color.a = 128;
      ghost->check_against = ENTITY_GROUP_NONE;
      ghost->group = ENTITY_GROUP_NONE;
      ghost->physics = ENTITY_PHYSICS_WORLD;
      ghost->mass = 0.0f;
    }

    // Spawn invisible ghost crates at every real crate position.
    // These persist even if the real crates crumble, giving the ghost solid
    // platforms to land on throughout the entire playback.
    entity_list_t crates = entities_by_type(ENTITY_TYPE_CRATE);
    for (uint32_t i = 0; i < crates.len; i++) {
      entity_t *c = entity_by_ref(crates.entities[i]);
      if (c) {
        entity_t *gc = entity_spawn(ENTITY_TYPE_GHOST_CRATE, c->pos);
        if (gc) {
          gc->ghost_crate.crate_type = c->crate.crate_type;
        }
      }
    }

    // Spawn invisible ghost bulbs at every real bulb position.
    // When the ghost overlaps one it receives +20 power, keeping its energy
    // curve identical to the recorded run.
    entity_list_t bulbs = entities_by_type(ENTITY_TYPE_BULB);
    for (uint32_t i = 0; i < bulbs.len; i++) {
      entity_t *b = entity_by_ref(bulbs.entities[i]);
      if (b) {
        entity_spawn(ENTITY_TYPE_GHOST_BULB, vec2(b->pos.x, b->bulb.spawn_y));
      }
    }
  }

  // Camera
  memset(&g.camera, 0, sizeof(g.camera));
  g.camera.speed = 5.0f;
  g.camera.deadzone = vec2(40, 40);
  g.camera.min_vel = 5.0f;
  g.camera.snap_to_platform = true;

  // Music — pick a random track for this level
  g.music_play_index = rand() % 4;
  sound_unpause(g.music_play[g.music_play_index]);

  // Tutorial messages for level 1 (shown before the generic direction hint)
  if (g.level_index == 0) {
    spawn_popup("Welcome to BUNGLUWA!");
    spawn_popup("Move around with Arrow Keys.");
    spawn_popup("Jump by pressing SPACE.");
    spawn_popup("You can jump in mid-air!");
    spawn_popup("Collect bulbs for energy.");
    spawn_popup("Now try to reach the top.");
  } else {
    // Level intro message for other levels
    switch (g.cam_mode) {
    case CAM_SCROLL_UP:
      spawn_popup("Get to the top!");
      break;
    case CAM_SCROLL_DOWN:
      spawn_popup("Get to the bottom!");
      break;
    case CAM_FREE:
      spawn_popup("Get to the exit!");
      break;
    }
  }

  // Load HUD and background images (always reload — images are freed on scene
  // transition)
  light_tex = image("assets/sprites/light.qoi");
  bg_img = image("assets/sprites/background.qoi");
  hud_bg = image("assets/sprites/hudbackground.qoi");
  battery_back = image("assets/sprites/battery_back.qoi");
  battery_hull = image("assets/sprites/battery.qoi");

  // AS3: bg.color = FlxU.random() * 0x222222 + 0x888888
  // Each RGB channel random 136-170
  bg_tint =
      rgba(136 + (rand() % 35), 136 + (rand() % 35), 136 + (rand() % 35), 255);
}

static void update(void) {
  // Game over screen — wait for input
  if (scene_play_game_over) {
    scene_play_game_over_timer += engine.tick;
    if (scene_play_game_over_timer > 1.0f) {
      if (input_pressed(A_UP) || input_pressed(A_DOWN)) {
        gameover_selection = (gameover_selection + 1) % 2;
        sound_play(snd_click);
      }
      if (input_pressed(A_JUMP) || input_pressed(A_CONFIRM)) {
        sound_play(snd_click);
        if (gameover_selection == 0) {
          engine_set_scene(&scene_play);
        } else {
          engine_set_scene(&scene_levelselect);
        }
        return;
      }
      if (input_pressed(A_PAUSE) || input_pressed(A_BACK)) {
        sound_play(snd_click);
        engine_set_scene(&scene_levelselect);
        return;
      }
    }
    return;
  }

  // Pause menu handling (must be checked BEFORE pause toggle)
  if (paused) {
    if (input_pressed(A_DOWN) || input_pressed(A_UP)) {
      pause_selection = (pause_selection + 1) % 2;
      sound_play(snd_click);
    }
    if (input_pressed(A_BACK) || input_pressed(A_PAUSE)) {
      sound_play(snd_click);
      paused = false;
      engine.time_scale = 1.0;
      return;
    }
    if (input_pressed(A_JUMP) || input_pressed(A_CONFIRM)) {
      sound_play(snd_click);
      engine.time_scale = 1.0;
      if (pause_selection == 0) {
        paused = false; // Resume
      } else {
        engine_set_scene(&scene_levelselect); // Quit to level select
      }
    }
    return;
  }

  // Pause toggle (only when not already paused)
  if (input_pressed(A_PAUSE) || input_pressed(A_BACK)) {
    paused = true;
    pause_selection = 0;
    engine.time_scale = 0;
    return;
  }

  // Time
  g.level_time += engine.tick;

  if (g.level_time >= g.time_limit) {
    game_player_died();
    return;
  }

  // Time warning at 12 seconds remaining
  float time_rem = g.time_limit - g.level_time;
  if (time_rem < 12.0f && !time_warning_shown) {
    time_warning_shown = true;
    spawn_popup("Hurry up! Only 10 seconds left!");
  }

  // Combo window
  if (g.combo_window > 0) {
    g.combo_window -= engine.tick;
    if (g.combo_window <= 0) {
      g.combo_count = 0;
    }
  }

  // Update all entities (physics + collisions)
  scene_base_update();

  // Camera — AS3 always follows the player; scroll modes just add kill
  // boundaries
  entity_t *p = entity_by_ref(g.player);
  if (p) {
    bool snap = g.level_time < 0.1f;
    camera_follow(&g.camera, g.player, snap);
    camera_update(&g.camera);

    // Kill player if off-screen in scroll modes
    if (g.cam_mode == CAM_SCROLL_UP) {
      if (p->pos.y > engine.viewport.y + RENDER_HEIGHT + 16) {
        game_player_died();
        return;
      }
    } else if (g.cam_mode == CAM_SCROLL_DOWN) {
      if (p->pos.y < engine.viewport.y - 16) {
        game_player_died();
        return;
      }
    }

    // Clamp viewport to map
    if (engine.collision_map) {
      float map_w =
          engine.collision_map->size.x * engine.collision_map->tile_size;
      float map_h =
          engine.collision_map->size.y * engine.collision_map->tile_size;
      engine.viewport.x =
          clamp(engine.viewport.x, 0, max(0, map_w - RENDER_WIDTH));
      engine.viewport.y =
          clamp(engine.viewport.y, 0, max(0, map_h - RENDER_HEIGHT));
    }
  }

  popups_update();

  // Debug: Hitbox toggle
  if (input_pressed(A_DEBUG)) {
    g.debug_hitboxes = !g.debug_hitboxes;
  }
}

static void draw_lights_layer(void) {
  if (!level_table[g.level_index].dark)
    return;

  // Darken
  render_set_blend_mode(RENDER_BLEND_NORMAL);
  render_draw(vec2(0, 0), vec2(RENDER_WIDTH, RENDER_HEIGHT), RENDER_NO_TEXTURE,
              vec2(0, 0), vec2(0, 0), rgba(0, 0, 0, 200));

  if (!light_tex)
    return;

  // Add lights around bulbs and player
  render_set_blend_mode(RENDER_BLEND_LIGHTER);

  vec2i_t light_size = image_size(light_tex);

  entity_list_t bulbs = entities_by_type(ENTITY_TYPE_BULB);
  for (uint32_t i = 0; i < bulbs.len; i++) {
    entity_t *b = entity_by_ref(bulbs.entities[i]);
    if (!b)
      continue;
    vec2_t s = vec2_sub(b->pos, engine.viewport);
    image_draw_ex(light_tex, vec2(0, 0), vec2_from_vec2i(light_size),
                  vec2(s.x - 24, s.y - 24), vec2(48, 48),
                  rgba(255, 240, 160, 200));
  }

  entity_t *p = entity_by_ref(g.player);
  if (p) {
    vec2_t s = vec2_sub(p->pos, engine.viewport);
    image_draw_ex(light_tex, vec2(0, 0), vec2_from_vec2i(light_size),
                  vec2(s.x - 48, s.y - 48), vec2(96, 96),
                  rgba(220, 220, 255, 230));
  }

  render_set_blend_mode(RENDER_BLEND_NORMAL);
}

static void draw_hud(void) {
  // Sidebar background at x=256 (right 64px)
  if (hud_bg) {
    image_draw(hud_bg, vec2(256, 0));
  } else {
    render_draw(vec2(256, 0), vec2(64, RENDER_HEIGHT), RENDER_NO_TEXTURE,
                vec2(0, 0), vec2(0, 0), rgba(34, 34, 34, 255));
  }

  if (!g.font)
    return;

  entity_t *p = entity_by_ref(g.player);
  char buf[64];
  rgba_t saved = g.font->color;

  // Highscore header (x=260, y=0) — yellow
  g.font->color = rgba(255, 216, 0, 255);
  font_draw(g.font, vec2(319, 0), "Highscore:", FONT_ALIGN_RIGHT);

  // Highscore value (x=280, y=10) — white
  g.font->color = rgba(255, 255, 255, 255);
  snprintf(buf, sizeof(buf), "%d", g.high_score);
  font_draw(g.font, vec2(319, 10), buf, FONT_ALIGN_RIGHT);

  // Score header (y=40) — yellow
  g.font->color = rgba(255, 216, 0, 255);
  font_draw(g.font, vec2(319, 40), "Score:", FONT_ALIGN_RIGHT);

  // Score value (y=50) — white
  g.font->color = rgba(255, 255, 255, 255);
  snprintf(buf, sizeof(buf), "%d", g.score);
  font_draw(g.font, vec2(319, 50), buf, FONT_ALIGN_RIGHT);

  // Bulbs header (y=80) — yellow
  g.font->color = rgba(255, 216, 0, 255);
  font_draw(g.font, vec2(319, 80), "Bulbs:", FONT_ALIGN_RIGHT);

  // Bulbs value (y=90) — white
  g.font->color = rgba(255, 255, 255, 255);
  snprintf(buf, sizeof(buf), "%d/%d", g.bulbs_collected, g.bulbs_total);
  font_draw(g.font, vec2(319, 90), buf, FONT_ALIGN_RIGHT);

  // Power header (y=120) — yellow
  g.font->color = rgba(255, 216, 0, 255);
  font_draw(g.font, vec2(319, 120), "Power:", FONT_ALIGN_RIGHT);

  // Power bar (battery graphic at x=305, y=135)
  if (battery_back)
    image_draw(battery_back, vec2(305, 135));
  if (p) {
    float pwr = clamp(p->player.power, 0, 100.0f);
    int h = (int)(32.0f * (pwr / 100.0f));
    if (h > 0) {
      // Green power bar, drawn from bottom up
      render_draw(vec2(305, 138 + (32 - h)), vec2(12, h), RENDER_NO_TEXTURE,
                  vec2(0, 0), vec2(0, 0), rgba(0, 255, 0, 136));
    }
  }
  if (battery_hull)
    image_draw(battery_hull, vec2(305, 135));

  // Time header (y=190) — yellow
  g.font->color = rgba(255, 216, 0, 255);
  font_draw(g.font, vec2(319, 190), "Time:", FONT_ALIGN_RIGHT);

  // Time value (y=200)
  float time_rem = g.time_limit - g.level_time;
  if (time_rem < 0)
    time_rem = 0;
  g.font->color =
      time_rem < 10 ? rgba(255, 80, 80, 255) : rgba(255, 255, 255, 255);
  snprintf(buf, sizeof(buf), "%d", (int)ceilf(time_rem));
  font_draw(g.font, vec2(319, 200), buf, FONT_ALIGN_RIGHT);

  g.font->color = saved;
}

static void draw_big_text(font_t *font, vec2_t pos, char *text, float scale,
                          font_align_t align) {
  if (align == FONT_ALIGN_CENTER || align == FONT_ALIGN_RIGHT) {
    int width = 0;
    for (char *c = text; *c != '\0' && *c != '\n'; c++) {
      if (*c >= font->first_char && *c <= font->last_char) {
        width += (font->glyphs[*c - font->first_char].advance +
                  font->letter_spacing) *
                 scale;
      }
    }
    pos.x -= align == FONT_ALIGN_CENTER ? (float)width / 2.0f : (float)width;
  }

  for (char *c = text; *c != '\0' && *c != '\n'; c++) {
    if (*c >= font->first_char && *c <= font->last_char) {
      font_glyph_t *g = &font->glyphs[(int)(*c) - font->first_char];
      vec2_t draw_pos =
          vec2(pos.x + g->offset.x * scale, pos.y + g->offset.y * scale);
      vec2_t draw_size = vec2(g->size.x * scale, g->size.y * scale);
      image_draw_ex(font->image, g->pos, g->size, draw_pos, draw_size,
                    font->color);
      pos.x += (g->advance + font->letter_spacing) * scale;
    }
  }
}

static void draw_pause_overlay(void) {
  render_draw(vec2(0, 0), vec2(RENDER_WIDTH, RENDER_HEIGHT), RENDER_NO_TEXTURE,
              vec2(0, 0), vec2(0, 0), rgba(0, 0, 0, 180));

  if (!g.font)
    return;

  rgba_t saved_hdr = g.font->color;
  g.font->color = rgba(255, 255, 100, 255);
  draw_big_text(g.font, vec2(RENDER_WIDTH / 2, 60), "PAUSED", 3.0f, FONT_ALIGN_CENTER);
  g.font->color = saved_hdr;

  const char *opts[] = {"RESUME", "QUIT"};
  for (int i = 0; i < 2; i++) {
    rgba_t saved = g.font->color;
    g.font->color = (i == pause_selection) ? rgba(255, 255, 100, 255)
                                           : rgba(180, 180, 180, 255);
    char buf[32];
    snprintf(buf, sizeof(buf), "%s %s", (i == pause_selection ? ">" : " "), opts[i]);
    font_draw(g.font, vec2(RENDER_WIDTH / 2, 110 + i * 20), buf,
              FONT_ALIGN_CENTER);
    g.font->color = saved;
  }
}

static void draw_game_over(void) {
  render_draw(vec2(0, 0), vec2(RENDER_WIDTH, RENDER_HEIGHT), RENDER_NO_TEXTURE,
              vec2(0, 0), vec2(0, 0), rgba(0, 0, 0, 180));

  if (!g.font)
    return;

  rgba_t saved_color = g.font->color;

  // Draw Game Over Header (scaled)
  g.font->color = rgba(35, 62, 88, 255); // #233e58 shadow
  draw_big_text(g.font, vec2(RENDER_WIDTH / 2 + 2, 42), "GAME OVER", 3.0f, FONT_ALIGN_CENTER);
  
  g.font->color = rgba(255, 0, 0, 255); // Red
  draw_big_text(g.font, vec2(RENDER_WIDTH / 2, 40), "GAME OVER", 3.0f, FONT_ALIGN_CENTER);

  // Draw Random Message
  g.font->color = rgba(255, 255, 255, 255);
  g.font->shadow = rgba(0, 0, 0, 200);
  font_draw(g.font, vec2(RENDER_WIDTH / 2, 85), (char *)gameover_messages[gameover_msg_idx], FONT_ALIGN_CENTER);

  if (scene_play_game_over_timer > 1.0f) {
    const char *opts[] = {"RETRY", "EXIT"};
    for (int i = 0; i < 2; i++) {
      rgba_t saved = g.font->color;
      g.font->color = (i == gameover_selection) ? rgba(255, 255, 100, 255)
                                             : rgba(180, 180, 180, 255);
      char buf[32];
      snprintf(buf, sizeof(buf), "%s %s", (i == gameover_selection ? ">" : " "), opts[i]);
      font_draw(g.font, vec2(RENDER_WIDTH / 2, 110 + i * 20), buf,
                FONT_ALIGN_CENTER);
      g.font->color = saved;
    }
  }

  g.font->color = saved_color;
  g.font->shadow = rgba(0, 0, 0, 0);
}

static void draw_debug_hitboxes(void) {
  if (!g.debug_hitboxes)
    return;

  for (int type = ENTITY_TYPE_NONE + 1; type < ENTITY_TYPES_COUNT; type++) {
    entity_list_t list = entities_by_type(type);
    for (uint32_t i = 0; i < list.len; i++) {
      entity_t *ent = entity_by_ref(list.entities[i]);
      if (!ent)
        continue;

      vec2_t p = vec2_sub(ent->pos, engine.viewport);
      rgba_t color = rgba(255, 0, 0, 160); // Semi-transparent red

      // Draw outline (4 lines)
      render_draw(vec2(p.x, p.y), vec2(ent->size.x, 1), RENDER_NO_TEXTURE,
                  vec2(0, 0), vec2(0, 0), color);
      render_draw(vec2(p.x, p.y + ent->size.y - 1), vec2(ent->size.x, 1),
                  RENDER_NO_TEXTURE, vec2(0, 0), vec2(0, 0), color);
      render_draw(vec2(p.x, p.y), vec2(1, ent->size.y), RENDER_NO_TEXTURE,
                  vec2(0, 0), vec2(0, 0), color);
      render_draw(vec2(p.x + ent->size.x - 1, p.y), vec2(1, ent->size.y),
                  RENDER_NO_TEXTURE, vec2(0, 0), vec2(0, 0), color);
    }
  }
}

static void draw(void) {
  // Background with parallax (scrollFactor 0.1) and per-level tint
  if (bg_img) {
    vec2_t bg_off = vec2(-engine.viewport.x * 0.1f, -engine.viewport.y * 0.1f);
    vec2i_t bgsz = image_size(bg_img);
    // Tile background to cover screen
    for (float bx = fmodf(bg_off.x, bgsz.x) - bgsz.x; bx < RENDER_WIDTH;
         bx += bgsz.x) {
      for (float by = fmodf(bg_off.y, bgsz.y) - bgsz.y; by < RENDER_HEIGHT;
           by += bgsz.y) {
        image_draw_ex(bg_img, vec2(0, 0), vec2(bgsz.x, bgsz.y), vec2(bx, by),
                      vec2(bgsz.x, bgsz.y), bg_tint);
      }
    }
  }

  // Draw tiles and entities (alarm lights draw themselves via entity draw)
  scene_base_draw();

  // Debug hitboxes
  draw_debug_hitboxes();

  // Lighting overlay (dark levels)
  draw_lights_layer();

  // HUD
  draw_hud();

  // Popups
  popups_draw();

  // Pause overlay on top
  if (paused) {
    draw_pause_overlay();
  }

  // Game over overlay
  if (scene_play_game_over) {
    draw_game_over();
  }
}

static void cleanup(void) {
  engine.time_scale = 1.0;
  sound_pause(g.music_play[g.music_play_index]);
}

scene_t scene_play = {
    .init = init,
    .update = update,
    .draw = draw,
    .cleanup = cleanup,
};
