#include "Common.h"

#include <boost/optional.hpp>

#include "LocalizerParamsWidget.h"
#include "pipeline/Localizer.h"
#include "pipeline/datastructure/settings.h"
#include "source/settings/Settings.h"

/**
 * try to lad all setting-entries from the general biotracker-settings into the specific pipeline-setting object.
 * If a setting entry is not found in biotracker-settings, the default is written back into biotracker-settings
 * (to make sure, that every parameter exists for configuration)
 *
 * @param settings settings general biotracker-settings
 * @param base string, in which node the settings where located
 */
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
/**
 *
 * @param settings general biotracker-settings
 * @return setting object for pipeline
 */
pipeline::settings::localizer_settings_t BeesBookCommon::getLocalizerSettings(
        Settings &settings) {
	pipeline::settings::localizer_settings_t localizerSettings;
	localizerSettings.loadValues(settings,
	                             pipeline::settings::Localizer::Params::BASE);
	return localizerSettings;
}
/**
 *
 * @param settings general biotracker-settings
 * @return setting object for pipeline
 */
pipeline::settings::ellipsefitter_settings_t BeesBookCommon::getEllipseFitterSettings(
        Settings &settings) {
	pipeline::settings::ellipsefitter_settings_t ellipsefitterSettings;
	ellipsefitterSettings.loadValues(settings,
	                                 pipeline::settings::EllipseFitter::Params::BASE);
	return ellipsefitterSettings;
}
/**
 *
 * @param settings general biotracker-settings
 * @return setting object for pipeline
 */
pipeline::settings::gridfitter_settings_t BeesBookCommon::getGridfitterSettings(
        Settings &settings) {

	pipeline::settings::gridfitter_settings_t gridfitterSettings;
	gridfitterSettings.loadValues(settings,
	                              pipeline::settings::Gridfitter::Params::BASE);
	return gridfitterSettings;
	return gridfitterSettings;
}
/**
 *
 * @param settings general biotracker-settings
 * @return setting object for pipeline
 */
pipeline::settings::preprocessor_settings_t BeesBookCommon::getPreprocessorSettings(
        Settings &settings) {
	pipeline::settings::preprocessor_settings_t preprocessorSettings;

	preprocessorSettings.loadValues(settings,
	                                pipeline::settings::Preprocessor::Params::BASE);

	return preprocessorSettings;
}
