#ifndef BeesBookImgAnalysisTracker_H
#define BeesBookImgAnalysisTracker_H

#include <mutex>

#include <opencv2/opencv.hpp>

#include <QApplication>

#include "source/settings/Settings.h"
#include "source/tracking/TrackingAlgorithm.h"
#include "source/tracking/algorithm/BeesBookImgAnalysisTracker/pipeline/Converter.h"
#include "source/tracking/algorithm/BeesBookImgAnalysisTracker/pipeline/Decoder.h"
#include "source/tracking/algorithm/BeesBookImgAnalysisTracker/pipeline/GridFitter.h"
#include "source/tracking/algorithm/BeesBookImgAnalysisTracker/pipeline/Localizer.h"
#include "source/tracking/algorithm/BeesBookImgAnalysisTracker/pipeline/Recognizer.h"
#include "source/tracking/algorithm/BeesBookImgAnalysisTracker/pipeline/Transformer.h"

class BeesBookImgAnalysisTracker : public TrackingAlgorithm {
    Q_OBJECT
public:
    BeesBookImgAnalysisTracker(Settings& settings, std::string& serializationPathName, QWidget* parent);

    void track(ulong frameNumber, cv::Mat& frame) override;
    void paint(cv::Mat& image) override;
    void reset() override;

    std::shared_ptr<QWidget> getParamsWidget() override {
        return _paramsWidget;
    }
    std::shared_ptr<QWidget> getToolsWidget() override {
        return _toolsWidget;
    }

public slots:
    //mouse click and move events
    void mouseMoveEvent(QMouseEvent*) override {
    }
    void mousePressEvent(QMouseEvent*) override {
    }
    void mouseReleaseEvent(QMouseEvent*) override {
    }
    void mouseWheelEvent(QWheelEvent*) override {
    }

private:
    enum class SelectedStage : uint8_t {
        NoProcessing = 0,
        Converter,
        Localizer,
        Recognizer,
        Transformer,
        GridFitter,
        Decoder
    };
    SelectedStage _stage;

    const std::shared_ptr<QWidget> _paramsWidget;
    const std::shared_ptr<QWidget> _toolsWidget;

    decoder::Converter _converter;
    decoder::Decoder _decoder;
    decoder::GridFitter _gridFitter;
    decoder::Recognizer _recognizer;
    decoder::Transformer _transformer;
    decoder::Localizer _localizer;

    std::vector<decoder::Tag> _taglist;
    std::mutex _tagListLock;

    void visualizeLocalizerOutput(cv::Mat& image) const;
    void visualizeRecognizerOutput(cv::Mat& image) const;
    void visualizeGridFitterOutput(cv::Mat& image) const;
    void visualizeTransformerOutput(cv::Mat& image) const;
    void visualizeDecoderOutput(cv::Mat& image) const;

private slots:
    void stageSelectionToogled(SelectedStage stage, bool checked);
};

#endif
