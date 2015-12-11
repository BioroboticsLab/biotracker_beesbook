#include "LocalizerParamsWidget.h"
#include "biotracker/settings/Settings.h"

#include "ui_LocalizerParamsWidget.h"

using namespace pipeline::settings::Localizer;

LocalizerParamsWidget::LocalizerParamsWidget(Settings &settings) :
		ParamsSubWidgetBase(settings)
{
    Ui::LocalizerParamsWidget paramsWidget;
    paramsWidget.setupUi(&_uiWidget);
    _layout.addWidget(&_uiWidget);

    connectSettingsWidget(paramsWidget.binary_threshold,
                          Params::BASE + Params::BINARY_THRESHOLD,
                          BeesBookCommon::Stage::Localizer);
    connectSettingsWidget(paramsWidget.first_dilation_iterations,
                          Params::BASE + Params::FIRST_DILATION_NUM_ITERATIONS,
                          BeesBookCommon::Stage::Localizer);
    connectSettingsWidget(paramsWidget.first_dilation_size,
                          Params::BASE + Params::FIRST_DILATION_SIZE,
                          BeesBookCommon::Stage::Localizer);
    connectSettingsWidget(paramsWidget.erosion_size,
                          Params::BASE + Params::EROSION_SIZE,
                          BeesBookCommon::Stage::Localizer);
    connectSettingsWidget(paramsWidget.second_dilation_size,
                          Params::BASE + Params::SECOND_DILATION_SIZE,
                          BeesBookCommon::Stage::Localizer);
    connectSettingsWidget(paramsWidget.tag_size,
                          Params::BASE + Params::TAG_SIZE,
                          BeesBookCommon::Stage::Localizer);
    connectSettingsWidget(paramsWidget.min_num_pixels,
                          Params::BASE + Params::MIN_NUM_PIXELS,
                          BeesBookCommon::Stage::Localizer);
    connectSettingsWidget(paramsWidget.max_num_pixels,
                          Params::BASE + Params::MAX_NUM_PIXELS,
                          BeesBookCommon::Stage::Localizer);

#if USE_DEEPLOCALIZER
    connectSettingsWidget(paramsWidget.use_deeplocalizer_filter,
                          Params::BASE + Params::DEEPLOCALIZER_FILTER,
                          BeesBookCommon::Stage::Localizer);
    connectSettingsWidget(paramsWidget.model_path,
                          Params::BASE + Params::DEEPLOCALIZER_MODEL_FILE,
                          BeesBookCommon::Stage::Localizer);
    connectSettingsWidget(paramsWidget.param_path,
                          Params::BASE + Params::DEEPLOCALIZER_PARAM_FILE,
                          BeesBookCommon::Stage::Localizer);
    connectSettingsWidget(paramsWidget.probability_threshold,
                          Params::BASE + Params::DEEPLOCALIZER_PROBABILITY_THRESHOLD,
                          BeesBookCommon::Stage::Localizer);
#endif
}
