#include "BeesBookImgAnalysisTracker.h"

#include <array>
#include <chrono>
#include <functional>
#include <set>
#include <vector>

#include <QFileDialog>
#include <QFontDatabase>
#include <QInputDialog>
#include <QMessageBox>

#include <opencv2/core/core.hpp>

#include <cereal/types/polymorphic.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/vector.hpp>

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/iostreams/device/file.hpp>
#include <boost/archive/xml_iarchive.hpp>

#include "Common.h"
#include "DecoderParamsWidget.h"
#include "GridFitterParamsWidget.h"
#include "LocalizerParamsWidget.h"
#include "EllipseFitterParamsWidget.h"
#include "PreprocessorParamsWidget.h"
#include "pipeline/util/CvHelper.h"
#include "pipeline/util/Util.h"
#include "pipeline/datastructure/Tag.h"
#include "pipeline/datastructure/TagCandidate.h"
#include "pipeline/datastructure/PipelineGrid.h"
#include "pipeline/datastructure/serialization.hpp"
#include "legacy/Grid3D.h"
#include "Utils.h"
#include "Visualization.h"

#include "ui_ToolWidget.h"

using namespace BeesBookCommon;
using namespace Visualization;
using namespace Utils;

BeesBookImgAnalysisTracker::BeesBookImgAnalysisTracker(BC::Settings &settings) :
    TrackingAlgorithm(settings),
    _selectedStage(BeesBookCommon::Stage::NoProcessing) {
    Ui::ToolWidget uiTools;
    uiTools.setupUi(&_toolsWidget);

    // store label pointer in groundtruth object for convenience (I suppose?)
    _groundTruthWidgets.labelNumFalsePositives = uiTools.labelNumFalsePositives;
    _groundTruthWidgets.labelNumFalseNegatives = uiTools.labelNumFalseNegatives;
    _groundTruthWidgets.labelNumTruePositives  = uiTools.labelNumTruePositives;
    _groundTruthWidgets.labelNumRecall         = uiTools.labelNumRecall;
    _groundTruthWidgets.labelNumPrecision      = uiTools.labelNumPrecision;

    _groundTruthWidgets.labelFalsePositives    = uiTools.labelFalsePositives;
    _groundTruthWidgets.labelFalseNegatives    = uiTools.labelFalseNegatives;
    _groundTruthWidgets.labelTruePositives     = uiTools.labelTruePositives;
    _groundTruthWidgets.labelRecall            = uiTools.labelRecall;
    _groundTruthWidgets.labelPrecision         = uiTools.labelPrecision;

    // lambda function that implements generic "new style connect"
    auto connectRadioButton = [&](QRadioButton* button, BeesBookCommon::Stage stage) {
        QObject::connect(button, &QRadioButton::toggled, [ = ](bool checked) {
            stageSelectionToogled(stage, checked);
        });
    };

    // connect radio buttons with their respective (lambda) handlers
    connectRadioButton(uiTools.radioButtonNoProcessing,
                       BeesBookCommon::Stage::NoProcessing);
    connectRadioButton(uiTools.radioButtonPreprocessor,
                       BeesBookCommon::Stage::Preprocessor);
    connectRadioButton(uiTools.radioButtonLocalizer,
                       BeesBookCommon::Stage::Localizer);
    connectRadioButton(uiTools.radioButtonEllipseFitter,
                       BeesBookCommon::Stage::EllipseFitter);
    connectRadioButton(uiTools.radioButtonGridFitter,
                       BeesBookCommon::Stage::GridFitter);
    connectRadioButton(uiTools.radioButtonDecoder,
                       BeesBookCommon::Stage::Decoder);

    // send forceTracking upon button "process"
    QObject::connect(uiTools.processButton, &QPushButton::pressed,
    [&]() {
        Q_EMIT forceTracking();
    });

    QObject::connect(uiTools.pushButtonLoadGroundTruth, &QPushButton::pressed,
                     this, &BeesBookImgAnalysisTracker::loadGroundTruthData);

    QObject::connect(uiTools.save_config, &QPushButton::pressed, this,
                     &BeesBookImgAnalysisTracker::exportConfiguration);

    QObject::connect(uiTools.pushButtonLoadTaglist, &QPushButton::pressed, this,
                     &BeesBookImgAnalysisTracker::loadTaglist);

    QObject::connect(uiTools.pushButtonLoadConfig, &QPushButton::pressed,
                     this, &BeesBookImgAnalysisTracker::loadConfig);

    // load settings from config file
    _preprocessor.loadSettings(BeesBookCommon::getPreprocessorSettings(m_settings));
    _localizer.loadSettings(BeesBookCommon::getLocalizerSettings(m_settings));
    _ellipsefitter.loadSettings(BeesBookCommon::getEllipseFitterSettings(m_settings));
    _gridFitter.loadSettings(BeesBookCommon::getGridfitterSettings(m_settings));

    _biotrackerWidgetLayout.addWidget(&_paramsWidget);
    _biotrackerWidgetLayout.addWidget(&_toolsWidget);

    getToolsWidget()->setLayout(&_biotrackerWidgetLayout);
}

void BeesBookImgAnalysisTracker::track(ulong /*frameNumber*/, const cv::Mat &frame) {
    cv::Mat frameGray;
    cv::cvtColor(frame, frameGray, CV_BGR2GRAY);

    // notify lambda function
    const auto notify =
    [&](std::string const& message) {
        Q_EMIT notifyGUI(message, BC::Messages::MessageType::NOTIFICATION);
    };

    // acquire mutex (released when leaving function scope)
    const std::lock_guard<std::mutex> lock(_tagListLock);

    // set wait cursor until end of track function
    const CursorOverrideRAII cursorOverride(Qt::WaitCursor);

    // invalidate previous visualizations (aka views)
    for (boost::optional<cv::Mat> &visualization : _visualizationData.visualizations) {
        visualization.reset();
    }

    // taglist holds the tags found by the pipeline
    _taglist.clear();

    // clear ground truth evaluation results
    if (_groundTruthEvaluation) {
        _groundTruthEvaluation.get().reset();

        for (QLabel *label : {
                    _groundTruthWidgets.labelNumFalsePositives,
                    _groundTruthWidgets.labelNumTruePositives,
                    _groundTruthWidgets.labelNumFalseNegatives,
                    _groundTruthWidgets.labelNumPrecision,
                    _groundTruthWidgets.labelNumRecall
                }) {
            label->setText("");
        }
    }

    // algorithm layer selection cascade
    if (_selectedStage < BeesBookCommon::Stage::Preprocessor) {
        return;
    }

    // keep code in extra block for measuring execution time in RAII-fashion
    pipeline::PreprocessorResult result;
    {
        // start the clock
        MeasureTimeRAII measure("Preprocessor", notify);

        // process current frame and store result frame in _image
        // as of now this is a sobel filtered image further processed
        result = _preprocessor.process(frameGray);
        _image = result.originalImage;

        // set preprocessor views
        _visualizationData.preprocessorImage          = result.preprocessedImage;
        _visualizationData.preprocessorClahe          = result.claheImage;
    }

    // end of preprocessor stage
    if (_selectedStage < BeesBookCommon::Stage::Localizer) {
        return;
    }

    {
        // start the clock
        MeasureTimeRAII measure("Localizer", notify);

        // process image, find ROIs with tags
        _taglist = _localizer.process(std::move(result));

        Q_EMIT notifyGUI(std::to_string(_taglist.size()));

        // set localizer views
        _visualizationData.localizerInputImage     =  _image.clone();
        _visualizationData.localizerBlobImage      = _localizer.getBlob().clone();
        _visualizationData.localizerThresholdImage = _localizer.getThresholdImage().clone();

        // evaluate localizer
        if (_groundTruthEvaluation) {
            _groundTruthEvaluation->evaluateLocalizer(getCurrentFrameNumber(), _taglist);
        }
    }

    // end of localizer stage
    if (_selectedStage < BeesBookCommon::Stage::EllipseFitter) {
        return;
    }

    {
        // start the clock
        MeasureTimeRAII measure("EllipseFitter", notify, _taglist.size());

        // find ellipses in taglist
        _taglist = _ellipsefitter.process(std::move(_taglist));

        // set ellipsefitter views
        // TODO: maybe only visualize areas with ROIs
        _visualizationData.ellipsefitterCannyEdge = _ellipsefitter.computeCannyEdgeMap(frameGray);

        // evaluate ellipsefitter
        if (_groundTruthEvaluation) {
            _groundTruthEvaluation->evaluateEllipseFitter(_taglist);
        }
    }

    // end of ellipsefitter stage
    if (_selectedStage < BeesBookCommon::Stage::GridFitter) {
        return;
    }

    {
        // start the clock
        MeasureTimeRAII measure("GridFitter", notify, _taglist.size());

        // fit grids to the ellipses found
        _taglist = _gridFitter.process(std::move(_taglist));

        // evaluate grids
        if (_groundTruthEvaluation) {
            _groundTruthEvaluation->evaluateGridFitter();
        }
    }

    // end of gridfitter stage
    if (_selectedStage < BeesBookCommon::Stage::Decoder) {
        return;
    }

    {
        // start the clock
        MeasureTimeRAII measure("Decoder", notify, _taglist.size());

        // decode grids that were matched to the image
        _taglist = _decoder.process(std::move(_taglist));

        // evaluate decodings
        if (_groundTruthEvaluation) {
            _groundTruthEvaluation->evaluateDecoder();
        }
    }
}

void BeesBookImgAnalysisTracker::visualizeLocalizerOutputOverlay(QPainter *painter) const {
    QPen pen = getDefaultPen(painter);

    // if there is no ground truth, draw all
    // pipeline ROIs in blue and return
    if (!_groundTruthEvaluation) {
        for (const pipeline::Tag &tag : _taglist) {
            pen.setColor(QCOLOR_LIGHT_BLUE);
            drawBox(tag.getRoi(), painter, pen);
        }
        return;
    }

    // ... GT available, that means localizer has been evaluated (localizerResults holds data)
    // get localizer results struct
    const GroundTruth::LocalizerEvaluationResults &results = _groundTruthEvaluation->getLocalizerResults();

    // correctly found
    for (const pipeline::Tag &tag : results.truePositives) {
        pen.setColor(QCOLOR_GREEN);
        drawBox(tag.getRoi(), painter, pen);
    }

    // false detections
    for (const pipeline::Tag &tag : results.falsePositives) {
        pen.setColor(QCOLOR_RED);
        drawBox(tag.getRoi(), painter, pen);
    }

    // missing detections
    for (const std::shared_ptr<PipelineGrid> &grid : results.falseNegatives) {
        pen.setColor(QCOLOR_ORANGE);
        drawBox(grid->getBoundingBox(), painter, pen);
    }

    const size_t numGroundTruth    = results.taggedGridsOnFrame.size();
    const size_t numTruePositives  = results.truePositives.size();
    const size_t numFalsePositives = results.falsePositives.size();
    const size_t numFalseNegatives = results.falseNegatives.size();

    _groundTruthWidgets.setResults(numGroundTruth, numTruePositives, numFalsePositives, numFalseNegatives);
}

void BeesBookImgAnalysisTracker::visualizeEllipseFitterOutput(cv::Mat &image) const {
    if (_groundTruthEvaluation) {
        const GroundTruth::EllipseFitterEvaluationResults &results = _groundTruthEvaluation->getEllipsefitterResults();

        for (const std::shared_ptr<PipelineGrid> &grid : results.falseNegatives) {
            grid->drawContours(image, 1.0);
        }
    }
}

void BeesBookImgAnalysisTracker::visualizeEllipseFitterOutputOverlay(QPainter *painter) const {
    QPen pen = getDefaultPen(painter);

    if (!_groundTruthEvaluation) {
        for (const pipeline::Tag &tag : _taglist) {
            if (!tag.getCandidatesConst().empty()) {
                // get best candidate
                const pipeline::TagCandidate &candidate = tag.getCandidatesConst()[0];
                const pipeline::Ellipse &ellipse = candidate.getEllipse();

                pen.setColor(QCOLOR_LIGHT_BLUE);
                painter->setPen(pen);

                drawEllipse(tag, pen, painter, ellipse);
            }
        }
        return;
    }

    const GroundTruth::EllipseFitterEvaluationResults &results = _groundTruthEvaluation->getEllipsefitterResults();

    for (const auto &tagCandidatePair : results.truePositives) {
        const pipeline::Tag &tag = tagCandidatePair.first;
        const pipeline::TagCandidate &candidate = tagCandidatePair.second;
        const pipeline::Ellipse &ellipse = candidate.getEllipse();

        pen.setColor(QCOLOR_GREEN);
        painter->setPen(pen);

        drawEllipse(tag, pen, painter, ellipse);
    }

    for (const pipeline::Tag &tag : results.falsePositives) {
        //TODO: should use ellipse with best score
        if (tag.getCandidatesConst().size()) {
            const pipeline::Ellipse &ellipse =
                tag.getCandidatesConst().at(0).getEllipse();

            pen.setColor(QCOLOR_RED);
            painter->setPen(pen);

            drawEllipse(tag, pen, painter, ellipse);
        }
    }

    for (const std::shared_ptr<PipelineGrid> &grid : results.falseNegatives) {
        pen.setColor(QCOLOR_ORANGE);

        drawBox(grid->getBoundingBox(), painter, pen);
    }

    const size_t numGroundTruth    = results.taggedGridsOnFrame.size();
    const size_t numTruePositives  = results.truePositives.size();
    const size_t numFalsePositives = results.falsePositives.size();
    const size_t numFalseNegatives = results.falseNegatives.size();

    _groundTruthWidgets.setResults(numGroundTruth, numTruePositives, numFalsePositives, numFalseNegatives);
}

void BeesBookImgAnalysisTracker::visualizeGridFitterOutput(cv::Mat &image) const {
    if (!_groundTruthEvaluation) {
        for (const pipeline::Tag &tag : _taglist) {
            if (!tag.getCandidatesConst().empty()) {

                if (! tag.getCandidatesConst().empty()) {
                    // get best candidate
                    const pipeline::TagCandidate &candidate = tag.getCandidatesConst()[0];

                    if (! candidate.getGridsConst().empty()) {
                        const PipelineGrid &grid = candidate.getGridsConst()[0];
                        grid.drawContours(image, 0.5);
                    }
                }
            }
        }
        return;
    }

    const GroundTruth::GridFitterEvaluationResults &results = _groundTruthEvaluation->getGridfitterResults();

    for (const PipelineGrid &pipegrid : results.truePositives) {
        pipegrid.drawContours(image, 0.5);
    }

    for (const PipelineGrid &pipegrid : results.falsePositives) {
        pipegrid.drawContours(image, 0.5);
    }

    for (const GroundTruthGridSPtr &grid : results.falseNegatives) {
        grid->drawContours(image, 0.5);
    }
}

QPen BeesBookImgAnalysisTracker::getDefaultPen(QPainter *painter) {
    QFont font(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    font.setPointSize(10);
    painter->setFont(font);

    QPen pen = painter->pen();
    pen.setCosmetic(true);

    return pen;
}

void BeesBookImgAnalysisTracker::visualizeGridFitterOutputOverlay(QPainter *painter) const {
    QPen pen = getDefaultPen(painter);

    if (!_groundTruthEvaluation) {
        for (const pipeline::Tag &tag : _taglist) {
            if (!tag.getCandidatesConst().empty()) {

                if (! tag.getCandidatesConst().empty()) {
                    // get best candidate
                    const pipeline::TagCandidate &candidate = tag.getCandidatesConst()[0];

                    if (! candidate.getGridsConst().empty()) {
                        const PipelineGrid &grid = candidate.getGridsConst()[0];
                        pen.setColor(QCOLOR_LIGHT_BLUE);

                        drawBox(grid.getBoundingBox(), painter, pen);
                    }
                }
            }
        }
        return;
    }

    const GroundTruth::GridFitterEvaluationResults &results = _groundTruthEvaluation->getGridfitterResults();

    for (const PipelineGrid &pipegrid : results.truePositives) {
        pen.setColor(QCOLOR_GREEN);

        drawBox((pipegrid.getBoundingBox() + cv::Size(20, 20)) - cv::Point(10, 10), painter, pen);
    }

    for (const PipelineGrid &pipegrid : results.falsePositives) {
        pen.setColor(QCOLOR_RED);

        drawBox((pipegrid.getBoundingBox() + cv::Size(20, 20)) - cv::Point(10, 10), painter, pen);
    }

    for (const GroundTruthGridSPtr &grid : results.falseNegatives) {
        pen.setColor(QCOLOR_ORANGE);

        drawBox((grid->getBoundingBox() + cv::Size(20, 20)) - cv::Point(10, 10), painter, pen);
    }

    const size_t numGroundTruth    = _groundTruthEvaluation->getEllipsefitterResults().taggedGridsOnFrame.size();
    const size_t numTruePositives  = results.truePositives.size();
    const size_t numFalsePositives = results.falsePositives.size();
    const size_t numFalseNegatives = results.falseNegatives.size();

    _groundTruthWidgets.setResults(numGroundTruth, numTruePositives, numFalsePositives, numFalseNegatives);
}

void BeesBookImgAnalysisTracker::visualizeDecoderOutput(cv::Mat &image) const {
    if (!_groundTruthEvaluation) {
        for (const pipeline::Tag &tag : _taglist) {
            if (!tag.getCandidatesConst().empty()) {
                const pipeline::TagCandidate &candidate = tag.getCandidatesConst()[0];
                if (candidate.getDecodings().size()) {
                    assert(candidate.getGridsConst().size());
                    const PipelineGrid &grid = candidate.getGridsConst()[0];
                    grid.drawContours(image, 0.5);
                }
            }
        }
        return;
    }

    const GroundTruth::EllipseFitterEvaluationResults &ellipseFitterResults =
        _groundTruthEvaluation->getEllipsefitterResults();
    const GroundTruth::DecoderEvaluationResults &results =
        _groundTruthEvaluation->getDecoderResults();

    for (const GroundTruth::DecoderEvaluationResults::result_t &result : results.evaluationResults) {
        result.pipelineGrid.get().drawContours(image, 0.5);
    }

    for (const GroundTruthGridSPtr &grid : ellipseFitterResults.falseNegatives) {
        grid->drawContours(image, 0.5);
    }
}

void BeesBookImgAnalysisTracker::visualizeDecoderOutputOverlay(QPainter *painter) const {
    static const int distance      = 10;

    QPen pen = getDefaultPen(painter);

    if (!_groundTruthEvaluation) {
        for (const pipeline::Tag &tag : _taglist) {
            if (!tag.getCandidatesConst().empty()) {
                const pipeline::TagCandidate &candidate = tag.getCandidatesConst()[0];
                if (candidate.getDecodings().size()) {
                    assert(candidate.getGridsConst().size());

                    const PipelineGrid &grid = candidate.getGridsConst()[0];
                    pipeline::decoding_t decoding = candidate.getDecodings()[0];

#ifdef SHIFT_DECODED_BITS
                    Util::rotateBitset(decoding, 3);
#endif

                    const QString idString = QString::fromStdString(decoding.to_string());

                    pen.setColor(QCOLOR_LIGHT_BLUE);

                    drawBox((grid.getBoundingBox() + cv::Size(20, 20)) - cv::Point(10, 10), painter, pen);

                    painter->setPen(pen);
                    painter->drawText(QPoint(tag.getRoi().x, tag.getRoi().y - distance -5),
                                      QString::number(decoding.to_ulong()));
                    painter->drawText(QPoint(tag.getRoi().x, tag.getRoi().y-5),
                                      idString);
                }
            }
        }
        return;
    }

    const GroundTruth::EllipseFitterEvaluationResults &ellipseFitterResults =
        _groundTruthEvaluation->getEllipsefitterResults();
    const GroundTruth::DecoderEvaluationResults &results =
        _groundTruthEvaluation->getDecoderResults();

    int matchNum           = 0;
    int partialMismatchNum = 0;
    int mismatchNum        = 0;
    int cumulHamming       = 0;

    for (const GroundTruth::DecoderEvaluationResults::result_t &result : results.evaluationResults) {
        // calculate stats
        QColor color;
        static const int threshold = 3;
        if (result.hammingDistance == 0) { // match
            ++matchNum;
            color = QCOLOR_GREEN;
        } else if (result.hammingDistance <= threshold) { // partial match
            ++partialMismatchNum;
            color = QCOLOR_GREENISH;
        } else { // mismatch
            ++mismatchNum;
            color = QCOLOR_RED;
        }
        cumulHamming = cumulHamming + result.hammingDistance;

        pen.setColor(color);
        drawBox(result.boundingBox, painter, pen);

        const int xpos = result.boundingBox.tl().x;
        const int ypos = result.boundingBox.tl().y;
        // paint text on image

        painter->setPen(pen);
        painter->drawText(QPoint(xpos, ypos - (distance * 2)-5),
                          QString::number(result.decodedTagId)+", d=" + QString::number(result.hammingDistance));
        painter->drawText(QPoint(xpos, ypos - distance -5),
                          "rs: " + QString::fromStdString(result.decodedTagIdStr));
        painter->drawText(QPoint(xpos, ypos-5),
                          "gt: " + QString::fromStdString(result.groundTruthTagIdStr));
    }

    for (const GroundTruthGridSPtr &grid : ellipseFitterResults.falseNegatives) {
        pen.setColor(QCOLOR_ORANGE);
        drawBox(grid->getBoundingBox(), painter, pen);
    }

    const size_t numResults = results.evaluationResults.size();

    const double avgHamming = numResults ? static_cast<double>(cumulHamming) / numResults : 0.;
    const double precMatch  = numResults ? (static_cast<double>(matchNum) / static_cast<double>(numResults)) * 100.: 0.;
    const double precPartly = numResults ? (static_cast<double>(matchNum + partialMismatchNum) / static_cast<double>
                                            (numResults)) * 100. : 0.;

    // statistics
    _groundTruthWidgets.labelFalsePositives->setText("Match: ");
    _groundTruthWidgets.labelNumFalsePositives->setText(QString::number(matchNum));
    _groundTruthWidgets.labelTruePositives->setText("Partial mismatch: ");
    _groundTruthWidgets.labelNumTruePositives->setText(QString::number(partialMismatchNum));
    _groundTruthWidgets.labelFalseNegatives->setText("Mismatch: ");
    _groundTruthWidgets.labelNumFalseNegatives->setText(QString::number(mismatchNum));
    _groundTruthWidgets.labelRecall->setText("Average hamming distance: ");
    _groundTruthWidgets.labelNumRecall->setText(QString::number(avgHamming));
    _groundTruthWidgets.labelPrecision->setText("Precision (matched, partial): ");
    _groundTruthWidgets.labelNumPrecision->setText(QString::number(precMatch, 'f', 2) + "%, " +
            QString::number(precPartly, 'f', 2) + "%");
}

void BeesBookImgAnalysisTracker::paint(size_t, BC::ProxyMat &image, View const &view) {
    cv::ellipse(image.getMat(), cv::RotatedRect(cv::Point2f(100, 100), cv::Size2f(50, 50), 0), cv::Scalar(255, 0, 0));

    if (_tagListLock.try_lock()) {
        // restore original ground truth labels after they have possibly been
        // modified by the decoder evaluation
        if (_selectedStage != BeesBookCommon::Stage::Decoder) {
            _groundTruthWidgets.labelFalsePositives->setText("False positives: ");
            _groundTruthWidgets.labelTruePositives->setText("True positives: ");
            _groundTruthWidgets.labelFalseNegatives->setText("False negatives: ");
            _groundTruthWidgets.labelRecall->setText("Recall: ");
            _groundTruthWidgets.labelPrecision->setText("Precision: ");
        }

        switch (_selectedStage) {
        case BeesBookCommon::Stage::Preprocessor:
            if ((view.name == "Preprocessor Output")
                    && (_visualizationData.preprocessorImage)) {
                image.setMat(rgbMatFromBwMat(
                                 _visualizationData.preprocessorImage.get(),
                                 image.getMat().type()));
            } else if ((view.name == "Clahe")
                       && (_visualizationData.preprocessorClahe)) {
                image.setMat(rgbMatFromBwMat(
                                 _visualizationData.preprocessorClahe.get(),
                                 image.getMat().type()));
            }
            break;
        case BeesBookCommon::Stage::Localizer:
            if ((view.name == "Blobs")
                    && (_visualizationData.localizerBlobImage)) {
                image.setMat(rgbMatFromBwMat(
                                 _visualizationData.localizerBlobImage.get(),
                                 image.getMat().type()));
            } else if ((view.name == "Input")
                       && (_visualizationData.localizerInputImage)) {
                image.setMat(rgbMatFromBwMat(
                                 _visualizationData.localizerInputImage.get(),
                                 image.getMat().type()));

            } else if ((view.name == "Threshold")
                       && (_visualizationData.localizerThresholdImage)) {
                image.setMat(rgbMatFromBwMat(
                                 _visualizationData.localizerThresholdImage.get(),
                                 image.getMat().type()));
            }
            break;
        case BeesBookCommon::Stage::EllipseFitter:
            if ((view.name == "Canny Edge")
                    && (_visualizationData.ellipsefitterCannyEdge)) {
                image.setMat(rgbMatFromBwMat(
                                 _visualizationData.ellipsefitterCannyEdge.get(),
                                 image.getMat().type()));
            }
            visualizeEllipseFitterOutput(image.getMat());
            break;
        case BeesBookCommon::Stage::GridFitter:
            visualizeGridFitterOutput(image.getMat());
            break;
        case BeesBookCommon::Stage::Decoder:
            visualizeDecoderOutput(image.getMat());
            break;
        default:
            break;
        }
        _tagListLock.unlock();
    } else {
        return;
    }
}

void BeesBookImgAnalysisTracker::paintOverlay(size_t, QPainter *painter, View const &) {
    painter->setPen(QColor(255, 0, 0));
    painter->drawEllipse(QRectF(QPointF(100.f, 100.f), QSize(100, 100)));
    if (_tagListLock.try_lock()) {

        switch (_selectedStage) {
        case BeesBookCommon::Stage::Preprocessor:
            break;
        case BeesBookCommon::Stage::Localizer:
            visualizeLocalizerOutputOverlay(painter);
            break;
        case BeesBookCommon::Stage::EllipseFitter:
            visualizeEllipseFitterOutputOverlay(painter);
            break;
        case BeesBookCommon::Stage::GridFitter:
            visualizeGridFitterOutputOverlay(painter);
            break;
        case BeesBookCommon::Stage::Decoder:
            visualizeDecoderOutputOverlay(painter);
            break;
        default:
            break;
        }
        _tagListLock.unlock();
    } else {
        return;
    }
}

void BeesBookImgAnalysisTracker::settingsChanged(
    const BeesBookCommon::Stage stage) {
    switch (stage) {
    case BeesBookCommon::Stage::Preprocessor:
        _preprocessor.loadSettings(
            BeesBookCommon::getPreprocessorSettings(m_settings));
        break;
    case BeesBookCommon::Stage::Localizer:
        _localizer.loadSettings(
            BeesBookCommon::getLocalizerSettings(m_settings));
        break;
    case BeesBookCommon::Stage::EllipseFitter:
        _ellipsefitter.loadSettings(
            BeesBookCommon::getEllipseFitterSettings(m_settings));
        break;
    case BeesBookCommon::Stage::GridFitter:
        _gridFitter.loadSettings(
            BeesBookCommon::getGridfitterSettings(m_settings));
        break;
    case BeesBookCommon::Stage::Decoder:
        // TODO
        break;
    default:
        break;
    }
}

void BeesBookImgAnalysisTracker::loadGroundTruthData() {
    QString filename = QFileDialog::getOpenFileName(QApplication::activeWindow(),
                       tr("Load tracking data"), "", tr("Data Files (*.tdat)"));

    if (filename.isEmpty()) {
        return;
    }

    std::ifstream is(filename.toStdString());

    cereal::JSONInputArchive ar(is);

    // load serialized data into member .data
    BC::Serialization::Data data;
    ar(data);

    _groundTruthEvaluation.emplace(std::move(data));

    const std::array<QLabel *, 10> labels { _groundTruthWidgets.labelFalsePositives,
              _groundTruthWidgets.labelFalseNegatives, _groundTruthWidgets.labelTruePositives,
              _groundTruthWidgets.labelRecall, _groundTruthWidgets.labelPrecision,
              _groundTruthWidgets.labelNumFalsePositives,
              _groundTruthWidgets.labelNumFalseNegatives,
              _groundTruthWidgets.labelNumTruePositives, _groundTruthWidgets.labelNumRecall,
              _groundTruthWidgets.labelNumPrecision };

    for (QLabel *label : labels) {
        label->setEnabled(true);
    }

    if (!_taglist.empty()) {
        _groundTruthEvaluation->evaluateLocalizer(getCurrentFrameNumber(), _taglist);
        _groundTruthEvaluation->evaluateEllipseFitter(_taglist);
        _groundTruthEvaluation->evaluateGridFitter();
        _groundTruthEvaluation->evaluateDecoder();
    }
    //TODO: maybe check filehash here
}

void BeesBookImgAnalysisTracker::loadConfig() {
    std::vector<boost::filesystem::path> files;

    const QString filename = QFileDialog::getOpenFileName(QApplication::activeWindow(), "Open config", "", "*.json");

    if (!filename.isEmpty()) {
        setPipelineConfig(filename.toStdString());
    }
}

void BeesBookImgAnalysisTracker::setPipelineConfig(const std::string &filename) {
    // TODO! Add GUI widgets

    pipeline::settings::preprocessor_settings_t preprocessor_settings;
    pipeline::settings::localizer_settings_t localizer_settings;
    pipeline::settings::ellipsefitter_settings_t ellipsefitter_settings;
    pipeline::settings::gridfitter_settings_t gridfitter_settings;

    for (pipeline::settings::settings_abs *settings :
        std::array<pipeline::settings::settings_abs *, 4>( {
        &preprocessor_settings,
        &localizer_settings,
        &ellipsefitter_settings,
        &gridfitter_settings
    })) {
        settings->loadFromJson(filename);
    }

    static const boost::filesystem::path deeplocalizer_model_path(BOOST_PP_STRINGIZE(MODEL_BASE_PATH));

    boost::filesystem::path model_path(localizer_settings.get_deeplocalizer_model_file());
    model_path = deeplocalizer_model_path / model_path.parent_path().leaf() / model_path.filename();

    boost::filesystem::path param_path(localizer_settings.get_deeplocalizer_param_file());
    param_path = deeplocalizer_model_path / model_path.parent_path().leaf() / param_path.filename();

    localizer_settings.setValue(pipeline::settings::Localizer::Params::DEEPLOCALIZER_MODEL_FILE,
                                model_path.string());
    localizer_settings.setValue(pipeline::settings::Localizer::Params::DEEPLOCALIZER_PARAM_FILE,
                                param_path.string());

    try {
        _preprocessor.loadSettings(preprocessor_settings);
        _localizer.loadSettings(localizer_settings);
        _ellipsefitter.loadSettings(ellipsefitter_settings);
        _gridFitter.loadSettings(gridfitter_settings);
    } catch (std::runtime_error err) {
        Q_EMIT notifyGUI(std::string("Unable to load settings: ") + err.what(), BC::Messages::MessageType::FAIL);
        return;
    }

    namespace S = pipeline::settings::Preprocessor;
    #define addToBioTracker(param) { \
    {                                \
        const auto paramBioTracker = S::Params::BASE + S::Params::param; \
        const auto paramPipeline = S::Params::param; \
        m_settings.setParam(paramBioTracker, \
                            preprocessor_settings.getValue<decltype(S::Defaults::param)>(paramPipeline)); \
    } \
    }

    addToBioTracker(COMB_DIFF_SIZE)
    addToBioTracker(OPT_USE_CONTRAST_STRETCHING)
    addToBioTracker(OPT_USE_EQUALIZE_HISTOGRAM)
    addToBioTracker(OPT_FRAME_SIZE)
    addToBioTracker(OPT_AVERAGE_CONTRAST_VALUE)
            /*
static const unsigned int OPT_FRAME_SIZE = 200;
static const double OPT_AVERAGE_CONTRAST_VALUE = 120;

static const bool COMB_ENABLED = true;
static const unsigned int COMB_MIN_SIZE = 65;
static const unsigned int COMB_MAX_SIZE = 80;
static const double COMB_THRESHOLD = 27;
static const unsigned int COMB_DIFF_SIZE = 15;
static const unsigned int COMB_LINE_WIDTH = 9;
static const unsigned int COMB_LINE_COLOR = 0;

static const bool HONEY_ENABLED = true;
static const double HONEY_STD_DEV = 0;
static const unsigned int HONEY_FRAME_SIZE = 30;
static const double HONEY_AVERAGE_VALUE = 80;
        */
    stageSelectionToogled(_selectedStage, true);
}


void BeesBookImgAnalysisTracker::exportConfiguration() {
    QString dir = QFileDialog::getExistingDirectory(0, tr("select directory"),
                  "", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (dir.isEmpty()) {
        Q_EMIT notifyGUI("config export: no directory given", BC::Messages::MessageType::FAIL);
        return;
    }
    bool ok;
    //std::string image_name = _settings.getValueOfParam<std::string>(PICTUREPARAM::PICTURE_FILES);
    QString filename = QInputDialog::getText(0,
                       tr("choose filename (without extension)"), tr("Filename:"),
                       QLineEdit::Normal, "Filename", &ok);
    if (!ok || filename.isEmpty()) {
        Q_EMIT notifyGUI("config export: no filename given", BC::Messages::MessageType::FAIL);
        return;
    }

    boost::property_tree::ptree pt = BeesBookCommon::getPreprocessorSettings(
                                         m_settings).getPTree();
    BeesBookCommon::getLocalizerSettings(m_settings).addToPTree(pt);
    BeesBookCommon::getEllipseFitterSettings(m_settings).addToPTree(pt);
    BeesBookCommon::getGridfitterSettings(m_settings).addToPTree(pt);

    try {
        boost::property_tree::write_json(
            dir.toStdString() + "/" + filename.toStdString() + ".json", pt);
        Q_EMIT notifyGUI("config export successfully", BC::Messages::MessageType::NOTIFICATION);
    } catch (std::exception &e) {
        Q_EMIT notifyGUI("config export failed (" + std::string(e.what()) + ")", BC::Messages::MessageType::FAIL);
    }

    return;
}

void BeesBookImgAnalysisTracker::loadTaglist() {
    QString path = QFileDialog::getOpenFileName(QApplication::activeWindow(),
                   tr("Load taglist data"), "",
                   tr("Data Files (*.dat)"));

    if (path.isEmpty()) {
        return;
    }

    try {
        _taglist = loadSerializedTaglist(path.toStdString());

        if (_groundTruthEvaluation) {
            _groundTruthEvaluation->evaluateLocalizer(getCurrentFrameNumber(), _taglist);
            _groundTruthEvaluation->evaluateEllipseFitter(_taglist);
            _groundTruthEvaluation->evaluateGridFitter();
            _groundTruthEvaluation->evaluateDecoder();
        }

        Q_EMIT update();
    } catch (std::exception const &e) {
        std::stringstream msg;
        msg << "Unable to load tracking data." << std::endl << std::endl;
        msg << "Error: " << e.what() << std::endl;
        QMessageBox::warning(QApplication::activeWindow(), "Unable to load tracking data",
                             QString::fromStdString(e.what()));
    }

    notifyGUI("import of serialized taglist data finished");
}

void BeesBookImgAnalysisTracker::stageSelectionToogled(BeesBookCommon::Stage stage, bool checked) {
    if (checked) {
        _selectedStage = stage;

        resetViews();
        switch (stage) {
        case BeesBookCommon::Stage::Preprocessor:
            setParamsWidget<PreprocessorParamsWidget>();
            Q_EMIT registerViews({ { "Preprocessor Output" }, { "Clahe" } });
            break;
        case BeesBookCommon::Stage::Localizer:
            setParamsWidget<LocalizerParamsWidget>();
            Q_EMIT registerViews({ { "Input" }, { "Threshold" }, { "Blobs" } });
            break;
        case BeesBookCommon::Stage::EllipseFitter:
            setParamsWidget<EllipseFitterParamsWidget>();
            Q_EMIT registerViews({ { "Canny Edge" } });
            break;
        case BeesBookCommon::Stage::GridFitter:
            setParamsWidget<GridFitterParamsWidget>();
            break;
        case BeesBookCommon::Stage::Decoder:
            setParamsWidget<DecoderParamsWidget>();
            break;
        default:
            _paramsWidget.setParamSubWidget(std::make_unique<QWidget>());
            break;
        }

        Q_EMIT update();
    }
}

void BeesBookImgAnalysisTracker::resetViews() {
    Q_EMIT registerViews({});
}

void GroundTruthWidgets::setResults(const size_t numGroundTruth, const size_t numTruePositives,
                                    const size_t numFalsePositives, const size_t numFalseNegatives) const {
    const double recall    = numGroundTruth ?
                             (static_cast<double>(numTruePositives) / static_cast<double>(numGroundTruth)) * 100. : 0.;
    const double precision = (numTruePositives + numFalsePositives) ?
                             (static_cast<double>(numTruePositives) / static_cast<double>(numTruePositives + numFalsePositives)) * 100. : 0.;

    labelNumFalseNegatives->setText(QString::number(numFalseNegatives));
    labelNumFalsePositives->setText(QString::number(numFalsePositives));
    labelNumTruePositives->setText(QString::number(numTruePositives));
    labelNumRecall->setText(QString::number(recall, 'f', 2) + "%");
    labelNumPrecision->setText(QString::number(precision, 'f', 2) + "%");
}
