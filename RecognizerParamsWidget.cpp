#include "RecognizerParamsWidget.h"

#include "source/settings/Settings.h"
#include "Common.h"

using namespace Recognizer;

RecognizerParamsWidget::RecognizerParamsWidget(Settings &settings, QWidget *parent)
	: ParamsSubWidgetBase(parent)
	, _settings(settings)
	, _layout(this)
	, _cannyThresholdLowSlider(this, &_layout, "Canny Threshold Low", 0, 200,
		settings.getValueOrDefault<int>(Params::CANNY_THRESHOLD_LOW, Defaults::CANNY_THRESHOLD_LOW), 1)
	, _cannyThresholdHighSlider(this, &_layout, "Canny Threshold High", 0, 200,
		settings.getValueOrDefault<int>(Params::CANNY_THRESHOLD_HIGH, Defaults::CANNY_THRESHOLD_HIGH), 1)
	, _minMajorAxisSlider(this, &_layout, "Min Major Axis", 0, 100,
		settings.getValueOrDefault<int>(Params::MIN_MAJOR_AXIS, Defaults::MIN_MAJOR_AXIS), 1)
	, _maxMajorAxisSlider(this, &_layout, "Max Major Axis", 0, 100,
		settings.getValueOrDefault<int>(Params::MAX_MAJOR_AXIS, Defaults::MAX_MAJOR_AXIS), 1)
	, _minMinorAxisSlider(this, &_layout, "Min Minor Axis", 0, 100,
		settings.getValueOrDefault<int>(Params::MIN_MINOR_AXIS, Defaults::MIN_MINOR_AXIS), 1)
	, _maxMinorAxisSlider(this, &_layout, "Max Minor Axis", 0, 100,
		settings.getValueOrDefault<int>(Params::MAX_MINOR_AXIS, Defaults::MAX_MINOR_AXIS), 1)
	, _thresholdEdgePixelsSlider(this, &_layout, "Threshold Edge Pixels", 0, 100,
		settings.getValueOrDefault<int>(Params::THRESHOLD_EDGE_PIXELS, Defaults::THRESHOLD_EDGE_PIXELS), 1)
	, _thresholdBestVoteSlider(this, &_layout, "Threshold Vote", 0, 5000,
		settings.getValueOrDefault<int>(Params::THRESHOLD_BEST_VOTE, Defaults::THRESHOLD_BEST_VOTE), 1)
	, _thresholdVoteSlider(this, &_layout, "Threshold Best Vote", 0, 5000,
		settings.getValueOrDefault<int>(Params::THRESHOLD_VOTE, Defaults::THRESHOLD_VOTE), 1)
{
	_layout.setSpacing(3);
	_layout.setMargin(3);

	auto connectSlider = [ & ](SpinBoxWithSlider* slider, const std::string& paramName) {
		QObject::connect(slider, &SpinBoxWithSlider::valueChanged, [ = ](int value) {
			_settings.setParam(paramName, value);
			emit settingsChanged(BeesBookCommon::Stage::Recognizer);
		});
	};

	connectSlider(&_cannyThresholdLowSlider, Params::CANNY_THRESHOLD_LOW);
	connectSlider(&_cannyThresholdHighSlider, Params::CANNY_THRESHOLD_HIGH);
	connectSlider(&_minMajorAxisSlider, Params::MIN_MAJOR_AXIS);
	connectSlider(&_maxMajorAxisSlider, Params::MAX_MAJOR_AXIS);
	connectSlider(&_minMinorAxisSlider, Params::MIN_MINOR_AXIS);
	connectSlider(&_minMajorAxisSlider, Params::MIN_MAJOR_AXIS);
	connectSlider(&_thresholdEdgePixelsSlider, Params::THRESHOLD_EDGE_PIXELS);
	connectSlider(&_thresholdBestVoteSlider, Params::THRESHOLD_BEST_VOTE);
	connectSlider(&_thresholdVoteSlider, Params::THRESHOLD_VOTE);
}
