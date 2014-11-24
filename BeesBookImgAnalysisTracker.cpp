#include "BeesBookImgAnalysisTracker.h"

#include <chrono>
#include <vector>

#include <opencv2/core/core.hpp>

#include "source/tracking/algorithm/algorithms.h"
#include "source/tracking/algorithm/BeesBookImgAnalysisTracker/pipeline/datastructure/Tag.h"

#include "ui_ToolWidget.h"
#include "ui_ParamsWidget.h"

namespace {
auto _ = Algorithms::Registry::getInstance()
             .register_tracker_type<BeesBookImgAnalysisTracker>(
                 "BeesBook ImgAnalysis");

struct CursorOverrideRAII {
    CursorOverrideRAII(Qt::CursorShape shape) { QApplication::setOverrideCursor(shape); }
    ~CursorOverrideRAII() { QApplication::restoreOverrideCursor(); }
};
}

BeesBookImgAnalysisTracker::BeesBookImgAnalysisTracker(
    Settings& settings, std::string& serializationPathName, QWidget* parent)
    : TrackingAlgorithm(settings, serializationPathName, parent)
    , _stage(SelectedStage::NoProcessing)
    , _paramsWidget(std::make_shared<QWidget>())
    , _toolsWidget(std::make_shared<QWidget>())
{
    Ui::ToolWidget uiTools;
    uiTools.setupUi(_toolsWidget.get());
    Ui::ParamsWidget uiParams;
    uiParams.setupUi(_paramsWidget.get());

    auto connectRadioButton = [&](QRadioButton* button, SelectedStage stage) {
        QObject::connect(button, &QRadioButton::toggled,
                         [=](bool checked) { stageSelectionToogled(stage, checked); });
    };

    connectRadioButton(uiTools.radioButtonNoProcessing, SelectedStage::NoProcessing);
    connectRadioButton(uiTools.radioButtonConverter, SelectedStage::Converter);
    connectRadioButton(uiTools.radioButtonLocalizer, SelectedStage::Localizer);
    connectRadioButton(uiTools.radioButtonRecognizer, SelectedStage::Recognizer);
    connectRadioButton(uiTools.radioButtonTransformer, SelectedStage::Transformer);
    connectRadioButton(uiTools.radioButtonGridFitter, SelectedStage::GridFitter);
    connectRadioButton(uiTools.radioButtonDecoder, SelectedStage::Decoder);
}

void BeesBookImgAnalysisTracker::track(ulong frameNumber, cv::Mat& frame) {
	std::lock_guard<std::mutex> lock(_tagListLock);
    const CursorOverrideRAII cursorOverride(Qt::WaitCursor);

    _taglist.clear();

    if (_stage < SelectedStage::Converter) return;
    cv::Mat image = _converter.process(frame);
    emit notifyGUI("Converter finished", MSGS::NOTIFICATION);
    if (_stage < SelectedStage::Localizer) return;
    _taglist = _localizer.process(std::move(image));
    emit notifyGUI("Localizer finished", MSGS::NOTIFICATION);
    if (_stage < SelectedStage::Recognizer) return;
    _recognizer.process(_taglist);
    emit notifyGUI("Recognizer finished", MSGS::NOTIFICATION);
    if (_stage < SelectedStage::Transformer) return;
    _transformer.process(_taglist);
    emit notifyGUI("Transformer finished", MSGS::NOTIFICATION);
    if (_stage < SelectedStage::GridFitter) return;
    _gridFitter.process(_taglist);
    emit notifyGUI("GridFitter finished", MSGS::NOTIFICATION);
    if (_stage < SelectedStage::Decoder) return;
    _decoder.process(_taglist);
    emit notifyGUI("Decoder finished", MSGS::NOTIFICATION);
}

void BeesBookImgAnalysisTracker::visualizeLocalizerOutput(cv::Mat& image) const {
	for (const decoder::Tag& tag : _taglist) {
		const cv::Rect& box = tag.getBox();
        cv::rectangle(image, box, cv::Scalar(255, 0, 0), 2);
	}
}

void BeesBookImgAnalysisTracker::visualizeRecognizerOutput(cv::Mat& image) const {
    for (const decoder::Tag& tag : _taglist) {
        for (const decoder::TagCandidate& candidate : tag.getCandidates()) {
            const decoder::Ellipse& ell{candidate.getEllipse()};
            //TODO
        }
    }
}

void BeesBookImgAnalysisTracker::paint(cv::Mat& image) {
	// don't try to visualize results while data processing is running
	if (_tagListLock.try_lock()) {
		visualizeLocalizerOutput(image);
		visualizeRecognizerOutput(image);
		_tagListLock.unlock();
	} else {
		return;
	}
}

void BeesBookImgAnalysisTracker::reset() {}

void BeesBookImgAnalysisTracker::stageSelectionToogled(BeesBookImgAnalysisTracker::SelectedStage stage, bool checked)
{
    if (checked) {
        _stage = stage;
        emit forceTracking();
    }
}

