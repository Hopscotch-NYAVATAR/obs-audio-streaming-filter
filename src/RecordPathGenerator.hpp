#pragma once

#include <filesystem>
#include <string>
#include <sstream>
#include <iomanip>

#include <util/config-file.h>

class RecordPathGenerator {
public:
	std::filesystem::path operator()(config_t *config)
	{
		return getFrontendRecordPath(config) / "test.opus";
	}

  std::filesystem::path getSegmentPath(config_t *config, std::string ext, std::string prefix, int segmentIndex)
  {
    std::ostringstream filename;
    filename << prefix << " " << std::setfill('0') << std::setw(6) << segmentIndex << "." << ext;
    return getFrontendRecordPath(config) / "obs-hopscotch-audio-support" / filename.str();
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
