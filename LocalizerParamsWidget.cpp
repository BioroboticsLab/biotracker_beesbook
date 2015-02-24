#include "LocalizerParamsWidget.h"
#include "source/settings/Settings.h"

using namespace Localizer;

LocalizerParamsWidget::LocalizerParamsWidget(Settings &settings)
: ParamsSubWidgetBase(settings)
	, _binaryThresholdSlider(this, &_layout, "Binary Threshold", 1, 100,
	settings.getValueOrDefault(Params::BINARY_THRESHOLD, Defaults::BINARY_THRESHOLD), 1)
	, _firstDilationNumIterationsSlider(this, &_layout, "First Dilation Iterations", 1, 20,
	settings.getValueOrDefault(Params::FIRST_DILATION_NUM_ITERATIONS, Defaults::FIRST_DILATION_NUM_ITERATIONS), 1)
	, _firstDilationSizeSlider(this, &_layout, "First Dilation Size", 1, 100,
	settings.getValueOrDefault(Params::FIRST_DILATION_SIZE, Defaults::FIRST_DILATION_SIZE), 1)
	, _erosionSizeSlider(this, &_layout, "Erosion Size", 1, 100,
	settings.getValueOrDefault(Params::EROSION_SIZE, Defaults::EROSION_SIZE), 1)
	, _secondDilationSizeSlider(this, &_layout, "Second Dilation Size", 1, 100,
	settings.getValueOrDefault(Params::SECOND_DILATION_SIZE, Defaults::SECOND_DILATION_SIZE), 1)
	, _maxTagSizeSlider(this, &_layout, "Maximum Tag Size", 1, 1000,
	settings.getValueOrDefault(Params::MAX_TAG_SIZE, Defaults::MAX_TAG_SIZE), 1)
	, _minBoundingBoxSizeSlider(this, &_layout, "Minimum Bounding Box Size", 1, 1000,
	settings.getValueOrDefault(Params::MIN_BOUNDING_BOX_SIZE, Defaults::MIN_BOUNDING_BOX_SIZE), 1)
{
	auto connectSlider = [ & ](SpinBoxWithSlider* slider, const std::string& paramName) {
		  QObject::connect(slider, &SpinBoxWithSlider::valueChanged, [ = ](int value) {
		        _settings.setParam(paramName, value);
		        emit settingsChanged(BeesBookCommon::Stage::Localizer);
			});
	  };

	connectSlider(&_binaryThresholdSlider, Params::BINARY_THRESHOLD);
	connectSlider(&_firstDilationNumIterationsSlider, Params::FIRST_DILATION_NUM_ITERATIONS);
	connectSlider(&_firstDilationSizeSlider, Params::FIRST_DILATION_SIZE);
	connectSlider(&_erosionSizeSlider, Params::EROSION_SIZE);
	connectSlider(&_secondDilationSizeSlider, Params::SECOND_DILATION_SIZE);
	connectSlider(&_maxTagSizeSlider, Params::MAX_TAG_SIZE);
	connectSlider(&_minBoundingBoxSizeSlider, Params::MIN_BOUNDING_BOX_SIZE);
}
