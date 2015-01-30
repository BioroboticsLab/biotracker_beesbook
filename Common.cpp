#include "Common.h"

#include <boost/optional.hpp>

#include "LocalizerParamsWidget.h"
#include "pipeline/Localizer.h"
#include "source/settings/Settings.h"

namespace {
template <typename ParamType>
void maybeSetParam(const Settings& settings, ParamType& paramLoc, const std::string& name) {
	const boost::optional<ParamType> param = settings.maybeGetValueOfParam<ParamType>(name);
	if (param) paramLoc = std::move(*param);
}
}

pipeline::localizer_settings_t BeesBookCommon::getLocalizerSettings(const Settings &settings)
{
	using namespace Localizer;

	pipeline::localizer_settings_t localizerSettings;

	maybeSetParam<int>(settings, localizerSettings.binary_threshold, Params::BINARY_THRESHOLD);
	maybeSetParam<int>(settings, localizerSettings.dilation_1_iteration_number, Params::FIRST_DILATION_NUM_ITERATIONS);
	maybeSetParam<int>(settings, localizerSettings.dilation_1_size, Params::FIRST_DILATION_SIZE);
	maybeSetParam<int>(settings, localizerSettings.dilation_2_size, Params::SECOND_DILATION_SIZE);
	maybeSetParam<int>(settings, localizerSettings.erosion_size, Params::EROSION_SIZE);
	maybeSetParam<unsigned int>(settings, localizerSettings.max_tag_size, Params::MAX_TAG_SIZE);
	maybeSetParam<int>(settings, localizerSettings.min_tag_size, Params::MIN_BOUNDING_BOX_SIZE);

	return localizerSettings;
}


pipeline::recognizer_settings_t BeesBookCommon::getRecognizerSettings(const Settings &settings)
{
	using namespace Recognizer;

	pipeline::recognizer_settings_t recognizerSettings;

	maybeSetParam<int>(settings, recognizerSettings.canny_threshold_high, Params::CANNY_THRESHOLD_HIGH);
	maybeSetParam<int>(settings, recognizerSettings.canny_threshold_low, Params::CANNY_THRESHOLD_LOW);
	maybeSetParam<int>(settings, recognizerSettings.max_major_axis, Params::MAX_MAJOR_AXIS);
	maybeSetParam<int>(settings, recognizerSettings.max_minor_axis, Params::MAX_MINOR_AXIS);
	maybeSetParam<int>(settings, recognizerSettings.min_major_axis, Params::MIN_MAJOR_AXIS);
	maybeSetParam<int>(settings, recognizerSettings.min_minor_axis, Params::MIN_MINOR_AXIS);
	maybeSetParam<int>(settings, recognizerSettings.threshold_best_vote, Params::THRESHOLD_BEST_VOTE);
	maybeSetParam<int>(settings, recognizerSettings.threshold_edge_pixels, Params::THRESHOLD_EDGE_PIXELS);
	maybeSetParam<int>(settings, recognizerSettings.threshold_vote, Params::THRESHOLD_VOTE);

	return recognizerSettings;
}


pipeline::gridfitter_settings_t BeesBookCommon::getGridfitterSettings(const Settings &settings)
{
	using namespace GridFitter;

	pipeline::gridfitter_settings_t gridfitterSettings;

//	maybeSetParam<int>(settings, gridfitterSettings.initial_step_size, Params::INITIAL_STEP_SIZE);
//	maybeSetParam<int>(settings, gridfitterSettings.final_step_size, Params::FINAL_STEP_SIZE);
//	maybeSetParam<float>(settings, gridfitterSettings.up_speed, Params::UP_SPEED);
//	maybeSetParam<float>(settings, gridfitterSettings.down_speed, Params::DOWN_SPEED);

	return gridfitterSettings;
}


pipeline::preprocessor_settings_t BeesBookCommon::getPreprocessorSettings(const Settings &settings)
{
	using namespace Preprocessor;

	pipeline::preprocessor_settings_t preprocessorSettings;

	maybeSetParam<double>(settings, preprocessorSettings.comb_threshold, Params::BASE+Params::COMB_THRESHOLD);
	maybeSetParam<unsigned int>(settings, preprocessorSettings.min_size_comb, Params::BASE+Params::MIN_COMB_SIZE);
	maybeSetParam<unsigned int>(settings, preprocessorSettings.max_size_comb, Params::BASE+Params::MAX_COMB_SIZE);
	maybeSetParam<double>(settings, preprocessorSettings.diff_size_combs, Params::BASE+Params::DIFF_COMB_SIZE);
	maybeSetParam<unsigned int>(settings, preprocessorSettings.line_color_combs, Params::BASE+Params::COMB_LINE_COLOR);
	maybeSetParam<unsigned int>(settings, preprocessorSettings.use_comb_detection,Params::BASE+ Params::USE_COMB_DETECTION);
	maybeSetParam<int>(settings, preprocessorSettings.line_width_combs, Params::BASE+Params::COMB_LINE_WIDTH);
	maybeSetParam<unsigned int>(settings, preprocessorSettings.use_equalize_histogram, Params::BASE+Params::USE_EQUALIZE_HISTOGRAM);

	return preprocessorSettings;
}

