#ifndef BeesBookImgAnalysisTracker_H
#define BeesBookImgAnalysisTracker_H

#include <mutex>

#include <opencv2/opencv.hpp>

#include <QApplication>
#include <QtWidgets/QLabel>

#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include "Common.h"
#include "ParamsWidget.h"
#include "pipeline/Converter.h"
#include "pipeline/Decoder.h"
#include "pipeline/GridFitter.h"
#include "pipeline/Localizer.h"
#include "pipeline/Recognizer.h"
#include "pipeline/Transformer.h"
#include "source/settings/Settings.h"
#include "source/tracking/TrackingAlgorithm.h"
#include "utility/stdext.h"

#include "source/tracking/serialization/SerializationData.h"

class Grid3D;

class BeesBookImgAnalysisTracker : public TrackingAlgorithm {
	Q_OBJECT
public:
	BeesBookImgAnalysisTracker(Settings& settings, QWidget* parent);

	void track(ulong frameNumber, cv::Mat& frame) override;
	void paint(cv::Mat& image) override;
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

	decoder::Converter _converter;
	decoder::Decoder _decoder;
	decoder::GridFitter _gridFitter;
	decoder::Recognizer _recognizer;
	decoder::Transformer _transformer;
	decoder::Localizer _localizer;

	std::vector<decoder::Tag> _taglist;
	std::mutex _tagListLock;

	struct LocalizerEvaluationResults {
		std::set<std::shared_ptr<Grid3D>> taggedGridsOnFrame;
		std::set<std::reference_wrapper<const decoder::Tag>> falsePositives;
		std::set<std::reference_wrapper<const decoder::Tag>> truePositives;
		std::set<std::shared_ptr<Grid3D>> falseNegatives;
		std::map<std::reference_wrapper<const decoder::Tag>, std::shared_ptr<Grid3D>> gridByTag;
	};

	struct RecognizerEvaluationResults {
		std::set<std::shared_ptr<Grid3D>> taggedGridsOnFrame;
		std::set<std::reference_wrapper<const decoder::Tag>> falsePositives;
		std::set<std::reference_wrapper<const decoder::Tag>> truePositives;
		std::set<std::shared_ptr<Grid3D>> falseNegatives;
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

	void visualizeLocalizerOutput(cv::Mat& image) const;
	void visualizeRecognizerOutput(cv::Mat& image) const;
	void visualizeGridFitterOutput(cv::Mat& image) const;
	void visualizeTransformerOutput(cv::Mat& image) const;
	void visualizeDecoderOutput(cv::Mat& image) const;

	void evaluateLocalizer();
	void evaluateRecognizer();

	int calculateVisualizationThickness() const;

	double compareGrids(const decoder::Tag& detectedTag, std::shared_ptr<Grid3D> const& grid) const;

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
