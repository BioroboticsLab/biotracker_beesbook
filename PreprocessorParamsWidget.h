#pragma once

#include "ParamsSubWidgetBase.h"

class Settings;

class PreprocessorParamsWidget : public ParamsSubWidgetBase
{
	Q_OBJECT
public:
	PreprocessorParamsWidget(Settings& settings);
};
