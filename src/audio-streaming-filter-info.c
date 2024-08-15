#include "audio-streaming-filter.h"

struct obs_source_info audio_streaming_filter_info = {
	.id = "audio_streaming_filter",
	.type = OBS_SOURCE_TYPE_FILTER,
	.output_flags = OBS_SOURCE_AUDIO,
	.get_name = audio_streaming_filter_get_name,
	.create = audio_streaming_filter_create,
	.destroy = audio_streaming_filter_destroy,
	.get_properties = audio_streaming_filter_get_properties,
	.update = audio_streaming_filter_update,
	.filter_video = audio_streaming_filter_video,
	.filter_audio = audio_streaming_filter_audio,
};
