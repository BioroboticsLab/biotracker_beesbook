#pragma once

#include <QWidget>
#include <QFormLayout>
#include <QCheckBox>
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

	void connectSlider(SpinBoxWithSlider* slider, const std::string& paramName, const BeesBookCommon::Stage &stage) {
		int initialValue = _settings.getValueOfParam<int>(paramName);
		slider->setValue(initialValue);
		QObject::connect(slider, &SpinBoxWithSlider::valueChanged, [ = ](int value) {
			_settings.setParam(paramName,value);

			emit settingsChanged(stage);
		});
	};

	void connectSettingsWidget(QCheckBox* check, const std::string& paramName,
	                           const BeesBookCommon::Stage &stage) {
		bool initialValue = _settings.getValueOfParam<bool>(paramName);
		if (initialValue) {
			check->setCheckState(Qt::CheckState::Checked);
		}
		QObject::connect(check, &QCheckBox::stateChanged, [ = ](int value) {
			if(value == Qt::CheckState::Checked) {
				_settings.setParam(paramName, true);
			} else if (value == Qt::CheckState::Unchecked) {
				_settings.setParam(paramName, false);
			}

			emit settingsChanged(stage);

		});
	}

	void connectSettingsWidget(QSpinBox* spin, const std::string& paramName,
	                           const BeesBookCommon::Stage &stage) {
		int initialValue = _settings.getValueOfParam<int>(paramName);

		spin->setValue(initialValue);

		QObject::connect(spin,
		                 static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [ = ](int value) {
			_settings.setParam(paramName, value);
			emit settingsChanged(stage);

		});
	}

	void connectSettingsWidget(QDoubleSpinBox* spin, const std::string& paramName,
	                           const BeesBookCommon::Stage &stage) {
		double initialValue = _settings.getValueOfParam<double>(paramName);
		spin->setValue(initialValue);

		QObject::connect(spin,
		                 static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [ = ](double value) {
			_settings.setParam(paramName, value);
			emit settingsChanged(stage);

		});
	}

signals:
	void settingsChanged(const BeesBookCommon::Stage stage);
};

