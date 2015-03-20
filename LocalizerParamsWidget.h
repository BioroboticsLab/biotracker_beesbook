#pragma once

#include <memory>

#include <QFormLayout>
#include <QWidget>

#include "ParamsSubWidgetBase.h"
#include "source/utility/SpinBoxWithSlider.h"

class Settings;

class LocalizerParamsWidget : public ParamsSubWidgetBase
{
	Q_OBJECT
public:
	LocalizerParamsWidget(Settings& settings);

private:
	QWidget     _formWidget;
	QFormLayout _formLayout;

	SpinBoxWithSlider _binaryThresholdSlider;
	SpinBoxWithSlider _firstDilationNumIterationsSlider;
	SpinBoxWithSlider _firstDilationSizeSlider;
	SpinBoxWithSlider _erosionSizeSlider;
	SpinBoxWithSlider _secondDilationSizeSlider;
	SpinBoxWithSlider _maxTagSizeSlider;
	SpinBoxWithSlider _minBoundingBoxSizeSlider;
};
