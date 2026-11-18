#include "../main.h"

// Forward declarations from save.c
extern bool level_is_unlocked(int idx);
extern int level_best_time_ms(int idx);
extern int level_best_score(int idx);
extern int level_best_bulbs(int idx);
extern bool achievement_is_done(int idx);
extern int save_get_total_time_s(void);
extern int save_get_total_score(void);
extern bool level_should_notify_unlock(int idx);
extern void level_mark_unlock_notified(int idx);

// Layout constants matching AS3 original
#define MAIN_LEVELS 18
#define BONUS_LEVELS 6
#define BTNS_PER_ROW 9
#define TILE_SIZE 16

// Panel focus: which section has keyboard focus
#define FOCUS_MAIN   0
#define FOCUS_BONUS  1
#define FOCUS_STATS  2
#define FOCUS_ACM    3

// Achievement names and descriptions (matching AS3 BungluwaAchievements)
static const char *acm_names[6] = {
	"Fast Robot", "Big Spender", "Greedypants",
	"Half the Rent", "BGW 10K", "Arcade Buster"
};
static const char *acm_descs[6] = {
	"Overall time < 900s",
	"Collect 333 bulbs",
	"Collect all 999 bulbs",
	"Beat level 9",
	"Beat level 18",
	"Beat bonus level F"
};
static const char *acm_images[6] = {
	"assets/sprites/acm_fastrobot.qoi",
	"assets/sprites/acm_bigspender.qoi",
	"assets/sprites/acm_greedypants.qoi",
	"assets/sprites/acm_halftherent.qoi",
	"assets/sprites/acm_bgw10k.qoi",
	"assets/sprites/acm_arcadebuster.qoi"
};

// State
static int cursor;
static int acm_cursor; // achievement selection (0-5)
static int focus;
static bool show_overall_stats;
static float scroll_x;
static float unlock_timer[NUM_LEVELS];

// AutoBot state
static float bot_x, bot_y, bot_vx, bot_vy;
static int bot_facing; // 0=right, 1=left
static int bot_frame;
static float bot_anim_timer;
#define BOT_SPEED 100.0f
#define BOT_GRAVITY 400.0f
#define BOT_FLOOR_Y (14 * TILE_SIZE) // tile row 14 is floor, bot walks on top

// Exhaust particles
#define EXHAUST_MAX 20
#define EXHAUST_INTERVAL 0.025f
typedef struct {
	float x, y, vx, vy;
	float life, max_life;
	bool active;
} exhaust_particle_t;
static exhaust_particle_t exhaust[EXHAUST_MAX];
static float exhaust_timer;

// Images
static image_t *btn_normal, *btn_hover, *btn_locked, *bigscreen, *bg_img;
static image_t *tileset;
static image_t *bot_img;
static image_t *btn_stats, *btn_stats_hover;
static image_t *acm_template, *acm_template_hover, *acm_template_shadow;
static image_t *acm_icons[6];

static void init(void) {
	cursor = 0;
	focus = FOCUS_MAIN;

	if (g.level_index >= 0 && g.level_index < NUM_LEVELS) {
		if (g.level_index < MAIN_LEVELS) {
			cursor = g.level_index;
			focus = FOCUS_MAIN;
		} else {
			cursor = g.level_index - MAIN_LEVELS;
			focus = FOCUS_BONUS;
		}
	}

	acm_cursor = 0;
	show_overall_stats = false;
	scroll_x = 0;

	sound_unpause(g.music_menu);

	// Load images
	btn_normal = image("assets/sprites/mapbutton.qoi");
	btn_hover = image("assets/sprites/mapbuttonhover.qoi");
	btn_locked = image("assets/sprites/lockedmapbutton.qoi");
	bigscreen = image("assets/sprites/bigscreen.qoi");
	bg_img = image("assets/sprites/background.qoi");
	tileset = image("assets/tiles/main.qoi");
	bot_img = image("assets/sprites/badrobot.qoi");
	btn_stats = image("assets/sprites/btn_overall_stats.qoi");
	btn_stats_hover = image("assets/sprites/btn_overall_stats_hover.qoi");
	acm_template = image("assets/sprites/acm_template.qoi");
	acm_template_hover = image("assets/sprites/acm_template_hover.qoi");
	acm_template_shadow = image("assets/sprites/acm_template_shadow.qoi");
	for (int i = 0; i < 6; i++) {
		acm_icons[i] = image((char *)acm_images[i]);
	}

	// Init autobot — starts at (50, floor) moving right
	bot_x = 50;
	bot_y = BOT_FLOOR_Y - 12; // sprite is 13px tall, stand on top of floor
	bot_vx = BOT_SPEED;
	bot_vy = 0;
	bot_facing = 0;
	bot_frame = 0;
	bot_anim_timer = 0;
	exhaust_timer = 0;
	for (int i = 0; i < EXHAUST_MAX; i++) exhaust[i].active = false;

	// Check for newly unlocked levels
	bool played_sound = false;
	for (int i = 0; i < NUM_LEVELS; i++) {
		if (level_should_notify_unlock(i)) {
			unlock_timer[i] = 1.0f;
			level_mark_unlock_notified(i);
			if (!played_sound) {
				sound_play(snd_unlocked[rand() % 4]);
				played_sound = true;
			}
		} else {
			unlock_timer[i] = 0;
		}
	}
}

static void update(void) {
	if (input_pressed(A_BACK)) {
		engine_set_scene(&scene_menu);
		return;
	}

	// Navigation
	if (input_pressed(A_RIGHT)) {
		if (focus == FOCUS_ACM) {
			if (acm_cursor % 3 < 2)
				acm_cursor++;
			else {
				// From rightmost achievement column → bonus panel same row
				focus = FOCUS_BONUS;
				cursor = (acm_cursor / 3) * 3; // row 0→0 (A), row 1→3 (D)
			}
		} else if (focus == FOCUS_MAIN) {
			if (cursor < MAIN_LEVELS - 1)
				cursor++;
			else {
				focus = FOCUS_BONUS;
				cursor = 0;
			}
		} else if (focus == FOCUS_BONUS) {
			if (cursor < BONUS_LEVELS - 1) cursor++;
		} else if (focus == FOCUS_STATS) {
			focus = FOCUS_BONUS;
			cursor = 3;
		}
		sound_play(snd_click);
	}
	if (input_pressed(A_LEFT)) {
		if (focus == FOCUS_BONUS) {
			if (cursor % 3 > 0)
				cursor--;
			else {
				// Left from bonus leftmost column → ACM rightmost column same row
				focus = FOCUS_ACM;
				acm_cursor = (cursor / 3) * 3 + 2; // row 0→2 (plate 3), row 1→5 (plate 6)
			}
		} else if (focus == FOCUS_MAIN) {
			if (cursor > 0)
				cursor--;
			else {
				focus = FOCUS_ACM;
				acm_cursor = 2; // top-right achievement
			}
		} else if (focus == FOCUS_STATS) {
			focus = FOCUS_ACM;
			acm_cursor = 5; // bottom-right achievement
		} else if (focus == FOCUS_ACM) {
			if (acm_cursor % 3 > 0) acm_cursor--;
		}
		sound_play(snd_click);
	}
	if (input_pressed(A_DOWN)) {
		if (focus == FOCUS_MAIN) {
			int next = cursor + BTNS_PER_ROW;
			if (next < MAIN_LEVELS)
				cursor = next;
			else {
				// From bottom row of main → bonus level A
				focus = FOCUS_BONUS;
				cursor = 0;
			}
		} else if (focus == FOCUS_BONUS) {
			int next = cursor + 3;
			if (next < BONUS_LEVELS)
				cursor = next;
			else {
				focus = FOCUS_STATS;
				cursor = 0;
			}
		} else if (focus == FOCUS_ACM) {
			int next = acm_cursor + 3;
			if (next < 6) acm_cursor = next;
		}
		sound_play(snd_click);
	}
	if (input_pressed(A_UP)) {
		if (focus == FOCUS_MAIN) {
			int next = cursor - BTNS_PER_ROW;
			if (next >= 0) cursor = next;
		} else if (focus == FOCUS_BONUS) {
			int next = cursor - 3;
			if (next >= 0)
				cursor = next;
			else {
				// From top row of bonus → level 1
				focus = FOCUS_MAIN;
				cursor = 0;
			}
		} else if (focus == FOCUS_STATS) {
			focus = FOCUS_MAIN;
			cursor = BTNS_PER_ROW;
		} else if (focus == FOCUS_ACM) {
			int next = acm_cursor - 3;
			if (next >= 0)
				acm_cursor = next;
			else {
				// From top row of achievements → level 1
				focus = FOCUS_MAIN;
				cursor = 0;
			}
		}
		sound_play(snd_click);
	}

	if (input_pressed(A_JUMP) || input_pressed(A_CONFIRM)) {
		if (focus == FOCUS_STATS) {
			show_overall_stats = !show_overall_stats;
			sound_play(snd_click);
		} else if (focus == FOCUS_ACM) {
			// No action on confirm for achievements, just viewing
			sound_play(snd_click);
		} else {
			int idx = (focus == FOCUS_MAIN) ? cursor : (MAIN_LEVELS + cursor);
			if (level_is_unlocked(idx)) {
				sound_play(snd_click);
				game_load_level(idx);
				engine_set_scene(&scene_play);
			}
		}
	}

	// Scrolling info text
	scroll_x -= 27.0f * engine.tick;
	if (scroll_x < -RENDER_WIDTH) scroll_x += RENDER_WIDTH;

	// Update autobot
	bot_vy += BOT_GRAVITY * engine.tick;
	bot_x += bot_vx * engine.tick;
	bot_y += bot_vy * engine.tick;

	// Floor collision (tile row 14, top at y=224)
	float floor_top = BOT_FLOOR_Y - 12;
	if (bot_y >= floor_top) {
		bot_y = floor_top;
		bot_vy = 0;
	}

	// Edge detection: turn around at tile boundaries
	// Floor tiles go from x=0 to x=320 (20 tiles * 16px)
	// Turn around if approaching edges (leave 16px margin for corner tiles)
	if (bot_vx > 0 && bot_x >= 320 - 20) {
		bot_vx = -BOT_SPEED;
		bot_facing = 1;
	} else if (bot_vx < 0 && bot_x <= 4) {
		bot_vx = BOT_SPEED;
		bot_facing = 0;
	}

	// Animation (walk: frames 0,1 at 10fps)
	bot_anim_timer += engine.tick;
	if (bot_anim_timer >= 0.1f) {
		bot_anim_timer -= 0.1f;
		bot_frame = (bot_frame + 1) % 2;
	}

	// Exhaust particles (AS3: spawn every 0.025s, drift up, fade out)
	exhaust_timer += engine.tick;
	while (exhaust_timer >= EXHAUST_INTERVAL) {
		exhaust_timer -= EXHAUST_INTERVAL;
		for (int i = 0; i < EXHAUST_MAX; i++) {
			if (!exhaust[i].active) {
				// Spawn at back of bot (opposite of facing direction)
				exhaust[i].x = (bot_facing == 0) ? bot_x : bot_x + 9;
				exhaust[i].y = bot_y + 8;
				exhaust[i].vx = ((float)(rand() % 200) - 100) / 100.0f * 10.0f;
				exhaust[i].vy = -30.0f - ((float)(rand() % 300) / 100.0f * 10.0f);
				exhaust[i].life = 0.4f;
				exhaust[i].max_life = 0.4f;
				exhaust[i].active = true;
				break;
			}
		}
	}
	for (int i = 0; i < EXHAUST_MAX; i++) {
		if (!exhaust[i].active) continue;
		exhaust[i].x += exhaust[i].vx * engine.tick;
		exhaust[i].y += exhaust[i].vy * engine.tick;
		exhaust[i].life -= engine.tick;
		if (exhaust[i].life <= 0) exhaust[i].active = false;
	}

	for (int i = 0; i < NUM_LEVELS; i++) {
		if (unlock_timer[i] > 0) unlock_timer[i] -= engine.tick;
	}

	popups_update();
}

static void draw_button(int idx, int x, int y, bool selected) {
	bool unlocked = level_is_unlocked(idx);

	// Shadow
	render_draw(vec2(x + 1, y + 1), vec2(17, 17),
		RENDER_NO_TEXTURE, vec2(0, 0), vec2(0, 0), rgba(51, 51, 51, 153));

	// Button sprite
	image_t *img = unlocked ? (selected ? btn_hover : btn_normal) : btn_locked;
	if (img) {
		image_draw(img, vec2(x, y));
	}

	// Unlock animation overlay
	if (unlock_timer[idx] > 0) {
		float alpha = (sinf(unlock_timer[idx] * 20.0f) * 0.5f + 0.5f) * 200.0f;
		render_draw(vec2(x, y), vec2(16, 16), RENDER_NO_TEXTURE, vec2(0,0), vec2(0,0), rgba(255, 255, 255, (uint8_t)alpha));
	}

	// Level number text on button
	if (g.font) {
		char buf[4];
		if (idx < MAIN_LEVELS) {
			snprintf(buf, sizeof(buf), "%d", idx + 1);
		} else {
			buf[0] = 'A' + (idx - MAIN_LEVELS);
			buf[1] = '\0';
		}
		rgba_t saved_col = g.font->color;
		g.font->color = selected ? rgba(255, 0, 0, 255) : rgba(255, 255, 255, 255);
		font_draw(g.font, vec2(x + 8, y + 4), buf, FONT_ALIGN_CENTER);
		g.font->color = saved_col;
	}
}

static void draw_bigscreen(void) {
	if (!bigscreen || !g.font) return;

	int sx = RENDER_WIDTH / 2 - 65;
	int sy = RENDER_HEIGHT / 2 - 30;

	// Shadow
	render_draw(vec2(sx + 1, sy + 1), vec2(131, 86),
		RENDER_NO_TEXTURE, vec2(0, 0), vec2(0, 0), rgba(51, 51, 51, 153));
	image_draw(bigscreen, vec2(sx, sy));

	rgba_t saved = g.font->color;

	if (focus == FOCUS_ACM && achievement_is_done(acm_cursor)) {
		// Achievement info mode (only for unlocked achievements)
		g.font->color = rgba(255, 216, 0, 192);
		font_draw(g.font, vec2(sx + 65, sy + 4), "Achievement", FONT_ALIGN_CENTER);
		font_draw(g.font, vec2(sx + 65, sy + 20), (char *)acm_names[acm_cursor], FONT_ALIGN_CENTER);

		g.font->color = rgba(255, 255, 255, 192);
		font_draw(g.font, vec2(sx + 65, sy + 38), (char *)acm_descs[acm_cursor], FONT_ALIGN_CENTER);

		g.font->color = rgba(0, 255, 0, 192);
		font_draw(g.font, vec2(sx + 65, sy + 56), "UNLOCKED", FONT_ALIGN_CENTER);
	} else if (focus == FOCUS_ACM) {
		// Locked achievement — show nothing specific
		g.font->color = rgba(255, 216, 0, 192);
		font_draw(g.font, vec2(sx + 65, sy + 4), "Achievement", FONT_ALIGN_CENTER);
		g.font->color = rgba(180, 180, 180, 192);
		font_draw(g.font, vec2(sx + 65, sy + 34), "???", FONT_ALIGN_CENTER);
	} else if (show_overall_stats || focus == FOCUS_STATS) {
		// Overall stats mode
		g.font->color = rgba(255, 216, 0, 192);
		font_draw(g.font, vec2(sx + 65, sy + 4), "Personal statistics", FONT_ALIGN_CENTER);
		font_draw(g.font, vec2(sx + 65, sy + 16), "OVERALL", FONT_ALIGN_CENTER);

		char buf[32];

		// Bulbs collected
		g.font->color = rgba(255, 255, 255, 192);
		font_draw(g.font, vec2(sx + 79, sy + 34), "Bulbs:", FONT_ALIGN_RIGHT);
		g.font->color = rgba(255, 216, 0, 192);
		snprintf(buf, sizeof(buf), "%d", g.total_bulbs);
		font_draw(g.font, vec2(sx + 82, sy + 34), buf, FONT_ALIGN_LEFT);

		// Best time
		g.font->color = rgba(255, 255, 255, 192);
		font_draw(g.font, vec2(sx + 79, sy + 44), "Best time:", FONT_ALIGN_RIGHT);
		g.font->color = rgba(255, 216, 0, 192);
		int ts = save_get_total_time_s();
		snprintf(buf, sizeof(buf), "%d.000", ts);
		font_draw(g.font, vec2(sx + 82, sy + 44), buf, FONT_ALIGN_LEFT);

		// Highscore
		g.font->color = rgba(255, 255, 255, 192);
		font_draw(g.font, vec2(sx + 79, sy + 54), "Highscore:", FONT_ALIGN_RIGHT);
		g.font->color = rgba(255, 216, 0, 192);
		snprintf(buf, sizeof(buf), "%d", save_get_total_score());
		font_draw(g.font, vec2(sx + 82, sy + 54), buf, FONT_ALIGN_LEFT);

		// Deaths
		g.font->color = rgba(255, 255, 255, 192);
		font_draw(g.font, vec2(sx + 79, sy + 64), "Deaths:", FONT_ALIGN_RIGHT);
		g.font->color = rgba(255, 216, 0, 192);
		snprintf(buf, sizeof(buf), "%d", g.total_deaths);
		font_draw(g.font, vec2(sx + 82, sy + 64), buf, FONT_ALIGN_LEFT);
	} else {
		// Per-level stats mode
		int idx = (focus == FOCUS_MAIN) ? cursor : (MAIN_LEVELS + cursor);

		g.font->color = rgba(255, 216, 0, 192);
		font_draw(g.font, vec2(sx + 65, sy + 4), "Personal statistics", FONT_ALIGN_CENTER);
		font_draw(g.font, vec2(sx + 65, sy + 16), (char *)level_table[idx].name, FONT_ALIGN_CENTER);

		bool unlocked = level_is_unlocked(idx);
		if (unlocked) {
			char buf[32];

			// Bulbs
			g.font->color = rgba(255, 255, 255, 192);
			font_draw(g.font, vec2(sx + 79, sy + 34), "Bulbs:", FONT_ALIGN_RIGHT);
			g.font->color = rgba(255, 216, 0, 192);
			snprintf(buf, sizeof(buf), "%d/%d", level_best_bulbs(idx), level_table[idx].bulbs_total);
			font_draw(g.font, vec2(sx + 82, sy + 34), buf, FONT_ALIGN_LEFT);

			// Best time
			g.font->color = rgba(255, 255, 255, 192);
			font_draw(g.font, vec2(sx + 79, sy + 44), "Best time:", FONT_ALIGN_RIGHT);
			g.font->color = rgba(255, 216, 0, 192);
			int bt = level_best_time_ms(idx);
			if (bt >= 0) {
				snprintf(buf, sizeof(buf), "%.3f", bt / 1000.0f);
			} else {
				snprintf(buf, sizeof(buf), "---");
			}
			font_draw(g.font, vec2(sx + 82, sy + 44), buf, FONT_ALIGN_LEFT);

			// Score
			g.font->color = rgba(255, 255, 255, 192);
			font_draw(g.font, vec2(sx + 79, sy + 54), "Highscore:", FONT_ALIGN_RIGHT);
			g.font->color = rgba(255, 216, 0, 192);
			snprintf(buf, sizeof(buf), "%d", level_best_score(idx));
			font_draw(g.font, vec2(sx + 82, sy + 54), buf, FONT_ALIGN_LEFT);
		} else {
			g.font->color = rgba(180, 180, 180, 192);
			if (idx >= MAIN_LEVELS) {
				font_draw(g.font, vec2(sx + 65, sy + 40), "Collect bulbs", FONT_ALIGN_CENTER);
				font_draw(g.font, vec2(sx + 65, sy + 52), "to unlock!", FONT_ALIGN_CENTER);
			} else {
				font_draw(g.font, vec2(sx + 65, sy + 40), "LOCKED", FONT_ALIGN_CENTER);
			}
		}
	}
	g.font->color = saved;
}

static void draw_overall_stats_button(void) {
	if (!btn_stats) return;
	int bx = 95, by = 180;

	// Shadow
	render_draw(vec2(bx + 1, by + 1), vec2(131, 25),
		RENDER_NO_TEXTURE, vec2(0, 0), vec2(0, 0), rgba(51, 51, 51, 153));

	bool highlighted = (focus == FOCUS_STATS) || show_overall_stats;
	image_t *img = highlighted ? btn_stats_hover : btn_stats;
	if (img) image_draw(img, vec2(bx, by));
}

static void draw_achievements_panel(void) {
	if (!g.font) return;

	// Left panel: achievements (AS3: x=5, y=height/2-30, width=85)
	int px = 5;
	int py = RENDER_HEIGHT / 2 - 30 + 8;

	rgba_t saved = g.font->color;
	g.font->color = rgba(255, 255, 255, 255);
	font_draw(g.font, vec2(px + 80, py - 8), "Achievements", FONT_ALIGN_RIGHT);
	g.font->color = saved;

	// 6 achievement buttons in 2 rows of 3
	for (int i = 0; i < 6; i++) {
		int row = i % 3;
		int col = i / 3;
		int ax = px + row * 30 - 30 + 30;
		int ay = py + 10 + col * 30 - 10 + 10;
		bool selected = (focus == FOCUS_ACM && acm_cursor == i);

		// Shadow
		if (acm_template_shadow)
			image_draw(acm_template_shadow, vec2(ax + 1, ay + 1));

		// Base template (hover if selected)
		image_t *tmpl = selected ? acm_template_hover : acm_template;
		if (tmpl) image_draw(tmpl, vec2(ax, ay));

		// Achievement icon overlay (if unlocked), otherwise show "?"
		if (achievement_is_done(i) && acm_icons[i]) {
			image_draw(acm_icons[i], vec2(ax, ay));
		} else {
			rgba_t sc = g.font->color;
			g.font->color = rgba(255, 255, 255, 255);
			font_draw(g.font, vec2(ax + 8, ay + 4), "?", FONT_ALIGN_CENTER);
			g.font->color = sc;
		}
	}

	// Show achievement count below icons
	saved = g.font->color;
	int done_count = 0;
	for (int i = 0; i < 6; i++) {
		if (achievement_is_done(i)) done_count++;
	}
	char buf[32];
	g.font->color = rgba(255, 216, 0, 255);
	snprintf(buf, sizeof(buf), "%d/6 unlocked", done_count);
	font_draw(g.font, vec2(px + 80, py + 72), buf, FONT_ALIGN_RIGHT);
	g.font->color = saved;
}

static void draw_floor_tiles(void) {
	if (!tileset) return;

	// AS3 MapLevelselection.txt: row 14 = "1,2,2,...,2,3" (20 tiles)
	// Map data is 1-indexed: tile 1 = left edge, tile 2 = middle, tile 3 = right edge
	int y = 14 * TILE_SIZE;
	vec2i_t ts = vec2i(TILE_SIZE, TILE_SIZE);

	// First tile (left corner) — map tile 1
	image_draw_tile(tileset, 1, ts, vec2(0, y));
	// Middle tiles — map tile 2
	for (int x = 1; x < 19; x++) {
		image_draw_tile(tileset, 2, ts, vec2(x * TILE_SIZE, y));
	}
	// Last tile (right corner) — map tile 3
	image_draw_tile(tileset, 3, ts, vec2(19 * TILE_SIZE, y));
}

static void draw_autobot(void) {
	// Draw exhaust particles (behind bot)
	for (int i = 0; i < EXHAUST_MAX; i++) {
		if (!exhaust[i].active) continue;
		float alpha = (exhaust[i].life / exhaust[i].max_life) * 180.0f;
		render_draw(vec2(exhaust[i].x, exhaust[i].y), vec2(2, 2),
			RENDER_NO_TEXTURE, vec2(0, 0), vec2(0, 0),
			rgba(200, 200, 200, (uint8_t)alpha));
	}

	if (!bot_img) return;

	// badrobot.png is 39x13 = 3 frames of 13x13
	// frames: 0,1 = walk, 2 = jump
	vec2i_t ts = vec2i(13, 13);
	bool flip = (bot_facing == 1);
	image_draw_tile_ex(bot_img, bot_frame, ts, vec2(bot_x, bot_y), flip, false, rgba(255, 255, 255, 255));
}

static void draw(void) {
	// Background
	if (bg_img) {
		image_draw(bg_img, vec2(0, 0));
	} else {
		render_draw(vec2(0, 0), vec2(RENDER_WIDTH, RENDER_HEIGHT),
			RENDER_NO_TEXTURE, vec2(0, 0), vec2(0, 0), rgba(20, 20, 40, 255));
	}

	if (!g.font) return;

	// Scrolling top text
	rgba_t saved = g.font->color;
	g.font->color = rgba(255, 255, 255, 192);
	font_draw(g.font, vec2((int)scroll_x, 1),
		"Select a level or press ESC to get back to menu", FONT_ALIGN_LEFT);
	font_draw(g.font, vec2((int)scroll_x + RENDER_WIDTH, 1),
		"Select a level or press ESC to get back to menu", FONT_ALIGN_LEFT);
	g.font->color = saved;

	// Floor tiles at bottom
	draw_floor_tiles();

	// Main level buttons: 2 rows of 9
	for (int i = 0; i < MAIN_LEVELS; i++) {
		int row = i % BTNS_PER_ROW;
		int col = i / BTNS_PER_ROW;
		int x = (row + 1) * 30;
		int y = (col + 1) * 30;
		bool selected = (focus == FOCUS_MAIN && i == cursor);
		draw_button(i, x, y, selected);
	}

	// Bigscreen in center
	draw_bigscreen();

	// Overall stats button
	draw_overall_stats_button();

	// Achievements panel (left side)
	draw_achievements_panel();

	// Bonus level buttons: right side panel, 2 rows of 3
	int bonus_x0 = RENDER_WIDTH - 85;
	int bonus_y0 = RENDER_HEIGHT / 2 - 30 + 8;

	saved = g.font->color;
	g.font->color = rgba(255, 255, 255, 255);
	font_draw(g.font, vec2(bonus_x0, bonus_y0 - 8), "Bonus levels", FONT_ALIGN_LEFT);
	g.font->color = saved;

	for (int i = 0; i < BONUS_LEVELS; i++) {
		int row = i % 3;
		int col = i / 3;
		int x = bonus_x0 + row * 30;
		int y = bonus_y0 + 10 + col * 30;
		bool selected = (focus == FOCUS_BONUS && i == cursor);
		draw_button(MAIN_LEVELS + i, x, y, selected);
	}

	// Bonus level unlock info text
	if (focus == FOCUS_BONUS) {
		int idx = MAIN_LEVELS + cursor;
		if (!level_is_unlocked(idx)) {
			saved = g.font->color;
			g.font->color = rgba(255, 255, 255, 255);
			static const char *bonus_reqs[6] = {
				"Collect 50\nbulbs to\nunlock",
				"Collect 200\nbulbs to\nunlock",
				"Collect 350\nbulbs to\nunlock",
				"Collect 500\nbulbs to\nunlock",
				"Collect 650\nbulbs to\nunlock",
				"Collect 800\nbulbs to\nunlock"
			};
			font_draw(g.font, vec2(bonus_x0, bonus_y0 + 72), (char *)bonus_reqs[cursor], FONT_ALIGN_LEFT);
			g.font->color = saved;
		}
	}

	// AutoBot walking on floor
	draw_autobot();

	popups_draw();
}

static void cleanup(void) {
	sound_pause(g.music_menu);
}

scene_t scene_levelselect = {
	.init = init,
	.update = update,
	.draw = draw,
	.cleanup = cleanup,
};
