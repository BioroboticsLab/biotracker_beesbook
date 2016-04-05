#pragma once

#include <array>
#include <mutex>
#include <QPainter>

#include <opencv2/opencv.hpp>

#include <QApplication>
#include <QtWidgets/QLabel>

#include <boost/optional.hpp>

#include "Common.h"
#include "GroundTruthEvaluator.h"
#include "ParamsWidget.h"
#include "pipeline/Preprocessor.h"
#include "pipeline/Localizer.h"
#include "pipeline/EllipseFitter.h"
#include "pipeline/GridFitter.h"
#include "pipeline/Decoder.h"
#include "biotracker/settings/Settings.h"
#include "biotracker/TrackingAlgorithm.h"
#include "biotracker/util/CvHelper.h"

#include "biotracker/serialization/SerializationData.h"

namespace BC = BioTracker::Core;

class PipelineGrid;

struct BBVisualizationData {
    boost::optional<cv::Mat> preprocessorImage;
    boost::optional<cv::Mat> preprocessorClahe;
    boost::optional<cv::Mat> localizerInputImage;
    boost::optional<cv::Mat> localizerThresholdImage;
    boost::optional<cv::Mat> localizerSobelImage;
    boost::optional<cv::Mat> localizerBlobImage;
    boost::optional<cv::Mat> ellipsefitterCannyEdge;

    // these references are just stored for convenience in order to invalidate all
    // visualizations in a loop
    typedef std::array<std::reference_wrapper<boost::optional<cv::Mat>>, 7> reference_array_t;

    reference_array_t visualizations = reference_array_t {
        preprocessorImage,
        preprocessorClahe,
        localizerInputImage,
        localizerThresholdImage,
        localizerSobelImage,
        localizerBlobImage,
        ellipsefitterCannyEdge };
};

struct GroundTruthWidgets {
    QLabel *labelNumFalsePositives;
    QLabel *labelNumFalseNegatives;
    QLabel *labelNumTruePositives;
    QLabel *labelNumRecall;
    QLabel *labelNumPrecision;

    QLabel *labelFalsePositives;
    QLabel *labelFalseNegatives;
    QLabel *labelTruePositives;
    QLabel *labelRecall;
    QLabel *labelPrecision;

    void setResults(const size_t numGroundTruth, const size_t numTruePositives,
                    const size_t numFalsePositives, const size_t numFalseNegatives) const;
};

class BeesBookImgAnalysisTracker : public BC::TrackingAlgorithm {
    Q_OBJECT
  public:
    BeesBookImgAnalysisTracker(BC::Settings &settings);

    void track(ulong frameNumber, const cv::Mat &frame) override;
    virtual void paint(size_t frameNumber, BC::ProxyMat &image, View const &view = OriginalView) override;
    virtual void paintOverlay(size_t frameNumber, QPainter *painter, View const &view = OriginalView) override;

  private:
    QVBoxLayout _biotrackerWidgetLayout;
    ParamsWidget _paramsWidget;
    QWidget      _toolsWidget;
    GroundTruthWidgets _groundTruthWidgets;

    BeesBookCommon::Stage _selectedStage;
    pipeline::Preprocessor  _preprocessor;
    pipeline::Localizer     _localizer;
    pipeline::EllipseFitter _ellipsefitter;
    pipeline::GridFitter    _gridFitter;
    pipeline::Decoder       _decoder;

    cv::Mat _image;
    std::mutex _tagListLock;
    taglist_t _taglist;

    boost::optional<GroundTruthEvaluation> _groundTruthEvaluation;
    BBVisualizationData _visualizationData;

    static QPen getDefaultPen(QPainter *painter);
    void visualizeLocalizerOutputOverlay(QPainter *painter) const;
    void visualizeEllipseFitterOutput(cv::Mat &image) const;
    void visualizeEllipseFitterOutputOverlay(QPainter *painter) const;
    void visualizeGridFitterOutput(cv::Mat &image) const;
    void visualizeGridFitterOutputOverlay(QPainter *painter) const;
    void visualizeDecoderOutput(cv::Mat &image) const;
    void visualizeDecoderOutputOverlay(QPainter *painter) const;

    template<typename Widget>
    void setParamsWidget() {
        std::unique_ptr<Widget> widget(std::make_unique<Widget>(m_settings));
        QObject::connect(widget.get(), &Widget::settingsChanged,
                         this, &BeesBookImgAnalysisTracker::settingsChanged);
        _paramsWidget.setParamSubWidget(std::move(widget));
    }

    void resetViews();

  private Q_SLOTS:
    void stageSelectionToogled(BeesBookCommon::Stage stage, bool checked);
    void settingsChanged(const BeesBookCommon::Stage stage);
    void loadGroundTruthData();
    void loadConfig();
    void setPipelineConfig(std::string const &filename);
    void exportConfiguration();
    void loadTaglist();
};
