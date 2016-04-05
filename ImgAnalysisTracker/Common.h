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

struct CursorOverrideRAII {
    CursorOverrideRAII(Qt::CursorShape shape) {
        QApplication::setOverrideCursor(shape);
    }
    ~CursorOverrideRAII() {
        QApplication::restoreOverrideCursor();
    }
};


class MeasureTimeRAII {
  public:
    MeasureTimeRAII(std::string const &what, std::function<void(std::string const &)> notify,
                    boost::optional<size_t> num = boost::optional<size_t>())
        : _start(std::chrono::steady_clock::now()),
          _what(what),
          _notify(notify),
          _num(num) {
    }

    ~MeasureTimeRAII() {
        const auto end = std::chrono::steady_clock::now();
        const auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end - _start).count();
        std::stringstream message;
        message << _what << " finished in " << dur << "ms.";
        if (_num) {
            const auto avg = dur / _num.get();
            message << " (Average: " << avg << "ms)";
        }
        message << std::endl;
        _notify(message.str());
        // display message right away
        QApplication::processEvents();
    }
  private:
    const std::chrono::steady_clock::time_point _start;
    const std::string _what;
    const std::function<void(std::string const &)> _notify;
    const boost::optional<size_t> _num;
};
}
