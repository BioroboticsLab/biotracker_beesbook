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
			if (param) {
				entry.field = boost::get<int>(param);
			} else {
				settings.setParam(base + entry.setting_name,
						boost::get<int>(entry.field));
			}

			break;
		}
		case (setting_entry_type::DOUBLE): {
			const boost::optional<double> param = settings.maybeGetValueOfParam<
					double>(base + entry.setting_name);
			if (param) {
				entry.field = boost::get<double>(param);
			} else {
				settings.setParam(base + entry.setting_name,
						boost::get<double>(entry.field));
			}
			break;
		}
		case (setting_entry_type::BOOL): {
			const boost::optional<bool> param = settings.maybeGetValueOfParam<
					bool>(base + entry.setting_name);
			if (param) {
				entry.field = boost::get<bool>(param);
			} else {
				settings.setParam(base + entry.setting_name,
						boost::get<bool>(entry.field));
			}
			break;
		}
		case (setting_entry_type::U_INT): {
			const boost::optional<unsigned int> param =
					settings.maybeGetValueOfParam<unsigned int>(
							base + entry.setting_name);
			if (param) {
				entry.field = boost::get<unsigned int>(param);
			} else {
				settings.setParam(base + entry.setting_name,
						boost::get<unsigned int>(entry.field));
			}
			break;
		}
		case (setting_entry_type::SIZE_T): {
			const boost::optional<size_t> param = settings.maybeGetValueOfParam<
					size_t>(base + entry.setting_name);
			if (param) {
				entry.field = boost::get<size_t>(param);
			} else {
				settings.setParam(base + entry.setting_name,
						boost::get<size_t>(entry.field));
			}
			break;
		}
		}

	}
}

pipeline::settings::localizer_settings_t BeesBookCommon::getLocalizerSettings(
		Settings &settings) {
	pipeline::settings::localizer_settings_t localizerSettings;
	localizerSettings.loadValues(settings,
			pipeline::settings::Localizer::Params::BASE);
	return localizerSettings;
}

pipeline::settings::recognizer_settings_t BeesBookCommon::getRecognizerSettings(
		Settings &settings) {
	pipeline::settings::recognizer_settings_t recognizerSettings;
	recognizerSettings.loadValues(settings,
			pipeline::settings::Recognizer::Params::BASE);
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

	preprocessorSettings.loadValues(settings,
			pipeline::settings::Preprocessor::Params::BASE);

	return preprocessorSettings;
}

