#include "BeesBookImgAnalysisTracker.h"

#include "source/tracking/algorithm/algorithms.h"

namespace {
auto _ = Algorithms::Registry::getInstance().register_tracker_type<BeesBookImgAnalysisTracker>("BeesBook ImgAnalysis");
}


BeesBookImgAnalysisTracker::BeesBookImgAnalysisTracker(Settings &settings, std::string &serializationPathName, QWidget *parent)
    : TrackingAlgorithm(settings, serializationPathName, parent)
{}

void BeesBookImgAnalysisTracker::track(ulong frameNumber, cv::Mat &frame)
{}

void BeesBookImgAnalysisTracker::paint(cv::Mat &image)
{}

void BeesBookImgAnalysisTracker::reset()
{}
