#include "Visualization.h"

#include <biotracker/util/CvHelper.h>
#include <pipeline/datastructure/Ellipse.h>
#include <pipeline/datastructure/Tag.h>

namespace BC = BioTracker::Core;

namespace Visualization {

void drawEllipse(const pipeline::Tag &tag, QPen &pen, QPainter *painter, const pipeline::Ellipse &ellipse) {
    static const QPoint offset(20, -20);

    cv::RotatedRect ellipseBox(ellipse.getCen(), ellipse.getAxis(), static_cast<float>(ellipse.getAngle()));

    //get ellipse definition
    QPoint center = BC::CvHelper::toQt(ellipse.getCen());
    qreal rx = static_cast<qreal>(ellipse.getAxis().width);
    qreal ry = static_cast<qreal>(ellipse.getAxis().height);

    //draw ellipse
    painter->save();
    painter->translate(tag.getRoi().x +center.x() ,tag.getRoi().y+center.y());
    painter->rotate(-ellipse.getAngle());
    painter->drawEllipse(QPointF(0,0),rx,ry);
    painter->restore();

    //draw score
    painter->setPen(pen);
    painter->save();
    painter->translate(tag.getRoi().x,tag.getRoi().y);
    painter->drawText(BC::CvHelper::toQt(ellipseBox.boundingRect().tl()) + offset,
                      "Score: " + QString::number(ellipse.getVote()));
    painter->restore();
}

void drawBox(const cv::Rect &box, QPainter *painter, QPen &pen) {
    const QRect qbox = BC::CvHelper::toQt(box);
    painter->setPen(pen);
    painter->drawRect(qbox);
}

cv::Mat rgbMatFromBwMat(const cv::Mat &mat, const int type) {
    // TODO: convert B&W to RGB
    // this could probably be implemented more efficiently
    cv::Mat image;
    cv::Mat channels[3] = { mat.clone(), mat.clone(), mat.clone() };
    cv::merge(channels, 3, image);
    image.convertTo(image, type);
    return image;
}

}
