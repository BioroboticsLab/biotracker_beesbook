#include "PreprocessorParamsWidget.h"
#include "source/settings/Settings.h"

using namespace Preprocessor;


PreprocessorParamsWidget::PreprocessorParamsWidget(Settings &settings)
	: ParamsSubWidgetBase()
	, _settings(settings)
,_useEqualizeHistogram(this, &_layout, "use histogramEqualization", 0, 1,
	settings.getValueOrDefault(Params::BASE+Params::USE_EQUALIZE_HISTOGRAM, Defaults::USE_EQUALIZE_HISTOGRAM), 1)
, _useCombDetection(this, &_layout, "use comb detection", 0, 1,
	settings.getValueOrDefault(Params::BASE+Params::USE_COMB_DETECTION, Defaults::USE_COMB_DETECTION), 1)
	, _binaryThresholdSlider(this, &_layout, "comb threshold", 1, 255,
	settings.getValueOrDefault(Params::BASE+Params::COMB_THRESHOLD, Defaults::COMB_THRESHOLD), 1)
	, _minCombSizeSlider(this, &_layout, "min comb size", 1, 200,
	settings.getValueOrDefault(Params::BASE+Params::MIN_COMB_SIZE, Defaults::MIN_COMB_SIZE), 1)
, _maxCombSizeSlider(this, &_layout, "max comb size", 1, 200,
settings.getValueOrDefault(Params::BASE+Params::MIN_COMB_SIZE, Defaults::MAX_COMB_SIZE), 1)
	, _diffCombSizeSlider(this, &_layout, "diff W/H combs", 1, 100,
	settings.getValueOrDefault(Params::BASE+Params::DIFF_COMB_SIZE, Defaults::DIFF_COMB_SIZE), 1)
	, _lineWidthSlider(this, &_layout, "line Width", 1, 50,
	settings.getValueOrDefault(Params::BASE+Params::COMB_LINE_WIDTH, Defaults::COMB_LINE_WIDTH), 1)
	, _lineColorSlider(this, &_layout, "line Color", 0, 255,
	settings.getValueOrDefault(Params::BASE+Params::COMB_LINE_COLOR, Defaults::COMB_LINE_COLOR), 1)



{
	auto connectSlider = [ & ](SpinBoxWithSlider* slider, const std::string& paramName) {
		  QObject::connect(slider, &SpinBoxWithSlider::valueChanged, [ = ](int value) {
		        _settings.setParam(paramName, value);
		        emit settingsChanged(BeesBookCommon::Stage::Preprocessor);
			});
	  };
	connectSlider(&_useEqualizeHistogram, Params::BASE+Params::USE_EQUALIZE_HISTOGRAM);
	connectSlider(&_useCombDetection, Params::BASE+Params::USE_COMB_DETECTION);
	connectSlider(&_binaryThresholdSlider,Params::BASE+ Params::COMB_THRESHOLD);
	connectSlider(&_minCombSizeSlider, Params::BASE+Params::MIN_COMB_SIZE);
	connectSlider(&_maxCombSizeSlider, Params::BASE+Params::MAX_COMB_SIZE);
	connectSlider(&_diffCombSizeSlider, Params::BASE+Params::DIFF_COMB_SIZE);
	connectSlider(&_lineWidthSlider, Params::BASE+Params::COMB_LINE_WIDTH);
	connectSlider(&_lineColorSlider, Params::BASE+Params::COMB_LINE_COLOR);



}
