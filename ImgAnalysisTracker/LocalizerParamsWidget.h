#pragma once

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
};
