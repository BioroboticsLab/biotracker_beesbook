#pragma once

#include <memory>

#include <QFormLayout>
#include <QWidget>

#include "ParamsSubWidgetBase.h"

class Settings;

class DecoderParamsWidget : public ParamsSubWidgetBase
{
	Q_OBJECT
public:
	DecoderParamsWidget(Settings& settings);

private:
	Settings& _settings;
	QFormLayout _layout;
};
