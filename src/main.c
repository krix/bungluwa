#include "main.h"
#include <time.h>

global_t g;

// Sound sources
sound_source_t *snd_jump;
sound_source_t *snd_land;
sound_source_t *snd_bulb;
sound_source_t *snd_explode;
sound_source_t *snd_extralife;
sound_source_t *snd_zap;
sound_source_t *snd_alarm;
sound_source_t *snd_exit;
sound_source_t *snd_click;
sound_source_t *snd_voice[3];
sound_source_t *snd_unlocked[4];

// Shared anim defs
anim_def_t *anim_smoke;
anim_def_t *anim_twinkle_def;
anim_def_t *anim_exit_particle;
anim_def_t *anim_gib_body;
anim_def_t *anim_gib_head;
anim_def_t *anim_gib_arm;
anim_def_t *anim_gib_wheel;
anim_def_t *anim_bulb_gibs;
anim_def_t *anim_sparks;

static image_t *popup_bg_image;

// ---------------------------------------------------------------------------
// Level table — transcribed from SaveManager.as / PlayState.as / MapKrx*.as
// Cross-check time_limit and bulbs_total against SaveManager.as lines 88-156.

const level_info_t level_table[NUM_LEVELS] = {
	// Ordering transcribed from PlayState.loadMap: case 1-18 = main, 19-24 = bonus
	//          path                       name              time bulbs cam_mode        scroll dark  bonus
	/*  0 */ { "assets/levels/krx2.json",  "LEVEL 01",        65,  17, CAM_SCROLL_UP,   0,  false, false },
	/*  1 */ { "assets/levels/krx3.json",  "LEVEL 02",        70,  18, CAM_FREE,        0,  false, false },
	/*  2 */ { "assets/levels/krx7.json",  "LEVEL 03",        55,  29, CAM_SCROLL_DOWN, 0,  false, false },
	/*  3 */ { "assets/levels/krx4.json",  "LEVEL 04",        60,  38, CAM_SCROLL_DOWN,10,  false, false },
	/*  4 */ { "assets/levels/krx10.json", "LEVEL 05",        50,  33, CAM_SCROLL_UP,  10,  false, false },
	/*  5 */ { "assets/levels/krx11.json", "LEVEL 06",        55,  39, CAM_SCROLL_UP,  10,  false, false },
	/*  6 */ { "assets/levels/krx8.json",  "LEVEL 07",        80,  44, CAM_SCROLL_UP,   0,  false, false },
	/*  7 */ { "assets/levels/krx12.json", "LEVEL 08",        60,  27, CAM_SCROLL_UP,  10,  false, false },
	/*  8 */ { "assets/levels/krx9.json",  "LEVEL 09",        90,  39, CAM_SCROLL_UP,   0,  false, false },
	/*  9 */ { "assets/levels/krx18.json", "LEVEL 10",        65,  32, CAM_FREE,        0,  false, false },
	/* 10 */ { "assets/levels/krx21.json", "LEVEL 11",        70,  37, CAM_SCROLL_DOWN, 0,  false, false },
	/* 11 */ { "assets/levels/krx14.json", "LEVEL 12",        75,  41, CAM_FREE,        0,  false, false },
	/* 12 */ { "assets/levels/krx24.json", "LEVEL 13",        70,  43, CAM_FREE,        0,  false, false },
	/* 13 */ { "assets/levels/krx20.json", "LEVEL 14",        65,  30, CAM_FREE,        0,  false, false },
	/* 14 */ { "assets/levels/krx17.json", "LEVEL 15",        60,  39, CAM_SCROLL_DOWN,10,  false, false },
	/* 15 */ { "assets/levels/krx22.json", "LEVEL 16",        75,  29, CAM_SCROLL_DOWN,10,  false, false },
	/* 16 */ { "assets/levels/krx15.json", "LEVEL 17",        95,  57, CAM_FREE,        0,  false, false },
	/* 17 */ { "assets/levels/krx16.json", "LEVEL 18",        65,  28, CAM_FREE,        0,  false, false },
	/* 18 */ { "assets/levels/krx5.json",  "BONUS A",         50, 129, CAM_SCROLL_UP,   0,  false, true  },
	/* 19 */ { "assets/levels/krx13.json", "BONUS B",         70,  58, CAM_SCROLL_UP,  10,  false, true  },
	/* 20 */ { "assets/levels/krx6.json",  "BONUS C",         80,  33, CAM_SCROLL_UP,   0,  false, true  },
	/* 21 */ { "assets/levels/krx23.json", "BONUS D",         55,  63, CAM_SCROLL_DOWN, 0,  false, true  },
	/* 22 */ { "assets/levels/krx19.json", "BONUS E",         80,  20, CAM_SCROLL_DOWN, 0,  false, true  },
	/* 23 */ { "assets/levels/krx1.json",  "BONUS F",         60,  76, CAM_SCROLL_UP,   0,  false, true  },
};

// ---------------------------------------------------------------------------
// Helpers

void spawn_popup(const char *text) {
	if (g.popup_count >= POPUP_MAX) return;
	popup_t *p = &g.popups[g.popup_count++];
	strncpy(p->text, text, sizeof(p->text) - 1);
	p->text[sizeof(p->text) - 1] = '\0';
	p->status = POPUP_STATUS_IN;
	p->y = -38; // start above screen (popup height = 38)
	p->wait_timer = 2.0f;
}


void popups_update(void) {
	if (g.popup_count <= 0) return;
	popup_t *p = &g.popups[0];

	float speed = 160.0f * engine.tick;
	switch (p->status) {
		case POPUP_STATUS_IN:
			p->y += speed;
			if (p->y >= 0) {
				p->y = 0;
				p->status = POPUP_STATUS_WAIT;
			}
			break;
		case POPUP_STATUS_WAIT:
			p->wait_timer -= engine.tick;
			if (p->wait_timer <= 0) {
				p->status = POPUP_STATUS_OUT;
			}
			break;
		case POPUP_STATUS_OUT:
			p->y -= speed;
			if (p->y < -38) {
				// Remove first element, shift queue
				for (int j = 0; j < g.popup_count - 1; j++) {
					g.popups[j] = g.popups[j + 1];
				}
				g.popup_count--;
			}
			break;
	}
}

void popups_draw(void) {
	if (g.popup_count <= 0) return;
	popup_t *p = &g.popups[0];
	if (p->status == POPUP_STATUS_DONE) return;
	if (!g.font) return;

	// Center horizontally (popup is 170x38)
	float px = (RENDER_WIDTH - 170) / 2.0f;

	if (popup_bg_image) {
		image_draw_ex(popup_bg_image,
			vec2(0, 0), vec2(170, 38),
			vec2(px, p->y), vec2(170, 38),
			rgba(255, 255, 255, 204));
	}

	// Text area inside popup: left margin 25px, top/right/bottom margin 10px
	// Available text area: x=[25, 160], width=135
	char wrapped[128];
	int max_w = 135;
	int total_w = font_line_width(g.font, p->text);
	int num_lines = 1;

	if (total_w > max_w) {
		// Find best space to break at (closest to half the text length)
		int best_break = -1;
		int best_diff = 9999;
		int len = (int)strlen(p->text);
		for (int i = 0; i < len; i++) {
			if (p->text[i] == ' ') {
				char tmp[128];
				strncpy(tmp, p->text, i);
				tmp[i] = '\0';
				int w1 = font_line_width(g.font, tmp);
				if (w1 <= max_w) {
					int w2 = font_line_width(g.font, p->text + i + 1);
					if (w2 <= max_w) {
						int diff = abs(w1 - w2);
						if (diff < best_diff) {
							best_diff = diff;
							best_break = i;
						}
					}
				}
			}
		}
		if (best_break >= 0) {
			strncpy(wrapped, p->text, best_break);
			wrapped[best_break] = '\n';
			strcpy(wrapped + best_break + 1, p->text + best_break + 1);
			num_lines = 2;
		} else {
			strcpy(wrapped, p->text);
		}
	} else {
		strcpy(wrapped, p->text);
	}

	// Left-aligned, top-aligned text at x=px+25, y=p->y+10
	float tx = px + 25;
	float ty = p->y + 10;

	rgba_t saved_color = g.font->color;
	rgba_t saved_shadow = g.font->shadow;
	g.font->shadow = rgba(35, 62, 88, 255);
	g.font->color = rgba(255, 255, 255, 255);
	font_draw(g.font, vec2(tx, ty), wrapped, FONT_ALIGN_LEFT);
	g.font->color = saved_color;
	g.font->shadow = saved_shadow;
}

entity_t *spawn_smoke(vec2_t pos, int count) {
	for (int i = 0; i < count; i++) {
		float a = rand_float(0, 2 * M_PI);
		float s = rand_float(20, 60);
		entity_t *e = entity_spawn(ENTITY_TYPE_PARTICLE, pos);
		if (e) {
			e->anim = anim(anim_smoke);
			e->vel = vec2(cosf(a) * s, sinf(a) * s);
			e->physics = ENTITY_PHYSICS_MOVE;
			e->gravity = 0.3f;
			e->particle.life_time = 0.5f;
			e->particle.fade_time = 0.5f;
		}
	}
	return NULL;
}

entity_t *spawn_floating_text(vec2_t pos, const char *text, rgba_t color) {
	entity_t *e = entity_spawn(ENTITY_TYPE_FLOATING_TEXT, pos);
	if (e) {
		strncpy(e->floating_text.text, text, sizeof(e->floating_text.text) - 1);
		e->floating_text.text[sizeof(e->floating_text.text) - 1] = '\0';
		e->floating_text.life = 1.0f;
		e->anim.color = color;
	}
	return e;
}

entity_t *spawn_twinkle(vec2_t pos) {
	entity_t *e = entity_spawn(ENTITY_TYPE_TWINKLE, pos);
	if (e) {
		e->anim = anim(anim_twinkle_def);
	}
	return e;
}

void spawn_gibs(vec2_t pos) {
	// AS3: 40 body gibs, 1 head, 2 arms, 1 wheel — all burst outward
	// Body gibs (botgibs.png: 14x2, 7 frames of 2x2)
	for (int i = 0; i < 20; i++) {
		entity_t *e = entity_spawn(ENTITY_TYPE_PARTICLE, pos);
		if (!e) break;
		e->anim = anim(anim_gib_body);
		e->anim.tile_offset = (uint16_t)(rand() % 7);
		e->vel = vec2(rand_float(-150, 150), rand_float(-250, -100));
		e->particle.life_time = 3.0f;
		e->particle.fade_time = 1.0f;
		e->particle.collide_world = true;
		e->gravity = 1.0f;
		e->size = vec2(2, 2);
	}
	// Head gib
	entity_t *h = entity_spawn(ENTITY_TYPE_PARTICLE, pos);
	if (h) {
		h->anim = anim(anim_gib_head);
		h->vel = vec2(rand_float(-80, 80), rand_float(-200, -120));
		h->particle.life_time = 3.0f;
		h->particle.fade_time = 1.0f;
		h->particle.collide_world = true;
		h->gravity = 1.0f;
		h->size = vec2(5, 5);
	}
	// Arm gibs (2)
	for (int i = 0; i < 2; i++) {
		entity_t *a = entity_spawn(ENTITY_TYPE_PARTICLE, pos);
		if (!a) break;
		a->anim = anim(anim_gib_arm);
		a->vel = vec2(rand_float(-120, 120), rand_float(-200, -100));
		a->particle.life_time = 3.0f;
		a->particle.fade_time = 1.0f;
		a->particle.collide_world = true;
		a->gravity = 1.0f;
		a->size = vec2(5, 5);
	}
	// Wheel gib
	entity_t *w = entity_spawn(ENTITY_TYPE_PARTICLE, pos);
	if (w) {
		w->anim = anim(anim_gib_wheel);
		w->vel = vec2(rand_float(-100, 100), rand_float(-180, -80));
		w->particle.life_time = 3.0f;
		w->particle.fade_time = 1.0f;
		w->particle.collide_world = true;
		w->gravity = 1.0f;
		w->size = vec2(4, 4);
	}
}

void spawn_exit_particles(entity_t *exit_ent) {
	// AS3: particles centered on exit, fly upward with light gravity
	for (int i = 0; i < 3; i++) {
		// Center horizontally on 16px sprite, spawn from middle height
		float px = exit_ent->pos.x + 6 + rand_float(-4, 4);
		float py = exit_ent->pos.y + 10;
		entity_t *e = entity_spawn(ENTITY_TYPE_PARTICLE, vec2(px, py));
		if (!e) break;
		e->anim = anim(anim_exit_particle);
		e->anim.tile_offset = (uint16_t)(rand() % 5);
		e->vel = vec2(rand_float(-10, 10), rand_float(-60, -30));
		e->particle.life_time = 0.8f;
		e->particle.fade_time = 0.5f;
		e->particle.collide_world = false;
		e->gravity = 0.25f; // AS3: gravity=100, engine.gravity=400 → 100/400
		e->size = vec2(2, 2);
	}
}

void spawn_bulb_gibs(vec2_t pos) {
	// AS3: gibs emitter with 10 burst particles, ImgGibs 10x2 (5 frames of 2x2)
	// speed X -150..150, Y -200..0, fade 0.5s
	for (int i = 0; i < 10; i++) {
		entity_t *e = entity_spawn(ENTITY_TYPE_PARTICLE, pos);
		if (!e) break;
		e->anim = anim(anim_bulb_gibs);
		e->anim.tile_offset = (uint16_t)(rand() % 5);
		e->vel = vec2(rand_float(-150, 150), rand_float(-200, 0));
		e->particle.life_time = 1.0f;
		e->particle.fade_time = 0.5f;
		e->particle.collide_world = true;
		e->gravity = 1.0f;
		e->size = vec2(2, 2);
	}
}

void spawn_sparks(vec2_t pos) {
	// AS3: 8 particles, x speed ±20, y speed ±20, gravity 200
	for (int i = 0; i < 8; i++) {
		float px = pos.x + rand_float(-4, 4);
		float py = pos.y + rand_float(-4, 4);
		entity_t *e = entity_spawn(ENTITY_TYPE_PARTICLE, vec2(px, py));
		if (!e) break;
		e->anim = anim(anim_sparks);
		e->anim.tile_offset = (uint16_t)(rand() % 5);
		e->vel = vec2(rand_float(-20, 20), rand_float(-20, 20));
		e->particle.life_time = 0.6f;
		e->particle.fade_time = 0.4f;
		e->particle.collide_world = true;
		e->gravity = 1.5f;
		e->size = vec2(2, 2);
	}
}

// ---------------------------------------------------------------------------
// Game flow helpers

void game_load_level(int index) {
	g.level_index = index;
	g.time_limit = level_table[index].time_limit;
	g.bulbs_total = level_table[index].bulbs_total;
	g.cam_mode = level_table[index].cam_mode;
	g.cam_scroll_speed = level_table[index].auto_scroll;
	// Level is actually loaded by scene_play.init() via engine_load_level()
}

void game_collect_bulb(entity_t *bulb_ent, entity_t *player) {
	g.bulbs_collected++;
	g.total_bulbs++;

	// AS3 scoring: no combo=100, combo2=200, combo3=500, combo4+=1000
	int pts;
	rgba_t color;
	if (g.combo_window > 0) {
		g.combo_count++;
	} else {
		g.combo_count = 1;
	}
	switch (g.combo_count) {
		case 1:  pts = 100;  color = rgba(255, 255, 255, 255); break; // white
		case 2:  pts = 200;  color = rgba(255, 216, 0,   255); break; // yellow
		case 3:  pts = 500;  color = rgba(255, 136, 0,   255); break; // orange
		default: pts = 1000; color = rgba(255, 0,   0,   255); break; // red
	}
	g.combo_window = 0.2f;
	g.score += pts;

	char buf[16];
	snprintf(buf, sizeof(buf), "+%d", pts);
	spawn_floating_text(bulb_ent->pos, buf, color);
	spawn_bulb_gibs(bulb_ent->pos);
	spawn_twinkle(bulb_ent->pos);
	sound_play(snd_bulb);

	// AS3: givePower(20) on bulb collect
	entity_t *p = entity_by_ref(g.player);
	if (p) {
		p->player.power = fminf(100.0f, p->player.power + 20.0f);
	}
}

void game_collect_extra(entity_t *extra, entity_t *player) {
	entity_t *p = entity_by_ref(g.player);
	if (p) {
		p->player.extras++;
	}
	g.extras++;
	sound_play(snd_extralife);
}

void game_trigger_alarm(void) {
	g.alarm_time = 5.0f;
	sound_play(snd_alarm);
}

void game_player_died(void) {
	g.deaths++;
	g.total_deaths++;
	engine.time_scale = 1.0;
	extern bool scene_play_game_over;
	extern float scene_play_game_over_timer;
	extern int gameover_msg_idx;
	scene_play_game_over = true;
	scene_play_game_over_timer = 0;
	gameover_msg_idx = rand() % 13;
}

void game_player_exited(void) {
	// Store level results
	g.levels_completed++;
	if (g.deaths == 0) g.levels_no_death++;
	if (g.bulbs_collected >= g.bulbs_total) g.levels_all_bulbs++;

	// Save current progress (bonuses calculated in level complete screen)
	save_store(true);
	achievements_check();
	engine_set_scene(&scene_levelcomplete);
}

// ---------------------------------------------------------------------------
// main_init / main_cleanup (called by the engine)

void main_init(void) {
	srand((unsigned int)time(NULL));

	// Keyboard
	input_bind(INPUT_KEY_LEFT,   A_LEFT);
	input_bind(INPUT_KEY_RIGHT,  A_RIGHT);
	input_bind(INPUT_KEY_UP,     A_UP);
	input_bind(INPUT_KEY_DOWN,   A_DOWN);
	input_bind(INPUT_KEY_X,      A_JUMP);
	input_bind(INPUT_KEY_C,      A_JUMP);
	input_bind(INPUT_KEY_SPACE,  A_JUMP);
	input_bind(INPUT_KEY_P,      A_PAUSE);
	input_bind(INPUT_KEY_RETURN, A_CONFIRM);
	input_bind(INPUT_KEY_ESCAPE, A_BACK);
	input_bind(INPUT_KEY_H,      A_DEBUG);
	input_bind(INPUT_KEY_BACKSPACE, A_KILL);
	input_bind(INPUT_KEY_O,      A_RESET_O);
	input_bind(INPUT_KEY_K,      A_RESET_K);

	// Gamepad
	input_bind(INPUT_GAMEPAD_DPAD_LEFT,     A_LEFT);
	input_bind(INPUT_GAMEPAD_DPAD_RIGHT,    A_RIGHT);
	input_bind(INPUT_GAMEPAD_DPAD_UP,       A_UP);
	input_bind(INPUT_GAMEPAD_DPAD_DOWN,     A_DOWN);
	input_bind(INPUT_GAMEPAD_L_STICK_LEFT,  A_LEFT);
	input_bind(INPUT_GAMEPAD_L_STICK_RIGHT, A_RIGHT);
	input_bind(INPUT_GAMEPAD_L_STICK_UP,    A_UP);
	input_bind(INPUT_GAMEPAD_L_STICK_DOWN,  A_DOWN);
	input_bind(INPUT_GAMEPAD_A,             A_JUMP);
	input_bind(INPUT_GAMEPAD_B,             A_BACK);
	input_bind(INPUT_GAMEPAD_X,             A_CONFIRM);
	input_bind(INPUT_GAMEPAD_Y,             A_JUMP);
	input_bind(INPUT_GAMEPAD_L_SHOULDER,    A_BACK);
	input_bind(INPUT_GAMEPAD_R_SHOULDER,    A_JUMP);
	input_bind(INPUT_GAMEPAD_START,         A_PAUSE);
	input_bind(INPUT_GAMEPAD_SELECT,        A_PAUSE);

	// Font
	g.font = font("assets/font_default.qoi", "assets/font_default.json");
	g.font->shadow = rgba(64, 64, 64, 255);

	// Music (looped nodes — created once, reused)
	g.music_menu = sound(sound_source("assets/music/menu.qoa"));
	g.music_play[0] = sound(sound_source("assets/music/play1.qoa"));
	g.music_play[1] = sound(sound_source("assets/music/play2.qoa"));
	g.music_play[2] = sound(sound_source("assets/music/play3.qoa"));
	g.music_play[3] = sound(sound_source("assets/music/play4.qoa"));
	sound_set_loop(g.music_menu, true);
	for (int i = 0; i < 4; i++) {
		sound_set_loop(g.music_play[i], true);
	}
	sound_set_global_volume(0.85f);

	// SFX sources
	snd_jump     = sound_source("assets/sounds/jump.qoa");
	snd_land     = sound_source("assets/sounds/land.qoa");
	snd_bulb     = sound_source("assets/sounds/bulb.qoa");
	snd_explode  = sound_source("assets/sounds/explode.qoa");
	snd_extralife= sound_source("assets/sounds/extralife.qoa");
	snd_zap      = sound_source("assets/sounds/zap.qoa");
	snd_alarm    = sound_source("assets/sounds/alarm.qoa");
	snd_exit     = sound_source("assets/sounds/exit.qoa");
	snd_click    = sound_source("assets/sounds/click.qoa");
	snd_voice[0] = sound_source("assets/sounds/voice1.qoa");
	snd_voice[1] = sound_source("assets/sounds/voice2.qoa");
	snd_voice[2] = sound_source("assets/sounds/voice3.qoa");
	snd_unlocked[0] = sound_source("assets/sounds/unlocked1.qoa");
	snd_unlocked[1] = sound_source("assets/sounds/unlocked2.qoa");
	snd_unlocked[2] = sound_source("assets/sounds/unlocked3.qoa");
	snd_unlocked[3] = sound_source("assets/sounds/unlocked4.qoa");

	// Shared particle anims
	// exhaust.png: 20x4, 5 frames of 4x4
	image_t *smoke_sheet = image("assets/sprites/smoke.qoi");
	anim_smoke = anim_def(smoke_sheet, vec2i(4, 4), 0.08, {0, 1, 2, 3, 4, 4, ANIM_STOP});

	// twinkle.png: 24x8, 3 frames of 8x8
	// AS3 anim: [0, 1, 1, 2, 2, 2, 1, 1, 0] at speed 16
	image_t *twinkle_sheet = image("assets/sprites/twinkle.qoi");
	anim_twinkle_def = anim_def(twinkle_sheet, vec2i(8, 8), 0.06,
		{0, 1, 1, 2, 2, 2, 1, 1, 0, 0, ANIM_STOP});

	// exitparticles.png: 10x2, 5 frames of 2x2
	image_t *exit_p_sheet = image("assets/sprites/exitparticles.qoi");
	anim_exit_particle = anim_def(exit_p_sheet, vec2i(2, 2), 1.0, {0});

	// Gib sprites (single-frame anims — tile_offset selects the piece)
	// botgibs.png: 14x2, 7 frames of 2x2
	image_t *gib_body_sheet = image("assets/sprites/botgibs.qoi");
	anim_gib_body = anim_def(gib_body_sheet, vec2i(2, 2), 1.0, {0});

	// botheadgib.png: 5x5
	image_t *gib_head_sheet = image("assets/sprites/botheadgib.qoi");
	anim_gib_head = anim_def(gib_head_sheet, vec2i(5, 5), 1.0, {0});

	// botarmgib.png: 5x5
	image_t *gib_arm_sheet = image("assets/sprites/botarmgib.qoi");
	anim_gib_arm = anim_def(gib_arm_sheet, vec2i(5, 5), 1.0, {0});

	// botwheelgib.png: 4x4
	image_t *gib_wheel_sheet = image("assets/sprites/botwheelgib.qoi");
	anim_gib_wheel = anim_def(gib_wheel_sheet, vec2i(4, 4), 1.0, {0});

	// gibs.png: 10x2, 5 frames of 2x2 (bulb collection gibs)
	image_t *bulb_gib_sheet = image("assets/sprites/gibs.qoi");
	anim_bulb_gibs = anim_def(bulb_gib_sheet, vec2i(2, 2), 1.0, {0});

	// sparks.png: 10x2, 5 frames of 2x2
	image_t *sparks_sheet = image("assets/sprites/sparks.qoi");
	anim_sparks = anim_def(sparks_sheet, vec2i(2, 2), 1.0, {0});

	// Popup notification background (loaded early so it persists across scenes)
	popup_bg_image = image("assets/sprites/popup.qoi");

	// Player ref starts as none
	g.player = entity_ref_none();

	// Load save
	save_load();

	// Start directly with menu (no intro scene)
	engine_set_scene(&scene_menu);
}

void main_cleanup(void) {
	save_store(false);
}
