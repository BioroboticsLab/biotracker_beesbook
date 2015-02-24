#include "GridFitterParamsWidget.h"
#include "source/settings/Settings.h"

using namespace GridFitter;

GridFitterParamsWidget::GridFitterParamsWidget(Settings &settings)
: ParamsSubWidgetBase(settings)
//    , _initialStepSizeSlider(this, &_layout, "Initial Step Size", 0, 100,
//	settings.getValueOrDefault(Params::INITIAL_STEP_SIZE, Defaults::INITIAL_STEP_SIZE), 1)
//	, _finalStepSizeSlider(this, &_layout, "Final Step Size", 0, 100,
//	settings.getValueOrDefault(Params::FINAL_STEP_SIZE, Defaults::FINAL_STEP_SIZE), 1)
//	, _upSpeedSlider(this, &_layout, "Up Speed", 1, 100,
//	settings.getValueOrDefault(Params::UP_SPEED, Defaults::UP_SPEED), 1)
//	, _downSpeedSlider(this, &_layout, "(Down Speed)", 1, 100,
//	settings.getValueOrDefault(Params::DOWN_SPEED, Defaults::DOWN_SPEED ), 1)
{
	auto connectSlider = [ & ](SpinBoxWithSlider* slider, const std::string& paramName) {
		  QObject::connect(slider, &SpinBoxWithSlider::valueChanged, [ = ](int value) {
		        _settings.setParam(paramName, value);
		        emit settingsChanged(BeesBookCommon::Stage::GridFitter);
			});
	  };

//	connectSlider(&_initialStepSizeSlider, Params::INITIAL_STEP_SIZE);
//	connectSlider(&_finalStepSizeSlider, Params::FINAL_STEP_SIZE);
//	connectSlider(&_upSpeedSlider, Params::UP_SPEED);
//	connectSlider(&_downSpeedSlider, Params::DOWN_SPEED);
}
