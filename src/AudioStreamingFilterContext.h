#pragma once

#include <obs.h>
#include <obs-frontend-api.h>

#include "RecordPathGenerator.hpp"

class AudioStreamingFilterContext {
	const obs_data_t *settings;
	const obs_source_t *source;

	obs_output_t *fileOutput;

	RecordPathGenerator recordPathGenerator;

public:
	AudioStreamingFilterContext(obs_data_t *_settings,
				    obs_source_t *_source);
	~AudioStreamingFilterContext(void);
	obs_properties_t *getProperties(void);
	obs_source_frame *filterVideo(obs_source_frame *frame);
	obs_audio_data *filterAudio(obs_audio_data *audio);
	void handleFrontendEvent(obs_frontend_event event);

private:
	void startedRecording(void);
	void stoppedRecording(void);
};
