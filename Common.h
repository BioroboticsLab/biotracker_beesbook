#pragma once

#include "pipeline/Localizer.h"
#include "pipeline/Recognizer.h"
#include "pipeline/GridFitter.h"
#include "pipeline/Preprocessor.h"
#include "pipeline/datastructure/settings.h"

class Settings;

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
pipeline::settings::recognizer_settings_t getRecognizerSettings(Settings& settings);
pipeline::settings::gridfitter_settings_t getGridfitterSettings(Settings& settings);
pipeline::settings::preprocessor_settings_t getPreprocessorSettings(Settings& settings);
}
