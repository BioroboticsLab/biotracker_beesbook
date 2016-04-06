#include "Common.h"

#include <boost/optional.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/archive/xml_iarchive.hpp>
#include <QPainter>

#include <pipeline/Localizer.h>
#include <pipeline/datastructure/Tag.h>
#include <biotracker/util/CvHelper.h>

#include "LocalizerParamsWidget.h"

namespace BC = BioTracker::Core;

namespace {
template <typename T>
void loadValue(BC::Settings &settings, std::string const &base, pipeline::settings::setting_entry &entry) {
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
void pipeline::settings::settings_abs::loadValues(BC::Settings &settings,
        std::string base) {

    typedef std::map<std::string, setting_entry>::iterator it_type;
    for (it_type it = _settings.begin(); it != _settings.end(); it++) {
        setting_entry &entry = it->second;

        switch (entry.type) {
        case (setting_entry_type::INT): {
            loadValue<int>(settings, base, entry);
            break;
        }
        case (setting_entry_type::BOOL): {
            loadValue<bool>(settings, base, entry);
            break;
        }
        case (setting_entry_type::DOUBLE): {
            loadValue<double>(settings, base, entry);
            break;
        }
        case (setting_entry_type::U_INT): {
            loadValue<unsigned int>(settings, base, entry);
            break;
        }
        case (setting_entry_type::SIZE_T): {
            loadValue<size_t>(settings, base, entry);
            break;
        }
        case (setting_entry_type::STRING): {
            loadValue<std::string>(settings, base, entry);
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
    BC::Settings &settings) {
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
    BC::Settings &settings) {
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
    BC::Settings &settings) {

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
    BC::Settings &settings) {
    pipeline::settings::preprocessor_settings_t preprocessorSettings;

    preprocessorSettings.loadValues(settings,
                                    pipeline::settings::Preprocessor::Params::BASE);

    return preprocessorSettings;
}

BeesBookCommon::taglist_t BeesBookCommon::loadSerializedTaglist(const std::__cxx11::string &path) {
    taglist_t taglist;

    std::ifstream ifs(path);
    boost::archive::xml_iarchive ia(ifs);
    ia &BOOST_SERIALIZATION_NVP(taglist);

    return taglist;
}

#define addToBioTracker(param) { \
{                                \
    const auto paramBioTracker = S::Params::BASE + S::Params::param; \
    const auto paramPipeline = S::Params::param; \
    bioTrackerSettings.setParam(paramBioTracker, \
                        settings.getValue<decltype(S::Defaults::param)>(paramPipeline)); \
} \
}

void BeesBookCommon::setPreprocessorSettings(BioTracker::Core::Settings &bioTrackerSettings,
                                             pipeline::settings::preprocessor_settings_t &settings)
{
    namespace S = pipeline::settings::Preprocessor;
    addToBioTracker(COMB_DIFF_SIZE)
    addToBioTracker(OPT_USE_CONTRAST_STRETCHING)
    addToBioTracker(OPT_USE_EQUALIZE_HISTOGRAM)
    addToBioTracker(OPT_FRAME_SIZE)
    addToBioTracker(OPT_AVERAGE_CONTRAST_VALUE)
    addToBioTracker(COMB_ENABLED)
    addToBioTracker(COMB_MIN_SIZE)
    addToBioTracker(COMB_MAX_SIZE)
    addToBioTracker(COMB_THRESHOLD)
    addToBioTracker(COMB_DIFF_SIZE)
    addToBioTracker(COMB_LINE_WIDTH)
    addToBioTracker(COMB_LINE_COLOR)
    addToBioTracker(HONEY_ENABLED)
    addToBioTracker(HONEY_STD_DEV)
    addToBioTracker(HONEY_FRAME_SIZE)
    addToBioTracker(HONEY_AVERAGE_VALUE)
}

void BeesBookCommon::setLocalizerSettings(BioTracker::Core::Settings &bioTrackerSettings,
                                          pipeline::settings::localizer_settings_t &settings)
{
    namespace S = pipeline::settings::Localizer;
    addToBioTracker(BINARY_THRESHOLD)
    addToBioTracker(FIRST_DILATION_NUM_ITERATIONS)
    addToBioTracker(FIRST_DILATION_SIZE)
    addToBioTracker(EROSION_SIZE)
    addToBioTracker(SECOND_DILATION_SIZE)
    addToBioTracker(MIN_NUM_PIXELS)
    addToBioTracker(MAX_NUM_PIXELS)
    addToBioTracker(TAG_SIZE)
    addToBioTracker(DEEPLOCALIZER_FILTER)
    addToBioTracker(DEEPLOCALIZER_MODEL_FILE)
    addToBioTracker(DEEPLOCALIZER_PARAM_FILE)
    addToBioTracker(DEEPLOCALIZER_PROBABILITY_THRESHOLD)
}

void BeesBookCommon::setEllipseFitterSettings(BioTracker::Core::Settings &bioTrackerSettings, pipeline::settings::ellipsefitter_settings_t &settings)
{
    namespace S = pipeline::settings::EllipseFitter;
    addToBioTracker(CANNY_INITIAL_HIGH)
    addToBioTracker(CANNY_VALUES_DISTANCE)
    addToBioTracker(CANNY_MEAN_MIN)
    addToBioTracker(CANNY_MEAN_MAX)
    addToBioTracker(MIN_MAJOR_AXIS)
    addToBioTracker(MAX_MAJOR_AXIS)
    addToBioTracker(MIN_MINOR_AXIS)
    addToBioTracker(MAX_MINOR_AXIS)
    addToBioTracker(ELLIPSE_REGULARISATION)
    addToBioTracker(THRESHOLD_EDGE_PIXELS)
    addToBioTracker(THRESHOLD_VOTE)
    addToBioTracker(THRESHOLD_BEST_VOTE)
    addToBioTracker(USE_XIE_AS_FALLBACK)
}

void BeesBookCommon::setGridFitterSettings(BioTracker::Core::Settings &bioTrackerSettings, pipeline::settings::gridfitter_settings_t &settings)
{
    namespace S = pipeline::settings::Gridfitter;
    addToBioTracker(ERR_FUNC_ALPHA_INNER)
    addToBioTracker(ERR_FUNC_ALPHA_OUTER)
    addToBioTracker(ERR_FUNC_ALPHA_VARIANCE)
    addToBioTracker(ERR_FUNC_ALPHA_OUTER_EDGE)
    addToBioTracker(ERR_FUNC_ALPHA_INNER_EDGE)
    addToBioTracker(SOBEL_THRESHOLD)
    addToBioTracker(ADAPTIVE_BLOCK_SIZE)
    addToBioTracker(ADAPTIVE_C)
    addToBioTracker(GRADIENT_NUM_INITIAL)
    addToBioTracker(GRADIENT_NUM_RESULTS)
    addToBioTracker(GRADIENT_ERROR_THRESHOLD)
    addToBioTracker(GRADIENT_MAX_ITERATIONS)
    addToBioTracker(EPS_ANGLE)
    addToBioTracker(EPS_POS)
    addToBioTracker(EPS_SCALE)
    addToBioTracker(ALPHA)
}
