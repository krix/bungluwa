#include "../main.h"

// Animated score counting (AS3: LevelCompleteState)
// Phase 0: count bulbs up, Phase 1: count time up, Phase 2: count total score up

static int phase; // 0=bulbs, 1=time, 2=score, 3=done
static int frame_counter;

static int bulb_counter;
static int bulb_bonus_val;
static float time_counter;
static float time_left;
static int time_bonus_val;
static int base_score;
static int total_score;
static int score_counter;
static bool all_bulbs;

static image_t *bg_img;
static image_t *twinkle_img;
static float twinkle_timer;

#define MAX_TWINKLES 8
static struct { float x, y, life; } twinkles[MAX_TWINKLES];
static int twinkle_count;

static void init(void) {
	phase = 0;
	frame_counter = 0;
	bulb_counter = 0;
	bulb_bonus_val = 0;
	time_counter = 0;
	time_left = g.time_limit - g.level_time;
	if (time_left < 0) time_left = 0;
	time_bonus_val = (int)(time_left * 1000);
	base_score = g.score;
	all_bulbs = (g.bulbs_collected >= g.bulbs_total && g.bulbs_total > 0);

	// Final total: base_score + bulb_bonus (x10 if all) + time_bonus
	total_score = base_score;
	if (all_bulbs)
		total_score += g.bulbs_collected * 100 * 10;
	else
		total_score += g.bulbs_collected * 100;
	total_score += time_bonus_val;

	score_counter = base_score;

	bg_img = image("assets/sprites/background.qoi");
	twinkle_img = image("assets/sprites/twinkle.qoi");
	twinkle_timer = 0;
	twinkle_count = 0;

	sound_play(snd_exit);
}

static void update(void) {
	frame_counter++;
	if (frame_counter > 60) frame_counter = 0;

	// Spawn random twinkles
	twinkle_timer -= engine.tick;
	if (twinkle_timer <= 0) {
		twinkle_timer = rand_float(0.15f, 0.4f);
		if (twinkle_count < MAX_TWINKLES) {
			int idx = twinkle_count++;
			twinkles[idx].x = rand_float(10, RENDER_WIDTH - 10);
			twinkles[idx].y = rand_float(10, RENDER_HEIGHT - 10);
			twinkles[idx].life = 0.6f;
		}
	}
	// Update twinkle lifetimes
	for (int i = twinkle_count - 1; i >= 0; i--) {
		twinkles[i].life -= engine.tick;
		if (twinkles[i].life <= 0) {
			twinkles[i] = twinkles[--twinkle_count];
		}
	}

	// Phase 0: count bulbs
	if (phase == 0) {
		if (frame_counter % 3 == 0 && bulb_counter < g.bulbs_collected) {
			bulb_counter++;
			bulb_bonus_val = bulb_counter * 100;
		}
		if (bulb_counter >= g.bulbs_collected) {
			phase = 1;
			if (all_bulbs)
				score_counter = base_score + bulb_bonus_val * 10;
			else
				score_counter = base_score + bulb_bonus_val;
		}
	}

	// Phase 1: count time
	if (phase == 1) {
		if (frame_counter % 3 == 0 && time_counter < time_left) {
			time_counter += 1.0f;
			if (time_counter > time_left) time_counter = time_left;
		}
		if (time_counter >= time_left) {
			phase = 2;
			score_counter += time_bonus_val;
		}
	}

	// Phase 2: count score
	if (phase == 2) {
		if (score_counter < total_score) {
			score_counter += (int)(rand_float(500, 1100));
			if (score_counter > total_score) score_counter = total_score;
		}
		if (score_counter >= total_score) {
			phase = 3;
		}
	}

	// Input: ENTER/JUMP skips counting or goes to level select
	if (input_pressed(A_JUMP) || input_pressed(A_CONFIRM)) {
		if (phase < 3) {
			bulb_counter = g.bulbs_collected;
			bulb_bonus_val = bulb_counter * 100;
			time_counter = time_left;
			score_counter = total_score;
			phase = 3;
		} else {
			sound_play(snd_click);
			save_complete_level();
			engine_set_scene(&scene_levelselect);
		}
	}

	if (input_pressed(A_BACK)) {
		save_complete_level();
		engine_set_scene(&scene_levelselect);
	}

	popups_update();
}

static void draw(void) {
	// Background
	if (bg_img) {
		image_draw(bg_img, vec2(0, 0));
	} else {
		render_draw(vec2(0, 0), vec2(RENDER_WIDTH, RENDER_HEIGHT),
			RENDER_NO_TEXTURE, vec2(0, 0), vec2(0, 0), rgba(20, 20, 40, 255));
	}

	// Draw twinkles (3 frames of 8x8 in twinkle.qoi)
	if (twinkle_img) {
		for (int i = 0; i < twinkle_count; i++) {
			int frame = (int)((0.6f - twinkles[i].life) / 0.1f);
			if (frame < 0) frame = 0;
			if (frame > 2) frame = 2;
			image_draw_ex(twinkle_img,
				vec2(frame * 8, 0), vec2(8, 8),
				vec2(twinkles[i].x - 4, twinkles[i].y - 4), vec2(8, 8),
				rgba(255, 255, 255, 255));
		}
	}

	if (!g.font) return;

	char buf[64];
	rgba_t saved = g.font->color;

	// Header — yellow
	g.font->color = rgba(255, 216, 0, 255);

	// Level name in header
	const char *lvl_name = level_table[g.level_index].name;
	snprintf(buf, sizeof(buf), "%s complete!", lvl_name);
	font_draw(g.font, vec2(RENDER_WIDTH / 2, 10), buf, FONT_ALIGN_CENTER);

	// Bulbs collected
	g.font->color = rgba(255, 255, 255, 255);
	font_draw(g.font, vec2(RENDER_WIDTH / 2 - 4, 50), "Bulbs collected:", FONT_ALIGN_RIGHT);
	g.font->color = rgba(255, 216, 0, 255);
	snprintf(buf, sizeof(buf), "%d/%d", bulb_counter, g.bulbs_total);
	font_draw(g.font, vec2(RENDER_WIDTH / 2, 50), buf, FONT_ALIGN_LEFT);

	// Bulb bonus
	g.font->color = rgba(255, 255, 255, 255);
	font_draw(g.font, vec2(RENDER_WIDTH / 2 - 4, 60), "Bulb bonus:", FONT_ALIGN_RIGHT);
	g.font->color = rgba(255, 136, 0, 255);
	if (all_bulbs && bulb_counter == g.bulbs_collected)
		snprintf(buf, sizeof(buf), "%d x10", bulb_bonus_val);
	else
		snprintf(buf, sizeof(buf), "%d", bulb_bonus_val);
	font_draw(g.font, vec2(RENDER_WIDTH / 2, 60), buf, FONT_ALIGN_LEFT);

	// Time left
	g.font->color = rgba(255, 255, 255, 255);
	font_draw(g.font, vec2(RENDER_WIDTH / 2 - 4, 100), "Time left:", FONT_ALIGN_RIGHT);
	g.font->color = rgba(255, 216, 0, 255);
	snprintf(buf, sizeof(buf), "%.0f", time_counter);
	font_draw(g.font, vec2(RENDER_WIDTH / 2, 100), buf, FONT_ALIGN_LEFT);

	// Time bonus
	g.font->color = rgba(255, 255, 255, 255);
	font_draw(g.font, vec2(RENDER_WIDTH / 2 - 4, 110), "Time bonus:", FONT_ALIGN_RIGHT);
	g.font->color = rgba(255, 136, 0, 255);
	snprintf(buf, sizeof(buf), "%d", time_bonus_val);
	font_draw(g.font, vec2(RENDER_WIDTH / 2, 110), buf, FONT_ALIGN_LEFT);

	// Level score
	g.font->color = rgba(255, 255, 255, 255);
	font_draw(g.font, vec2(RENDER_WIDTH / 2 - 4, 150), "Level score:", FONT_ALIGN_RIGHT);
	g.font->color = rgba(255, 136, 0, 255);
	snprintf(buf, sizeof(buf), "%d", base_score);
	font_draw(g.font, vec2(RENDER_WIDTH / 2, 150), buf, FONT_ALIGN_LEFT);

	// Total score
	g.font->color = rgba(255, 255, 255, 255);
	font_draw(g.font, vec2(RENDER_WIDTH / 2 - 4, 160), "Total score:", FONT_ALIGN_RIGHT);
	g.font->color = rgba(255, 0, 0, 255);
	snprintf(buf, sizeof(buf), "%d", score_counter);
	font_draw(g.font, vec2(RENDER_WIDTH / 2, 160), buf, FONT_ALIGN_LEFT);

	// Continue prompt (only when counting done)
	if (phase >= 3) {
		g.font->color = rgba(255, 255, 255, 255);
		font_draw(g.font, vec2(RENDER_WIDTH / 2, 200), "Continue with ENTER", FONT_ALIGN_CENTER);
	}

	g.font->color = saved;

	popups_draw();
}

static void cleanup(void) {
}

scene_t scene_levelcomplete = {
	.init = init,
	.update = update,
	.draw = draw,
	.cleanup = cleanup,
};
