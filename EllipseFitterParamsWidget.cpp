#include "RecognizerParamsWidget.h"

#include "source/settings/Settings.h"
#include "Common.h"

#include "ui_RecognizerParamsWidget.h"

using namespace pipeline::settings::Recognizer;

RecognizerParamsWidget::RecognizerParamsWidget(Settings &settings)
: ParamsSubWidgetBase(settings)
	/*, _cannyThresholdLowSlider(this, &_layout, "Canny Mean Min", 0, 200,0, 1)
	, _cannyThresholdHighSlider(this, &_layout, "Canny Mean Max", 0, 200,0, 1)
	, _minMajorAxisSlider(this, &_layout, "Min Major Axis", 0, 100,0, 1)
	, _maxMajorAxisSlider(this, &_layout, "Max Major Axis", 0, 100,0, 1)
	, _minMinorAxisSlider(this, &_layout, "Min Minor Axis", 0, 100,0, 1)
	, _maxMinorAxisSlider(this, &_layout, "Max Minor Axis", 0, 100,0, 1)
	, _thresholdEdgePixelsSlider(this, &_layout, "Threshold Edge Pixels", 0, 100,0, 1)
	, _thresholdBestVoteSlider(this, &_layout, "Threshold Vote", 0, 5000,0, 1)
	, _thresholdVoteSlider(this, &_layout, "Threshold Best Vote", 0, 5000,0, 1)*/
{

	Ui::RecognizerParamsWidget paramsWidget;
	paramsWidget.setupUi(this);

	/**
	 * canny
	 */
	connectSettingsWidget(paramsWidget.canny_inital_high,
				Params::BASE + Params::CANNY_INITIAL_HIGH,
				BeesBookCommon::Stage::Recognizer);
	connectSettingsWidget(paramsWidget.canny_max_mean,
				Params::BASE + Params::CANNY_MEAN_MAX,
				BeesBookCommon::Stage::Recognizer);
	connectSettingsWidget(paramsWidget.canny_min_mean,
				Params::BASE + Params::CANNY_MEAN_MIN,
				BeesBookCommon::Stage::Recognizer);
	connectSettingsWidget(paramsWidget.canny_value_distance,
				Params::BASE + Params::CANNY_VALUES_DISTANCE,
				BeesBookCommon::Stage::Recognizer);

	/**
	 * size
	 *
	 */

	connectSettingsWidget(paramsWidget.max_major_axis,
				Params::BASE + Params::MAX_MAJOR_AXIS,
				BeesBookCommon::Stage::Recognizer);
	connectSettingsWidget(paramsWidget.max_minor_axis,
				Params::BASE + Params::MAX_MINOR_AXIS,
				BeesBookCommon::Stage::Recognizer);
	connectSettingsWidget(paramsWidget.min_major_axis,
				Params::BASE + Params::MIN_MAJOR_AXIS,
				BeesBookCommon::Stage::Recognizer);
	connectSettingsWidget(paramsWidget.min_minor_axis,
				Params::BASE + Params::MIN_MINOR_AXIS,
				BeesBookCommon::Stage::Recognizer);
	/**
	 * vote
	 */

	connectSettingsWidget(paramsWidget.honey_enabled,
				Params::BASE + Params::USE_XIE_AS_FALLBACK,
				BeesBookCommon::Stage::Recognizer);
	connectSettingsWidget(paramsWidget.threshold_best_vote,
				Params::BASE + Params::THRESHOLD_BEST_VOTE,
				BeesBookCommon::Stage::Recognizer);
	connectSettingsWidget(paramsWidget.threshold_edge_pixels,
				Params::BASE + Params::THRESHOLD_EDGE_PIXELS,
				BeesBookCommon::Stage::Recognizer);
	connectSettingsWidget(paramsWidget.threshold_vote,
				Params::BASE + Params::THRESHOLD_VOTE,
				BeesBookCommon::Stage::Recognizer);

}
