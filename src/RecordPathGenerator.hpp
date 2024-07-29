#pragma once

#include <filesystem>
#include <string>

#include <util/config-file.h>

class RecordPathGenerator {
public:
	std::filesystem::path operator()(config_t *config)
	{
		return getFrontendRecordPath(config) / "test.mp4";
	}

private:
	std::filesystem::path getFrontendRecordPath(config_t *config)
	{
		std::string output =
			config_get_string(config, "Output", "Mode");
		if (output == "Advanced" || output == "advanced") {
			std::string advOut =
				config_get_string(config, "AdvOut", "RecType");
			if (advOut == "Standard" || advOut == "standard") {
				return config_get_string(config, "AdvOut",
							 "RecFilePath");
			} else {
				return config_get_string(config, "AdvOut",
							 "FFFilePath");
			}
		} else {
			return config_get_string(config, "SimpleOutput",
						 "FilePath");
		}
	}
};
