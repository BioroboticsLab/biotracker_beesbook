#include "GridFitterParamsWidget.h"
#include "source/settings/Settings.h"

using namespace pipeline::settings::Gridfitter;


#include "ui_GridfitterParamsWidget.h"

GridFitterParamsWidget::GridFitterParamsWidget(Settings &settings)
    : ParamsSubWidgetBase(settings)
{
	Ui::GridfitterParamsWidget paramsWidget;
	paramsWidget.setupUi(this);

	/*
	 * err function
	 */
	connectSettingsWidget(paramsWidget.err_func_alpha_inner,
	                      Params::BASE + Params::ERR_FUNC_ALPHA_INNER,
	                      BeesBookCommon::Stage::GridFitter);
	connectSettingsWidget(paramsWidget.err_func_alpha_inner_edge,
	                      Params::BASE + Params::ERR_FUNC_ALPHA_INNER_EDGE,
	                      BeesBookCommon::Stage::GridFitter);
	connectSettingsWidget(paramsWidget.err_func_alpha_outer,
	                      Params::BASE + Params::ERR_FUNC_ALPHA_OUTER,
	                      BeesBookCommon::Stage::GridFitter);
	connectSettingsWidget(paramsWidget.err_func_alpha_outer_edge,
	                      Params::BASE + Params::ERR_FUNC_ALPHA_OUTER_EDGE,
	                      BeesBookCommon::Stage::GridFitter);
	connectSettingsWidget(paramsWidget.err_func_alpha_variance,
	                      Params::BASE + Params::ERR_FUNC_ALPHA_VARIANCE,
	                      BeesBookCommon::Stage::GridFitter);

	/*
	 * adaptive
	 */

	connectSettingsWidget(paramsWidget.adaptive_block_size,
	                      Params::BASE + Params::ADAPTIVE_BLOCK_SIZE,
	                      BeesBookCommon::Stage::GridFitter);
	connectSettingsWidget(paramsWidget.adaptive_c,
	                      Params::BASE + Params::ADAPTIVE_C,
	                      BeesBookCommon::Stage::GridFitter);
	/*
	 * gradient descent
	 */
	connectSettingsWidget(paramsWidget.gradient_max_iterations,
	                      Params::BASE + Params::GRADIENT_MAX_ITERATIONS,
	                      BeesBookCommon::Stage::GridFitter);
	connectSettingsWidget(paramsWidget.gradient_num_initial,
	                      Params::BASE + Params::GRADIENT_NUM_INITIAL,
	                      BeesBookCommon::Stage::GridFitter);
	connectSettingsWidget(paramsWidget.gradient_threshold,
	                      Params::BASE + Params::GRADIENT_ERROR_THRESHOLD,
	                      BeesBookCommon::Stage::GridFitter);
	connectSettingsWidget(paramsWidget.gradient_num_results,
	                      Params::BASE + Params::GRADIENT_NUM_RESULTS,
	                      BeesBookCommon::Stage::GridFitter);

	connectSettingsWidget(paramsWidget.eps_angle,
	                      Params::BASE + Params::EPS_ANGLE,
	                      BeesBookCommon::Stage::GridFitter);
	connectSettingsWidget(paramsWidget.eps_pos,
	                      Params::BASE + Params::EPS_POS,
	                      BeesBookCommon::Stage::GridFitter);
	connectSettingsWidget(paramsWidget.eps_scale,
	                      Params::BASE + Params::EPS_SCALE,
	                      BeesBookCommon::Stage::GridFitter);
	connectSettingsWidget(paramsWidget.alpha,
	                      Params::BASE + Params::ALPHA,
	                      BeesBookCommon::Stage::GridFitter);
}
