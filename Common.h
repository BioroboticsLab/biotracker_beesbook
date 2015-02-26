#pragma once

#include "pipeline/Localizer.h"
#include "pipeline/Recognizer.h"
#include "pipeline/GridFitter.h"
#include "pipeline/Preprocessor.h"
#include "pipeline/datastructure/settings.h"

class Settings;



namespace Recognizer {
namespace Params {
static const std::string BASE = "BEESBOOKPIPELINE.RECOGNIZER.";
static const std::string CANNY_THRESHOLD_LOW = BASE + "CANNY_THRESHOLD_LOW";
static const std::string CANNY_THRESHOLD_HIGH = BASE + "CANNY_THRESHOLD_HIGH";
static const std::string MIN_MAJOR_AXIS = BASE + "MIN_MAJOR_AXIS";
static const std::string MAX_MAJOR_AXIS = BASE + "MAX_MAJOR_AXIS";
static const std::string MIN_MINOR_AXIS = BASE + "MIN_MINOR_AXIS";
static const std::string MAX_MINOR_AXIS = BASE + "MAX_MINOR_AXIS";
static const std::string THRESHOLD_EDGE_PIXELS = BASE + "THRESHOLD_EDGE_PIXELS";
static const std::string THRESHOLD_VOTE = BASE + "THRESHOLD_VOTE";
static const std::string THRESHOLD_BEST_VOTE = BASE + "THRESHOLD_BEST_VOTE";
}
namespace Defaults {
static const int CANNY_THRESHOLD_LOW = 70;
static const int CANNY_THRESHOLD_HIGH = 90;
static const int MIN_MAJOR_AXIS = 42;
static const int MAX_MAJOR_AXIS = 54;
static const int MIN_MINOR_AXIS = 30;
static const int MAX_MINOR_AXIS = 54;
static const int THRESHOLD_EDGE_PIXELS = 25;
static const int THRESHOLD_VOTE = 1800;
static const int THRESHOLD_BEST_VOTE = 3000;
}
}

namespace GridFitter {
namespace Params {
static const std::string BASE = "BEESBOOKPIPELINE.GRIDFITTER.";
}
namespace Defaults {
}
}




namespace BeesBookCommon {
enum class Stage : uint8_t {
	NoProcessing = 0,
	Preprocessor,
	Localizer,
	Recognizer,
	Transformer,
	GridFitter,
	Decoder
};

pipeline::settings::localizer_settings_t getLocalizerSettings(Settings& settings);
pipeline::recognizer_settings_t getRecognizerSettings(Settings const& settings);
pipeline::gridfitter_settings_t getGridfitterSettings(Settings const& settings);
pipeline::settings::preprocessor_settings_t getPreprocessorSettings(Settings& settings);
}
