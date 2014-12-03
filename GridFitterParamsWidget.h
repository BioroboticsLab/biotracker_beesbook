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
	GridFitterParamsWidget(Settings& settings, QWidget* parent = nullptr);

private:
	Settings& _settings;
	QFormLayout _layout;
};
