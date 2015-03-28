#pragma once

#include "ParamsSubWidgetBase.h"

class Settings;

class DecoderParamsWidget : public ParamsSubWidgetBase
{
	Q_OBJECT
public:
	DecoderParamsWidget(Settings& settings);
};
