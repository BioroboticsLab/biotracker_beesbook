#include "LocalizerParamsWidget.h"
#include "source/settings/Settings.h"

using namespace pipeline::settings::Localizer;

LocalizerParamsWidget::LocalizerParamsWidget(Settings &settings) :
		ParamsSubWidgetBase(settings)
	  , _binaryThresholdSlider(this, &_formLayout, "Binary Threshold", 1, 100, 1, 1)
	  , _firstDilationNumIterationsSlider(this, &_formLayout, "First Dilation Iterations", 1, 20, 1, 1)
	  , _firstDilationSizeSlider(this, &_formLayout, "First Dilation Size", 1, 100, 1, 1)
	  , _erosionSizeSlider(this, &_formLayout, "Erosion Size", 1, 100, 1, 1)
	  , _secondDilationSizeSlider(this, &_formLayout, "Second Dilation Size", 1, 100, 1, 1)
	  , _maxTagSizeSlider(this, &_formLayout, "Maximum Tag Size", 1, 1000, 1, 1)
	  , _minBoundingBoxSizeSlider(this, &_formLayout, "Minimum Bounding Box Size", 1, 1000, 1, 1)
{
	_formLayout.setSpacing(3);
	_formLayout.setMargin(3);
	_formWidget.setLayout(&_formLayout);

	_layout.addWidget(&_formWidget);

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
