#pragma once

#include "ParamsSubWidgetBase.h"

class Settings;

class EllipseFitterParamsWidget : public ParamsSubWidgetBase {
    Q_OBJECT
  public:
    EllipseFitterParamsWidget(BC::Settings &settings);
};
