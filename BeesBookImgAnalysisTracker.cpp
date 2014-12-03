#include "BeesBookImgAnalysisTracker.h"

#include <chrono>
#include <vector>

#include <opencv2/core/core.hpp>

#include "source/tracking/algorithm/BeesBookImgAnalysisTracker/pipeline/datastructure/Tag.h"
#include "source/tracking/algorithm/algorithms.h"

#include "ui_ParamsWidget.h"
#include "ui_ToolWidget.h"
#include "ui_DecoderParamsWidget.h"
#include "ui_GridFitterParamsWidget.h"
#include "ui_LocalizerParamsWidget.h"
#include "ui_RecognizerParamsWidget.h"
#include "ui_TransformerParamsWidget.h"

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
            [ = ](bool checked) { stageSelectionToogled(stage, checked); });
      };

    connectRadioButton(uiTools.radioButtonNoProcessing, SelectedStage::NoProcessing);
    connectRadioButton(uiTools.radioButtonConverter, SelectedStage::Converter);
    connectRadioButton(uiTools.radioButtonLocalizer, SelectedStage::Localizer);
    connectRadioButton(uiTools.radioButtonRecognizer, SelectedStage::Recognizer);
    connectRadioButton(uiTools.radioButtonTransformer, SelectedStage::Transformer);
    connectRadioButton(uiTools.radioButtonGridFitter, SelectedStage::GridFitter);
    connectRadioButton(uiTools.radioButtonDecoder, SelectedStage::Decoder);
}

void BeesBookImgAnalysisTracker::track(ulong /*frameNumber*/, cv::Mat& frame) {
    static const auto notify = [&](std::string const& message) { emit notifyGUI(message, MSGS::NOTIFICATION); };

    std::lock_guard<std::mutex> lock(_tagListLock);
    const CursorOverrideRAII cursorOverride(Qt::WaitCursor);

    _taglist.clear();

    if (_stage < SelectedStage::Converter) return;
    cv::Mat image;
    {
        MeasureTimeRAII measure("Converter", notify);
        image = _converter.process(frame);
    }
    if (_stage < SelectedStage::Localizer) return;
    {
        MeasureTimeRAII measure("Localizer", notify);
        _taglist = _localizer.process(std::move(image));
    }
    if (_stage < SelectedStage::Recognizer) return;
    {
        MeasureTimeRAII measure("Recognizer", notify);
        _taglist = _recognizer.process(std::move(_taglist));
    }
    if (_stage < SelectedStage::Transformer) return;
    {
        MeasureTimeRAII measure("Transformer", notify);
        _taglist = _transformer.process(std::move(_taglist));
    }
    if (_stage < SelectedStage::GridFitter) return;
    {
        MeasureTimeRAII measure("GridFitter", notify);
        _taglist = _gridFitter.process(std::move(_taglist));
    }
    if (_stage < SelectedStage::Decoder) return;
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

void BeesBookImgAnalysisTracker::visualizeRecognizerOutput(cv::Mat& /*image*/) const {
    //TODO
}

void BeesBookImgAnalysisTracker::visualizeGridFitterOutput(cv::Mat& /*image*/) const {
    //TODO
}

void BeesBookImgAnalysisTracker::visualizeTransformerOutput(cv::Mat& /*image*/) const {
    //TODO
}

void BeesBookImgAnalysisTracker::visualizeDecoderOutput(cv::Mat& image) const {
    for (const decoder::Tag& tag : _taglist) {
        if (tag.getCandidatesConst().size()) {
            const decoder::TagCandidate& candidate = tag.getCandidatesConst()[0];
            if (candidate.getDecodings().size()) {
                const decoder::Decoding& decoding = candidate.getDecodings()[0];
                cv::putText(image, std::to_string(decoding.tagId),
                            cv::Point(tag.getBox().x, tag.getBox(). y),
                            cv::FONT_HERSHEY_COMPLEX_SMALL, 3.0,
                            cv::Scalar(0, 255, 0), 3, CV_AA);
            }
        }
    }
}

void BeesBookImgAnalysisTracker::paint(cv::Mat& image) {
    // don't try to visualize results while data processing is running
    if (_tagListLock.try_lock()) {
        switch (_stage) {
        case SelectedStage::Localizer:
            visualizeLocalizerOutput(image);
            break;
        case SelectedStage::Recognizer:
            visualizeRecognizerOutput(image);
            break;
        case SelectedStage::Transformer:
            visualizeTransformerOutput(image);
            break;
        case SelectedStage::GridFitter:
            visualizeGridFitterOutput(image);
            break;
        case SelectedStage::Decoder:
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

void BeesBookImgAnalysisTracker::stageSelectionToogled(BeesBookImgAnalysisTracker::SelectedStage stage, bool checked)
{
    if (checked) {
        _stage = stage;

        _currentParamsWidget = std::make_unique<QWidget>();
        switch (stage) {
        case SelectedStage::Converter:
            break;
        case SelectedStage::Localizer:
            setParamsWidget<Ui::LocalizerParamsWidget>();
            break;
        case SelectedStage::Recognizer:
            setParamsWidget<Ui::RecognizerParamsWidget>();
            break;
        case SelectedStage::Transformer:
            setParamsWidget<Ui::TransformerParamsWidget>();
            break;
        case SelectedStage::GridFitter:
            setParamsWidget<Ui::GridFitterParamsWidget>();
            break;
        case SelectedStage::Decoder:
            setParamsWidget<Ui::DecoderParamsWidget>();
            break;
        default:
            break;
        }

        emit forceTracking();
    }
}

