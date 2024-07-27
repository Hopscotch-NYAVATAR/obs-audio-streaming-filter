#pragma once

#include <obs-module.h>

#ifdef __cplusplus
extern "C" {
#endif

const char *audio_streaming_filter_get_name(void *unused);
void *audio_streaming_filter_create(obs_data_t *settings, obs_source_t *source);
void audio_streaming_filter_destroy(void *data);
void audio_streaming_filter_filter_audio(void *data);
struct obs_source_frame *audio_streaming_filter_video(void *data, struct obs_source_frame *frame);
struct obs_audio_data *audio_streaming_filter_audio(void *data, struct obs_audio_data *audio);

#ifdef __cplusplus
}
#endif
