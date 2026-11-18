#include "../main.h"

static image_t *help_img;
static image_t *bg;

static void init(void) {
	help_img = image("assets/sprites/help_screen.qoi");
	bg = image("assets/sprites/background.qoi");
}

static void update(void) {
	if (input_pressed(A_BACK) || input_pressed(A_JUMP) || input_pressed(A_CONFIRM)) {
		engine_set_scene(&scene_menu);
	}
}

static void draw(void) {
	// Background pattern (darker tint)
	if (bg) {
		vec2i_t bgsz = image_size(bg);
		image_draw_ex(bg, vec2(0, 0), vec2(bgsz.x, bgsz.y),
			vec2(0, 0), vec2(RENDER_WIDTH, RENDER_HEIGHT), rgba(140, 140, 140, 255));
	}

	// Help image centered on top
	if (help_img) {
		vec2i_t sz = image_size(help_img);
		float x = (RENDER_WIDTH - sz.x) / 2.0f;
		float y = (RENDER_HEIGHT - sz.y) / 2.0f;
		image_draw(help_img, vec2(x, y));
	}
}

static void cleanup(void) {
}

scene_t scene_help = {
	.init = init,
	.update = update,
	.draw = draw,
	.cleanup = cleanup,
};
