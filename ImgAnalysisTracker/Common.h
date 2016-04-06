#pragma once

#include <chrono>

#include <QApplication>
#include <QColor>

#include "pipeline/Localizer.h"
#include "pipeline/EllipseFitter.h"
#include "pipeline/GridFitter.h"
#include "pipeline/Preprocessor.h"

#include "pipeline/settings/LocalizerSettings.h"
#include "pipeline/settings/EllipseFitterSettings.h"
#include "pipeline/settings/GridFitterSettings.h"
#include "pipeline/settings/PreprocessorSettings.h"

#include <biotracker/settings/Settings.h>

namespace BC = BioTracker::Core;

namespace BeesBookCommon {

static const cv::Scalar COLOR_ORANGE(0, 102, 255);
static const cv::Scalar COLOR_GREEN(0, 255, 0);
static const cv::Scalar COLOR_RED(0, 0, 255);
static const cv::Scalar COLOR_BLUE(255, 0, 0);
static const cv::Scalar COLOR_LIGHT_BLUE(255, 200, 150);
static const cv::Scalar COLOR_GREENISH(13, 255, 182);

static const QColor QCOLOR_ORANGE(255, 102, 0);
static const QColor QCOLOR_GREEN(0, 255, 0);
static const QColor QCOLOR_RED(255, 0, 0);
static const QColor QCOLOR_BLUE(0, 0, 255);
static const QColor QCOLOR_LIGHT_BLUE(150, 200, 255);
static const QColor QCOLOR_GREENISH(182, 255, 13);

enum class Stage : uint8_t {
    NoProcessing = 0,
    Preprocessor,
    Localizer,
    EllipseFitter,
    Transformer,
    GridFitter,
    Decoder
};

pipeline::settings::localizer_settings_t getLocalizerSettings(BC::Settings &settings);
pipeline::settings::ellipsefitter_settings_t getEllipseFitterSettings(BC::Settings &settings);
pipeline::settings::gridfitter_settings_t getGridfitterSettings(BC::Settings &settings);
pipeline::settings::preprocessor_settings_t getPreprocessorSettings(BC::Settings &settings);

typedef std::vector<pipeline::Tag> taglist_t;

taglist_t loadSerializedTaglist(std::string const &path);

void setPreprocessorSettings(BC::Settings &bioTrackerSettings,
                             pipeline::settings::preprocessor_settings_t &settings);
void setLocalizerSettings(BC::Settings &bioTrackerSettings,
                          pipeline::settings::localizer_settings_t &settings);
void setEllipseFitterSettings(BC::Settings &bioTrackerSettings,
                              pipeline::settings::ellipsefitter_settings_t &settings);
void setGridFitterSettings(BC::Settings &bioTrackerSettings,
                           pipeline::settings::gridfitter_settings_t &settings);
}
