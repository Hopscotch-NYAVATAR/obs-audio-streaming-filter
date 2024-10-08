#pragma once

#include <string>
#include <memory>

#include <opusenc.h>

#include <obs.h>
#include <obs-frontend-api.h>

#include "AudioRecordClient.hpp"
#include "AuthClient.hpp"
#include "RecordPathGenerator.hpp"
#include "OpusUploader.hpp"

class AudioStreamingFilterContext {
	const obs_source_t *source;

	std::string secretURL;
	uint64_t previousSegmentTimestamp;
	std::unique_ptr<OpusUploader> opusUploader;
	float *pcmBuffer;

	AudioRecordClient audioRecordClient;
	AuthClient authClient;
	RecordPathGenerator recordPathGenerator;

public:
	AudioStreamingFilterContext(obs_data_t *settings,
				    obs_source_t *_source);
	~AudioStreamingFilterContext(void);
	obs_properties_t *getProperties(void);
	void update(obs_data_t *settings);
	obs_source_frame *filterVideo(obs_source_frame *frame);
	obs_audio_data *filterAudio(obs_audio_data *audio);
	void handleFrontendEvent(obs_frontend_event event);

private:
	void startedRecording(void);
	void stoppedRecording(void);
};
