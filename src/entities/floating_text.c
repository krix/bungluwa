#include "../main.h"

EDITOR_IGNORE(true);

static void init(entity_t *self) {
	self->physics = ENTITY_PHYSICS_NONE;
	self->floating_text.life = 1.0f;
	self->draw_order = 100; // draw on top
}

static void update(entity_t *self) {
	self->pos.y -= 20.0f * engine.tick;
	self->floating_text.life -= engine.tick;
	if (self->floating_text.life <= 0) {
		entity_kill(self);
	}
}

static void draw(entity_t *self, vec2_t viewport) {
	if (!g.font) return;
	float alpha = clamp(self->floating_text.life, 0, 1);
	vec2_t screen_pos = vec2_sub(self->pos, viewport);
	rgba_t saved = g.font->color;
	g.font->color = rgba(self->anim.color.r, self->anim.color.g, self->anim.color.b,
		(uint8_t)(alpha * 255));
	font_draw(g.font, screen_pos, self->floating_text.text, FONT_ALIGN_CENTER);
	g.font->color = saved;
}

entity_vtab_t entity_vtab_floating_text = {
	.init = init,
	.update = update,
	.draw = draw,
};
