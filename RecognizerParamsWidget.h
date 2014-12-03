#pragma once

#include <memory>

#include <QFormLayout>
#include <QWidget>

#include "ParamsSubWidgetBase.h"

class Settings;

class RecognizerParamsWidget : public ParamsSubWidgetBase
{
	Q_OBJECT
public:
	RecognizerParamsWidget(Settings& settings, QWidget* parent = nullptr);

private:
	Settings& _settings;
	QFormLayout _layout;
};
