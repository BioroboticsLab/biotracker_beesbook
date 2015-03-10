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

#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>

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

BeesBookImgAnalysisTracker::BeesBookImgAnalysisTracker(Settings& settings, QWidget* parent) :
        TrackingAlgorithm(settings, parent),
        _selectedStage( BeesBookCommon::Stage::NoProcessing),
                        _paramsWidget(  std::make_shared<ParamsWidget>()),
                        _toolsWidget( std::make_shared<QWidget>())
{
	Ui::ToolWidget uiTools;
	uiTools.setupUi(_toolsWidget.get());

    // store label pointer in groundtruth object for convenience (I suppose?)
	_groundTruth.labelNumFalsePositives = uiTools.labelNumFalsePositives;
	_groundTruth.labelNumFalseNegatives = uiTools.labelNumFalseNegatives;
    _groundTruth.labelNumTruePositives  = uiTools.labelNumTruePositives;
    _groundTruth.labelNumRecall         = uiTools.labelNumRecall;
    _groundTruth.labelNumPrecision      = uiTools.labelNumPrecision;

    _groundTruth.labelFalsePositives    = uiTools.labelFalsePositives;
    _groundTruth.labelFalseNegatives    = uiTools.labelFalseNegatives;
    _groundTruth.labelTruePositives     = uiTools.labelTruePositives;
    _groundTruth.labelRecall            = uiTools.labelRecall;
    _groundTruth.labelPrecision         = uiTools.labelPrecision;

    // lambda function that implements generic "new style connect"
    auto connectRadioButton = [&](QRadioButton* button, BeesBookCommon::Stage stage)
    {
        QObject::connect(button, &QRadioButton::toggled, [ = ](bool checked)
        {
            stageSelectionToogled(stage, checked);
        } );
    };

    // connect radio buttons with their respective (lambda) handlers
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

    // send forceTracking upon button "process"
	QObject::connect(uiTools.processButton, &QPushButton::pressed,
			[&]() {emit forceTracking();});

    //
	QObject::connect(uiTools.pushButtonLoadGroundTruth, &QPushButton::pressed,
			this, &BeesBookImgAnalysisTracker::loadGroundTruthData);

    QObject::connect(uiTools.save_config, &QPushButton::pressed, this,
			&BeesBookImgAnalysisTracker::exportConfiguration);

    // load settings from config file
	_preprocessor.loadSettings(BeesBookCommon::getPreprocessorSettings(_settings));
	_localizer.loadSettings(BeesBookCommon::getLocalizerSettings(_settings));
	_recognizer.loadSettings(BeesBookCommon::getRecognizerSettings(_settings));
	_gridFitter.loadSettings(BeesBookCommon::getGridfitterSettings(_settings));
}

void BeesBookImgAnalysisTracker::track(ulong /*frameNumber*/, cv::Mat& frame)
{
    // notify lambda function
    const auto notify =
			[&](std::string const& message) {emit notifyGUI(message, MSGS::NOTIFICATION);};

    // acquire mutex (released when leaving function scope)
	const std::lock_guard<std::mutex> lock(_tagListLock);

    // set wait cursor until end of track function
	const CursorOverrideRAII cursorOverride(Qt::WaitCursor);

    // invalidate previous visualizations (aka views)
	for (boost::optional<cv::Mat>& visualization : _visualizationData.visualizations) {
		visualization.reset();
	}

    // taglist holds the tags found by the pipeline
	_taglist.clear();

    // algorithm layer selection cascade
	if (_selectedStage < BeesBookCommon::Stage::Preprocessor)
		return;

    // keep code in extra block for measuring execution time in RAII-fashion
    cv::Mat image;
    {
        // start the clock
        MeasureTimeRAII measure("Preprocessor", notify);

        // process current frame and store result frame in _image
        // as of now this is a sobel filtered image further processed
        _image = _preprocessor.process(frame);

        // set preprocessor views
        _visualizationData.preprocessorImage            = _image.clone();
        _visualizationData.preprocessorOptImage         = _preprocessor.getOptsImage();
        _visualizationData.preprocessorHoneyImage       = _preprocessor.getHoneyImage();
        _visualizationData.preprocessorThresholdImage   = _preprocessor.getThresholdImage();

	}

    // end of preprocessor stage
	if (_selectedStage < BeesBookCommon::Stage::Localizer)
		return;

    {
        // start the clock
		MeasureTimeRAII measure("Localizer", notify);

        // process image, find ROIs with tags
        _taglist = _localizer.process(std::move(frame), std::move(_image));

        // set localizer views
        _visualizationData.localizerInputImage      =  _image.clone();
        _visualizationData.localizerBlobImage       = _localizer.getBlob().clone();
        _visualizationData.localizerThresholdImage  = _localizer.getThresholdImage().clone();

        // evaluate localizer
        if (_groundTruth.available)
			evaluateLocalizer();
	}

    // end of localizer stage
	if (_selectedStage < BeesBookCommon::Stage::Recognizer)
		return;

	{
        // start the clock
		MeasureTimeRAII measure("Recognizer", notify);

        // find ellipses in taglist
		_taglist = _recognizer.process(std::move(_taglist));

        // set ellipsefitter views
        // TODO: maybe only visualize areas with ROIs
        _visualizationData.recognizerCannyEdge = _recognizer.computeCannyEdgeMap(frame);

        // evaluate ellipsefitter
        if (_groundTruth.available)
			evaluateRecognizer();
	}

    // end of ellipsefitter stage
	if (_selectedStage < BeesBookCommon::Stage::GridFitter)
		return;

	{
        // start the clock
		MeasureTimeRAII measure("GridFitter", notify);

        // fit grids to the ellipses found
        _taglist = _gridFitter.process(std::move(_taglist));

        // evaluate grids
        if (_groundTruth.available)
			evaluateGridfitter();
	}

    // end of gridfitter stage
	if (_selectedStage < BeesBookCommon::Stage::Decoder)
		return;

	{
        // start the clock
		MeasureTimeRAII measure("Decoder", notify);

        // decode grids that were matched to the image
        _taglist = _decoder.process(std::move(_taglist));

        // evaluate decodings
        if (_groundTruth.available)
			evaluateDecoder();
	}
}

void BeesBookImgAnalysisTracker::visualizeLocalizerOutput(cv::Mat& image) const
{
	const int thickness = calculateVisualizationThickness();

    // if there is no ground truth, draw all
    // pipeline ROIs in blue and return
    if (!_groundTruth.available)
    {
        for (const pipeline::Tag& tag : _taglist)
        {
			const cv::Rect& box = tag.getBox();
			cv::rectangle(image, box, COLOR_BLUE, thickness, CV_AA);
		}
		return;
	}

    // ... GT available, that means localizer has been evaluated (localizerResults holds data)
    // get localizer results struct
	const LocalizerEvaluationResults& results = _groundTruth.localizerResults;

    // correctly found
    for (const pipeline::Tag& tag : results.truePositives)
    {
		cv::rectangle(image, tag.getBox(), COLOR_GREEN, thickness, CV_AA);
	}

    // false detections
    for (const pipeline::Tag& tag : results.falsePositives)
    {
		cv::rectangle(image, tag.getBox(), COLOR_RED, thickness, CV_AA);
	}

    // missing detections
    for (const std::shared_ptr<PipelineGrid>& grid : results.falseNegatives)
    {
        cv::rectangle(image, grid->getBoundingBox(), COLOR_ORANGE, thickness, CV_AA);
	}

    // calculate recall / precision
    const float recall      = static_cast<float>(results.truePositives.size()) /
                              static_cast<float>(results.taggedGridsOnFrame.size()) * 100.f;

    const float precision   = static_cast<float>(results.truePositives.size()) /
                              static_cast<float>(results.truePositives.size() +
                                                 results.falsePositives.size()) * 100.f;

    // set text in statistics labels
    // ToDo encapsulate in member function
    _groundTruth.labelNumFalseNegatives->setText(QString::number(results.falseNegatives.size()));
    _groundTruth.labelNumFalsePositives->setText(QString::number(results.falsePositives.size()));
    _groundTruth.labelNumTruePositives->setText(QString::number(results.truePositives.size()));
	_groundTruth.labelNumRecall->setText(QString::number(recall, 'f', 2) + "%");
    _groundTruth.labelNumPrecision->setText(QString::number(precision, 'f', 2) + "%");
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
		grid->drawContours(image, 1.0);
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
    const int thickness = calculateVisualizationThickness();

	if (!_groundTruth.available) {
		for (const pipeline::Tag& tag : _taglist) {
			if (!tag.getCandidates().empty()) {

				if (! tag.getCandidates().empty())
				{
					// get best candidate
					const pipeline::TagCandidate& candidate = tag.getCandidates()[0];

					if (! candidate.getGridsConst().empty())
					{
						const PipelineGrid& grid = candidate.getGridsConst()[0];
						cv::rectangle(image, (grid.getBoundingBox() + cv::Size(20, 20)) - cv::Point(10, 10), COLOR_GREEN, thickness, CV_AA);
						grid.drawContours(image, 0.5);
					}
				}
			}
		}
		return;
	}

    for (const PipelineGrid & pipegrid : _groundTruth.gridfitterResults.truePositives)
    {
        pipegrid.drawContours(image, 0.5);
        cv::rectangle(image, (pipegrid.getBoundingBox() + cv::Size(20, 20)) - cv::Point(10, 10), COLOR_GREEN, thickness, CV_AA);
    }

    for (const PipelineGrid & pipegrid : _groundTruth.gridfitterResults.falsePositives)
    {
        pipegrid.drawContours(image, 0.5);
        cv::rectangle(image, (pipegrid.getBoundingBox() + cv::Size(20, 20)) - cv::Point(10, 10), COLOR_RED, thickness, CV_AA);
    }

//    if (_groundTruth.available)
//    {
//        //precision = TP / Positives
//        //recall = TP / GroundTruth_N
//        unsigned int matches = _groundTruth.gridfitterResults.matches;
//        unsigned int mismatches = _groundTruth.gridfitterResults.mismatches;

//        _groundTruth.labelNumTruePositives->setText(QString::number(matches));

//        if (_groundTruth.recognizerResults.taggedGridsOnFrame.size() != 0)
//            _groundTruth.labelNumRecall->setText( QString::number( 100 * matches / _groundTruth.recognizerResults.taggedGridsOnFrame.size(), 'f', 2) + "%");

//        if ((matches + mismatches) != 0)
//            _groundTruth.labelNumPrecision->setText( QString::number(matches / (matches + mismatches), 'f', 2) + "%");
//    }
}

void BeesBookImgAnalysisTracker::visualizeDecoderOutput(cv::Mat& image) const {
	const int thickness = 2;
	const int size = 3;
	const int distance = 30;

	if (!_groundTruth.available) {
		for (const pipeline::Tag& tag : _taglist) {
			if (!tag.getCandidates().empty()) {
				const pipeline::TagCandidate& candidate = tag.getCandidates()[0];
				if (candidate.getDecodings().size()) {
					assert(candidate.getGridsConst().size());

					const PipelineGrid& grid = candidate.getGridsConst()[0];
					pipeline::decoding_t decoding = candidate.getDecodings()[0];
					int idInt = decoding.to_ulong();
					std::string idString = "";
					while (idInt > 1){
						idString = std::to_string(idInt%2) + idString;
						idInt = idInt >> 1;
					}
					idString = std::to_string(idInt%2) + idString;

					cv::rectangle(image, (grid.getBoundingBox() + cv::Size(20, 20)) - cv::Point(10, 10), COLOR_GREEN, thickness, CV_AA);
					cv::putText(image, std::to_string(decoding.to_ulong()),
					  cv::Point(tag.getBox().x, tag.getBox().y-distance),
	    			cv::FONT_HERSHEY_COMPLEX_SMALL, size,
					  cv::Scalar(0, 255, 0), thickness, CV_AA);
					cv::putText(image, idString,
					  cv::Point(tag.getBox().x, tag.getBox().y),
					  cv::FONT_HERSHEY_COMPLEX_SMALL, size/2,
					  cv::Scalar(0, 255, 0), thickness, CV_AA);
				}
			}
		}
		return;
	}

	const DecoderEvaluationResults& results = _groundTruth.decoderResults;
	int matchNum = 0;
	int partlyMismatchNum = 0;
	int mismatchNum = 0;
	int cumulHamming = 0;

	for (const auto& tuple : results.tuples) {
		int xpos = std::get<0>(tuple);
		int ypos = std::get<1>(tuple);
		int tagIdDecDet = std::get<2>(tuple);
		std::string tagIdStringDet = std::get<3>(tuple);
		std::string tagIdStringGT = std::get<4>(tuple);
		int hamming = std::get<5>(tuple);

		// calculate stats
		cv::Scalar color;
		static const int threshold = 3;
		if (hamming == 0){ // match
			++matchNum;
			color = cv::Scalar(0,255,0); 
		} else if (hamming <= threshold){ // partly match
			++partlyMismatchNum;
			color = cv::Scalar(0,125,255); 
		} else { // mismatch
			++mismatchNum;
			color = cv::Scalar(0,0,255); 
		}
		cumulHamming = cumulHamming + hamming;

		// paint text on image
		cv::putText(image, std::to_string(tagIdDecDet) + ", d=" + std::to_string(hamming),
						  cv::Point(xpos, ypos - (distance * 2)),
						  cv::FONT_HERSHEY_COMPLEX_SMALL, size,
						  color, thickness, CV_AA);
		cv::putText(image, tagIdStringDet,
						  cv::Point(xpos, ypos - distance),
						  cv::FONT_HERSHEY_COMPLEX_SMALL, size/2,
						  color, thickness, CV_AA);
		cv::putText(image, tagIdStringGT,
						  cv::Point(xpos, ypos),
						  cv::FONT_HERSHEY_COMPLEX_SMALL, size/2,
						  color, thickness, CV_AA);
	}

	// statistics
	_groundTruth.labelFalsePositives->setText("Match: ");
	_groundTruth.labelNumFalsePositives->setText(QString::number(matchNum));
	_groundTruth.labelTruePositives->setText("partly Mismatch");
	_groundTruth.labelNumTruePositives->setText(QString::number(partlyMismatchNum));
	_groundTruth.labelFalseNegatives->setText("Mismatch: ");
	_groundTruth.labelNumFalseNegatives->setText(QString::number(mismatchNum));
	_groundTruth.labelRecall->setText("average Hamming Distance: ");	
	_groundTruth.labelNumRecall->setText(QString::number(static_cast<float> (cumulHamming)/results.tuples.size()));
	_groundTruth.labelPrecision->setText("Precision (matched, partly): ");
	_groundTruth.labelNumPrecision ->setText(QString::number(static_cast<float> (matchNum)/results.tuples.size())  + ", " 
		+ QString::number((static_cast<float> (matchNum)+partlyMismatchNum)/results.tuples.size()));

	return;

}

void BeesBookImgAnalysisTracker::evaluateLocalizer()
{
	assert(_groundTruth.available);

    // convenience variable, is moved to _groundtruth.localizerResults later
    LocalizerEvaluationResults results;


    const int currentFrameNumber = getCurrentFrameNumber();

    // iterate over ground truth data (typed Grid3D), convert them to PipelineGrids
    // and store them in the std::set taggedGridsOnFrame
    for (TrackedObject const& object : _groundTruth.data.getTrackedObjects())
    {
        const std::shared_ptr<Grid3D> grid3d = object.maybeGet<Grid3D>(currentFrameNumber);
        if (!grid3d)
            continue;
        // convert to PipelineGrid
        const auto grid = std::make_shared<PipelineGrid>(
                grid3d->getCenter(), grid3d->getPixelRadius(),
                grid3d->getZRotation(), grid3d->getYRotation(),
                grid3d->getXRotation());

        // insert in set
        if (grid)
            results.taggedGridsOnFrame.insert(grid);
    }

    // Detect False Negatives
    for (const std::shared_ptr<PipelineGrid>& grid : results.taggedGridsOnFrame)
    {
        // ROI of ground truth
        const cv::Rect gridBox = grid->getBoundingBox();

		bool inGroundTruth = false;
        for (const pipeline::Tag& tag : _taglist)
        {
            // ROI of pipeline tag
			const cv::Rect& tagBox = tag.getBox();

            // target property is complete containment
            if (tagBox.contains(gridBox.tl())
                && tagBox.contains(gridBox.br()))
            {
				inGroundTruth = true;
				break;
			}
		}

        // mark current ground truth box as missing detection (false negative)
		if (!inGroundTruth)
			results.falseNegatives.insert(grid);
	}

    // Detect False Positives
    // ground truth grids
    std::set<std::shared_ptr<PipelineGrid>> notYetDetectedGrids = results.taggedGridsOnFrame;

    // iterate over all pipeline tags
    for (const pipeline::Tag& tag : _taglist)
    {
        // ROI of pipeline tag
        const cv::Rect& tagBox = tag.getBox();

        bool inGroundTruth = false;

        // iterate over all ground truth grids
        for (const std::shared_ptr<PipelineGrid>& grid : notYetDetectedGrids)
        {
            // ROI of ground truth grid
			const cv::Rect gridBox = grid->getBoundingBox();

            if (tagBox.contains(gridBox.tl())
                && tagBox.contains(gridBox.br()))
            {
                // match!
				inGroundTruth = true;

                // store pipeline tag in extra set
				results.truePositives.insert(tag);

                // store mapping from roi -> grid
				results.gridByTag[tag] = grid;

                // this modifies the container in a range based for loop, which may invalidate
				// the iterators. This is not problem in this specific case because we exit the loop
                // right away. (beware if that is going to be changed)
				notYetDetectedGrids.erase(grid);
				break;
			}
		}

        // if no match was found, the pipeline-reported roi is a false detection
		if (!inGroundTruth)
			results.falsePositives.insert(tag);
	}

    // move local results to global member struct
	_groundTruth.localizerResults = std::move(results);
}

void BeesBookImgAnalysisTracker::evaluateRecognizer()
{
    // ToDo
	static const double threshold = 100.;

	assert(_groundTruth.available);

    // local convenience variable
	RecognizerEvaluationResults results;

    // copy ground truth grids from localizer results struct
    results.taggedGridsOnFrame = _groundTruth.localizerResults.taggedGridsOnFrame;

    // iterate over pipeline ROIs
    for (const pipeline::Tag& tag : _taglist)
    {
        // find ground truth grid associated to current ROI
		auto it = _groundTruth.localizerResults.gridByTag.find(tag);

        // if valid iterator (and grid)
        if (it != _groundTruth.localizerResults.gridByTag.end())
        {
            // get ground truth grid
			const std::shared_ptr<PipelineGrid>& grid = (*it).second;

            // if the pipeline ROI has ellipses
            if (!tag.getCandidates().empty())
            {
                // calculate similarity score (or distance measure?)
                // and store it in std::pair with the
                // ROI's ellipse
				auto scoreCandidatePair = compareGrids(tag, grid);

                // get the score
                const double score = scoreCandidatePair.first;

                // print the score
                std::cout << "Score " << score  << std::endl;

                // get the ellipse
                const pipeline::TagCandidate& candidate = scoreCandidatePair.second;

                // store the ROI-ellipse pair if match (why that way?)
				if (score <= threshold)
					results.truePositives.push_back( { tag, candidate });
                else // store as false detection
					results.falsePositives.insert(tag);
            }
            // if no ellipses are found by pipeline,
            // this tag (ROI) is counted as a missing detection
            else
				results.falseNegatives.insert(grid);

        }
        // there is no ground truth data for the specified ROI
        // --> also count it as a false detection
        else
            if (tag.getCandidates().size())
            {
                results.falsePositives.insert(tag);
            }
	}

    // mark all ground truth grids that the localizer wasn't able to find
    // as missing detections on the ellipsefitter layer as well
    for (const std::shared_ptr<PipelineGrid>& grid : _groundTruth.localizerResults.falseNegatives)
    {
		results.falseNegatives.insert(grid);
	}

    // move local struct to global member
	_groundTruth.recognizerResults = std::move(results);
}

void BeesBookImgAnalysisTracker::evaluateGridfitter()
{
	assert(_groundTruth.available);

    // iterate over all correctly found ROI / ellipse pairs
    for (auto& candidateByGrid : _groundTruth.recognizerResults.truePositives)
    {
        // get ROI
		const pipeline::Tag& tag = candidateByGrid.first;

        // get ellipse
		const pipeline::TagCandidate& bestCandidate = candidateByGrid.second;

        // if there are grids stored with that ellipse
        if (!bestCandidate.getGridsConst().empty())
        {
            // get the best grid of that ellipse
            const PipelineGrid& bestFoundGrid = bestCandidate.getGridsConst().at(0);

            // use the ROI (tag) to lookup the groundtruth grid (the map was build in evaluateLocalizer)
            const std::shared_ptr<PipelineGrid> groundTruthGrid = _groundTruth.localizerResults.gridByTag.at(tag);

            // compare ground truth grid to pipeline grid
            if (groundTruthGrid->compare(bestFoundGrid) > 1.0)
            {
                _groundTruth.gridfitterResults.matches++;
                _groundTruth.gridfitterResults.truePositives.push_back(bestFoundGrid);
            }
            else
            {
                _groundTruth.gridfitterResults.mismatches++;
                _groundTruth.gridfitterResults.falsePositives.push_back(bestFoundGrid);
            }
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
	assert(_groundTruth.available);
	DecoderEvaluationResults results;
	for (pipeline::Tag& tag : _taglist) {
		auto it = _groundTruth.localizerResults.gridByTag.find(tag);
		if (it != _groundTruth.localizerResults.gridByTag.end()) {
			const std::shared_ptr<PipelineGrid>& grid = (*it).second;
            if (!tag.getCandidates().empty()) {
				//idarray_t wrongGroundTruthIDArray = grid->getIdArray();
				std::array<boost::tribool, 12> wrongGroundTruthIDArray = {false,false,false,false,false,false,false,false,false,false,false,false};

				// Correct GT Array for different Decoding Order
				int indices [12] = {8,7,6,5,4,3,2,1,0,11,10,9};
				std::array<boost::tribool, 12> groundTruthIDArray;
				for (int i = 0; i<12; i++){
					groundTruthIDArray[i] = wrongGroundTruthIDArray[indices[i]];
				}

				// GroundTruth Array to String
				std::string idStringGT = "";
				for (int i = 0; i < 12; i++)
				{
					if (groundTruthIDArray[i] == true){
						idStringGT =  "1" + idStringGT;
					} else if (groundTruthIDArray[i] == false){
						idStringGT = "0" + idStringGT;
					} else {
						idStringGT = "?" + idStringGT;
					} 
				}



				// detected ID int to binary String
				std::bitset<12> idArray = tag.getCandidates()[0].getDecodings()[0];
				unsigned int idInt = idArray.to_ulong();

				std::string idString = "";
				for (int i = 0; i < 12; i++)
				{
					if (idArray[i] == true){
						idString =  "1" + idString;
					} else if (idArray[i] == false){
						idString = "0" + idString;
					} 
				}

				// calculate hamming distance
				int hamming = 0;
				for (int i=0; i<12; i++){
					if ((idString[i] != idStringGT[i]) && (idStringGT[i] != '?')){
						++hamming;
					}
				}

				// save results
				results.tuples.push_back(make_tuple(tag.getBox().x, tag.getBox().y, idInt, idString, idStringGT, hamming));
			} 
		} 
	}

	_groundTruth.decoderResults = std::move(results);

}

int BeesBookImgAnalysisTracker::calculateVisualizationThickness() const
{
    const int radius = _settings.getValueOfParam<int>(
					pipeline::settings::Localizer::Params::BASE
							+ pipeline::settings::Localizer::Params::MIN_BOUNDING_BOX_SIZE)
					/ 2;
	// calculate actual pixel size of grid based on current zoom level
	double displayTagSize = radius / getCurrentZoomLevel();
	displayTagSize = displayTagSize > 50. ? 50 : displayTagSize;
	// thickness of rectangle of grid is based on actual pixel size
	// of the grid. if the radius is 50px or more, the rectangle has
	// a thickness of 1px.
	const int thickness = static_cast<int>(1. / (displayTagSize / 50.));
	return thickness;
}

std::pair<double, std::reference_wrapper<const pipeline::TagCandidate> > BeesBookImgAnalysisTracker::compareGrids(
		const pipeline::Tag &detectedTag,
		const std::shared_ptr<PipelineGrid> &grid) const {
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

				double res = std::abs((x * x) / (a * a) + (y * y) / (b * b) - (cosa * cosa) - (sina * sina)) + cv::norm(cen - point);

				std::cout << "Punkt "<< point.x << " " << point.y << " in  Ellipse "<< cen.x << ", " << cen.y <<", " << axis.width << "," << axis.height << " : " << res << std::endl;
				return res;
			};

	pipeline::TagCandidate const* bestCandidate = nullptr;
	const std::vector<cv::Point> outerPoints =
			grid->getOuterRingEdgeCoordinates();
	double bestDeviation = std::numeric_limits<double>::max();
	for (pipeline::TagCandidate const& candidate : detectedTag.getCandidates()) {
		double sumDeviation = 0.;
		const pipeline::Ellipse& ellipse = candidate.getEllipse();
		for (cv::Point const& point : outerPoints) {
			cv::Point rel_point = cv::Point(point.x -detectedTag.getBox().x,point.y -detectedTag.getBox().y);
			sumDeviation += deviation(ellipse.getCen(), ellipse.getAxis(),
					ellipse.getAngle(), rel_point);
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

cv::Mat BeesBookImgAnalysisTracker::rgbMatFromBwMat(const cv::Mat &mat,
		const int type) const {
	// TODO: convert B&W to RGB
	// this could probably be implemented more efficiently
	cv::Mat image;
	cv::Mat channels[3] = { mat.clone(), mat.clone(), mat.clone() };
	cv::merge(channels, 3, image);
	image.convertTo(image, type);
	return image;
}

void BeesBookImgAnalysisTracker::paint(cv::Mat& image, const View& view) {
	// don't try to visualize results while data processing is running

	if (_tagListLock.try_lock()) {
		switch (_selectedStage) {
		case BeesBookCommon::Stage::Preprocessor:
			if ((view.name == "Preprocessor Output")
					&& (_visualizationData.preprocessorImage)) {
				image = rgbMatFromBwMat(
						_visualizationData.preprocessorImage.get(),
						image.type());
			} else if ((view.name == "Opts")
					&& (_visualizationData.preprocessorOptImage)) {
				image = rgbMatFromBwMat(
						_visualizationData.preprocessorOptImage.get(),
						image.type());

			} else if ((view.name == "Honeyfilter")
					&& (_visualizationData.preprocessorHoneyImage)) {
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
			if ((view.name == "Canny Edge")
					&& (_visualizationData.recognizerCannyEdge)) {
				image = rgbMatFromBwMat(
						_visualizationData.recognizerCannyEdge.get(),
						image.type());
			}
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

void BeesBookImgAnalysisTracker::visualizePreprocessorOutput(
		cv::Mat &image) const {

}

void BeesBookImgAnalysisTracker::settingsChanged(
		const BeesBookCommon::Stage stage) {
	switch (stage) {
	case BeesBookCommon::Stage::Preprocessor:
		_preprocessor.loadSettings(
				BeesBookCommon::getPreprocessorSettings(_settings));
		break;
	case BeesBookCommon::Stage::Localizer:
		_localizer.loadSettings(
				BeesBookCommon::getLocalizerSettings(_settings));
		break;
	case BeesBookCommon::Stage::Recognizer:
		_recognizer.loadSettings(
				BeesBookCommon::getRecognizerSettings(_settings));
		break;
	case BeesBookCommon::Stage::GridFitter:
		_gridFitter.loadSettings(
		        BeesBookCommon::getGridfitterSettings(_settings));
		break;
	case BeesBookCommon::Stage::Decoder:
		// TODO
		break;
	default:
		break;
	}
}

void BeesBookImgAnalysisTracker::loadGroundTruthData() {
	QString filename = QFileDialog::getOpenFileName(nullptr,
			tr("Load tracking data"), "", tr("Data Files (*.tdat)"));

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
			"", QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
	if (dir.isEmpty()) {
		emit notifyGUI("config export: no directory given", MSGS::FAIL);
		return;
	}
	bool ok;
	//std::string image_name = _settings.getValueOfParam<std::string>(PICTUREPARAM::PICTURE_FILES);
	QString filename = QInputDialog::getText(0,
			tr("choose filename (without extension)"), tr("Filename:"),
			QLineEdit::Normal, "Filename", &ok);
	if (!ok || filename.isEmpty()) {
		emit notifyGUI("config export: no filename given", MSGS::FAIL);
		return;
	}

	boost::property_tree::ptree pt = BeesBookCommon::getPreprocessorSettings(
			_settings).getPTree();
	BeesBookCommon::getLocalizerSettings(_settings).addToPTree(pt);
	BeesBookCommon::getRecognizerSettings(_settings).addToPTree(pt);

	try {
		boost::property_tree::write_json(
				dir.toStdString() + "/" + filename.toStdString() + ".json", pt);
		emit notifyGUI("config export successfully", MSGS::NOTIFICATION);
	} catch (std::exception &e) {
		emit notifyGUI("config export failed", MSGS::FAIL);
	}

	return;
}

void BeesBookImgAnalysisTracker::stageSelectionToogled(
		BeesBookCommon::Stage stage, bool checked) {
	if (checked) {
		_selectedStage = stage;

		resetViews();
		switch (stage) {
		case BeesBookCommon::Stage::Preprocessor:
			setParamsWidget<PreprocessorParamsWidget>();
			emit registerViews( { { "Preprocessor Output" }, { "Opts" }, {
					"Honeyfilter" }, { "Threshold-Comb" } });
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

