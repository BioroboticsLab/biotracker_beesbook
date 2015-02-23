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
#include "pipeline/Recognizer.h"
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
	void paint(cv::Mat& image, View const& view = OriginalView) override;
	void reset() override;

	std::shared_ptr<QWidget> getParamsWidget() override {
		return _paramsWidget;
	}
	std::shared_ptr<QWidget> getToolsWidget() override {
		return _toolsWidget;
	}

private:
	BeesBookCommon::Stage _selectedStage;

	const std::shared_ptr<ParamsWidget> _paramsWidget;
	const std::shared_ptr<QWidget> _toolsWidget;

	pipeline::Preprocessor _preprocessor;
	pipeline::Localizer _localizer;
	pipeline::Recognizer _recognizer;
	pipeline::GridFitter _gridFitter;
	pipeline::Decoder _decoder;

	cv::Mat _image;
	std::vector<pipeline::Tag> _taglist;
	std::mutex _tagListLock;

	struct {
		boost::optional<cv::Mat> preprocessorImage;
		boost::optional<cv::Mat> preprocessorThresholdImage;
		boost::optional<cv::Mat> localizerInputImage;
		boost::optional<cv::Mat> localizerThresholdImage;
		boost::optional<cv::Mat> localizerSobelImage;
		boost::optional<cv::Mat> localizerBlobImage;
		boost::optional<cv::Mat> recognizerCannyEdge;

		// these references are just stored for convenience in order to invalidate all
		// visualizations in a loop
		typedef std::array<std::reference_wrapper<boost::optional<cv::Mat>>, 7> reference_array_t;
		reference_array_t visualizations = reference_array_t{preprocessorImage,
			preprocessorThresholdImage, localizerInputImage, localizerThresholdImage,
				localizerSobelImage, localizerBlobImage, recognizerCannyEdge };
	} _visualizationData;

	struct LocalizerEvaluationResults {
		std::set<std::shared_ptr<PipelineGrid>> taggedGridsOnFrame;
		std::set<std::reference_wrapper<const pipeline::Tag>> falsePositives;
		std::set<std::reference_wrapper<const pipeline::Tag>> truePositives;
		std::set<std::shared_ptr<PipelineGrid>> falseNegatives;
		std::map<std::reference_wrapper<const pipeline::Tag>, std::shared_ptr<PipelineGrid>> gridByTag;
	};

	struct RecognizerEvaluationResults {
		std::set<std::shared_ptr<PipelineGrid>> taggedGridsOnFrame;
		std::set<std::reference_wrapper<const pipeline::Tag>> falsePositives;
		std::vector<std::pair<std::reference_wrapper<const pipeline::Tag>, std::reference_wrapper<const pipeline::TagCandidate>>> truePositives;
		std::set<std::shared_ptr<PipelineGrid>> falseNegatives;
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
		RecognizerEvaluationResults recognizerResults;
	} _groundTruth;

	void visualizePreprocessorOutput(cv::Mat& image) const;
	void visualizeLocalizerOutput(cv::Mat& image) const;
	void visualizeRecognizerOutput(cv::Mat& image) const;
	void visualizeGridFitterOutput(cv::Mat& image) const;
	void visualizeDecoderOutput(cv::Mat& image) const;

	void evaluateLocalizer();
	void evaluateRecognizer();
	void evaluateGridfitter();
	void evaluateDecoder();

	int calculateVisualizationThickness() const;

	std::pair<double, std::reference_wrapper<const pipeline::TagCandidate>> compareGrids(const pipeline::Tag& detectedTag, std::shared_ptr<PipelineGrid> const& grid) const;

	cv::Mat rgbMatFromBwMat(const cv::Mat& mat, const int type) const;

	template<typename Widget>
	void setParamsWidget() {
		std::unique_ptr<Widget> widget(std::make_unique<Widget>(_settings));
		QObject::connect(widget.get(), &Widget::settingsChanged,
		  this, &BeesBookImgAnalysisTracker::settingsChanged);
		_paramsWidget->setParamSubWidget(std::move(widget));
	}

private slots:
	void stageSelectionToogled(BeesBookCommon::Stage stage, bool checked);
	void settingsChanged(const BeesBookCommon::Stage stage);
	void loadGroundTruthData();
};

#endif
