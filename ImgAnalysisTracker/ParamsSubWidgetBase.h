#pragma once

#include <QWidget>
#include <QFormLayout>
#include <QCheckBox>
#include <QLineEdit>
#include <QSpinBox>

#include "Common.h"
#include "source/settings/Settings.h"
#include "source/utility/SpinBoxWithSlider.h"

class ParamsSubWidgetBase: public QWidget {
	Q_OBJECT
public:
	// memory is managed by ParamsWidget, therefore initialize QWidget with nullptr
	ParamsSubWidgetBase(Settings &settings) :
	    QWidget(nullptr), _layout(this), _settings(settings) {
		_layout.setMargin(0);
		_layout.setSpacing(0);
	}
	virtual ~ParamsSubWidgetBase() {
	}
protected:
	QVBoxLayout _layout;
	QWidget _uiWidget;

	Settings& _settings;

    void connectSlider(SpinBoxWithSlider* slider, const std::string& paramName, const BeesBookCommon::Stage &stage);

    void connectSettingsWidget(QCheckBox* check, const std::string& paramName,
                               const BeesBookCommon::Stage &stage);

    void connectSettingsWidget(QSpinBox* spin, const std::string& paramName,
                               const BeesBookCommon::Stage &stage);

    void connectSettingsWidget(QDoubleSpinBox* spin, const std::string& paramName,
                               const BeesBookCommon::Stage &stage);

    void connectSettingsWidget(QLineEdit* lineEdit, const std::string& paramName,
                               const BeesBookCommon::Stage &stage);

signals:
	void settingsChanged(const BeesBookCommon::Stage stage);
};

