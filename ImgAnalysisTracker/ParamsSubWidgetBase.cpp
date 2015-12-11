#include "ParamsSubWidgetBase.h"

void ParamsSubWidgetBase::connectSlider(SpinBoxWithSlider *slider, const std::string &paramName, const BeesBookCommon::Stage &stage) {
    int initialValue = _settings.getValueOfParam<int>(paramName);
    slider->setValue(initialValue);
    QObject::connect(slider, &SpinBoxWithSlider::valueChanged, [ = ](int value) {
        _settings.setParam(paramName,value);

        Q_EMIT settingsChanged(stage);
    });
}

void ParamsSubWidgetBase::connectSettingsWidget(QCheckBox *check, const std::string &paramName, const BeesBookCommon::Stage &stage) {
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

        Q_EMIT settingsChanged(stage);

    });
}

void ParamsSubWidgetBase::connectSettingsWidget(QSpinBox *spin, const std::string &paramName, const BeesBookCommon::Stage &stage) {
    int initialValue = _settings.getValueOfParam<int>(paramName);

    spin->setValue(initialValue);

    QObject::connect(spin,
                     static_cast<void (QSpinBox::*)(int)>(&QSpinBox::valueChanged), [ = ](int value) {
        _settings.setParam(paramName, value);
        Q_EMIT settingsChanged(stage);

    });
}

void ParamsSubWidgetBase::connectSettingsWidget(QDoubleSpinBox *spin, const std::string &paramName, const BeesBookCommon::Stage &stage) {
    double initialValue = _settings.getValueOfParam<double>(paramName);
    spin->setValue(initialValue);

    QObject::connect(spin,
                     static_cast<void (QDoubleSpinBox::*)(double)>(&QDoubleSpinBox::valueChanged), [ = ](double value) {
        _settings.setParam(paramName, value);
        Q_EMIT settingsChanged(stage);

    });
}

void ParamsSubWidgetBase::connectSettingsWidget(QLineEdit *lineEdit, const std::string &paramName, const BeesBookCommon::Stage &stage)
{
    std::string initialValue = _settings.getValueOfParam<std::string>(paramName);
    lineEdit->setText(QString::fromStdString(initialValue));

    QObject::connect(lineEdit,
                     static_cast<void (QLineEdit::*)()>(&QLineEdit::editingFinished), [ = ]() {
        _settings.setParam(paramName, lineEdit->text().toStdString());
        Q_EMIT settingsChanged(stage);
    });

}
