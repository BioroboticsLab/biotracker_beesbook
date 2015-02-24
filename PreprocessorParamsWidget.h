#pragma once

#include <memory>

#include <QFormLayout>
#include <QWidget>
#include <QCheckBox>

#include "ParamsSubWidgetBase.h"
#include "source/utility/SpinBoxWithSlider.h"

class Settings;

class PreprocessorParamsWidget : public ParamsSubWidgetBase
{
	Q_OBJECT
public:
	PreprocessorParamsWidget(Settings& settings);

private:



};
