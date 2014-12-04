#include "Common.h"

#include "LocalizerParamsWidget.h"
#include "pipeline/Localizer.h"
#include "source/settings/Settings.h"

decoder::localizer_settings_t BeesBookCommon::getLocalizerSettings(const Settings &settings)
{
	using namespace Localizer;

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


decoder::recognizer_settings_t BeesBookCommon::getRecognizerSettings(const Settings &settings)
{
	using namespace Recognizer;

	decoder::recognizer_settings_t recognizerSettings;

	recognizerSettings.canny_threshold_high = settings.getValueOfParam<int>(Params::CANNY_THRESHOLD_HIGH);
	recognizerSettings.canny_threshold_low = settings.getValueOfParam<int>(Params::CANNY_THRESHOLD_LOW);
	recognizerSettings.max_major_axis = settings.getValueOfParam<int>(Params::MAX_MAJOR_AXIS);
	recognizerSettings.max_minor_axis = settings.getValueOfParam<int>(Params::MAX_MINOR_AXIS);
	recognizerSettings.min_major_axis = settings.getValueOfParam<int>(Params::MIN_MAJOR_AXIS);
	recognizerSettings.min_minor_axis = settings.getValueOfParam<int>(Params::MIN_MINOR_AXIS);
	recognizerSettings.threshold_best_vote = settings.getValueOfParam<int>(Params::THRESHOLD_BEST_VOTE);
	recognizerSettings.threshold_edge_pixels = settings.getValueOfParam<int>(Params::THRESHOLD_EDGE_PIXELS);
	recognizerSettings.threshold_vote = settings.getValueOfParam<int>(Params::THRESHOLD_VOTE);

	return recognizerSettings;
}
