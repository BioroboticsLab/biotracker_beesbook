#pragma once

#include <memory>

#include <QFormLayout>
#include <QWidget>

#include "ParamsSubWidgetBase.h"
#include "source/utility/SpinBoxWithSlider.h"

class Settings;

class GridFitterParamsWidget : public ParamsSubWidgetBase
{
	Q_OBJECT
public:
	GridFitterParamsWidget(Settings& settings);

private:
	Settings& _settings;

	SpinBoxWithSlider _initialStepSizeSlider;
	SpinBoxWithSlider _finalStepSizeSlider;
	SpinBoxWithSlider _upSpeedSlider;
	SpinBoxWithSlider _downSpeedSlider;
};
