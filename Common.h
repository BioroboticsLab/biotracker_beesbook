#pragma once


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
pipeline::settings::ellipsefitter_settings_t getRecognizerSettings(Settings& settings);
pipeline::settings::gridfitter_settings_t getGridfitterSettings(Settings& settings);
pipeline::settings::preprocessor_settings_t getPreprocessorSettings(Settings& settings);
}
