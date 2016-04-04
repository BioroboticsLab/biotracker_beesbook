#pragma once

#include <QWidget>
#include <QFormLayout>
#include <QCheckBox>
#include <QLineEdit>
#include <QSpinBox>

#include "Common.h"
#include "biotracker/settings/Settings.h"
#include "biotracker/widgets/SpinBoxWithSlider.h"

class ParamsSubWidgetBase: public QWidget {
    Q_OBJECT
  public:
    // memory is managed by ParamsWidget, therefore initialize QWidget with nullptr
    ParamsSubWidgetBase(BC::Settings &settings) :
        QWidget(nullptr), _layout(this), _settings(settings) {
        _layout.setMargin(0);
        _layout.setSpacing(0);
    }
    virtual ~ParamsSubWidgetBase() {
    }
  protected:
    QVBoxLayout _layout;
    QWidget _uiWidget;

    BC::Settings &_settings;

    void connectSlider(BioTracker::Widgets::SpinBoxWithSlider *slider,
                       const std::string &paramName,
                       const BeesBookCommon::Stage &stage);

    void connectSettingsWidget(QCheckBox *check, const std::string &paramName,
                               const BeesBookCommon::Stage &stage);

    void connectSettingsWidget(QSpinBox *spin, const std::string &paramName,
                               const BeesBookCommon::Stage &stage);

    void connectSettingsWidget(QDoubleSpinBox *spin, const std::string &paramName,
                               const BeesBookCommon::Stage &stage);

    void connectSettingsWidget(QLineEdit *lineEdit, const std::string &paramName,
                               const BeesBookCommon::Stage &stage);

  Q_SIGNALS:
    void settingsChanged(const BeesBookCommon::Stage stage);
};

