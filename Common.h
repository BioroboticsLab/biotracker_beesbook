#pragma once

#include "pipeline/Localizer.h"
#include "pipeline/Recognizer.h"
#include "pipeline/GridFitter.h"
#include "pipeline/Preprocessor.h"
#include "pipeline/datastructure/settings.h"

class Settings;

namespace Localizer {
namespace Params {
static const std::string BASE = "BEESBOOKPIPELINE.LOCALIZER.";
static const std::string BINARY_THRESHOLD = BASE + "BINARY_THRESHOLD";
static const std::string FIRST_DILATION_NUM_ITERATIONS = BASE + "FIRST_DILATION_NUM_ITERATIONS";
static const std::string FIRST_DILATION_SIZE           = BASE + "FIRST_DILATION_SIZE";
static const std::string EROSION_SIZE                  = BASE + "EROSION_SIZE";
static const std::string SECOND_DILATION_SIZE          = BASE + "SECOND_DILATION_SIZE";
static const std::string MAX_TAG_SIZE                  = BASE + "MAX_TAG_SIZE";
static const std::string MIN_BOUNDING_BOX_SIZE         = BASE + "MIN_BOUNDING_BOX_SIZE";
}

namespace Defaults {
static const int BINARY_THRESHOLD = 29;
static const int FIRST_DILATION_NUM_ITERATIONS = 4;
static const int FIRST_DILATION_SIZE           = 2;
static const int EROSION_SIZE                  = 25;
static const int SECOND_DILATION_SIZE          = 2;
static const int MAX_TAG_SIZE                  = 250;
static const int MIN_BOUNDING_BOX_SIZE         = 100;
}
}

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
static const std::string INITIAL_STEP_SIZE = BASE + "INITIAL_STEP_SIZE";
static const std::string FINAL_STEP_SIZE = BASE + "FINAL_STEP_SIZE";
static const std::string UP_SPEED = BASE + "UP_SPEED";
static const std::string DOWN_SPEED = BASE + "DOWN_SPEED";
}
namespace Defaults {
static const int INITIAL_STEP_SIZE = 20;
static const int FINAL_STEP_SIZE = 0;
static const int UP_SPEED = 3;
static const int DOWN_SPEED = 1; // TODO: should be 0.5 
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

decoder::localizer_settings_t getLocalizerSettings(Settings const& settings);
decoder::recognizer_settings_t getRecognizerSettings(Settings const& settings);
decoder::gridfitter_settings_t getGridfitterSettings(Settings const& settings);
decoder::preprocessor_settings_t getPreprocessorSettings(Settings const& settings);
}
