#ifndef MAIN_H
#define MAIN_H

#include "high_impact.h"

// ---------------------------------------------------------------------------
// Input actions

typedef enum {
	A_LEFT,
	A_RIGHT,
	A_UP,
	A_DOWN,
	A_JUMP,
	A_PAUSE,
	A_CONFIRM,
	A_BACK,
	A_DEBUG,
	A_KILL,
	A_RESET_O,
	A_RESET_K,
	A_NUM
} action_t;

// ---------------------------------------------------------------------------
// Camera modes

typedef enum {
	CAM_FREE,
	CAM_SCROLL_UP,
	CAM_SCROLL_DOWN,
} cam_mode_t;

// ---------------------------------------------------------------------------
// Level table

typedef struct {
	const char *path;
	const char *name;
	float time_limit;
	int bulbs_total;
	cam_mode_t cam_mode;
	float auto_scroll;
	bool dark;
	bool bonus;
} level_info_t;

#define NUM_LEVELS 24
extern const level_info_t level_table[NUM_LEVELS];

// ---------------------------------------------------------------------------
// Popup queue (sliding notification system)

#define POPUP_MAX 16
#define POPUP_STATUS_IN   0
#define POPUP_STATUS_WAIT 1
#define POPUP_STATUS_OUT  2
#define POPUP_STATUS_DONE 3

typedef struct {
	char text[64];
	int status;
	float y;
	float wait_timer;
} popup_t;

// ---------------------------------------------------------------------------
// Ghost data
 
typedef struct {
	uint8_t input;
	float tick;
} ghost_frame_t;

typedef struct {
	ghost_frame_t *frames;
	int count;
	int capacity;
} ghost_data_t;

// ---------------------------------------------------------------------------
// Globals

typedef struct {
	font_t *font;
	sound_t music_play[4];
	sound_t music_menu;
	int music_play_index;
	entity_ref_t player;

	// Per-level transient
	int    level_index;
	float  level_time;
	float  time_limit;
	int    bulbs_collected;
	int    bulbs_total;
	int    score;
	int    high_score;
	int    extras;
	int    deaths;

	// Combo
	float  combo_window;
	int    combo_count;

	// Alarm
	float  alarm_time;

	// Camera
	camera_t camera;
	cam_mode_t cam_mode;
	float cam_scroll_speed;

	// Popups
	popup_t popups[POPUP_MAX];
	int popup_count;

	// Cumulative stats (persisted)
	int total_bulbs;
	int total_deaths;
	int total_jumps;
	int levels_completed;
	int levels_no_death;
	int levels_all_bulbs;

	// Ghost
	ghost_data_t recording;
	ghost_data_t playback;

	bool debug_hitboxes;
} global_t;

extern global_t g;

// ---------------------------------------------------------------------------
// Scenes

extern scene_t scene_menu;
extern scene_t scene_help;
extern scene_t scene_levelselect;
extern scene_t scene_play;
extern scene_t scene_levelcomplete;
extern scene_t scene_credits;

// ---------------------------------------------------------------------------
// Helpers

void game_load_level(int index);
void game_player_died(void);
void game_player_exited(void);
void game_collect_bulb(entity_t *bulb, entity_t *player);
void game_collect_extra(entity_t *extra, entity_t *player);
void game_trigger_alarm(void);

void spawn_popup(const char *text);
void popups_update(void);
void popups_draw(void);
entity_t *spawn_smoke(vec2_t pos, int count);
entity_t *spawn_floating_text(vec2_t pos, const char *text, rgba_t color);
entity_t *spawn_twinkle(vec2_t pos);

// Sound sources (loaded once in main_init)
extern sound_source_t *snd_jump;
extern sound_source_t *snd_land;
extern sound_source_t *snd_bulb;
extern sound_source_t *snd_explode;
extern sound_source_t *snd_extralife;
extern sound_source_t *snd_zap;
extern sound_source_t *snd_alarm;
extern sound_source_t *snd_exit;
extern sound_source_t *snd_click;
extern sound_source_t *snd_voice[3];
extern sound_source_t *snd_unlocked[4];

// Shared anim defs (particles)
extern anim_def_t *anim_smoke;
extern anim_def_t *anim_twinkle_def;
extern anim_def_t *anim_exit_particle;
extern anim_def_t *anim_gib_body;
extern anim_def_t *anim_gib_head;
extern anim_def_t *anim_gib_arm;
extern anim_def_t *anim_gib_wheel;
extern anim_def_t *anim_bulb_gibs;
extern anim_def_t *anim_sparks;

void spawn_gibs(vec2_t pos);
void spawn_exit_particles(entity_t *exit_ent);
void spawn_bulb_gibs(vec2_t pos);
void spawn_sparks(vec2_t pos);

// Save system
void save_load(void);
void save_store(bool update_level_results);
void save_reset(void);
void save_complete_level(void);
void achievements_check(void);
bool achievement_is_done(int idx);
int level_best_bulbs(int idx);
int save_get_total_time_s(void);
int save_get_total_score(void);

// Ghost system
void ghost_reset(ghost_data_t *data);
void ghost_record_frame(ghost_data_t *data, uint8_t input, float tick);
void ghost_save(int level_index);
void ghost_load(int level_index);

#endif
