#include "../main.h"

static int selection;
static const int NUM_OPTIONS = 4;
static const char *options[] = { "PLAY", "HELP", "CREDITS", "RESET" };
static const char *descriptions[] = {
	"get into action!",
	"learn how to play",
	"meet the creators",
	"reset all your progress"
};
static image_t *bg;
static image_t *logo;
static image_t *reset_warning_img;
static bool confirm_reset;

// Draw text at 2x scale (uses font's shadow setting automatically)
static void draw_text_scaled(font_t *f, vec2_t pos, char *text, font_align_t align, rgba_t color, float scale) {
	// Measure width at 1x to compute alignment offset
	int width = font_line_width(f, text);
	float scaled_width = width * scale;
	float x = pos.x;
	if (align == FONT_ALIGN_CENTER) x -= scaled_width / 2;
	else if (align == FONT_ALIGN_RIGHT) x -= scaled_width;

	// Shadow pass (offset scales with text size)
	if (f->shadow.a > 0) {
		float soff = scale; // shadow offset proportional to scale
		float sx = x + soff;
		for (char *c = text; *c != '\0' && *c != '\n'; c++) {
			if (*c >= f->first_char && *c <= f->last_char) {
				font_glyph_t *gl = &f->glyphs[(int)(*c) - f->first_char];
				vec2_t dst_pos = vec2(sx + gl->offset.x * scale, pos.y + soff + gl->offset.y * scale);
				vec2_t dst_size = vec2(gl->size.x * scale, gl->size.y * scale);
				image_draw_ex(f->image, gl->pos, gl->size, dst_pos, dst_size, f->shadow);
				sx += (gl->advance + f->letter_spacing) * scale;
			}
		}
	}

	// Main text pass
	for (char *c = text; *c != '\0' && *c != '\n'; c++) {
		if (*c >= f->first_char && *c <= f->last_char) {
			font_glyph_t *gl = &f->glyphs[(int)(*c) - f->first_char];
			vec2_t dst_pos = vec2(x + gl->offset.x * scale, pos.y + gl->offset.y * scale);
			vec2_t dst_size = vec2(gl->size.x * scale, gl->size.y * scale);
			image_draw_ex(f->image, gl->pos, gl->size, dst_pos, dst_size, color);
			x += (gl->advance + f->letter_spacing) * scale;
		}
	}
}

static void init(void) {
	selection = 0;
	confirm_reset = false;
	sound_unpause(g.music_menu);
	bg = image("assets/sprites/background.qoi");
	logo = image("assets/sprites/logo.qoi");
	reset_warning_img = image("assets/sprites/reset_warning.qoi");
	entities_reset();
	sound_play(snd_voice[rand() % 3]);
}

static void update(void) {
	// Reset confirmation dialog
	if (confirm_reset) {
		if (input_state(A_RESET_O) > 0.5f && input_state(A_RESET_K) > 0.5f) {
			sound_play(snd_click);
			save_reset();
			confirm_reset = false;
			spawn_popup("Save data reset!");
		}
		if (input_pressed(A_BACK) || input_pressed(A_PAUSE)) {
			sound_play(snd_click);
			confirm_reset = false;
		}
		return;
	}

	if (input_pressed(A_DOWN)) {
		selection = (selection + 1) % NUM_OPTIONS;
		sound_play(snd_click);
	}
	if (input_pressed(A_UP)) {
		selection = (selection - 1 + NUM_OPTIONS) % NUM_OPTIONS;
		sound_play(snd_click);
	}

	if (input_pressed(A_JUMP) || input_pressed(A_CONFIRM)) {
		sound_play(snd_click);
		switch (selection) {
			case 0: engine_set_scene(&scene_levelselect); break;
			case 1: engine_set_scene(&scene_help); break;
			case 2: engine_set_scene(&scene_credits); break;
			case 3: confirm_reset = true; break;
		}
	}

	if (input_pressed(A_BACK)) {
		platform_exit();
	}
	
	// Random twinkles around the logo
	if (logo && (rand() % 100) > 96) {
		vec2i_t lsz = image_size(logo);
		float lx = (RENDER_WIDTH - lsz.x) / 2.0f;
		float tx = lx + (float)(rand() % lsz.x);
		float ty = 20.0f + (float)(rand() % lsz.y);
		spawn_twinkle(vec2(tx, ty));
	}

	scene_base_update();
	popups_update();
}

static void draw(void) {
	// Background image
	if (bg) {
		vec2i_t bgsz = image_size(bg);
		image_draw_ex(bg, vec2(0, 0), vec2(bgsz.x, bgsz.y),
			vec2(0, 0), vec2(RENDER_WIDTH, RENDER_HEIGHT), rgba(255, 255, 255, 255));
	} else {
		render_draw(vec2(0, 0), vec2(RENDER_WIDTH, RENDER_HEIGHT),
			RENDER_NO_TEXTURE, vec2(0, 0), vec2(0, 0), rgba(20, 20, 40, 255));
	}

	// Logo
	if (logo) {
		vec2i_t lsz = image_size(logo);
		float lx = (RENDER_WIDTH - lsz.x) / 2.0f;
		image_draw(logo, vec2(lx, 20));
	}

	if (!g.font) return;

	// Menu options at 2x scale
	int menu_y = logo ? (int)(20 + image_size(logo).y + 20) : 100;
	float scale = 2.0f;
	int spacing = 22;

	for (int i = 0; i < NUM_OPTIONS; i++) {
		int y = menu_y + i * spacing;
		rgba_t col;
		if (i == selection) {
			col = rgba(255, 216, 0, 255); // yellow #FFD800 when selected
		} else {
			col = rgba(255, 255, 255, 255); // white when not selected
		}

		draw_text_scaled(g.font, vec2(RENDER_WIDTH / 2, y),
			(char *)options[i], FONT_ALIGN_CENTER, col, scale);
	}

	// Description text at bottom for selected option
	rgba_t saved = g.font->color;
	g.font->color = rgba(255, 255, 255, 192);
	font_draw(g.font, vec2(RENDER_WIDTH / 2, RENDER_HEIGHT - 20),
		(char *)descriptions[selection], FONT_ALIGN_CENTER);
	g.font->color = saved;

	// Reset confirmation overlay — show original warning image centered
	if (confirm_reset && reset_warning_img) {
		vec2i_t sz = image_size(reset_warning_img);
		float rx = (RENDER_WIDTH - sz.x) / 2.0f;
		float ry = (RENDER_HEIGHT - sz.y) / 2.0f;
		image_draw(reset_warning_img, vec2(rx, ry));
	}

	scene_base_draw();
	popups_draw();
}

static void cleanup(void) {
	sound_pause(g.music_menu);
}

scene_t scene_menu = {
	.init = init,
	.update = update,
	.draw = draw,
	.cleanup = cleanup,
};
