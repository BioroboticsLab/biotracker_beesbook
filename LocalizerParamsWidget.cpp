#include "LocalizerParamsWidget.h"
#include "source/settings/Settings.h"

using namespace pipeline::settings::Localizer;

LocalizerParamsWidget::LocalizerParamsWidget(Settings &settings) :
		ParamsSubWidgetBase(settings), _binaryThresholdSlider(this, &_layout,
				"Binary Threshold", 1, 100, 1, 1), _firstDilationNumIterationsSlider(
				this, &_layout, "First Dilation Iterations", 1, 20, 1, 1), _firstDilationSizeSlider(
				this, &_layout, "First Dilation Size", 1, 100, 1, 1), _erosionSizeSlider(
				this, &_layout, "Erosion Size", 1, 100, 1, 1), _secondDilationSizeSlider(
				this, &_layout, "Second Dilation Size", 1, 100, 1, 1), _maxTagSizeSlider(
				this, &_layout, "Maximum Tag Size", 1, 1000, 1, 1), _minBoundingBoxSizeSlider(
				this, &_layout, "Minimum Bounding Box Size", 1, 1000, 1, 1) {

	connectSlider(&_binaryThresholdSlider,
	              Params::BASE + Params::BINARY_THRESHOLD,
	              BeesBookCommon::Stage::Localizer);
	connectSlider(&_firstDilationNumIterationsSlider,
	              Params::BASE + Params::FIRST_DILATION_NUM_ITERATIONS,
	              BeesBookCommon::Stage::Localizer);
	connectSlider(&_firstDilationSizeSlider,
	              Params::BASE + Params::FIRST_DILATION_SIZE,
	              BeesBookCommon::Stage::Localizer);
	connectSlider(&_erosionSizeSlider, Params::BASE + Params::EROSION_SIZE,
	              BeesBookCommon::Stage::Localizer);
	connectSlider(&_secondDilationSizeSlider,
	              Params::BASE + Params::SECOND_DILATION_SIZE,
	              BeesBookCommon::Stage::Localizer);
	connectSlider(&_maxTagSizeSlider, Params::BASE + Params::MAX_TAG_SIZE,
	              BeesBookCommon::Stage::Localizer);
	connectSlider(&_minBoundingBoxSizeSlider,
	              Params::BASE + Params::MIN_BOUNDING_BOX_SIZE,
	              BeesBookCommon::Stage::Localizer);
}
