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

class PipelineGrid;

class BeesBookImgAnalysisTracker : public TrackingAlgorithm {
	Q_OBJECT
public:
    BeesBookImgAnalysisTracker(Settings& settings);

	void track(ulong frameNumber, const cv::Mat& frame) override;
    virtual void paint(cv::Mat &image, View const& = OriginalView) override;
    virtual void paintOverlay(QPainter *painter, View const& = OriginalView) override;

    /*
	std::shared_ptr<QWidget> getParamsWidget() override {
		return _paramsWidget;
	}
    */
	std::shared_ptr<QWidget> getToolsWidget() override {
		return _toolsWidget;
	}

	// return keys that are handled by the tracker
	std::set<Qt::Key> const& grabbedKeys() const override;

	void setGroundTruthStats(const size_t numGroundTruth, const size_t numTruePositives, const size_t numFalsePositives, const size_t numFalseNegatives) const;

	QPen getDefaultPen(QPainter *painter) const;
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
	std::mutex _tagListLock;

	typedef std::vector<pipeline::Tag> taglist_t;
	taglist_t _taglist;

	taglist_t loadSerializedTaglist(std::string const& path);

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

	boost::optional<GroundTruthEvaluation> _groundTruthEvaluation;

	struct
	{
		QLabel* labelNumFalsePositives;
		QLabel* labelNumFalseNegatives;
		QLabel* labelNumTruePositives;
		QLabel* labelNumRecall;
		QLabel* labelNumPrecision;

		QLabel* labelFalsePositives;
		QLabel* labelFalseNegatives;
		QLabel* labelTruePositives;
		QLabel* labelRecall;
		QLabel* labelPrecision;
	} _groundTruthWidgets;

	void visualizePreprocessorOutput(cv::Mat& image) const;
    void visualizePreprocessorOutputOverlay(QPainter *painter) const;
    void visualizeLocalizerOutputOverlay(QPainter *painter) const;
	void visualizeEllipseFitterOutput(cv::Mat& image) const;
    void visualizeEllipseFitterOutputOverlay(QPainter *painter) const;
	void visualizeGridFitterOutput(cv::Mat& image) const;
    void visualizeGridFitterOutputOverlay(QPainter *painter) const;
	void visualizeDecoderOutput(cv::Mat& image) const;
    void visualizeDecoderOutputOverlay(QPainter *painter) const;

	void drawBox(const cv::Rect& box, QPainter *painter, QPen &pen) const;
	void drawEllipse(const pipeline::Tag& tag, QPen &pen, QPainter *painter, const pipeline::Ellipse& ellipse) const;

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

	int findGridInGroundTruth();

	void resetViews();

protected:
	bool event(QEvent* event) override;

private Q_SLOTS:
	void stageSelectionToogled(BeesBookCommon::Stage stage, bool checked);
	void settingsChanged(const BeesBookCommon::Stage stage);
	void loadGroundTruthData();
	void exportConfiguration();
	void loadTaglist();
};
