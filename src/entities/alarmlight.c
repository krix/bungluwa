#include "../main.h"

EDITOR_SIZE(16, 16);
EDITOR_COLOR(255, 200, 0);

// AS3 Alarmlight: creates a Light with ImgLightTriangle at (x-16, y-18),
// color 0xff5959, alpha 0.5, blur 14.
// In C port: per-alarm light with its own timer, re-triggerable, rendered
// as a soft glow using the light texture.

static anim_def_t *a_idle;
static image_t *red_light_img;

static void load(void) {
	image_t *sh = image("assets/sprites/alarmlight.qoi");
	a_idle = anim_def(sh, vec2i(16, 16), 1.0, {0});
	red_light_img = image("assets/sprites/red_light.qoi");
}

static void init(entity_t *self) {
	self->size = vec2(16, 16);
	self->anim = anim(a_idle);
	self->physics = ENTITY_PHYSICS_NONE;
	self->group = ENTITY_GROUP_ITEM;
	self->draw_order = 100;
	self->alarmlight.triggered = true;
	self->alarmlight.timer = 0;
}

static void update(entity_t *self) {
	// Always on — advance timer for pulsing effect
	self->alarmlight.timer += engine.tick;
}

static void draw(entity_t *self, vec2_t viewport) {
	entity_base_draw(self, viewport);

	if (red_light_img) {
		float pulse = sinf(self->alarmlight.timer * 10.0f) * 0.15f + 0.45f;
		uint8_t alpha = (uint8_t)(pulse * 255);
		vec2_t s = vec2_sub(self->pos, viewport);

		vec2i_t sz = image_size(red_light_img);
		float lx = s.x + 8 - sz.x * 0.5f;
		float ly = s.y;

		render_set_blend_mode(RENDER_BLEND_LIGHTER);
		image_draw_ex(red_light_img,
			vec2(0, 0), vec2_from_vec2i(sz),
			vec2(lx, ly), vec2_from_vec2i(sz),
			rgba(255, 255, 255, alpha));
		render_set_blend_mode(RENDER_BLEND_NORMAL);
	}
}

entity_vtab_t entity_vtab_alarmlight = {
	.load = load,
	.init = init,
	.update = update,
	.draw = draw,
};
