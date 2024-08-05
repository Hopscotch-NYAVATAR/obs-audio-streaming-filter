#include "AudioStreamingFilterContext.h"

#include <vector>
#include <stdint.h>

#include <obs-frontend-api.h>

#include "plugin-support.h"

static void handleFrontendEventCallback(obs_frontend_event event,
					void *private_data)
{
	AudioStreamingFilterContext *self =
		reinterpret_cast<AudioStreamingFilterContext *>(private_data);
	self->handleFrontendEvent(event);
}

AudioStreamingFilterContext::AudioStreamingFilterContext(obs_data_t *_settings,
							 obs_source_t *_source)
	: settings(_settings),
	  source(_source)
{
	obs_frontend_add_event_callback(handleFrontendEventCallback, this);
}

AudioStreamingFilterContext::~AudioStreamingFilterContext(void)
{
	obs_frontend_remove_event_callback(handleFrontendEventCallback, this);
}

obs_properties_t *AudioStreamingFilterContext::getProperties(void)
{
	obs_properties_t *props = obs_properties_create();
	return props;
}

obs_source_frame *
AudioStreamingFilterContext::filterVideo(struct obs_source_frame *frame)
{
	return frame;
}

obs_audio_data *
AudioStreamingFilterContext::filterAudio(struct obs_audio_data *audio)
{
	if (enc) {
		std::vector<float> buf(audio->frames * 2) float **planarData =
			(float **)audio->data;
		for (uint32_t i = 0; i < audio->frames; i++) {
			buf[i * 2 + 0] = planarData[0][i];
			buf[i * 2 + 1] = planarData[1][i];
		}
		ope_encoder_write_float(enc, buf.data(), audio->frames);
	}

	return audio;
}

void AudioStreamingFilterContext::handleFrontendEvent(obs_frontend_event event)
{
	if (event == OBS_FRONTEND_EVENT_STREAMING_STARTING) {
		obs_log(LOG_INFO, "Streaming starting!");
	} else if (event == OBS_FRONTEND_EVENT_STREAMING_STOPPING) {
		obs_log(LOG_INFO, "Streaming stopping!");
	} else if (event == OBS_FRONTEND_EVENT_RECORDING_STARTED) {
		startedRecording();
	} else if (event == OBS_FRONTEND_EVENT_RECORDING_STOPPED) {
		stoppedRecording();
	}
}

void AudioStreamingFilterContext::startedRecording(void)
{
	const std::filesystem::path outputPath =
		recordPathGenerator(obs_frontend_get_profile_config());
	const std::string outputPathString = outputPath.string<char>();
	comments = ope_comments_create();
	int error;
	enc = ope_encoder_create_file(outputPathString.c_str(), comments, 44100,
				      2, 0, &error);
}

void AudioStreamingFilterContext::stoppedRecording(void)
{
	OggOpusEnc *_enc = enc;
	enc = nullptr;
	ope_encoder_drain(_enc);
	ope_encoder_destroy(_enc);
	ope_comments_destroy(comments);
	comments = nullptr;
}
