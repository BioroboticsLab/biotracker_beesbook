#include "Common.h"

#include "LocalizerParamsWidget.h"
#include "pipeline/Localizer.h"
#include "source/settings/Settings.h"

decoder::localizer_settings_t BeesBookCommon::getLocalizerSettings(const Settings &settings)
{
	decoder::localizer_settings_t localizerSettings;

	localizerSettings.binary_threshold = settings.getValueOfParam<int>(Params::BINARY_THRESHOLD);
	localizerSettings.dilation_1_iteration_number = settings.getValueOfParam<int>(Params::FIRST_DILATION_NUM_ITERATIONS);
	localizerSettings.dilation_1_size = settings.getValueOfParam<int>(Params::FIRST_DILATION_SIZE);
	localizerSettings.dilation_2_size = settings.getValueOfParam<int>(Params::SECOND_DILATION_SIZE);
	localizerSettings.erosion_size    = settings.getValueOfParam<int>(Params::EROSION_SIZE);
	localizerSettings.max_tag_size    = settings.getValueOfParam<unsigned int>(Params::MAX_TAG_SIZE);
	localizerSettings.min_tag_size    = settings.getValueOfParam<int>(Params::MIN_BOUNDING_BOX_SIZE);

	return localizerSettings;
}
