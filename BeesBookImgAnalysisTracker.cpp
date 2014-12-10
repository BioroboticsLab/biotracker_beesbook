#include "BeesBookImgAnalysisTracker.h"

#include <chrono>
#include <vector>

#include <opencv2/core/core.hpp>

#include "DecoderParamsWidget.h"
#include "GridFitterParamsWidget.h"
#include "LocalizerParamsWidget.h"
#include "RecognizerParamsWidget.h"
#include "pipeline/datastructure/Tag.h"
#include "source/tracking/algorithm/algorithms.h"

#include "ui_ToolWidget.h"

namespace {
auto _ = Algorithms::Registry::getInstance()
  .register_tracker_type<BeesBookImgAnalysisTracker>("BeesBook ImgAnalysis");

struct CursorOverrideRAII {
	CursorOverrideRAII(Qt::CursorShape shape) {
		QApplication::setOverrideCursor(shape);
	}
	~CursorOverrideRAII() {
		QApplication::restoreOverrideCursor();
	}
};

class MeasureTimeRAII {
public:
	MeasureTimeRAII(std::string const& what, std::function<void(std::string const&)> notify)
		: _start(std::chrono::steady_clock::now())
		, _what(what)
		, _notify(notify)
	{
	}
	~MeasureTimeRAII()
	{
		const auto end = std::chrono::steady_clock::now();
		const std::string message { _what + " finished in " +
			                        std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(end - _start).count()) +
			                        +"ms.\n" };
		_notify(message);
		// display message right away
		QApplication::processEvents();
	}
private:
	const std::chrono::steady_clock::time_point _start;
	const std::string _what;
	const std::function<void(std::string const&)> _notify;
};
}

BeesBookImgAnalysisTracker::BeesBookImgAnalysisTracker(
    Settings& settings, QWidget* parent)
	: TrackingAlgorithm(settings, parent)
	, _selectedStage(BeesBookCommon::Stage::NoProcessing)
	, _paramsWidget(std::make_shared<ParamsWidget>())
	, _toolsWidget(std::make_shared<QWidget>())
{
	Ui::ToolWidget uiTools;
	uiTools.setupUi(_toolsWidget.get());

	auto connectRadioButton = [&](QRadioButton* button, BeesBookCommon::Stage stage) {
		  QObject::connect(button, &QRadioButton::toggled,
		    [ = ](bool checked) { stageSelectionToogled(stage, checked); });
	  };

	connectRadioButton(uiTools.radioButtonNoProcessing, BeesBookCommon::Stage::NoProcessing);
	connectRadioButton(uiTools.radioButtonConverter, BeesBookCommon::Stage::Converter);
	connectRadioButton(uiTools.radioButtonLocalizer, BeesBookCommon::Stage::Localizer);
	connectRadioButton(uiTools.radioButtonRecognizer, BeesBookCommon::Stage::Recognizer);
	connectRadioButton(uiTools.radioButtonTransformer, BeesBookCommon::Stage::Transformer);
	connectRadioButton(uiTools.radioButtonGridFitter, BeesBookCommon::Stage::GridFitter);
	connectRadioButton(uiTools.radioButtonDecoder, BeesBookCommon::Stage::Decoder);

	QObject::connect(uiTools.processButton, &QPushButton::pressed,
	  [&]() { emit forceTracking(); });

	_localizer.loadSettings(BeesBookCommon::getLocalizerSettings(_settings));
	_recognizer.loadSettings(BeesBookCommon::getRecognizerSettings(_settings));
	//TODO
}

void BeesBookImgAnalysisTracker::track(ulong /*frameNumber*/, cv::Mat& frame) {
	const auto notify = [&](std::string const& message) { emit notifyGUI(message, MSGS::NOTIFICATION); };

	const std::lock_guard<std::mutex> lock(_tagListLock);
	const CursorOverrideRAII cursorOverride(Qt::WaitCursor);

	_taglist.clear();

	if (_selectedStage < BeesBookCommon::Stage::Converter) return;
	cv::Mat image;
	{
		MeasureTimeRAII measure("Converter", notify);
		image = _converter.process(frame);
	}
	if (_selectedStage < BeesBookCommon::Stage::Localizer) return;
	{
		MeasureTimeRAII measure("Localizer", notify);
		_taglist = _localizer.process(std::move(image));
	}
	if (_selectedStage < BeesBookCommon::Stage::Recognizer) return;
	{
		MeasureTimeRAII measure("Recognizer", notify);
		_taglist = _recognizer.process(std::move(_taglist));
	}
	if (_selectedStage < BeesBookCommon::Stage::Transformer) return;
	{
		MeasureTimeRAII measure("Transformer", notify);
		_taglist = _transformer.process(std::move(_taglist));
	}
	if (_selectedStage < BeesBookCommon::Stage::GridFitter) return;
	{
		MeasureTimeRAII measure("GridFitter", notify);
		_taglist = _gridFitter.process(std::move(_taglist));
	}
	if (_selectedStage < BeesBookCommon::Stage::Decoder) return;
	{
		MeasureTimeRAII measure("Decoder", notify);
		_taglist = _decoder.process(std::move(_taglist));
	}
}

void BeesBookImgAnalysisTracker::visualizeLocalizerOutput(cv::Mat& image) const {
	for (const decoder::Tag& tag : _taglist) {
		const cv::Rect& box = tag.getBox();
		cv::rectangle(image, box, cv::Scalar(0, 255, 0), 5);
	}
}

void BeesBookImgAnalysisTracker::visualizeRecognizerOutput(cv::Mat& image) const {
	for (const decoder::Tag& tag : _taglist) {
		if (!tag.getCandidates().empty()) {
			// get best candidate
			const decoder::TagCandidate& candidate = tag.getCandidates()[0];
			const decoder::Ellipse& ellipse = candidate.getEllipse();
			cv::ellipse(image, tag.getBox().tl() + ellipse.getCen(), ellipse.getAxis(),
						ellipse.getAngle(), 0, 360, cv::Scalar(0, 255, 0), 3);
			cv::putText(image, "Score: " + std::to_string(ellipse.getVote()),
						tag.getBox().tl() + cv::Point(80, 80), cv::FONT_HERSHEY_COMPLEX_SMALL, 3.0,
						cv::Scalar(0, 255, 0), 2, CV_AA);
		}
	}
}

void BeesBookImgAnalysisTracker::visualizeGridFitterOutput(cv::Mat& /*image*/) const {
	//TODO
}

void BeesBookImgAnalysisTracker::visualizeTransformerOutput(cv::Mat& /*image*/) const {
	//TODO
}

void BeesBookImgAnalysisTracker::visualizeDecoderOutput(cv::Mat& image) const {
	for (const decoder::Tag& tag : _taglist) {
		if (tag.getCandidates().size()) {
			const decoder::TagCandidate& candidate = tag.getCandidates()[0];
			if (candidate.getDecodings().size()) {
				const decoder::Decoding& decoding = candidate.getDecodings()[0];
				cv::putText(image, std::to_string(decoding.tagId),
				  cv::Point(tag.getBox().x, tag.getBox().y),
				  cv::FONT_HERSHEY_COMPLEX_SMALL, 3.0,
				  cv::Scalar(0, 255, 0), 2, CV_AA);
			}
		}
	}
}

void BeesBookImgAnalysisTracker::paint(cv::Mat& image) {
	// don't try to visualize results while data processing is running
	if (_tagListLock.try_lock()) {
		switch (_selectedStage) {
		case BeesBookCommon::Stage::Localizer:
			visualizeLocalizerOutput(image);
			break;
		case BeesBookCommon::Stage::Recognizer:
			visualizeRecognizerOutput(image);
			break;
		case BeesBookCommon::Stage::Transformer:
			visualizeTransformerOutput(image);
			break;
		case BeesBookCommon::Stage::GridFitter:
			visualizeGridFitterOutput(image);
			break;
		case BeesBookCommon::Stage::Decoder:
			visualizeDecoderOutput(image);
			break;
		default:
			break;
		}
		_tagListLock.unlock();
	} else {
		return;
	}
}

void BeesBookImgAnalysisTracker::reset() {
}

void BeesBookImgAnalysisTracker::settingsChanged(const BeesBookCommon::Stage stage)
{
	switch (stage) {
	case BeesBookCommon::Stage::Localizer:
		_localizer.loadSettings(BeesBookCommon::getLocalizerSettings(_settings));
		break;
	case BeesBookCommon::Stage::Recognizer:
		_recognizer.loadSettings(BeesBookCommon::getRecognizerSettings(_settings));
		break;
	case BeesBookCommon::Stage::GridFitter:
		// TODO
		break;
	case BeesBookCommon::Stage::Decoder:
		// TODO
		break;
	default:
		break;
	}
}

void BeesBookImgAnalysisTracker::stageSelectionToogled(BeesBookCommon::Stage stage, bool checked)
{
	if (checked) {
		_selectedStage = stage;

		switch (stage) {
		case BeesBookCommon::Stage::Localizer:
			setParamsWidget<LocalizerParamsWidget>();
			break;
		case BeesBookCommon::Stage::Recognizer:
			setParamsWidget<RecognizerParamsWidget>();
			break;
		case BeesBookCommon::Stage::GridFitter:
			setParamsWidget<GridFitterParamsWidget>();
			break;
		case BeesBookCommon::Stage::Decoder:
			setParamsWidget<DecoderParamsWidget>();
			break;
		default:
			_paramsWidget->setParamSubWidget(std::make_unique<QWidget>());
			break;
		}

		emit update();
	}
}

