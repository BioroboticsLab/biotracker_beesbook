#pragma once

#include <memory>

#include <QFormLayout>
#include <QWidget>

#include "ParamsSubWidgetBase.h"
#include "source/utility/SpinBoxWithSlider.h"

class Settings;

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

class LocalizerParamsWidget : public ParamsSubWidgetBase
{
	Q_OBJECT
public:
	LocalizerParamsWidget(Settings& settings, QWidget* parent = nullptr);

private:
	Settings& _settings;
	QFormLayout _layout;

	SpinBoxWithSlider _binaryThresholdSlider;
	SpinBoxWithSlider _firstDilationNumIterationsSlider;
	SpinBoxWithSlider _firstDilationSizeSlider;
	SpinBoxWithSlider _erosionSizeSlider;
	SpinBoxWithSlider _secondDilationSizeSlider;
	SpinBoxWithSlider _maxTagSizeSlider;
	SpinBoxWithSlider _minBoundingBoxSizeSlider;
};
