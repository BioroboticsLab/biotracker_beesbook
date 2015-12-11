#include "Common.h"

#include <boost/optional.hpp>

#include "LocalizerParamsWidget.h"
#include "biotracker/settings/Settings.h"
#include "pipeline/Localizer.h"

namespace {
template <typename T>
void loadValue(Settings& settings, std::string const& base, pipeline::settings::setting_entry& entry) {
    const boost::optional<T> param =
            settings.maybeGetValueOfParam<T>(base + entry.setting_name);

    if (param) {
        entry.field = boost::get<T>(param);
    } else {
        settings.setParam(base + entry.setting_name, boost::get<T>(entry.field));
    }
}
}

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
        case (setting_entry_type::INT): { loadValue<int>(settings, base, entry); break; }
        case (setting_entry_type::BOOL): { loadValue<bool>(settings, base, entry); break; }
        case (setting_entry_type::DOUBLE): { loadValue<double>(settings, base, entry); break; }
        case (setting_entry_type::U_INT): { loadValue<unsigned int>(settings, base, entry); break; }
        case (setting_entry_type::SIZE_T): { loadValue<size_t>(settings, base, entry); break; }
        case (setting_entry_type::STRING): { loadValue<std::string>(settings, base, entry); break; }
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
