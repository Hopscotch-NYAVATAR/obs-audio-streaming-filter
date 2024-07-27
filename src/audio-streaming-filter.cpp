#include "audio-streaming-filter.h"

#include <new>

#include "plugin-support.h"
#include "AudioStreamingFilterContext.h"


const char *audio_streaming_filter_get_name(void *unused) {
  UNUSED_PARAMETER(unused);
  return obs_module_text("AudioStreamingFilter");
}

void *audio_streaming_filter_create(obs_data_t *settings, obs_source_t *source)
{
  void *data = bzalloc(sizeof(AudioStreamingFilterContext));
  AudioStreamingFilterContext *context = new (data) AudioStreamingFilterContext(settings, source);
  return context;
}

void audio_streaming_filter_destroy(void *data)
{
  AudioStreamingFilterContext *context = reinterpret_cast<AudioStreamingFilterContext*>(data);
  context->~AudioStreamingFilterContext();
  bfree(data);
  return;
}

obs_properties_t *audio_streaming_filter_get_destroy(void *data)
{
  AudioStreamingFilterContext *context = reinterpret_cast<AudioStreamingFilterContext*>(data);
  return context->getProperties();
}

obs_source_frame *audio_streaming_filter_video(void *data, struct obs_source_frame *frame)
{
  AudioStreamingFilterContext *context = reinterpret_cast<AudioStreamingFilterContext*>(data);
  return context->filterVideo(frame);
}

obs_audio_data *audio_streaming_filter_audio(void *data, struct obs_audio_data *audio)
{
  AudioStreamingFilterContext *context = reinterpret_cast<AudioStreamingFilterContext*>(data);
  return context->filterAudio(audio);
}
