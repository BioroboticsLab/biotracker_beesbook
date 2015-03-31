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
#include <boost/iostreams/stream.hpp>
#include <boost/archive/xml_iarchive.hpp>

#include "Common.h"
#include "DecoderParamsWidget.h"
#include "GridFitterParamsWidget.h"
#include "LocalizerParamsWidget.h"
#include "EllipseFitterParamsWidget.h"
#include "PreprocessorParamsWidget.h"
#include "pipeline/datastructure/Tag.h"
#include "pipeline/datastructure/TagCandidate.h"
#include "pipeline/datastructure/PipelineGrid.h"
#include "pipeline/datastructure/serialization.hpp"
#include "source/tracking/algorithm/algorithms.h"
#include "source/utility/CvHelper.h"
#include "source/utility/util.h"

#include "Grid3D.h"

#include "ui_ToolWidget.h"

/* shift decoded bits so that bit index 0 is the first bit on the "left side"
 * of the white semicircle */
#define SHIFT_DECODED_BITS

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

static const cv::Scalar COLOR_ORANGE(0, 102, 255);
static const cv::Scalar COLOR_GREEN(0, 255, 0);
static const cv::Scalar COLOR_RED(0, 0, 255);
static const cv::Scalar COLOR_BLUE(255, 0, 0);
static const cv::Scalar COLOR_LIGHT_BLUE(255, 200, 150);
static const cv::Scalar COLOR_GREENISH(13, 255, 182);

static const QColor QCOLOR_ORANGE(255, 102, 0);
static const QColor QCOLOR_GREEN(0, 255, 0);
static const QColor QCOLOR_RED(255, 0, 0);
static const QColor QCOLOR_BLUE(0, 0, 255);
static const QColor QCOLOR_LIGHT_BLUE(150, 200, 255);
static const QColor QCOLOR_GREENISH(182, 255, 13);

class MeasureTimeRAII {
public:
	MeasureTimeRAII(std::string const& what, std::function<void(std::string const&)> notify,
	                boost::optional<size_t> num = boost::optional<size_t>())
	    : _start(std::chrono::steady_clock::now()),
	      _what(what),
	      _notify(notify),
	      _num(num)
	{}

	~MeasureTimeRAII() {
		const auto end = std::chrono::steady_clock::now();
		const auto dur = std::chrono::duration_cast<std::chrono::milliseconds>(end - _start).count();
		std::stringstream message;
		message << _what << " finished in " << dur << "ms.";
		if (_num) {
			const auto avg = dur / _num.get();
			message << " (Average: " << avg << "ms)";
		}
		message << std::endl;
		_notify(message.str());
		// display message right away
		QApplication::processEvents();
	}
private:
	const std::chrono::steady_clock::time_point _start;
	const std::string _what;
	const std::function<void(std::string const&)> _notify;
	const boost::optional<size_t> _num;
};
}

BeesBookImgAnalysisTracker::BeesBookImgAnalysisTracker(Settings& settings, QWidget* parent) :
    TrackingAlgorithm(settings, parent),
    _interactionGrid(cv::Point2i( 300, 300), 23, 0.0, 0.0, 0.0),
    _selectedStage( BeesBookCommon::Stage::NoProcessing),
    _paramsWidget(  std::make_shared<ParamsWidget>()),
    _toolsWidget( std::make_shared<QWidget>()),
    _lastMouseEventTime(std::chrono::system_clock::now())

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
	connectRadioButton(uiTools.radioButtonEllipseFitter,
	                   BeesBookCommon::Stage::EllipseFitter);
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

	QObject::connect(uiTools.pushButtonLoadTaglist, &QPushButton::pressed, this,
	                 &BeesBookImgAnalysisTracker::loadTaglist);

	// load settings from config file
	_preprocessor.loadSettings(BeesBookCommon::getPreprocessorSettings(_settings));
	_localizer.loadSettings(BeesBookCommon::getLocalizerSettings(_settings));
	_ellipsefitter.loadSettings(BeesBookCommon::getEllipseFitterSettings(_settings));
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

	// clear ground truth evaluation data
	if (_groundTruth.available) {
		_groundTruth.localizerResults     = LocalizerEvaluationResults();
		_groundTruth.ellipsefitterResults = EllipseFitterEvaluationResults();
		_groundTruth.gridfitterResults    = GridFitterEvaluationResults();
		_groundTruth.decoderResults       = DecoderEvaluationResults();

		_groundTruth.labelNumTruePositives->setText("");
		_groundTruth.labelNumFalsePositives->setText("");
		_groundTruth.labelNumFalseNegatives->setText("");
		_groundTruth.labelNumPrecision->setText("");
		_groundTruth.labelNumRecall->setText("");
	}

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
		_visualizationData.preprocessorImage          = _image.clone();
		_visualizationData.preprocessorOptImage       = _preprocessor.getOptsImage();
		_visualizationData.preprocessorHoneyImage     = _preprocessor.getHoneyImage();
		_visualizationData.preprocessorThresholdImage = _preprocessor.getThresholdImage();
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
		_visualizationData.localizerInputImage     =  _image.clone();
		_visualizationData.localizerBlobImage      = _localizer.getBlob().clone();
		_visualizationData.localizerThresholdImage = _localizer.getThresholdImage().clone();

		// evaluate localizer
		if (_groundTruth.available)
			evaluateLocalizer();
	}

	// end of localizer stage
	if (_selectedStage < BeesBookCommon::Stage::EllipseFitter)
		return;

	{
		// start the clock
		MeasureTimeRAII measure("EllipseFitter", notify, _taglist.size());

		// find ellipses in taglist
		_taglist = _ellipsefitter.process(std::move(_taglist));

		// set ellipsefitter views
		// TODO: maybe only visualize areas with ROIs
		_visualizationData.ellipsefitterCannyEdge = _ellipsefitter.computeCannyEdgeMap(frame);

		// evaluate ellipsefitter
		if (_groundTruth.available)
			evaluateEllipseFitter();
	}

	// end of ellipsefitter stage
	if (_selectedStage < BeesBookCommon::Stage::GridFitter)
		return;

	{
		// start the clock
		MeasureTimeRAII measure("GridFitter", notify, _taglist.size());

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
		MeasureTimeRAII measure("Decoder", notify, _taglist.size());

		// decode grids that were matched to the image
		_taglist = _decoder.process(std::move(_taglist));

		// evaluate decodings
		if (_groundTruth.available)
			evaluateDecoder();
	}
}

void BeesBookImgAnalysisTracker::setGroundTruthStats(const size_t numGroundTruth, const size_t numTruePositives, const size_t numFalsePositives, const size_t numFalseNegatives) const
{
	const double recall    = numGroundTruth ?
	            (static_cast<double>(numTruePositives) / static_cast<double>(numGroundTruth)) * 100. : 0.;
	const double precision = (numTruePositives + numFalsePositives) ?
	            (static_cast<double>(numTruePositives) / static_cast<double>(numTruePositives + numFalsePositives)) * 100. : 0.;

	_groundTruth.labelNumFalseNegatives->setText(QString::number(numFalseNegatives));
	_groundTruth.labelNumFalsePositives->setText(QString::number(numFalsePositives));
	_groundTruth.labelNumTruePositives->setText(QString::number(numTruePositives));
	_groundTruth.labelNumRecall->setText(QString::number(recall, 'f', 2) + "%");
	_groundTruth.labelNumPrecision->setText(QString::number(precision, 'f', 2) + "%");
}

void BeesBookImgAnalysisTracker::drawEllipse(const pipeline::Tag& tag, QPen& pen, QPainter *painter, const pipeline::Ellipse& ellipse) const
{
	static const QPoint offset(20, -20);

	cv::RotatedRect ellipseBox(ellipse.getCen(), ellipse.getAxis(), ellipse.getAngle());

	//get ellipse definition
	QPoint center = CvHelper::toQt(ellipse.getCen());
	qreal rx = static_cast<qreal>(ellipse.getAxis().width);
	qreal ry = static_cast<qreal>(ellipse.getAxis().height);

	//draw ellipse
	painter->save();
	painter->translate(tag.getBox().x,tag.getBox().y);
	painter->translate(center.x(),center.y());
	painter->rotate(-ellipse.getAngle());
	painter->drawEllipse(QPointF(0,0),rx,ry);
	painter->restore();

	//draw score
	painter->setPen(pen);
	painter->save();
	painter->translate(tag.getBox().x,tag.getBox().y);
	painter->drawText(CvHelper::toQt(ellipseBox.boundingRect().tl()) + offset,
	                  "Score: " + QString::number(ellipse.getVote()));
	painter->restore();
}

void BeesBookImgAnalysisTracker::drawBox(const cv::Rect &box, QPainter* painter, QPen& pen) const
{
	const QRect qbox = CvHelper::toQt(box);
	painter->setPen(pen);
	painter->drawRect(qbox);
}

void BeesBookImgAnalysisTracker::visualizeLocalizerOutputOverlay(QPainter *painter) const
{
	QPen pen = getDefaultPen(painter);

	// if there is no ground truth, draw all
	// pipeline ROIs in blue and return
	if (!_groundTruth.available)
	{
		for (const pipeline::Tag& tag : _taglist)
		{
			pen.setColor(QCOLOR_LIGHT_BLUE);
			drawBox(tag.getBox(), painter, pen);
		}
		return;
	}

	// ... GT available, that means localizer has been evaluated (localizerResults holds data)
	// get localizer results struct
	const LocalizerEvaluationResults& results = _groundTruth.localizerResults;

	// correctly found
	for (const pipeline::Tag& tag : results.truePositives)
	{
		pen.setColor(QCOLOR_GREEN);
		drawBox(tag.getBox(), painter, pen);
	}

	// false detections
	for (const pipeline::Tag& tag : results.falsePositives)
	{
		pen.setColor(QCOLOR_RED);
		drawBox(tag.getBox(), painter, pen);
	}

	// missing detections
	for (const std::shared_ptr<PipelineGrid>& grid : results.falseNegatives)
	{
		pen.setColor(QCOLOR_ORANGE);
		drawBox(grid->getBoundingBox(), painter, pen);
	}

	const size_t numGroundTruth    = results.taggedGridsOnFrame.size();
	const size_t numTruePositives  = results.truePositives.size();
	const size_t numFalsePositives = results.falsePositives.size();
	const size_t numFalseNegatives = results.falseNegatives.size();

	setGroundTruthStats(numGroundTruth, numTruePositives, numFalsePositives, numFalseNegatives);
}

void BeesBookImgAnalysisTracker::visualizeEllipseFitterOutput(
        cv::Mat& image) const {

	const EllipseFitterEvaluationResults& results = _groundTruth.ellipsefitterResults;

	for (const std::shared_ptr<PipelineGrid>& grid : results.falseNegatives) {
		grid->drawContours(image, 1.0);
	}
}

void BeesBookImgAnalysisTracker::visualizeEllipseFitterOutputOverlay(
        QPainter *painter) const {
	QPen pen = getDefaultPen(painter);

	if (!_groundTruth.available) {
		for (const pipeline::Tag& tag : _taglist) {
			if (!tag.getCandidatesConst().empty()) {
				// get best candidate
				const pipeline::TagCandidate& candidate = tag.getCandidatesConst()[0];
				const pipeline::Ellipse& ellipse = candidate.getEllipse();

				pen.setColor(QCOLOR_LIGHT_BLUE);
				painter->setPen(pen);

				drawEllipse(tag, pen, painter, ellipse);
			}
		}
		return;
	}

	const EllipseFitterEvaluationResults& results = _groundTruth.ellipsefitterResults;

	for (const auto& tagCandidatePair : results.truePositives) {
		const pipeline::Tag& tag = tagCandidatePair.first;
		const pipeline::TagCandidate& candidate = tagCandidatePair.second;
		const pipeline::Ellipse& ellipse = candidate.getEllipse();

		pen.setColor(QCOLOR_GREEN);
		painter->setPen(pen);

		drawEllipse(tag, pen, painter, ellipse);
	}

	for (const pipeline::Tag& tag : results.falsePositives) {
		//TODO: should use ellipse with best score
		if (tag.getCandidatesConst().size()) {
			const pipeline::Ellipse& ellipse =
			        tag.getCandidatesConst().at(0).getEllipse();

			pen.setColor(QCOLOR_RED);
			painter->setPen(pen);

			drawEllipse(tag, pen, painter, ellipse);
		}
	}

	for (const std::shared_ptr<PipelineGrid>& grid : results.falseNegatives) {
		pen.setColor(QCOLOR_ORANGE);

		drawBox(grid->getBoundingBox(), painter, pen);
	}

	const size_t numGroundTruth    = results.taggedGridsOnFrame.size();
	const size_t numTruePositives  = results.truePositives.size();
	const size_t numFalsePositives = results.falsePositives.size();
	const size_t numFalseNegatives = results.falseNegatives.size();

	setGroundTruthStats(numGroundTruth, numTruePositives, numFalsePositives, numFalseNegatives);
}

void BeesBookImgAnalysisTracker::visualizeGridFitterOutput(cv::Mat& image) const
{
	if (!_groundTruth.available) {
		for (const pipeline::Tag& tag : _taglist) {
			if (!tag.getCandidatesConst().empty()) {

				if (! tag.getCandidatesConst().empty())
				{
					// get best candidate
					const pipeline::TagCandidate& candidate = tag.getCandidatesConst()[0];

					if (! candidate.getGridsConst().empty())
					{
						const PipelineGrid& grid = candidate.getGridsConst()[0];
						grid.drawContours(image, 0.5);
					}
				}
			}
		}
		return;
	}

	const GridFitterEvaluationResults& results = _groundTruth.gridfitterResults;

	for (const PipelineGrid & pipegrid : results.truePositives)
	{
		pipegrid.drawContours(image, 0.5);
	}

	for (const PipelineGrid & pipegrid : results.falsePositives)
	{
		pipegrid.drawContours(image, 0.5);
	}

	for (const GroundTruthGridSPtr& grid : results.falseNegatives)
	{
		grid->drawContours(image, 0.5);
	}
}

QPen BeesBookImgAnalysisTracker::getDefaultPen(QPainter *painter) const
{
	QFont font(QFontDatabase::systemFont(QFontDatabase::FixedFont));
	font.setPointSize(10);
	painter->setFont(font);

	QPen pen = painter->pen();
	pen.setCosmetic(true);

	return pen;
}

BeesBookImgAnalysisTracker::taglist_t BeesBookImgAnalysisTracker::loadSerializedTaglist(const std::string &path)
{
	taglist_t taglist;

	std::ifstream ifs(path);
	boost::archive::xml_iarchive ia(ifs);
	ia & BOOST_SERIALIZATION_NVP(taglist);

	return taglist;
}

void BeesBookImgAnalysisTracker::visualizeGridFitterOutputOverlay(QPainter *painter) const
{
	QPen pen = getDefaultPen(painter);

	if (!_groundTruth.available) {
		for (const pipeline::Tag& tag : _taglist) {
			if (!tag.getCandidatesConst().empty()) {

				if (! tag.getCandidatesConst().empty())
				{
					// get best candidate
					const pipeline::TagCandidate& candidate = tag.getCandidatesConst()[0];

					if (! candidate.getGridsConst().empty())
					{
						const PipelineGrid& grid = candidate.getGridsConst()[0];
						pen.setColor(QCOLOR_LIGHT_BLUE);

						drawBox(grid.getBoundingBox(), painter, pen);
					}
				}
			}
		}
		return;
	}

	const GridFitterEvaluationResults& results = _groundTruth.gridfitterResults;

	for (const PipelineGrid & pipegrid : results.truePositives)
	{
		pen.setColor(QCOLOR_GREEN);

		drawBox((pipegrid.getBoundingBox() + cv::Size(20, 20)) - cv::Point(10, 10), painter, pen);
	}

	for (const PipelineGrid & pipegrid : results.falsePositives)
	{
		pen.setColor(QCOLOR_RED);

		drawBox((pipegrid.getBoundingBox() + cv::Size(20, 20)) - cv::Point(10, 10), painter, pen);
	}

	for (const GroundTruthGridSPtr& grid : results.falseNegatives)
	{
		pen.setColor(QCOLOR_ORANGE);

		drawBox((grid->getBoundingBox() + cv::Size(20, 20)) - cv::Point(10, 10), painter, pen);
	}

	const size_t numGroundTruth    = _groundTruth.ellipsefitterResults.taggedGridsOnFrame.size();
	const size_t numTruePositives  = results.truePositives.size();
	const size_t numFalsePositives = results.falsePositives.size();
	const size_t numFalseNegatives = results.falseNegatives.size();

	setGroundTruthStats(numGroundTruth, numTruePositives, numFalsePositives, numFalseNegatives);
}

void BeesBookImgAnalysisTracker::visualizeDecoderOutput(cv::Mat& image) const {
	if (!_groundTruth.available) {
		for (const pipeline::Tag& tag : _taglist) {
			if (!tag.getCandidatesConst().empty()) {
				const pipeline::TagCandidate& candidate = tag.getCandidatesConst()[0];
				if (candidate.getDecodings().size()) {
					assert(candidate.getGridsConst().size());
					const PipelineGrid& grid = candidate.getGridsConst()[0];
					grid.drawContours(image, 0.5);
				}
			}
		}
		return;
	}

	const DecoderEvaluationResults& results = _groundTruth.decoderResults;

	for (const DecoderEvaluationResults::result_t& result : results.evaluationResults) {
		result.pipelineGrid.get().drawContours(image, 0.5);
	}

	for (const GroundTruthGridSPtr& grid : _groundTruth.ellipsefitterResults.falseNegatives)
	{
		grid->drawContours(image, 0.5);
	}
}

void BeesBookImgAnalysisTracker::visualizeDecoderOutputOverlay(QPainter *painter) const {
	static const int distance      = 10;

	QPen pen = getDefaultPen(painter);

	if (!_groundTruth.available) {
		for (const pipeline::Tag& tag : _taglist) {
			if (!tag.getCandidatesConst().empty()) {
				const pipeline::TagCandidate& candidate = tag.getCandidatesConst()[0];
				if (candidate.getDecodings().size()) {
					assert(candidate.getGridsConst().size());

					const PipelineGrid& grid = candidate.getGridsConst()[0];
					pipeline::decoding_t decoding = candidate.getDecodings()[0];

#ifdef SHIFT_DECODED_BITS
					util::rotateBitset(decoding, 3);
#endif

					const QString idString = QString::fromStdString(decoding.to_string());

					pen.setColor(QCOLOR_LIGHT_BLUE);

					drawBox((grid.getBoundingBox() + cv::Size(20, 20)) - cv::Point(10, 10), painter, pen);

					painter->setPen(pen);
					painter->drawText(QPoint(tag.getBox().x, tag.getBox().y - distance -5),
					                  QString::number(decoding.to_ulong()));
					painter->drawText(QPoint(tag.getBox().x, tag.getBox().y-5),
					                  idString);
				}
			}
		}
		return;
	}

	const DecoderEvaluationResults& results = _groundTruth.decoderResults;
	int matchNum           = 0;
	int partialMismatchNum = 0;
	int mismatchNum        = 0;
	int cumulHamming       = 0;

	for (const DecoderEvaluationResults::result_t& result : results.evaluationResults) {
		// calculate stats
		QColor color;
		static const int threshold = 3;
		if (result.hammingDistance == 0){ // match
			++matchNum;
			color = QCOLOR_GREEN;
		} else if (result.hammingDistance <= threshold){ // partial match
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

	for (const GroundTruthGridSPtr& grid : _groundTruth.ellipsefitterResults.falseNegatives)
	{
		pen.setColor(QCOLOR_ORANGE);
		drawBox(grid->getBoundingBox(), painter, pen);
	}

	const size_t numResults = results.evaluationResults.size();

	const double avgHamming = numResults ? static_cast<double>(cumulHamming) / numResults : 0.;
	const double precMatch  = numResults ? (static_cast<double>(matchNum) / static_cast<double>(numResults)) * 100.: 0.;
	const double precPartly = numResults ? (static_cast<double>(matchNum + partialMismatchNum) / static_cast<double>(numResults)) * 100. : 0.;

	// statistics
	_groundTruth.labelFalsePositives->setText("Match: ");
	_groundTruth.labelNumFalsePositives->setText(QString::number(matchNum));
	_groundTruth.labelTruePositives->setText("Partial mismatch: ");
	_groundTruth.labelNumTruePositives->setText(QString::number(partialMismatchNum));
	_groundTruth.labelFalseNegatives->setText("Mismatch: ");
	_groundTruth.labelNumFalseNegatives->setText(QString::number(mismatchNum));
	_groundTruth.labelRecall->setText("Average hamming distance: ");
	_groundTruth.labelNumRecall->setText(QString::number(avgHamming));
	_groundTruth.labelPrecision->setText("Precision (matched, partial): ");
	_groundTruth.labelNumPrecision->setText(QString::number(precMatch, 'f', 2) + "%, " +
	                                        QString::number(precPartly, 'f', 2) + "%");
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
		grid->setIdArray(grid3d->getIdArray());

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

void BeesBookImgAnalysisTracker::evaluateEllipseFitter()
{
	// ToDo
	static const double threshold = 100.;

	assert(_groundTruth.available);

	// local convenience variable
	EllipseFitterEvaluationResults results;

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
			if (!tag.getCandidatesConst().empty())
			{
				// calculate similarity score (or distance measure?)
				// and store it in std::pair with the
				// ROI's ellipse
				auto scoreCandidatePair = compareGrids(tag, grid);

				// get the score
				const double score = scoreCandidatePair.first;

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
			if (tag.getCandidatesConst().size())
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
	_groundTruth.ellipsefitterResults = std::move(results);
}

void BeesBookImgAnalysisTracker::evaluateGridfitter()
{
	assert(_groundTruth.available);

	// iterate over all correctly found ROI / ellipse pairs
	for (auto const& candidateByGrid : _groundTruth.ellipsefitterResults.truePositives)
	{
		// get ROI
		const pipeline::Tag& tag = candidateByGrid.first;

		// get ellipse
		const pipeline::TagCandidate& bestCandidate = candidateByGrid.second;

		// use the ROI (tag) to lookup the groundtruth grid (the map was build in evaluateLocalizer)
		// TODO: Workaround, remove this!
		assert(_groundTruth.localizerResults.gridByTag.count(tag));
		if (!_groundTruth.localizerResults.gridByTag.count(tag)) continue;

		const std::shared_ptr<PipelineGrid> groundTruthGrid = _groundTruth.localizerResults.gridByTag.at(tag);

		// GridFitter should always return a Pipeline for a TagCandidate
		assert(!bestCandidate.getGridsConst().empty());
		bool foundMatch = false;
		boost::optional<std::pair<double, std::reference_wrapper<const PipelineGrid>>> bestFoundGrid;
		for (const PipelineGrid& foundGrid : bestCandidate.getGridsConst())
		{
			const double score = groundTruthGrid->compare(foundGrid);

			// only continue if score is better than the score of the best found grid as of yet
			if (bestFoundGrid) {
				if (score <= bestFoundGrid.get().first) {
					continue;
				}
			}

			bestFoundGrid = { score, foundGrid };

			// compare ground truth grid to pipeline grid
			if (groundTruthGrid->compare(foundGrid) > 0.4)
			{
				_groundTruth.gridfitterResults.truePositives.push_back(foundGrid);
				foundMatch = true;
				break;
			}
		}

		// if no candidate has a acceptable score -> false positive
		assert(bestFoundGrid);
		if (!foundMatch) {
			_groundTruth.gridfitterResults.falsePositives.push_back(bestFoundGrid.get().second);
		}
	}

	// as long as GridFitter always returns a PipelineGrid for each candidate, false negatives for
	// GridFitter should be same as for the EllipseFitter
	_groundTruth.gridfitterResults.falseNegatives.insert(
	            _groundTruth.ellipsefitterResults.falseNegatives.begin(),
	            _groundTruth.ellipsefitterResults.falseNegatives.end());
}

void BeesBookImgAnalysisTracker::evaluateDecoder()
{
	assert(_groundTruth.available);
	DecoderEvaluationResults results;

	for (const auto gridTagPair : _groundTruth.localizerResults.gridByTag) {

		pipeline::Tag const& tag = gridTagPair.first;
		GroundTruthGridSPtr const& groundTruthGrid = gridTagPair.second;
		if (tag.getCandidatesConst().empty()) continue;

		boost::optional<DecoderEvaluationResults::result_t> bestResult;
		for (pipeline::TagCandidate const& gridCandidate : tag.getCandidatesConst()) {
			assert(!gridCandidate.getDecodings().empty());
			assert(gridCandidate.getDecodings().size() == gridCandidate.getGridsConst().size());
			if (gridCandidate.getDecodings().empty()) continue;

			for (size_t idx = 0; idx < gridCandidate.getDecodings().size(); ++idx) {
				pipeline::decoding_t decoding = gridCandidate.getDecodings()[idx];
				const PipelineGridRef pipelineGrid  = gridCandidate.getGridsConst()[idx];

				DecoderEvaluationResults::result_t result(pipelineGrid);

				result.boundingBox     = groundTruthGrid->getBoundingBox();

#ifdef SHIFT_DECODED_BITS
				util::rotateBitset(decoding, 3);
#endif

				result.decodedTagId    = decoding.to_ulong();
				result.decodedTagIdStr = decoding.to_string();

				result.groundTruthTagId = groundTruthGrid->getIdArray();

#ifdef SHIFT_DECODED_BITS
				std::rotate(result.groundTruthTagId.begin(),
				            result.groundTruthTagId.begin() + Grid::NUM_MIDDLE_CELLS - 3,
				            result.groundTruthTagId.end());
#endif

				std::stringstream groundTruthIdStr;
				for (int64_t idx = Grid::NUM_MIDDLE_CELLS - 1; idx >= 0; --idx) {
					groundTruthIdStr << result.groundTruthTagId[idx];
				}
				result.groundTruthTagIdStr = groundTruthIdStr.str();

				// hamming distance calculation
				result.hammingDistance = 0;
				for (size_t idx = 0; idx < Grid::NUM_MIDDLE_CELLS; ++idx) {
					if ((result.groundTruthTagId[idx] != decoding[idx]) &&
					    (!boost::indeterminate(result.groundTruthTagId[idx])))
					{
						++result.hammingDistance;
					}
				}

				if (!bestResult) {
					bestResult = result;
				} else {
					if (bestResult.get().hammingDistance > result.hammingDistance) {
						bestResult = result;
					}
				}
			}
		}

		if (bestResult) {
			results.evaluationResults.push_back(bestResult.get());
		}
	}

	_groundTruth.decoderResults = std::move(results);
}

std::pair<double, std::reference_wrapper<const pipeline::TagCandidate> > BeesBookImgAnalysisTracker::compareGrids(
        const pipeline::Tag &detectedTag,
        const std::shared_ptr<PipelineGrid> &grid) const {
	assert(!detectedTag.getCandidatesConst().empty());
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

		return res;
	};

	const std::vector<cv::Point> outerPoints = grid->getOuterRingEdgeCoordinates();
	boost::optional<PipelineTagCandidateRef> bestCandidate;
	double bestDeviation = std::numeric_limits<double>::max();

	for (pipeline::TagCandidate const& candidate : detectedTag.getCandidatesConst()) {
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
			bestCandidate = candidate;
		}
	}
	assert(bestCandidate);
	return {bestDeviation, bestCandidate.get()};
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

void BeesBookImgAnalysisTracker::paint(ProxyPaintObject &proxy, const TrackingAlgorithm::View &view)
{
	cv::Mat& image = proxy.getmat();

	// don't try to visualize results while data processing is running
	double similarity = 0;

	int i = findGridInGroundTruth();

	if ( i >= 0)
	{
		TrackedObject trObj = _groundTruth.data.getTrackedObjects().at(i);
		// get grid pointer of current object in list
		std::shared_ptr<Grid3D> pGrid = trObj.maybeGet<Grid3D>(getCurrentFrameNumber());

		// if the pointer is valid
		if ( pGrid )
		{
			// check if grids are sufficiently close to each other
			if ( cv::norm( (pGrid->getCenter() - _interactionGrid.getCenter()) ) < 10.0 )
			{
				PipelineGrid temp(pGrid->getCenter(), pGrid->getPixelRadius(), pGrid->getZRotation(), pGrid->getYRotation(), pGrid->getXRotation());
				similarity = _interactionGrid.compare(temp);
			}
		}
	}

	_interactionGrid.drawContours(image, 0.5, cv::Vec3b(static_cast<uint8_t>((1 - similarity) * 255), static_cast<uint8_t>(similarity*255.0), 0));
	_groundTruth.labelNumTruePositives->setText( QString::number(similarity, 'f', 2));

	if (_tagListLock.try_lock()) {
		// restore original ground truth labels after they have possibly been
		// modified by the decoder evaluation
		if (_selectedStage != BeesBookCommon::Stage::Decoder) {
			_groundTruth.labelFalsePositives->setText("False positives: ");
			_groundTruth.labelTruePositives->setText("True positives: ");
			_groundTruth.labelFalseNegatives->setText("False negatives: ");
			_groundTruth.labelRecall->setText("Recall: ");
			_groundTruth.labelPrecision->setText("Precision: ");
		}

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
			break;
		case BeesBookCommon::Stage::EllipseFitter:
			if ((view.name == "Canny Edge")
			    && (_visualizationData.ellipsefitterCannyEdge)) {
				image = rgbMatFromBwMat(
				            _visualizationData.ellipsefitterCannyEdge.get(),
				            image.type());
			}
			visualizeEllipseFitterOutput(image);
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

void BeesBookImgAnalysisTracker::paintOverlay(QPainter *painter)
{
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

void BeesBookImgAnalysisTracker::reset() {
}

void BeesBookImgAnalysisTracker::visualizePreprocessorOutput(cv::Mat &) const {
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
	case BeesBookCommon::Stage::EllipseFitter:
		_ellipsefitter.loadSettings(
		            BeesBookCommon::getEllipseFitterSettings(_settings));
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
	BeesBookCommon::getEllipseFitterSettings(_settings).addToPTree(pt);
	BeesBookCommon::getGridfitterSettings(_settings).addToPTree(pt);

	try {
		boost::property_tree::write_json(
		            dir.toStdString() + "/" + filename.toStdString() + ".json", pt);
		emit notifyGUI("config export successfully", MSGS::NOTIFICATION);
	} catch (std::exception &e) {
		emit notifyGUI("config export failed (" + std::string(e.what()) + ")", MSGS::FAIL);
	}

	return;
}

void BeesBookImgAnalysisTracker::loadTaglist()
{
	QString path = QFileDialog::getOpenFileName(nullptr, tr("Load taglist data"), "", tr("Data Files (*.dat)"));

	if (path.isEmpty())
		return;

	try {
		_taglist = loadSerializedTaglist(path.toStdString());

		_groundTruth.available = false;

		emit update();
	} catch (std::exception const& e) {
		std::stringstream msg;
		msg << "Unable to load tracking data." << std::endl << std::endl;
		msg << "Error: " << e.what() << std::endl;
		QMessageBox::warning(QApplication::activeWindow(), "Unable to load tracking data",
		                     QString::fromStdString(e.what()));
	}
}

void BeesBookImgAnalysisTracker::stageSelectionToogled( BeesBookCommon::Stage stage, bool checked)
{
	if (checked)
	{
		_selectedStage = stage;

		resetViews();
		switch (stage) {
		case BeesBookCommon::Stage::Preprocessor:
			setParamsWidget<PreprocessorParamsWidget>();
			emit registerViews( { { "Preprocessor Output" }, { "Opts" }, { "Honeyfilter" }, { "Threshold-Comb" } });
			break;
		case BeesBookCommon::Stage::Localizer:
			setParamsWidget<LocalizerParamsWidget>();
			emit registerViews( { { "Input" }, { "Threshold" }, { "Blobs" } });
			break;
		case BeesBookCommon::Stage::EllipseFitter:
			setParamsWidget<EllipseFitterParamsWidget>();
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


bool BeesBookImgAnalysisTracker::event(QEvent *event)
{
	const QEvent::Type etype = event->type();
	switch (etype) {
	case QEvent::KeyPress:
		keyPressEvent(static_cast<QKeyEvent*>(event));
		return true;
	case QEvent::MouseButtonPress:
		mousePressEvent(static_cast<QMouseEvent*>(event));
		return true;
	case QEvent::MouseButtonRelease:
		mouseReleaseEvent(static_cast<QMouseEvent*>(event));
		return true;
	case QEvent::MouseMove:
		mouseMoveEvent(static_cast<QMouseEvent*>(event));
		return true;
	default:
		event->ignore();
		return false;
	}
}


const std::set<Qt::Key> &BeesBookImgAnalysisTracker::grabbedKeys() const
{
	static const std::set<Qt::Key> keys {
		Qt::Key_W, Qt::Key_A,
		        Qt::Key_S, Qt::Key_D,
		        Qt::Key_Q, Qt::Key_E,
		        Qt::Key_R,
	};
	return keys;
}


void BeesBookImgAnalysisTracker::keyPressEvent(QKeyEvent *e)
{
	static const double rotateIncrement = 0.05;

	switch (e->key())
	{
	// rotate
	case Qt::Key_W:
	{
		_interactionGrid.setXRotation(_interactionGrid.getXRotation() + rotateIncrement);
		break;

	}
	case Qt::Key_S:
	{
		_interactionGrid.setXRotation(_interactionGrid.getXRotation() - rotateIncrement);
		break;
	}
	case Qt::Key_A:
	{
		_interactionGrid.setYRotation(_interactionGrid.getYRotation() + rotateIncrement);
		break;

	}
	case Qt::Key_D:
	{
		_interactionGrid.setYRotation(_interactionGrid.getYRotation() - rotateIncrement);
		break;
	}
	case Qt::Key_Q:
	{
		_interactionGrid.setZRotation(_interactionGrid.getZRotation() + rotateIncrement);
		break;

	}
	case Qt::Key_E:
	{
		_interactionGrid.setZRotation(_interactionGrid.getZRotation() - rotateIncrement);
		break;
	}
	case Qt::Key_R:
	{
		int i = findGridInGroundTruth();

		if ( i >= 0)
		{
			TrackedObject trObj = _groundTruth.data.getTrackedObjects().at(i);
			// get grid pointer of current object in list
			std::shared_ptr<Grid3D> pGrid = trObj.maybeGet<Grid3D>(getCurrentFrameNumber());

			// if the pointer is valid
			if ( pGrid )
			{
				// check if grids are sufficiently close to each other
				if ( cv::norm( (pGrid->getCenter() - _interactionGrid.getCenter()) ) < 10.0 )
				{
					_interactionGrid = PipelineGrid(pGrid->getCenter(), pGrid->getPixelRadius(), pGrid->getZRotation(), pGrid->getYRotation(), pGrid->getXRotation());
				}
			}
		}
		break;
	}
	default:
		break;
	} // END: switch (e->key())
	//} // END: _activeGrid

	// TODO: skip "emit update()" if event doesn't alter image (i.e. ctrl + 0)
	emit update();
}


// called when MOUSE BUTTON IS CLICKED
void BeesBookImgAnalysisTracker::mousePressEvent(QMouseEvent * e)
{
	// position of mouse cursor
	cv::Point mousePosition(e->x(), e->y());

	_interactionGrid.setCenter(mousePosition);

	emit update();
}

// called when mouse pointer MOVES
void BeesBookImgAnalysisTracker::mouseMoveEvent(QMouseEvent * e)
{
	// get current time
	const auto elapsed = std::chrono::system_clock::now() - _lastMouseEventTime;

	// slow down update rate to 1000 Hz max
	if (std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() > 10)
	{
		if (e->buttons() & Qt::LeftButton)
		{
			cv::Point mousePosition(e->x(), e->y());
			_interactionGrid.setCenter(mousePosition);
			emit update();
		}
		_lastMouseEventTime = std::chrono::system_clock::now();
	}
}

int BeesBookImgAnalysisTracker::findGridInGroundTruth()
{
	if(_groundTruth.available)
	{
		int framenum = getCurrentFrameNumber();
		std::vector<TrackedObject> GTObjects = _groundTruth.data.getTrackedObjects();

		//iterate over all ground truth objects
		//        for (const TrackedObject trObj : GTObjects)
		for (unsigned int i = 0; i < GTObjects.size(); i++)
		{
			TrackedObject trObj = GTObjects.at(i);
			// get grid pointer of current object in list
			std::shared_ptr<Grid3D> pGrid = trObj.maybeGet<Grid3D>(framenum);

			// if the pointer is valid
			if ( pGrid )
			{
				// check if grids are sufficiently close to each other
				if ( cv::norm( (pGrid->getCenter() - _interactionGrid.getCenter()) ) < 10.0 )
				{
					return i;
				}
			}
		}
	}
	return -1;
}

