#include "AudioStreamingFilterContext.h"

#include "plugin-support.h"

#include <obs-frontend-api.h>

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
	obs_output_t *recordingOutput = obs_frontend_get_recording_output();
	obs_encoder_t *videoEncoder =
		obs_output_get_video_encoder(recordingOutput);
	obs_encoder_t *audioEncoder =
		obs_output_get_audio_encoder(recordingOutput, 0);
	obs_output_release(recordingOutput);

	const char *sourceName = obs_source_get_name(source);
	obs_data_t *outputSettings = obs_data_create();
	std::filesystem::path outputPath =
		recordPathGenerator(obs_frontend_get_profile_config());
	std::string pathString = outputPath.string<char>();
	obs_data_set_string(outputSettings, "path", pathString.c_str());
	fileOutput = obs_output_create("ffmpeg_muxer", sourceName,
				       outputSettings, nullptr);
	obs_data_release(outputSettings);

	obs_output_set_video_encoder(fileOutput, videoEncoder);
	obs_output_set_audio_encoder(fileOutput, audioEncoder, 0);

	obs_output_start(fileOutput);
}

void AudioStreamingFilterContext::stoppedRecording(void)
{
	obs_output_stop(fileOutput);
	obs_output_release(fileOutput);
}
