#include "main.h"
#include <string.h>

// Save data (persisted as JSON to userdata)

typedef struct {
	int best_time_ms[NUM_LEVELS];
	int best_score[NUM_LEVELS];
	int bulbs_per_level[NUM_LEVELS];
	bool no_death[NUM_LEVELS];
	bool all_bulbs_flag[NUM_LEVELS];

	int total_bulbs;
	int total_deaths;
	int total_jumps;
	int levels_completed;

	bool achievements[6];
	bool bonus_unlocked[6];
	bool unlock_notified[NUM_LEVELS];
	bool muted;
} save_t;

static save_t save_data;

void save_load(void) {
	memset(&save_data, 0, sizeof(save_data));
	for (int i = 0; i < NUM_LEVELS; i++) {
		save_data.best_time_ms[i] = -1;
	}
	save_data.unlock_notified[0] = true;

	uint32_t len = 0;
	uint8_t *data = platform_load_userdata("save.json", &len);
	if (!data || len == 0) return;

	json_t *root = json_parse(data, len);
	if (!root) return;

	// Load per-level data
	json_t *times = json_value_for_key(root, "best_time_ms");
	json_t *scores = json_value_for_key(root, "best_score");
	json_t *bulbs = json_value_for_key(root, "bulbs_per_level");

	for (int i = 0; i < NUM_LEVELS; i++) {
		json_t *tv = times ? json_value_at(times, i) : NULL;
		json_t *sv = scores ? json_value_at(scores, i) : NULL;
		json_t *bv = bulbs ? json_value_at(bulbs, i) : NULL;
		if (tv) save_data.best_time_ms[i] = json_number(tv);
		if (sv) save_data.best_score[i] = json_number(sv);
		if (bv) save_data.bulbs_per_level[i] = json_number(bv);
	}

	// Load cumulative stats
	json_t *v;
	if ((v = json_value_for_key(root, "total_bulbs")))     save_data.total_bulbs     = json_number(v);
	if ((v = json_value_for_key(root, "total_deaths")))    save_data.total_deaths    = json_number(v);
	if ((v = json_value_for_key(root, "total_jumps")))     save_data.total_jumps     = json_number(v);
	if ((v = json_value_for_key(root, "levels_completed")))save_data.levels_completed= json_number(v);
	
	json_t *notified = json_value_for_key(root, "unlock_notified");
	for (int i = 0; notified && i < NUM_LEVELS; i++) {
		json_t *nv = json_value_at(notified, i);
		if (nv) save_data.unlock_notified[i] = json_number(nv) != 0;
	}

	if ((v = json_value_for_key(root, "muted")))           save_data.muted           = json_number(v) != 0;

	// Sync to globals
	g.total_bulbs = save_data.total_bulbs;
	g.total_deaths = save_data.total_deaths;
	g.total_jumps = save_data.total_jumps;
	g.levels_completed = save_data.levels_completed;

	// Free temp allocations (json_parse + file_load use temp_alloc)
	temp_free(root);
	temp_free(data);
}

void save_store(bool update_level_results) {
	// Sync from globals
	save_data.total_bulbs = g.total_bulbs;
	save_data.total_deaths = g.total_deaths;
	save_data.total_jumps = g.total_jumps;
	save_data.levels_completed = g.levels_completed;

	// Update current level results
	if (update_level_results) {
		int idx = g.level_index;
		int time_ms = (int)(g.level_time * 1000);
		if (save_data.best_time_ms[idx] < 0 || time_ms < save_data.best_time_ms[idx]) {
			save_data.best_time_ms[idx] = time_ms;
			ghost_save(idx);
		}
		if (g.score > save_data.best_score[idx]) {
			save_data.best_score[idx] = g.score;
		}
		if (g.bulbs_collected > save_data.bulbs_per_level[idx]) {
			save_data.bulbs_per_level[idx] = g.bulbs_collected;
		}
		if (g.deaths == 0)
			save_data.no_death[idx] = true;
		if (g.bulbs_collected >= g.bulbs_total)
			save_data.all_bulbs_flag[idx] = true;
	}

	// Serialize to JSON string (manual — pl_json is read-only)
	char buf[4096];
	int off = 0;
	off += snprintf(buf + off, sizeof(buf) - off, "{\n");

	// best_time_ms
	off += snprintf(buf + off, sizeof(buf) - off, "  \"best_time_ms\": [");
	for (int i = 0; i < NUM_LEVELS; i++)
		off += snprintf(buf + off, sizeof(buf) - off, "%d%s", save_data.best_time_ms[i], i < NUM_LEVELS-1 ? "," : "");
	off += snprintf(buf + off, sizeof(buf) - off, "],\n");

	// best_score
	off += snprintf(buf + off, sizeof(buf) - off, "  \"best_score\": [");
	for (int i = 0; i < NUM_LEVELS; i++)
		off += snprintf(buf + off, sizeof(buf) - off, "%d%s", save_data.best_score[i], i < NUM_LEVELS-1 ? "," : "");
	off += snprintf(buf + off, sizeof(buf) - off, "],\n");

	// bulbs_per_level
	off += snprintf(buf + off, sizeof(buf) - off, "  \"bulbs_per_level\": [");
	for (int i = 0; i < NUM_LEVELS; i++)
		off += snprintf(buf + off, sizeof(buf) - off, "%d%s", save_data.bulbs_per_level[i], i < NUM_LEVELS-1 ? "," : "");
	off += snprintf(buf + off, sizeof(buf) - off, "],\n");

	// Cumulative
	off += snprintf(buf + off, sizeof(buf) - off, "  \"total_bulbs\": %d,\n", save_data.total_bulbs);
	off += snprintf(buf + off, sizeof(buf) - off, "  \"total_deaths\": %d,\n", save_data.total_deaths);
	off += snprintf(buf + off, sizeof(buf) - off, "  \"total_jumps\": %d,\n", save_data.total_jumps);
	off += snprintf(buf + off, sizeof(buf) - off, "  \"levels_completed\": %d,\n", save_data.levels_completed);
	
	off += snprintf(buf + off, sizeof(buf) - off, "  \"unlock_notified\": [");
	for (int i = 0; i < NUM_LEVELS; i++)
		off += snprintf(buf + off, sizeof(buf) - off, "%d%s", save_data.unlock_notified[i] ? 1 : 0, i < NUM_LEVELS-1 ? "," : "");
	off += snprintf(buf + off, sizeof(buf) - off, "],\n");

	off += snprintf(buf + off, sizeof(buf) - off, "  \"muted\": %d\n", save_data.muted ? 1 : 0);

	off += snprintf(buf + off, sizeof(buf) - off, "}\n");

	platform_store_userdata("save.json", buf, off);
}

void save_reset(void) {
	memset(&save_data, 0, sizeof(save_data));
	for (int i = 0; i < NUM_LEVELS; i++) {
		save_data.best_time_ms[i] = -1;
		save_data.best_score[i] = -1;
		save_data.bulbs_per_level[i] = -1;

		// Effectively delete ghost by overwriting with 0 count
		char filename[64];
		snprintf(filename, sizeof(filename), "ghost_%d.dat", i);
		uint32_t zero = 0;
		platform_store_userdata(filename, &zero, sizeof(zero));
	}
	g.total_bulbs = 0;
	g.total_deaths = 0;
	g.total_jumps = 0;
	g.levels_completed = 0;
	platform_store_userdata("save.json", "{}", 2);
}

// Bonus level unlock thresholds
static const int bonus_thresholds[6] = { 50, 200, 350, 500, 650, 800 };

bool level_is_unlocked(int idx) {
	if (idx == 0) return true;
	if (idx < 18) return save_data.best_time_ms[idx - 1] >= 0;
	// Bonus levels
	int bonus_idx = idx - 18;
	if (bonus_idx < 0 || bonus_idx >= 6) return false;
	return g.total_bulbs >= bonus_thresholds[bonus_idx];
}

int level_best_time_ms(int idx) {
	return save_data.best_time_ms[idx];
}

int level_best_score(int idx) {
	return save_data.best_score[idx];
}

int level_best_bulbs(int idx) {
	return save_data.bulbs_per_level[idx];
}

bool achievement_is_done(int idx) {
	if (idx < 0 || idx >= 6) return false;
	return save_data.achievements[idx];
}

bool level_should_notify_unlock(int idx) {
	if (idx < 0 || idx >= NUM_LEVELS) return false;
	return level_is_unlocked(idx) && !save_data.unlock_notified[idx];
}

void level_mark_unlock_notified(int idx) {
	if (idx < 0 || idx >= NUM_LEVELS) return;
	save_data.unlock_notified[idx] = true;
	save_store(false);
}

int save_get_total_time_s(void) {
	int total = 0;
	for (int i = 0; i < NUM_LEVELS; i++) {
		if (save_data.best_time_ms[i] >= 0)
			total += save_data.best_time_ms[i];
	}
	return total / 1000;
}

int save_get_total_score(void) {
	int total = 0;
	for (int i = 0; i < NUM_LEVELS; i++)
		total += save_data.best_score[i];
	return total;
}

void save_complete_level(void) {
	int idx = g.level_index;
	bool first_completion = (save_data.best_time_ms[idx] < 0);

	// Save stats (updates best_time_ms, best_score, bulbs_per_level)
	save_store(true);

	// Recalculate total bulbs from per-level
	int total = 0;
	for (int i = 0; i < NUM_LEVELS; i++) {
		total += save_data.bulbs_per_level[i];
	}
	save_data.total_bulbs = total;
	g.total_bulbs = total;

	// Unlock next main level on first completion (levels 0-17 are main)
	if (first_completion && idx < 17) {
		char buf[48];
		snprintf(buf, sizeof(buf), "Level %d unlocked!", idx + 2);
		spawn_popup(buf);
	}

	// Check bonus level unlocks
	static const char *bonus_names[6] = {"A", "B", "C", "D", "E", "F"};
	for (int i = 0; i < 6; i++) {
		if (!save_data.bonus_unlocked[i] && total >= bonus_thresholds[i]) {
			save_data.bonus_unlocked[i] = true;
			char buf[48];
			snprintf(buf, sizeof(buf), "Bonus level %s unlocked!", bonus_names[i]);
			spawn_popup(buf);
		}
	}

	// Check achievements
	achievements_check();

	// Save again (with bonus_unlocked flags)
	save_store(false);

	spawn_popup("Progress saved!");
}

// Achievements (matching AS3 BungluwaAchievements)
// 0: Fast Robot — overall time < 900s
// 1: Big Spender — 333 bulbs
// 2: Greedypants — 999 bulbs
// 3: Half the Rent — beat level 9
// 4: BGW 10K — beat level 18
// 5: Arcade Buster — beat bonus level F (index 23)
void achievements_check(void) {
	// 0: Fast Robot — overall time < 900 seconds
	if (!save_data.achievements[0]) {
		int total_s = 0;
		int completed_count = 0;
		for (int i = 0; i < NUM_LEVELS; i++) {
			if (save_data.best_time_ms[i] >= 0) {
				total_s += save_data.best_time_ms[i] / 1000;
				completed_count++;
			}
		}
		if (completed_count == NUM_LEVELS && total_s <= 900) {
			save_data.achievements[0] = true;
			spawn_popup("Achievement: Fast Robot!");
		}
	}
	// 1: Big Spender — collect 333 bulbs
	if (!save_data.achievements[1] && g.total_bulbs >= 333) {
		save_data.achievements[1] = true;
		spawn_popup("Achievement: Big Spender!");
	}
	// 2: Greedypants — collect all 999 bulbs
	if (!save_data.achievements[2] && g.total_bulbs >= 999) {
		save_data.achievements[2] = true;
		spawn_popup("Achievement: Greedypants!");
	}
	// 3: Half the Rent — beat level 9 (index 8)
	if (!save_data.achievements[3] && save_data.best_time_ms[8] >= 0) {
		save_data.achievements[3] = true;
		spawn_popup("Achievement: Half the Rent!");
	}
	// 4: BGW 10K — beat level 18 (index 17)
	if (!save_data.achievements[4] && save_data.best_time_ms[17] >= 0) {
		save_data.achievements[4] = true;
		spawn_popup("Achievement: BGW 10K!");
	}
	// 5: Arcade Buster — beat bonus level F (index 23)
	if (!save_data.achievements[5] && save_data.best_time_ms[23] >= 0) {
		save_data.achievements[5] = true;
		spawn_popup("Achievement: Arcade Buster!");
	}
}
