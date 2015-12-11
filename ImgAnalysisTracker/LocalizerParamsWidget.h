#pragma once

#include <QFormLayout>
#include <QWidget>

#include "ParamsSubWidgetBase.h"
#include "biotracker/widgets/SpinBoxWithSlider.h"

class Settings;

class LocalizerParamsWidget : public ParamsSubWidgetBase
{
	Q_OBJECT
public:
	LocalizerParamsWidget(Settings& settings);
};
