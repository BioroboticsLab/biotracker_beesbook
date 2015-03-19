#pragma once

#include <memory>

#include <QFormLayout>
#include <QWidget>

#include "ParamsSubWidgetBase.h"
#include "source/utility/SpinBoxWithSlider.h"

class Settings;

class RecognizerParamsWidget : public ParamsSubWidgetBase
{
	Q_OBJECT
public:
	RecognizerParamsWidget(Settings& settings);

private:
/*

	SpinBoxWithSlider _cannyThresholdLowSlider;
	SpinBoxWithSlider _cannyThresholdHighSlider;
	SpinBoxWithSlider _minMajorAxisSlider;
	SpinBoxWithSlider _maxMajorAxisSlider;
	SpinBoxWithSlider _minMinorAxisSlider;
	SpinBoxWithSlider _maxMinorAxisSlider;
	SpinBoxWithSlider _thresholdEdgePixelsSlider;
	SpinBoxWithSlider _thresholdBestVoteSlider;
	SpinBoxWithSlider _thresholdVoteSlider;*/
};
