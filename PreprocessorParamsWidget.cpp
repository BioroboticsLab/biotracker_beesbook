#include "PreprocessorParamsWidget.h"
#include "source/settings/Settings.h"

#include "ui_PreprocessorParamsWidget.h"

PreprocessorParamsWidget::PreprocessorParamsWidget(Settings &settings) :
    ParamsSubWidgetBase(settings)

{
	using namespace pipeline::settings::Preprocessor;

	Ui::PreprocessorParamsWidget paramsWidget;
	paramsWidget.setupUi(&_uiWidget);
	_layout.addWidget(&_uiWidget);

	/*
	 * general
	 */
	connectSettingsWidget(paramsWidget.opt_use_contrast_stretching,
	                      Params::BASE + Params::OPT_USE_CONTRAST_STRETCHING,
	                      BeesBookCommon::Stage::Preprocessor);

	connectSettingsWidget(paramsWidget.opt_use_equalize_histogram,
	                      Params::BASE + Params::OPT_USE_EQUALIZE_HISTOGRAM,
	                      BeesBookCommon::Stage::Preprocessor);

	connectSettingsWidget(paramsWidget.opt_frame_size,
	                      Params::BASE + Params::OPT_FRAME_SIZE,
	                      BeesBookCommon::Stage::Preprocessor);

	connectSettingsWidget(paramsWidget.opt_average_contrast_value,
	                      Params::BASE + Params::OPT_AVERAGE_CONTRAST_VALUE,
	                      BeesBookCommon::Stage::Preprocessor);

	/*
	 * combs
	 */

	connectSettingsWidget(paramsWidget.comb_enabled,
	                      Params::BASE + Params::COMB_ENABLED,
	                      BeesBookCommon::Stage::Preprocessor);

	connectSettingsWidget(paramsWidget.comb_threshold,
	                      Params::BASE + Params::COMB_THRESHOLD,
	                      BeesBookCommon::Stage::Preprocessor);

	connectSettingsWidget(paramsWidget.comb_min_size,
	                      Params::BASE + Params::COMB_MIN_SIZE,
	                      BeesBookCommon::Stage::Preprocessor);
	connectSettingsWidget(paramsWidget.comb_max_size,
	                      Params::BASE + Params::COMB_MAX_SIZE,
	                      BeesBookCommon::Stage::Preprocessor);
	connectSettingsWidget(paramsWidget.comb_line_width,
	                      Params::BASE + Params::COMB_LINE_WIDTH,
	                      BeesBookCommon::Stage::Preprocessor);
	connectSettingsWidget(paramsWidget.comb_line_color,
	                      Params::BASE + Params::COMB_LINE_COLOR,
	                      BeesBookCommon::Stage::Preprocessor);

	/*
	 * honey
	 */
	connectSettingsWidget(paramsWidget.honey_enabled,
	                      Params::BASE + Params::HONEY_ENABLED,
	                      BeesBookCommon::Stage::Preprocessor);

	connectSettingsWidget(paramsWidget.honey_std_dev,
	                      Params::BASE + Params::HONEY_STD_DEV,
	                      BeesBookCommon::Stage::Preprocessor);

	connectSettingsWidget(paramsWidget.honey_frame_size,
	                      Params::BASE + Params::HONEY_FRAME_SIZE,
	                      BeesBookCommon::Stage::Preprocessor);
	connectSettingsWidget(paramsWidget.honey_average_value,
	                      Params::BASE + Params::HONEY_AVERAGE_VALUE,
	                      BeesBookCommon::Stage::Preprocessor);
}
