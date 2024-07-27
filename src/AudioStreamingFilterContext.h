#pragma once

#include <obs.h>
#include <obs-frontend-api.h>

class AudioStreamingFilterContext {
	const obs_data_t *settings;
	const obs_source_t *source;

public:
	AudioStreamingFilterContext(obs_data_t *settings, obs_source_t *source);
	~AudioStreamingFilterContext(void);
	obs_properties_t *getProperties(void);
	obs_source_frame *filterVideo(obs_source_frame *frame);
	obs_audio_data *filterAudio(obs_audio_data *audio);
	void handleFrontendEvent(obs_frontend_event event);
};
