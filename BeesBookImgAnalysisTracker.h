#ifndef BeesBookImgAnalysisTracker_H
#define BeesBookImgAnalysisTracker_H

#include <array>
#include <mutex>

#include <opencv2/opencv.hpp>

#include <QApplication>
#include <QtWidgets/QLabel>

#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include "Common.h"
#include "ParamsWidget.h"
#include "pipeline/Preprocessor.h"
#include "pipeline/Localizer.h"
#include "pipeline/EllipseFitter.h"
#include "pipeline/GridFitter.h"
#include "pipeline/Decoder.h"
#include "source/settings/Settings.h"
#include "source/tracking/TrackingAlgorithm.h"
#include "utility/stdext.h"

#include "source/tracking/serialization/SerializationData.h"

class PipelineGrid;

class BeesBookImgAnalysisTracker : public TrackingAlgorithm {
	Q_OBJECT
public:
	BeesBookImgAnalysisTracker(Settings& settings, QWidget* parent);

	void track(ulong frameNumber, cv::Mat& frame) override;
	void paint(ProxyPaintObject& proxy, View const& view = OriginalView) override;
	void reset() override;

	std::shared_ptr<QWidget> getParamsWidget() override {
		return _paramsWidget;
	}
	std::shared_ptr<QWidget> getToolsWidget() override {
		return _toolsWidget;
	}

	// return keys that are handled by the tracker
	std::set<Qt::Key> const& grabbedKeys() const override;

private:
	// a grid to interact with
	PipelineGrid          _interactionGrid;

	BeesBookCommon::Stage _selectedStage;

	const std::shared_ptr<ParamsWidget> _paramsWidget;
	const std::shared_ptr<QWidget>      _toolsWidget;

	pipeline::Preprocessor  _preprocessor;
	pipeline::Localizer     _localizer;
	pipeline::EllipseFitter _ellipsefitter;
	pipeline::GridFitter    _gridFitter;
	pipeline::Decoder       _decoder;

	cv::Mat _image;
	std::vector<pipeline::Tag> _taglist;
	std::mutex _tagListLock;

	static const size_t NUM_MIDDLE_CELLS = 12;
	typedef std::array<boost::tribool, NUM_MIDDLE_CELLS> idarray_t;

	struct
	{
		boost::optional<cv::Mat> preprocessorImage;
		boost::optional<cv::Mat> preprocessorOptImage;
		boost::optional<cv::Mat> preprocessorHoneyImage;
		boost::optional<cv::Mat> preprocessorThresholdImage;
		boost::optional<cv::Mat> localizerInputImage;
		boost::optional<cv::Mat> localizerThresholdImage;
		boost::optional<cv::Mat> localizerSobelImage;
		boost::optional<cv::Mat> localizerBlobImage;
		boost::optional<cv::Mat> ellipsefitterCannyEdge;

		// these references are just stored for convenience in order to invalidate all
		// visualizations in a loop
		typedef std::array<std::reference_wrapper<boost::optional<cv::Mat>>, 7> reference_array_t;
		
		reference_array_t visualizations = reference_array_t
		{ preprocessorImage,
		  preprocessorThresholdImage,
		  localizerInputImage,
		  localizerThresholdImage,
		  localizerSobelImage,
		  localizerBlobImage,
		  ellipsefitterCannyEdge };
	} _visualizationData;

	struct LocalizerEvaluationResults 		{
		std::set<std::shared_ptr<PipelineGrid>> taggedGridsOnFrame;
		std::set<std::reference_wrapper<const pipeline::Tag>> falsePositives;
		std::set<std::reference_wrapper<const pipeline::Tag>> truePositives;
		std::set<std::shared_ptr<PipelineGrid>> falseNegatives;
		std::map<std::reference_wrapper<const pipeline::Tag>, std::shared_ptr<PipelineGrid>> gridByTag;
	};

	struct EllipseFitterEvaluationResults
	{
		std::set<std::shared_ptr<PipelineGrid>> taggedGridsOnFrame;
		std::set<std::reference_wrapper<const pipeline::Tag>> falsePositives;

		//mapping of pipeline tag to its best ellipse (only if it matches ground truth ellipse)
		std::vector<std::pair<std::reference_wrapper<const pipeline::Tag>, std::reference_wrapper<const pipeline::TagCandidate>>> truePositives;
		std::set<std::shared_ptr<PipelineGrid>> falseNegatives;
	};

	struct GridFitterEvaluationResults
	{
		unsigned int matches        = 0;
		unsigned int mismatches     = 0;
		std::vector<std::reference_wrapper<const PipelineGrid>> truePositives;
		std::vector<std::reference_wrapper<const PipelineGrid>> falsePositives;
	};

	struct DecoderEvaluationResults {
		typedef struct {
			cv::Rect boundingBox;
			int decodedTagId;
			std::string decodedTagIdStr;
			idarray_t groundTruthTagId;
			std::string groundTruthTagIdStr;
			int hammingDistance;
		} result_t;
		std::vector<result_t> evaluationResults;
	};

	struct {
		bool available = false;
		Serialization::Data data;

		QLabel* labelFalsePositives = nullptr;
		QLabel* labelFalseNegatives = nullptr;
		QLabel* labelTruePositives  = nullptr;
		QLabel* labelRecall         = nullptr;
		QLabel* labelPrecision      = nullptr;

		QLabel* labelNumFalsePositives = nullptr;
		QLabel* labelNumFalseNegatives = nullptr;
		QLabel* labelNumTruePositives  = nullptr;
		QLabel* labelNumRecall         = nullptr;
		QLabel* labelNumPrecision      = nullptr;

		LocalizerEvaluationResults localizerResults;
		EllipseFitterEvaluationResults ellipsefitterResults;
		GridFitterEvaluationResults gridfitterResults;
		DecoderEvaluationResults decoderResults;
	} _groundTruth;

	void visualizePreprocessorOutput(cv::Mat& image) const;
	void visualizeLocalizerOutput(cv::Mat& image) const;
	void visualizeEllipseFitterOutput(cv::Mat& image) const;
	void visualizeGridFitterOutput(cv::Mat& image) const;
	void visualizeDecoderOutput(cv::Mat& image) const;

	void evaluateLocalizer();
	void evaluateEllipseFitter();
	void evaluateGridfitter();
	void evaluateDecoder();

	int calculateVisualizationThickness() const;

	std::pair<double, std::reference_wrapper<const pipeline::TagCandidate>> compareGrids(const pipeline::Tag& detectedTag, std::shared_ptr<PipelineGrid> const& grid) const;
	int compareDecodings(pipeline::Tag &detectedTag, const std::shared_ptr<PipelineGrid> &grid) const;

	cv::Mat rgbMatFromBwMat(const cv::Mat& mat, const int type) const;

	template<typename Widget>
	void setParamsWidget() {
		std::unique_ptr<Widget> widget(std::make_unique<Widget>(_settings));
		QObject::connect(widget.get(), &Widget::settingsChanged,
		                 this, &BeesBookImgAnalysisTracker::settingsChanged);
		_paramsWidget->setParamSubWidget(std::move(widget));
	}

	std::chrono::system_clock::time_point _lastMouseEventTime;

	void keyPressEvent(QKeyEvent *e) override;
	void mouseMoveEvent    (QMouseEvent * e) override;
	void mousePressEvent   (QMouseEvent * e) override;

	int findGridInGroundTruth(PipelineGrid & templateGrid);

protected:
	bool event(QEvent* event) override;

private slots:
	void stageSelectionToogled(BeesBookCommon::Stage stage, bool checked);
	void settingsChanged(const BeesBookCommon::Stage stage);
	void loadGroundTruthData();
	void exportConfiguration();
};

#endif
