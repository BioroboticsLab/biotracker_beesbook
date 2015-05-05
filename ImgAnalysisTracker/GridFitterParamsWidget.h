#pragma once

#include "ParamsSubWidgetBase.h"

class Settings;

class GridFitterParamsWidget : public ParamsSubWidgetBase
{
	Q_OBJECT
public:
	GridFitterParamsWidget(Settings& settings);
};
