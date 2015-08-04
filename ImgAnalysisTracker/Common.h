#pragma once

#include "pipeline/Localizer.h"
#include "pipeline/EllipseFitter.h"
#include "pipeline/GridFitter.h"
#include "pipeline/Preprocessor.h"

#include "pipeline/settings/LocalizerSettings.h"
#include "pipeline/settings/EllipseFitterSettings.h"
#include "pipeline/settings/GridFitterSettings.h"
#include "pipeline/settings/PreprocessorSettings.h"

class Settings;

namespace BeesBookCommon {
enum class Stage : uint8_t {
	NoProcessing = 0,
	Preprocessor,
	Localizer,
	EllipseFitter,
	Transformer,
	GridFitter,
	Decoder
};

pipeline::settings::localizer_settings_t getLocalizerSettings(Settings& settings);
pipeline::settings::ellipsefitter_settings_t getEllipseFitterSettings(Settings& settings);
pipeline::settings::gridfitter_settings_t getGridfitterSettings(Settings& settings);
pipeline::settings::preprocessor_settings_t getPreprocessorSettings(Settings& settings);
}
