#include "EllipseFitterParamsWidget.h"

#include "biotracker/settings/Settings.h"
#include "Common.h"

#include "ui_EllipseFitterParamsWidget.h"

using namespace pipeline::settings::EllipseFitter;

EllipseFitterParamsWidget::EllipseFitterParamsWidget(Settings &settings)
    : ParamsSubWidgetBase(settings)
{
	Ui::EllipseFitterParamsWidget paramsWidget;
	paramsWidget.setupUi(&_uiWidget);
	_layout.addWidget(&_uiWidget);

	/**
	 * canny
	 */
	connectSettingsWidget(paramsWidget.canny_inital_high,
	                      Params::BASE + Params::CANNY_INITIAL_HIGH,
	                      BeesBookCommon::Stage::EllipseFitter);
	connectSettingsWidget(paramsWidget.canny_max_mean,
	                      Params::BASE + Params::CANNY_MEAN_MAX,
	                      BeesBookCommon::Stage::EllipseFitter);
	connectSettingsWidget(paramsWidget.canny_min_mean,
	                      Params::BASE + Params::CANNY_MEAN_MIN,
	                      BeesBookCommon::Stage::EllipseFitter);
	connectSettingsWidget(paramsWidget.canny_value_distance,
	                      Params::BASE + Params::CANNY_VALUES_DISTANCE,
	                      BeesBookCommon::Stage::EllipseFitter);

	/**
	 * size
	 *
	 */

	connectSettingsWidget(paramsWidget.max_major_axis,
	                      Params::BASE + Params::MAX_MAJOR_AXIS,
	                      BeesBookCommon::Stage::EllipseFitter);
	connectSettingsWidget(paramsWidget.max_minor_axis,
	                      Params::BASE + Params::MAX_MINOR_AXIS,
	                      BeesBookCommon::Stage::EllipseFitter);
	connectSettingsWidget(paramsWidget.min_major_axis,
	                      Params::BASE + Params::MIN_MAJOR_AXIS,
	                      BeesBookCommon::Stage::EllipseFitter);
	connectSettingsWidget(paramsWidget.min_minor_axis,
	                      Params::BASE + Params::MIN_MINOR_AXIS,
	                      BeesBookCommon::Stage::EllipseFitter);
	/**
	 * vote
	 */

	connectSettingsWidget(paramsWidget.honey_enabled,
	                      Params::BASE + Params::USE_XIE_AS_FALLBACK,
	                      BeesBookCommon::Stage::EllipseFitter);
	connectSettingsWidget(paramsWidget.threshold_best_vote,
	                      Params::BASE + Params::THRESHOLD_BEST_VOTE,
	                      BeesBookCommon::Stage::EllipseFitter);
	connectSettingsWidget(paramsWidget.threshold_edge_pixels,
	                      Params::BASE + Params::THRESHOLD_EDGE_PIXELS,
	                      BeesBookCommon::Stage::EllipseFitter);
	connectSettingsWidget(paramsWidget.threshold_vote,
	                      Params::BASE + Params::THRESHOLD_VOTE,
	                      BeesBookCommon::Stage::EllipseFitter);
}
