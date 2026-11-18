#include "main.h"

void ghost_reset(ghost_data_t *data) {
	if (data->frames) {
		free(data->frames);
	}
	data->frames = NULL;
	data->count = 0;
	data->capacity = 0;
}

void ghost_record_frame(ghost_data_t *data, uint8_t input, float tick) {
	if (data->count >= data->capacity) {
		data->capacity = data->capacity == 0 ? 1024 : data->capacity * 2;
		data->frames = realloc(data->frames, data->capacity * sizeof(ghost_frame_t));
	}
	data->frames[data->count].input = input;
	data->frames[data->count].tick = tick;
	data->count++;
}

void ghost_save(int level_index) {
	if (g.recording.count == 0) return;

	char filename[64];
	snprintf(filename, sizeof(filename), "ghost_%d.dat", level_index);

	// Header: count
	uint32_t count = g.recording.count;
	
	// We'll write the count and then the frames
	// Total size: 4 + count * sizeof(ghost_frame_t)
	int total_size = sizeof(uint32_t) + count * sizeof(ghost_frame_t);
	uint8_t *buffer = malloc(total_size);
	
	memcpy(buffer, &count, sizeof(uint32_t));
	memcpy(buffer + sizeof(uint32_t), g.recording.frames, count * sizeof(ghost_frame_t));

	platform_store_userdata(filename, buffer, total_size);
	free(buffer);
}

void ghost_load(int level_index) {
	ghost_reset(&g.playback);

	char filename[64];
	snprintf(filename, sizeof(filename), "ghost_%d.dat", level_index);

	uint32_t bytes_read = 0;
	uint8_t *buffer = platform_load_userdata(filename, &bytes_read);
	if (!buffer) return;

	if (bytes_read < sizeof(uint32_t)) {
		temp_free(buffer);
		return;
	}

	uint32_t count;
	memcpy(&count, buffer, sizeof(uint32_t));

	if (bytes_read < sizeof(uint32_t) + count * sizeof(ghost_frame_t)) {
		temp_free(buffer);
		return;
	}

	g.playback.count = count;
	g.playback.capacity = count;
	g.playback.frames = malloc(count * sizeof(ghost_frame_t));
	memcpy(g.playback.frames, buffer + sizeof(uint32_t), count * sizeof(ghost_frame_t));

	temp_free(buffer);
}
