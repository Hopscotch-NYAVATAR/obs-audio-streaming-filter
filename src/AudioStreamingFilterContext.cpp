#include "AudioStreamingFilterContext.h"

#include <vector>
#include <stdint.h>
#include <sstream>

#include <obs-module.h>
#include <obs-frontend-api.h>

#include <curlpp/cURLpp.hpp>
#include <curlpp/Easy.hpp>
#include <curlpp/Options.hpp>
#include <nlohmann/json.hpp>

#include "plugin-support.h"
#include "AuthClient.hpp"

static void handleFrontendEventCallback(obs_frontend_event event,
					void *private_data)
{
	AudioStreamingFilterContext *self =
		reinterpret_cast<AudioStreamingFilterContext *>(private_data);
	self->handleFrontendEvent(event);
}

AudioStreamingFilterContext::AudioStreamingFilterContext(obs_data_t *settings,
							 obs_source_t *_source)
	: source(_source)
{
	obs_frontend_add_event_callback(handleFrontendEventCallback, this);
	update(settings);
}

AudioStreamingFilterContext::~AudioStreamingFilterContext(void)
{
	obs_frontend_remove_event_callback(handleFrontendEventCallback, this);
}

obs_properties_t *AudioStreamingFilterContext::getProperties(void)
{
	obs_properties_t *props = obs_properties_create();
	obs_properties_add_text(props, "secret_url",
				obs_module_text("SecretURL"), OBS_TEXT_DEFAULT);
	return props;
}

void AudioStreamingFilterContext::update(obs_data_t *settings)
{
	secretURL = obs_data_get_string(settings, "secret_url");
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
		std::vector<float> buf(audio->frames * 2);
		float **planarData = reinterpret_cast<float **>(audio->data);
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
	const auto hashIndex = secretURL.find_first_of("#");
	if (hashIndex == std::string::npos) {
		obs_log(LOG_WARNING, "Secret URL does not contain token!");
		return;
	}

	const auto indefiniteAccessTokenExchangeEndpoint =
		secretURL.substr(0, hashIndex);
	const auto indefiniteAccessToken = secretURL.substr(hashIndex + 1, -1);

	const auto authenticateResult =
		authClient.authenticateWithIndefiniteAccessToken(
			indefiniteAccessTokenExchangeEndpoint,
			indefiniteAccessToken);
	if (!authenticateResult.success) {
		obs_log(LOG_INFO, "Authentication failed!");
		return;
	}

	audioRecordClient.init(
		authenticateResult
			.batchIssueAudioRecordUploadDestinationEndpoint);

	const std::string idToken = authClient.getIdToken();
	const auto destinationResponse =
		audioRecordClient.batchGetUploadDestination(idToken, 0, 5,
							    "test");
	if (destinationResponse.success) {
		obs_log(LOG_INFO, "%s",
			destinationResponse.destinations[0].c_str());
	}

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
