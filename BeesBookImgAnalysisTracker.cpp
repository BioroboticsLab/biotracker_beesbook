#include "BeesBookImgAnalysisTracker.h"

#include <array>
#include <chrono>
#include <functional>
#include <set>
#include <vector>

#include <QFileDialog>
#include <QInputDialog>

#include <opencv2/core/core.hpp>

#include <cereal/types/polymorphic.hpp>
#include <cereal/archives/json.hpp>
#include <cereal/types/vector.hpp>

#include "Common.h"
#include "DecoderParamsWidget.h"
#include "GridFitterParamsWidget.h"
#include "LocalizerParamsWidget.h"
#include "RecognizerParamsWidget.h"
#include "PreprocessorParamsWidget.h"
#include "pipeline/datastructure/Tag.h"
#include "pipeline/datastructure/TagCandidate.h"
#include "pipeline/datastructure/PipelineGrid.h"
#include "source/tracking/algorithm/algorithms.h"

#include "Grid3D.h"

#include "ui_ToolWidget.h"

namespace {
auto _ = Algorithms::Registry::getInstance().register_tracker_type<
		BeesBookImgAnalysisTracker>("BeesBook ImgAnalysis");

struct CursorOverrideRAII {
	CursorOverrideRAII(Qt::CursorShape shape) {
		QApplication::setOverrideCursor(shape);
	}
	~CursorOverrideRAII() {
		QApplication::restoreOverrideCursor();
	}
};

static const cv::Scalar COLOR_ORANGE = cv::Scalar(0, 102, 255);
static const cv::Scalar COLOR_GREEN = cv::Scalar(0, 255, 0);
static const cv::Scalar COLOR_RED = cv::Scalar(0, 0, 255);
static const cv::Scalar COLOR_BLUE = cv::Scalar(255, 0, 0);

class MeasureTimeRAII {
public:
	MeasureTimeRAII(std::string const& what,
			std::function<void(std::string const&)> notify) :
			_start(std::chrono::steady_clock::now()), _what(what), _notify(
					notify) {
	}
	~MeasureTimeRAII() {
		const auto end = std::chrono::steady_clock::now();
		const std::string message { _what + " finished in "
				+ std::to_string(
						std::chrono::duration_cast<std::chrono::milliseconds>(
								end - _start).count()) + +"ms.\n" };
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

BeesBookImgAnalysisTracker::BeesBookImgAnalysisTracker(Settings& settings, QWidget* parent)
    : TrackingAlgorithm(settings, parent)
    , _selectedStage(BeesBookCommon::Stage::NoProcessing)
    , _paramsWidget(std::make_shared<ParamsWidget>())
    , _toolsWidget(std::make_shared<QWidget>())
{
	Ui::ToolWidget uiTools;
	uiTools.setupUi(_toolsWidget.get());

	_groundTruth.labelNumFalsePositives = uiTools.labelNumFalsePositives;
	_groundTruth.labelNumFalseNegatives = uiTools.labelNumFalseNegatives;
	_groundTruth.labelNumTruePositives = uiTools.labelNumTruePositives;
	_groundTruth.labelNumRecall = uiTools.labelNumRecall;
	_groundTruth.labelNumPrecision = uiTools.labelNumPrecision;

	_groundTruth.labelFalsePositives = uiTools.labelFalsePositives;
	_groundTruth.labelFalseNegatives = uiTools.labelFalseNegatives;
	_groundTruth.labelTruePositives = uiTools.labelTruePositives;
	_groundTruth.labelRecall = uiTools.labelRecall;
	_groundTruth.labelPrecision = uiTools.labelPrecision;

	auto connectRadioButton =
			[&](QRadioButton* button, BeesBookCommon::Stage stage) {
				QObject::connect(button, &QRadioButton::toggled,
						[ = ](bool checked) {stageSelectionToogled(stage, checked);});
			};
	connectRadioButton(uiTools.radioButtonNoProcessing,
			BeesBookCommon::Stage::NoProcessing);
	connectRadioButton(uiTools.radioButtonPreprocessor,
			BeesBookCommon::Stage::Preprocessor);
	connectRadioButton(uiTools.radioButtonLocalizer,
			BeesBookCommon::Stage::Localizer);
	connectRadioButton(uiTools.radioButtonRecognizer,
			BeesBookCommon::Stage::Recognizer);
	connectRadioButton(uiTools.radioButtonGridFitter,
			BeesBookCommon::Stage::GridFitter);
	connectRadioButton(uiTools.radioButtonDecoder,
			BeesBookCommon::Stage::Decoder);

	QObject::connect(uiTools.processButton, &QPushButton::pressed,
			[&]() {emit forceTracking();});

	QObject::connect(uiTools.pushButtonLoadGroundTruth, &QPushButton::pressed,
			this, &BeesBookImgAnalysisTracker::loadGroundTruthData);
	QObject::connect(uiTools.save_config, &QPushButton::pressed,
				this, &BeesBookImgAnalysisTracker::exportConfiguration);

	_preprocessor.loadSettings(
			BeesBookCommon::getPreprocessorSettings(_settings));
	_localizer.loadSettings(BeesBookCommon::getLocalizerSettings(_settings));
	_recognizer.loadSettings(BeesBookCommon::getRecognizerSettings(_settings));
	//TODO
}

void BeesBookImgAnalysisTracker::track(ulong /*frameNumber*/, cv::Mat& frame) {
	const auto notify =
			[&](std::string const& message) {emit notifyGUI(message, MSGS::NOTIFICATION);};

	const std::lock_guard<std::mutex> lock(_tagListLock);
	const CursorOverrideRAII cursorOverride(Qt::WaitCursor);

	// invalidate previous visualizations
	for (boost::optional<cv::Mat>& visualization : _visualizationData.visualizations) {
		visualization.reset();
	}

	_taglist.clear();

	if (_selectedStage < BeesBookCommon::Stage::Preprocessor)
		return;
	cv::Mat image;
	{
		MeasureTimeRAII measure("Preprocessor", notify);
		_image = _preprocessor.process(frame);
		_visualizationData.preprocessorImage = _image.clone();
		_visualizationData.preprocessorOptImage =
						_preprocessor.getOptsImage();
		_visualizationData.preprocessorHoneyImage =
						_preprocessor.getHoneyImage();
		_visualizationData.preprocessorThresholdImage =
				_preprocessor.getThresholdImage();

	}
	if (_selectedStage < BeesBookCommon::Stage::Localizer)
		return;
	{
		MeasureTimeRAII measure("Localizer", notify);

		_visualizationData.localizerInputImage = _image.clone();
		_taglist = _localizer.process(std::move(frame), std::move(_image));
		_visualizationData.localizerBlobImage = _localizer.getBlob().clone();
		_visualizationData.localizerThresholdImage =
				_localizer.getThresholdImage().clone();
		if (_groundTruth.available)
			evaluateLocalizer();
	}
	if (_selectedStage < BeesBookCommon::Stage::Recognizer)
		return;
	{
		MeasureTimeRAII measure("Recognizer", notify);
		_taglist = _recognizer.process(std::move(_taglist));
		// TODO: maybe only visualize areas with ROIs
		_visualizationData.recognizerCannyEdge =
				_recognizer.computeCannyEdgeMap(frame);
		if (_groundTruth.available)
			evaluateRecognizer();
	}
	if (_selectedStage < BeesBookCommon::Stage::GridFitter)
		return;
	{
		MeasureTimeRAII measure("GridFitter", notify);
		_taglist = _gridFitter.process(std::move(_taglist));
		if (_groundTruth.available)
			evaluateGridfitter();
	}
	if (_selectedStage < BeesBookCommon::Stage::Decoder)
		return;
	{
		MeasureTimeRAII measure("Decoder", notify);
		_taglist = _decoder.process(std::move(_taglist));
		if (_groundTruth.available)
			evaluateDecoder();
	}
}

void BeesBookImgAnalysisTracker::visualizeLocalizerOutput(
		cv::Mat& image) const {
	const int thickness = calculateVisualizationThickness();

	if (!_groundTruth.available) {
		for (const pipeline::Tag& tag : _taglist) {
			const cv::Rect& box = tag.getBox();
			cv::rectangle(image, box, COLOR_BLUE, thickness, CV_AA);
		}
		return;
	}

	const LocalizerEvaluationResults& results = _groundTruth.localizerResults;

	for (const pipeline::Tag& tag : results.truePositives) {
		cv::rectangle(image, tag.getBox(), COLOR_GREEN, thickness, CV_AA);
	}

	for (const pipeline::Tag& tag : results.falsePositives) {
		cv::rectangle(image, tag.getBox(), COLOR_RED, thickness, CV_AA);
	}

	for (const std::shared_ptr<PipelineGrid>& grid : results.falseNegatives) {
		cv::rectangle(image, grid->getBoundingBox(), COLOR_ORANGE, thickness, CV_AA);
	}

	const float recall = static_cast<float>(results.truePositives.size())
			/ static_cast<float>(results.taggedGridsOnFrame.size()) * 100.f;
	const float precision = static_cast<float>(results.truePositives.size())
			/ static_cast<float>(results.truePositives.size()
					+ results.falsePositives.size()) * 100.f;

	_groundTruth.labelNumFalseNegatives->setText(
			QString::number(results.falseNegatives.size()));
	_groundTruth.labelNumFalsePositives->setText(
			QString::number(results.falsePositives.size()));
	_groundTruth.labelNumTruePositives->setText(
			QString::number(results.truePositives.size()));
	_groundTruth.labelNumRecall->setText(QString::number(recall, 'f', 2) + "%");
	_groundTruth.labelNumPrecision->setText(
	            QString::number(precision, 'f', 2) + "%");
}

void BeesBookImgAnalysisTracker::visualizeRecognizerOutput(
		cv::Mat& image) const {
	const int thickness = calculateVisualizationThickness();

	if (!_groundTruth.available) {
		for (const pipeline::Tag& tag : _taglist) {
			if (!tag.getCandidates().empty()) {
				// get best candidate
				const pipeline::TagCandidate& candidate = tag.getCandidates()[0];
				const pipeline::Ellipse& ellipse = candidate.getEllipse();
				cv::ellipse(image, tag.getBox().tl() + ellipse.getCen(),
						ellipse.getAxis(), ellipse.getAngle(), 0, 360,
						cv::Scalar(0, 255, 0), 3);
				cv::putText(image,
						"Score: " + std::to_string(ellipse.getVote()),
						tag.getBox().tl() + cv::Point(80, 80),
						cv::FONT_HERSHEY_COMPLEX_SMALL, 3.0,
						cv::Scalar(0, 255, 0), 2, CV_AA);
			}
		}
		return;
	}

	const RecognizerEvaluationResults& results = _groundTruth.recognizerResults;

	for (const auto& tagCandidatePair : results.truePositives) {
		const pipeline::Tag& tag = tagCandidatePair.first;
		const pipeline::TagCandidate& candidate = tagCandidatePair.second;
		const pipeline::Ellipse& ellipse = candidate.getEllipse();
		cv::ellipse(image, tag.getBox().tl() + ellipse.getCen(),
				ellipse.getAxis(), ellipse.getAngle(), 0, 360, COLOR_GREEN,
				thickness);
	}

	for (const pipeline::Tag& tag : results.falsePositives) {
		//TODO: should use ellipse with best score
		if (tag.getCandidates().size()) {
			const pipeline::Ellipse& ellipse =
					tag.getCandidates().at(0).getEllipse();
			cv::ellipse(image, tag.getBox().tl() + ellipse.getCen(),
					ellipse.getAxis(), ellipse.getAngle(), 0, 360, COLOR_RED,
					thickness);
		}
	}

	for (const std::shared_ptr<PipelineGrid>& grid : results.falseNegatives) {
		cv::rectangle(image, grid->getBoundingBox(), COLOR_ORANGE, thickness,
		CV_AA);
		grid->draw(image, 1.0);
	}

	const float recall = static_cast<float>(results.truePositives.size())
			/ static_cast<float>(results.taggedGridsOnFrame.size()) * 100.f;
	const float precision = static_cast<float>(results.truePositives.size())
			/ static_cast<float>(results.truePositives.size()
					+ results.falsePositives.size()) * 100.f;

	_groundTruth.labelNumFalseNegatives->setText(
			QString::number(results.falseNegatives.size()));
	_groundTruth.labelNumFalsePositives->setText(
			QString::number(results.falsePositives.size()));
	_groundTruth.labelNumTruePositives->setText(
			QString::number(results.truePositives.size()));
	_groundTruth.labelNumRecall->setText(QString::number(recall, 'f', 2) + "%");
	_groundTruth.labelNumPrecision->setText(
			QString::number(precision, 'f', 2) + "%");
}

void BeesBookImgAnalysisTracker::visualizeGridFitterOutput(cv::Mat& image) const 
{
	//TODO
	const int thickness = calculateVisualizationThickness();

	if (!_groundTruth.available) 
    {
		for (const pipeline::Tag& tag : _taglist) 
        {
			if (!tag.getCandidates().empty()) 
            {
				// get best candidate
				const pipeline::TagCandidate& candidate = tag.getCandidates()[0];
				const PipelineGrid& grid = candidate.getGridsConst()[0];
				cv::rectangle(image, (grid.getBoundingBox() + cv::Size(20, 20)) - cv::Point(10, 10), COLOR_GREEN, thickness, CV_AA);
				grid.drawContours(image, 0.5);
			}
		}
		return;
	}

    //precision = TP / Positives
    //recall = TP / GroundTruth_N
    unsigned int matches = _groundTruth.gridfitterResults.matches;
    unsigned int mismatches = _groundTruth.gridfitterResults.mismatches;

    _groundTruth.labelNumTruePositives->setText( QString::number(matches) );
    _groundTruth.labelNumRecall->setText(QString::number(100* matches / _groundTruth.recognizerResults.taggedGridsOnFrame.size(), 'f', 2) + "%");
    _groundTruth.labelNumPrecision->setText( QString::number(matches / ( matches + mismatches ), 'f', 2) + "%");

}

void BeesBookImgAnalysisTracker::visualizeDecoderOutput(cv::Mat& image) const {
	//TODO
	if (!_groundTruth.available) {
		for (const pipeline::Tag& tag : _taglist) {
			if (tag.getCandidates().size()) {
				const pipeline::TagCandidate& candidate = tag.getCandidates()[0];
				if (candidate.getDecodings().size()) {
					const pipeline::decoding_t decoding = candidate.getDecodings()[0];
					cv::putText(image, std::to_string(decoding.to_ulong()),
							cv::Point(tag.getBox().x, tag.getBox().y),
							cv::FONT_HERSHEY_COMPLEX_SMALL, 3.0,
							cv::Scalar(0, 255, 0), 2, CV_AA);
				}
			}
		}
		return;
	}
}

void BeesBookImgAnalysisTracker::evaluateLocalizer() {
	assert(_groundTruth.available);
	LocalizerEvaluationResults results;

	{
		const int currentFrameNumber = getCurrentFrameNumber();
		for (TrackedObject const& object : _groundTruth.data.getTrackedObjects()) {
            const std::shared_ptr<Grid3D> grid3d = object.maybeGet<Grid3D>(currentFrameNumber);
            const auto grid = std::make_shared<PipelineGrid>(grid3d->getCenter(), grid3d->getPixelRadius(), grid3d->getZRotation(),
                                                             grid3d->getYRotation(), grid3d->getXRotation());
			if (grid) results.taggedGridsOnFrame.insert(grid);
		}
	}

	// detect false negatives
	for (const std::shared_ptr<PipelineGrid>& grid : results.taggedGridsOnFrame) {
		const cv::Rect gridBox = grid->getBoundingBox();

		bool inGroundTruth = false;
		for (const pipeline::Tag& tag : _taglist) {
			const cv::Rect& tagBox = tag.getBox();
			if (tagBox.contains(gridBox.tl())
					&& tagBox.contains(gridBox.br())) {
				inGroundTruth = true;
				break;
			}
		}

		if (!inGroundTruth)
			results.falseNegatives.insert(grid);
	}

	// detect false positives
	std::set<std::shared_ptr<PipelineGrid>> notYetDetectedGrids = results.taggedGridsOnFrame;
	for (const pipeline::Tag& tag : _taglist) {
		const cv::Rect& tagBox = tag.getBox();
		bool inGroundTruth = false;
		for (const std::shared_ptr<PipelineGrid>& grid : notYetDetectedGrids) {
			const cv::Rect gridBox = grid->getBoundingBox();
			if (tagBox.contains(gridBox.tl())
					&& tagBox.contains(gridBox.br())) {
				inGroundTruth = true;
				results.truePositives.insert(tag);
				results.gridByTag[tag] = grid;
				// this modifies the container in a range based for loop, which may invalidate
				// the iterators. This is not problem in this specific case because we exit the loop
				// right away.
				notYetDetectedGrids.erase(grid);
				break;
			}
		}

		if (!inGroundTruth)
			results.falsePositives.insert(tag);
	}

	_groundTruth.localizerResults = std::move(results);
}

void BeesBookImgAnalysisTracker::evaluateRecognizer() {
	static const double threshold = 100.;

	assert(_groundTruth.available);
	RecognizerEvaluationResults results;
	results.taggedGridsOnFrame =
			_groundTruth.localizerResults.taggedGridsOnFrame;

	for (const pipeline::Tag& tag : _taglist) {
		auto it = _groundTruth.localizerResults.gridByTag.find(tag);
		if (it != _groundTruth.localizerResults.gridByTag.end()) {
			const std::shared_ptr<PipelineGrid>& grid = (*it).second;
			if (!tag.getCandidates().empty()) {
				auto scoreCandidatePair = compareGrids(tag, grid);
				const double score = scoreCandidatePair.first;
				const pipeline::TagCandidate& candidate =
						scoreCandidatePair.second;

				if (score <= threshold)
					results.truePositives.push_back( { tag, candidate });
				else
					results.falsePositives.insert(tag);
			} else
				results.falsePositives.insert(tag);
		} else if (tag.getCandidates().size()) {
			results.falsePositives.insert(tag);
		}
	}

	for (const std::shared_ptr<PipelineGrid>& grid : _groundTruth.localizerResults.falseNegatives) {
		results.falseNegatives.insert(grid);
	}

	_groundTruth.recognizerResults = std::move(results);
}

void BeesBookImgAnalysisTracker::evaluateGridfitter()
{
    assert(_groundTruth.available);
    //int framenum                = getCurrentFrameNumber();



    for (auto& candidateByGrid : _groundTruth.recognizerResults.truePositives) {
        const pipeline::Tag& tag = candidateByGrid.first;
        const pipeline::TagCandidate& bestCandidate = candidateByGrid.second;
        assert(!bestCandidate.getGridsConst().empty());
        const PipelineGrid& bestFoundGrid = bestCandidate.getGridsConst().at(0);
        const std::shared_ptr<PipelineGrid> groundTruthGrid = _groundTruth.localizerResults.gridByTag.at(tag);

        if (groundTruthGrid->compare(bestFoundGrid) > 1.0) {
            _groundTruth.gridfitterResults.matches++;
        }
        else {
            _groundTruth.gridfitterResults.mismatches++;
        }
    }



//    //iterate over all objects found by pipeline
//    for (pipeline::Tag& tag : _taglist)
//    {
//        if (!tag.getCandidates().empty())
//        {
//            // take best grid fit of best ellipse candidate (aka TagCandidate) of current ROI (aka Tag)
//            PipelineGrid const & pipegrid = tag.getCandidates().at(0).getGrids().at(0);

//            //iterate over all ground truth objects
//            for (const TrackedObject trObj : GTObjects)
//            {
//                // get grid pointer of current object in list
//                std::shared_ptr<Grid3D> pGrid = trObj.maybeGet<Grid3D>(framenum);

//                // if the pointer is valid
//                if ( pGrid )
//                {
//                    // check if grids are sufficiently close to each other
//                    if ( cv::norm( (pGrid->getCenter() - pipegrid.getCenter()) ) < 10.0 )
//                    {
//                        Grid temp(pGrid->getCenter(), pGrid->getPixelRadius(), pGrid->getZRotation(), pGrid->getYRotation(), pGrid->getXRotation());

//                        //if ( pipegrid.compare(temp) > 1.0 )
//                        {
//                            //results.taggedGridsOnFrame.insert( TODO XXX pipegrid);
//                        }
//                    }
//                }
//            }

//        }
//    }
}

void BeesBookImgAnalysisTracker::evaluateDecoder()
{
	// TODO
}

int BeesBookImgAnalysisTracker::calculateVisualizationThickness() const {
	const int radius = _settings.getValueOfParam<int>(
			pipeline::settings::Localizer::Params::MIN_BOUNDING_BOX_SIZE) / 2;
	// calculate actual pixel size of grid based on current zoom level
	double displayTagSize = radius / getCurrentZoomLevel();
	displayTagSize = displayTagSize > 50. ? 50 : displayTagSize;
	// thickness of rectangle of grid is based on actual pixel size
	// of the grid. if the radius is 50px or more, the rectangle has
	// a thickness of 1px.
	const int thickness = static_cast<int>(1. / (displayTagSize / 50.));
	return thickness;
}

std::pair<double, std::reference_wrapper<const pipeline::TagCandidate> > BeesBookImgAnalysisTracker::compareGrids(const pipeline::Tag &detectedTag, const std::shared_ptr<PipelineGrid> &grid) const
{
	assert(!detectedTag.getCandidates().empty());
	auto deviation =
			[](cv::Point2i const& cen, cv::Size const& axis, double angle, cv::Point const& point)
			{
				const double sina = std::sin(angle);
				const double cosa = std::cos(angle);
				const int x = point.x;
				const int y = point.y;
				const int a = axis.width;
				const int b = axis.height;
				return std::abs((x * x) / (a * a) + (y * y) / (b * b) - (cosa * cosa) - (sina * sina)) + cv::norm(cen - point);
			};

	pipeline::TagCandidate const* bestCandidate = nullptr;
	const std::vector<cv::Point> outerPoints = grid->getOuterRingEdgeCoordinates();
	double bestDeviation = std::numeric_limits<double>::max();
	for (pipeline::TagCandidate const& candidate : detectedTag.getCandidates()) {
		double sumDeviation = 0.;
		const pipeline::Ellipse& ellipse = candidate.getEllipse();
		for (cv::Point const& point : outerPoints) {
			sumDeviation += deviation(ellipse.getCen(), ellipse.getAxis(),
					ellipse.getAngle(), point);
		}
		const double deviation = sumDeviation / outerPoints.size();
		if (deviation < bestDeviation) {
			bestDeviation = deviation;
			bestCandidate = &candidate;
		}
	}
	assert(bestCandidate);
	return {bestDeviation, *bestCandidate};
}

cv::Mat BeesBookImgAnalysisTracker::rgbMatFromBwMat(const cv::Mat &mat, const int type) const {
	// TODO: convert B&W to RGB
	// this could probably be implemented more efficiently
	cv::Mat image;
	cv::Mat channels[3] = { mat.clone(), mat.clone(), mat.clone() };
	cv::merge(channels, 3, image);
	image.convertTo(image, type);
	return image;
}

void BeesBookImgAnalysisTracker::paint(cv::Mat& image, const View& view) 
{
	// don't try to visualize results while data processing is running
	if (_tagListLock.try_lock()) {
		switch (_selectedStage) {
		case BeesBookCommon::Stage::Preprocessor:
			if ((view.name == "Preprocessor Output")
					&& (_visualizationData.preprocessorImage)) {
				image = rgbMatFromBwMat(
						_visualizationData.preprocessorImage.get(),
						image.type());
			} else if ((view.name == "Opts")&& (_visualizationData.preprocessorOptImage)) {
				image = rgbMatFromBwMat(
						_visualizationData.preprocessorOptImage.get(),
						image.type());

		} else if ((view.name == "Honeyfilter")&& (_visualizationData.preprocessorHoneyImage)) {
			image = rgbMatFromBwMat(
					_visualizationData.preprocessorHoneyImage.get(),
					image.type());

			} else if ((view.name == "Threshold-Comb")
					&& (_visualizationData.preprocessorThresholdImage)) {
				image = rgbMatFromBwMat(
						_visualizationData.preprocessorThresholdImage.get(),
						image.type());
			}
			break;
		case BeesBookCommon::Stage::Localizer:
			/*if ((view.name == "Sobel Edge")
			 && (_visualizationData.localizerSobelImage)) {
			 image = rgbMatFromBwMat(
			 _visualizationData.localizerSobelImage.get(),
			 image.type());*/
			if ((view.name == "Blobs")
					&& (_visualizationData.localizerBlobImage)) {
				image = rgbMatFromBwMat(
						_visualizationData.localizerBlobImage.get(),
						image.type());
			} else if ((view.name == "Input")
					&& (_visualizationData.localizerInputImage)) {
				image = rgbMatFromBwMat(
						_visualizationData.localizerInputImage.get(),
						image.type());

			} else if ((view.name == "Threshold")
					&& (_visualizationData.localizerThresholdImage)) {
				image = rgbMatFromBwMat(
						_visualizationData.localizerThresholdImage.get(),
						image.type());
			}
			visualizeLocalizerOutput(image);
			break;
		case BeesBookCommon::Stage::Recognizer:
			visualizeRecognizerOutput(image);
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

void BeesBookImgAnalysisTracker::visualizePreprocessorOutput(cv::Mat &image) const
{

}

void BeesBookImgAnalysisTracker::settingsChanged(const BeesBookCommon::Stage stage)
{
    switch (stage)
    {
        case BeesBookCommon::Stage::Preprocessor : _preprocessor.loadSettings(BeesBookCommon::getPreprocessorSettings(_settings));
		break;
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

void BeesBookImgAnalysisTracker::loadGroundTruthData()
{
    QString filename = QFileDialog::getOpenFileName(nullptr, tr("Load tracking data"), "", tr("Data Files (*.tdat)"));

    if (filename.isEmpty())
		return;

	std::ifstream is(filename.toStdString());

    cereal::JSONInputArchive ar(is);

    // load serialized data into member .data
    ar(_groundTruth.data);

	_groundTruth.available = true;

	const std::array<QLabel*, 10> labels { _groundTruth.labelFalsePositives,
			_groundTruth.labelFalseNegatives, _groundTruth.labelTruePositives,
			_groundTruth.labelRecall, _groundTruth.labelPrecision,
			_groundTruth.labelNumFalsePositives,
			_groundTruth.labelNumFalseNegatives,
			_groundTruth.labelNumTruePositives, _groundTruth.labelNumRecall,
			_groundTruth.labelNumPrecision };

	for (QLabel* label : labels) {
		label->setEnabled(true);
	}

	emit forceTracking();
	//TODO: maybe check filehash here
}

void BeesBookImgAnalysisTracker::exportConfiguration() {
	 QString dir = QFileDialog::getExistingDirectory(0, tr("select directory"),
	                                                 "",
	                                                 QFileDialog::ShowDirsOnly| QFileDialog::DontResolveSymlinks);
	 if (dir.isEmpty()){
		 emit notifyGUI("config export: no directory given", MSGS::FAIL);
	 		return;
	 }
	 bool ok;
	 //std::string image_name = _settings.getValueOfParam<std::string>(PICTUREPARAM::PICTURE_FILES);
	 QString filename = QInputDialog::getText(0, tr("choose filename (without extension)"),
	                                           tr("Filename:"), QLineEdit::Normal, "Filename"  , &ok);
	 if (!ok || filename.isEmpty()){
		 emit notifyGUI("config export: no filename given", MSGS::FAIL);
		return;
	 }



	 pipeline::settings::preprocessor_settings_t ps= BeesBookCommon::getPreprocessorSettings(_settings);
	if(ps.writeToJson(dir.toStdString() + "/" +filename.toStdString()+ ".json")){
		emit notifyGUI("config export successfully", MSGS::NOTIFICATION);
	}else{
		 emit notifyGUI("config export failed", MSGS::FAIL);
	}

    return;
}

void BeesBookImgAnalysisTracker::stageSelectionToogled(
		BeesBookCommon::Stage stage, bool checked) {
	if (checked) {
		_selectedStage = stage;

		emit registerViews( { } );
        
		switch (stage) {
		case BeesBookCommon::Stage::Preprocessor:
			setParamsWidget<PreprocessorParamsWidget>();
			emit registerViews( { { "Preprocessor Output" },{"Opts"},{"Honeyfilter"}, { "Threshold-Comb" } });
			break;
		case BeesBookCommon::Stage::Localizer:
			setParamsWidget<LocalizerParamsWidget>();
			emit registerViews( { { "Input" }, { "Threshold" }, { "Blobs" } });
			break;
		case BeesBookCommon::Stage::Recognizer:
			setParamsWidget<RecognizerParamsWidget>();
			emit registerViews( { { "Canny Edge" } });
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

