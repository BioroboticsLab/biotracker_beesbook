#pragma once

#include <memory>

#include <QFormLayout>
#include <QWidget>
#include <QCheckBox>

#include "ParamsSubWidgetBase.h"
#include "source/utility/SpinBoxWithSlider.h"

class Settings;

class PreprocessorParamsWidget : public ParamsSubWidgetBase
{
	Q_OBJECT
public:
	PreprocessorParamsWidget(Settings& settings);

private:
	Settings& _settings;
	SpinBoxWithSlider _useEqualizeHistogram;
	SpinBoxWithSlider _useCombDetection;
	SpinBoxWithSlider _binaryThresholdSlider;
	SpinBoxWithSlider _minCombSizeSlider;
	SpinBoxWithSlider _maxCombSizeSlider;
	SpinBoxWithSlider _diffCombSizeSlider;
	SpinBoxWithSlider _lineWidthSlider;
	SpinBoxWithSlider _lineColorSlider;



};
