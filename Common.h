#pragma once

#include "pipeline/Localizer.h"

class Settings;

namespace BeesBookCommon {
enum class Stage : uint8_t {
	NoProcessing = 0,
	Converter,
	Localizer,
	Recognizer,
	Transformer,
	GridFitter,
	Decoder
};

decoder::localizer_settings_t getLocalizerSettings(Settings const& settings);
}
