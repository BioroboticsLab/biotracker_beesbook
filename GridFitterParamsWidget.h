#pragma once

#include <memory>

#include <QFormLayout>
#include <QWidget>

#include "ParamsSubWidgetBase.h"

class Settings;

class GridFitterParamsWidget : public ParamsSubWidgetBase
{
	Q_OBJECT
public:
	GridFitterParamsWidget(Settings& settings);

private:
	Settings& _settings;
};
