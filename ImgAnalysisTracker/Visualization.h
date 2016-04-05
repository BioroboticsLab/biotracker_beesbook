#pragma once

#include <opencv2/core/core.hpp>
#include <QPainter>
#include <QPen>

namespace pipeline {
class Ellipse;
class Tag;
}

namespace Visualization {

void drawEllipse(const pipeline::Tag &tag, QPen &pen, QPainter *painter, const pipeline::Ellipse &ellipse);

void drawBox(const cv::Rect &box, QPainter *painter, QPen &pen);

cv::Mat rgbMatFromBwMat(const cv::Mat &mat, const int type);

}
