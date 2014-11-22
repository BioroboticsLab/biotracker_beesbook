#ifndef BeesBookImgAnalysisTracker_H
#define BeesBookImgAnalysisTracker_H

#include <opencv2/opencv.hpp>

#include "source/settings/Settings.h"
#include "source/tracking/TrackingAlgorithm.h"

class BeesBookImgAnalysisTracker : public TrackingAlgorithm {
	Q_OBJECT
  public:
	BeesBookImgAnalysisTracker(Settings& settings, std::string& serializationPathName, QWidget* parent);

	void track(ulong frameNumber, cv::Mat& frame) override;
	void paint(cv::Mat& image) override;
	void reset() override;

	std::shared_ptr<QWidget> getParamsWidget() override { return _paramsWidget; }
	std::shared_ptr<QWidget> getToolsWidget() override { return _toolsWidget; }

  public slots:
	//mouse click and move events
	void mouseMoveEvent(QMouseEvent*) override {}
	void mousePressEvent(QMouseEvent*) override {}
	void mouseReleaseEvent(QMouseEvent*) override {}
	void mouseWheelEvent(QWheelEvent*) override {}

  private:
	const std::shared_ptr<QWidget> _paramsWidget;
	const std::shared_ptr<QWidget> _toolsWidget;
};

#endif
