#include "Common.h"

#include <boost/optional.hpp>

#include "LocalizerParamsWidget.h"
#include "pipeline/Localizer.h"
#include "pipeline/datastructure/settings.h"
#include "source/settings/Settings.h"

namespace {
template<typename ParamType>
void maybeSetParam(const Settings& settings, ParamType& paramLoc,
		const std::string& name) {
	const boost::optional<ParamType> param = settings.maybeGetValueOfParam<
			ParamType>(name);
	if (param)
		paramLoc = std::move(*param);
}
}

void pipeline::settings::settings_abs::loadValues(Settings& settings,
		std::string base) {

	typedef std::map<std::string, setting_entry>::iterator it_type;
	for (it_type it = _settings.begin(); it != _settings.end(); it++) {
		setting_entry& entry = it->second;

		switch (entry.type) {
		case (setting_entry_type::INT): {
			const boost::optional<int> param =
					settings.maybeGetValueOfParam<int>(
							base + entry.setting_name);
			if (param){
				entry.field = boost::get<int>(param);
		}else{
			settings.setParam(base + entry.setting_name, boost::get<int>(entry.field));
		}

			break;
		}
		case (setting_entry_type::DOUBLE): {
			const boost::optional<double> param = settings.maybeGetValueOfParam<
					double>(base + entry.setting_name);
			if (param){
				entry.field = boost::get<double>(param);
			}else{
						settings.setParam(base + entry.setting_name, boost::get<double>(entry.field));
					}
			break;
		}
		case (setting_entry_type::BOOL): {
			const boost::optional<bool> param = settings.maybeGetValueOfParam<
					bool>(base + entry.setting_name);
			if (param){
				entry.field = boost::get<bool>(param);
			}else{
						settings.setParam(base + entry.setting_name, boost::get<bool>(entry.field));
					}
			break;
		}
		case (setting_entry_type::U_INT): {
			const boost::optional<unsigned int> param =
					settings.maybeGetValueOfParam<unsigned int>(
							base + entry.setting_name);
			if (param){
				entry.field = boost::get<unsigned int>(param);
			}else{
				settings.setParam(base + entry.setting_name, boost::get<unsigned int>(entry.field));
			}
			break;
		}
		}


	}
}

pipeline::settings::localizer_settings_t BeesBookCommon::getLocalizerSettings(Settings &settings) {
	pipeline::settings::localizer_settings_t localizerSettings;
	localizerSettings.loadValues(settings, pipeline::settings::Localizer::Params::BASE);
	return localizerSettings;
}

pipeline::recognizer_settings_t BeesBookCommon::getRecognizerSettings(
		const Settings &settings) {
	using namespace Recognizer;

	pipeline::recognizer_settings_t recognizerSettings;

	maybeSetParam<int>(settings, recognizerSettings.canny_threshold_high,
			Params::CANNY_THRESHOLD_HIGH);
	maybeSetParam<int>(settings, recognizerSettings.canny_threshold_low,
			Params::CANNY_THRESHOLD_LOW);
	maybeSetParam<int>(settings, recognizerSettings.max_major_axis,
			Params::MAX_MAJOR_AXIS);
	maybeSetParam<int>(settings, recognizerSettings.max_minor_axis,
			Params::MAX_MINOR_AXIS);
	maybeSetParam<int>(settings, recognizerSettings.min_major_axis,
			Params::MIN_MAJOR_AXIS);
	maybeSetParam<int>(settings, recognizerSettings.min_minor_axis,
			Params::MIN_MINOR_AXIS);
	maybeSetParam<int>(settings, recognizerSettings.threshold_best_vote,
			Params::THRESHOLD_BEST_VOTE);
	maybeSetParam<int>(settings, recognizerSettings.threshold_edge_pixels,
			Params::THRESHOLD_EDGE_PIXELS);
	maybeSetParam<int>(settings, recognizerSettings.threshold_vote,
			Params::THRESHOLD_VOTE);

	return recognizerSettings;
}

pipeline::gridfitter_settings_t BeesBookCommon::getGridfitterSettings(
		const Settings &settings) {
	using namespace GridFitter;

	pipeline::gridfitter_settings_t gridfitterSettings;

//	maybeSetParam<int>(settings, gridfitterSettings.initial_step_size, Params::INITIAL_STEP_SIZE);
//	maybeSetParam<int>(settings, gridfitterSettings.final_step_size, Params::FINAL_STEP_SIZE);
//	maybeSetParam<float>(settings, gridfitterSettings.up_speed, Params::UP_SPEED);
//	maybeSetParam<float>(settings, gridfitterSettings.down_speed, Params::DOWN_SPEED);

	return gridfitterSettings;
}

pipeline::settings::preprocessor_settings_t BeesBookCommon::getPreprocessorSettings(
		Settings &settings) {
	pipeline::settings::preprocessor_settings_t preprocessorSettings;

	preprocessorSettings.loadValues(settings, pipeline::settings::Preprocessor::Params::BASE);

	return preprocessorSettings;
}

