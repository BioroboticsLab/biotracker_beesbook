#include "RecognizerParamsWidget.h"

#include "source/settings/Settings.h"
#include "Common.h"

using namespace pipeline::settings::Recognizer;

RecognizerParamsWidget::RecognizerParamsWidget(Settings &settings)
: ParamsSubWidgetBase(settings)
	, _cannyThresholdLowSlider(this, &_layout, "Canny Mean Min", 0, 200,0, 1)
	, _cannyThresholdHighSlider(this, &_layout, "Canny Mean Max", 0, 200,0, 1)
	, _minMajorAxisSlider(this, &_layout, "Min Major Axis", 0, 100,0, 1)
	, _maxMajorAxisSlider(this, &_layout, "Max Major Axis", 0, 100,0, 1)
	, _minMinorAxisSlider(this, &_layout, "Min Minor Axis", 0, 100,0, 1)
	, _maxMinorAxisSlider(this, &_layout, "Max Minor Axis", 0, 100,0, 1)
	, _thresholdEdgePixelsSlider(this, &_layout, "Threshold Edge Pixels", 0, 100,0, 1)
	, _thresholdBestVoteSlider(this, &_layout, "Threshold Vote", 0, 5000,0, 1)
	, _thresholdVoteSlider(this, &_layout, "Threshold Best Vote", 0, 5000,0, 1)
{


	connectSlider(&_cannyThresholdLowSlider, Params::BASE+ Params::CANNY_MEAN_MIN,BeesBookCommon::Stage::Recognizer);
	connectSlider(&_cannyThresholdHighSlider, Params::BASE+ Params::CANNY_MEAN_MAX,BeesBookCommon::Stage::Recognizer);
	connectSlider(&_minMajorAxisSlider, Params::BASE+ Params::MIN_MAJOR_AXIS,BeesBookCommon::Stage::Recognizer);
	connectSlider(&_maxMajorAxisSlider, Params::BASE+ Params::MAX_MAJOR_AXIS,BeesBookCommon::Stage::Recognizer);
	connectSlider(&_minMinorAxisSlider, Params::BASE+ Params::MIN_MINOR_AXIS,BeesBookCommon::Stage::Recognizer);
	connectSlider(&_maxMinorAxisSlider, Params::BASE+ Params::MAX_MINOR_AXIS,BeesBookCommon::Stage::Recognizer);
	connectSlider(&_thresholdEdgePixelsSlider, Params::BASE+ Params::THRESHOLD_EDGE_PIXELS,BeesBookCommon::Stage::Recognizer);
	connectSlider(&_thresholdBestVoteSlider, Params::BASE+ Params::THRESHOLD_BEST_VOTE,BeesBookCommon::Stage::Recognizer);
	connectSlider(&_thresholdVoteSlider, Params::BASE+ Params::THRESHOLD_VOTE,BeesBookCommon::Stage::Recognizer);
}
