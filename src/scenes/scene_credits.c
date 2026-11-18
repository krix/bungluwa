#include "../main.h"

#define CREDITS_SCROLL_SPEED 25.0f
#define LINE_SPACING 9.0f

typedef struct {
	const char *text;
	bool is_header;
} credit_line_t;

static const credit_line_t lines[] = {
	{"B U N G L U W A", true},
	{"C R E D I T S", true},
	{"", false},
	{"", false},
	{"", false},
	{"Design and Code", true},
	{"Dirk \"krix\" Bunk", false},
	{"", false},
	{"", false},
	{"Music, Sound FX and Additional Support", true},
	{"Andreas \"fate\" Loesch", false},
	{"", false},
	{"", false},
	{"Beta Testers", true},
	{"Andreas \"fate\" Loesch", false},
	{"Dominic \"chaos\" Szablewski", false},
	{"Tim \"hate\" Juraschka", false},
	{"Patrick \"rib\" Thomann", false},
	{"Christian \"madcow\" Nauheimer", false},
	{"Robin \"rob\" Croenlein", false},
	{"Jens \"william\" Willhardt", false},
	{"Lilly M.", false},
	{"", false},
	{"", false},
	{"Special Thanks to", true},
	{"Adam \"atomic\" Saltsman", false},
	{"E. Hashimoto", false},
	{"", false},
	{"", false},
	{"", false},
	{"", false},
	{"", false}, // professor sprite would go here
	{"", false},
	{"", false},
	{"", false},
	{"", false},
	{"", false},
	{"", false},
	{"", false},
	{"", false},
	{"", false},
	{"BUNGLUWA", true},
	{"(c) 2 0 1 0", true},
};

#define NUM_LINES (sizeof(lines) / sizeof(lines[0]))

static float scroll_y;
static bool stopped;
static float fade_timer;
static image_t *bg;

static void init(void) {
	scroll_y = (float)RENDER_HEIGHT;
	stopped = false;
	fade_timer = 0;
	bg = image("assets/sprites/background.qoi");
}

static void update(void) {
	// Skip credits
	if (input_pressed(A_CONFIRM) || input_pressed(A_JUMP) || input_pressed(A_PAUSE) || input_pressed(A_BACK)) {
		engine_set_scene(&scene_menu);
		return;
	}

	if (!stopped) {
		scroll_y -= CREDITS_SCROLL_SPEED * engine.tick;
		// Check if last line reached mid-screen
		float last_y = scroll_y + (NUM_LINES - 1) * LINE_SPACING;
		if (last_y < RENDER_HEIGHT / 2.0f) {
			stopped = true;
			fade_timer = 0;
		}
	} else {
		fade_timer += engine.tick;
		if (fade_timer > 4.0f) {
			engine_set_scene(&scene_menu);
			return;
		}
	}
}

static void draw(void) {
	// Background (dimmed)
	if (bg) {
		vec2i_t bgsz = image_size(bg);
		image_draw_ex(bg, vec2(0, 0), vec2(bgsz.x, bgsz.y),
			vec2(0, 0), vec2(RENDER_WIDTH, RENDER_HEIGHT), rgba(100, 100, 100, 255));
	}

	if (!g.font) return;

	for (int i = 0; i < (int)NUM_LINES; i++) {
		float y = scroll_y + i * LINE_SPACING;
		if (y < -10 || y > RENDER_HEIGHT + 10) continue;
		if (lines[i].text[0] == '\0') continue;

		rgba_t saved = g.font->color;
		if (lines[i].is_header) {
			g.font->color = rgba(255, 216, 0, 255); // gold
		} else {
			g.font->color = rgba(255, 255, 255, 255);
		}
		font_draw(g.font, vec2(RENDER_WIDTH / 2, y), (char *)lines[i].text, FONT_ALIGN_CENTER);
		g.font->color = saved;
	}

	// Fade out overlay
	if (stopped && fade_timer > 0) {
		uint8_t alpha = (uint8_t)min(fade_timer / 4.0f * 255, 255);
		render_draw(vec2(0, 0), vec2(RENDER_WIDTH, RENDER_HEIGHT),
			RENDER_NO_TEXTURE, vec2(0, 0), vec2(0, 0), rgba(0, 0, 0, alpha));
	}
}

scene_t scene_credits = {
	.init = init,
	.update = update,
	.draw = draw,
};
