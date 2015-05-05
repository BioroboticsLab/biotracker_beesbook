#include "InteractiveGrid.h"

#include <numeric>

#include "utility/CvHelper.h"

#include <source/tracking/serialization/types.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/array.hpp>

#include <cereal/archives/json.hpp>
#include <cereal/types/polymorphic.hpp>

InteractiveGrid::InteractiveGrid()
	: InteractiveGrid(cv::Point2i(0, 0), 0., 0., 0., 0.)
{}

InteractiveGrid::InteractiveGrid(cv::Point2i center, double radius_px, double angle_z, double angle_y, double angle_x)
	: Grid(center, radius_px, angle_z, angle_y, angle_x)
	, _transparency(0.5)
	, _bitsTouched(false)
	, _isSettable(true)
{
	prepare_visualization_data();
}

InteractiveGrid::~InteractiveGrid() = default;

/**
 * draws the tag at position center in image dst
 *
 * @param img dst
 * @param center center of the tag
 */
void InteractiveGrid::draw(cv::Mat &img, const cv::Point &center, const bool isActive) const {
	static const cv::Scalar white(255, 255, 255);
	static const cv::Scalar black(0, 0, 0);
	static const cv::Scalar red(0, 0, 255);
	static const cv::Scalar yellow(0, 255, 255);

	const cv::Scalar &outerColor = isActive ? yellow : white;

	for (size_t i = INDEX_MIDDLE_CELLS_BEGIN; i < INDEX_MIDDLE_CELLS_BEGIN + NUM_MIDDLE_CELLS; ++i)
	{
		CvHelper::drawPolyline(img, _coordinates2D, i, white, false, center);
	}
	CvHelper::drawPolyline(img, _coordinates2D, INDEX_OUTER_WHITE_RING,       outerColor, false, center);
	CvHelper::drawPolyline(img, _coordinates2D, INDEX_INNER_WHITE_SEMICIRCLE, white,      false, center);
	CvHelper::drawPolyline(img, _coordinates2D, INDEX_INNER_BLACK_SEMICIRCLE, black,      false, center);

	for (size_t i = 0; i < NUM_MIDDLE_CELLS; ++i)
	{
		cv::Scalar color = tribool2Color(_ID[i]);
		cv::circle(img, _interactionPoints[i] + center, 1, color);
	}
	cv::circle(img, _interactionPoints.back() + center, 1, red);
}

/**
* draw grid on image. this function implements the transparency feature. 
*/
void InteractiveGrid::draw(cv::Mat &img, const bool isActive) const
{
	const int radius = static_cast<int>(std::ceil(_radius));
	const cv::Point subimage_origin( std::max(       0, _center.x - radius), std::max(       0, _center.y - radius) );
	const cv::Point subimage_end   ( std::min(img.cols, _center.x + radius), std::min(img.rows, _center.y + radius) );

	// draw only if subimage has a valid size (i.e. width & height > 0)
	if (subimage_origin.x < subimage_end.x && subimage_origin.y < subimage_end.y)
	{
		const cv::Point subimage_center( std::min(radius, _center.x), std::min(radius, _center.y) );

		cv::Mat subimage      = img( cv::Rect(subimage_origin, subimage_end) );
		cv::Mat subimage_copy = subimage.clone();

		draw(subimage_copy, subimage_center, isActive);

		cv::addWeighted(subimage_copy, _transparency, subimage, 1.0 - _transparency, 0.0, subimage);
	}
}

void InteractiveGrid::setTransparency(float transparency) {
	if (transparency < 0.0 || transparency > 1.0) {
		throw std::invalid_argument("transparency not in range[0.0, 1.0]");
	}
    _transparency = transparency;
}

/**
* interate over keypoints and return the first close enough to point
*/
int InteractiveGrid::getKeyPointIndex(cv::Point p) const
{
	for (size_t i = 0; i < _interactionPoints.size(); ++i)
	{
		if (cv::norm(_center + _interactionPoints[i] - p) < (_radius / 10) )
			return static_cast<int>(i);
	}
	return -1;
}

void InteractiveGrid::toggleIdBit(size_t cell_id, bool indeterminate)
{ 
    _bitsTouched = true;

    // if set to indeterminate, switch it to true, because we want to flip the bit in the next line
	if (_ID[cell_id].value == boost::logic::tribool::value_t::indeterminate_value)
		_ID[cell_id] = true;

	_ID[cell_id] = indeterminate ? boost::logic::indeterminate : !_ID[cell_id]; 
}

cv::Scalar InteractiveGrid::tribool2Color(const boost::logic::tribool &tribool) const
{
	int value;
	switch (tribool.value) {
	case boost::logic::tribool::value_t::true_value:
		value = 255;
		break;
	case boost::logic::tribool::value_t::indeterminate_value:
		value = static_cast<int>(0.5 * 255);
		break;
	case boost::logic::tribool::value_t::false_value:
		value = 0;
		break;
	default:
		assert(false);
		value = 0;
		break;
	}

	return cv::Scalar(value, value, value);
}

void InteractiveGrid::zRotateTowardsPointInPlane(cv::Point p)
{
    // still seems to flutter when heavily rotated ... hmmm ..

	// vector of grid center to mouse pointer
    cv::Point d_p = (p - _center);
    
    // angular bisection of current orientation
    double d_a = fmod( _angle_z - atan2(d_p.y, d_p.x), 2*CV_PI );
    d_a = (d_a > CV_PI)     ? d_a - CV_PI: d_a;
    d_a = (d_a < -CV_PI)    ? d_a + CV_PI: d_a;

    // current rotation axis
    cv::Point2d axis0(_angle_x, _angle_y);

    // new rotation axis
    cv::Point2d axis(cos(-d_a) * _angle_x + sin(-d_a) * _angle_y, -sin(-d_a) * _angle_x + cos(-d_a) * _angle_y);
    
    // if rotation axis is rotated to far, flip it back. 
    // otherwise the tag is pitched into the other direction
    if (axis0.dot(axis) < 0)
        axis = -axis;

    // update rotation parameters
    _angle_x = axis.x;
    _angle_y = axis.y;
    _angle_z = atan2(d_p.y, d_p.x);

    prepare_visualization_data();
}

void InteractiveGrid::xyRotateIntoPlane(float angle_y, float angle_x)
{
	_angle_x = angle_x;
	_angle_y = angle_y;
	prepare_visualization_data();
}

void InteractiveGrid::toggleTransparency()
{
	_transparency = std::abs(_transparency - 0.6f);
}

void InteractiveGrid::setWorldRadius(const double radius)
{
	_radius = radius;
	prepare_visualization_data();
}

Grid::coordinates2D_t InteractiveGrid::generate_3D_coordinates_from_parameters_and_project_to_2D()
{
	coordinates2D_t result = Grid::generate_3D_coordinates_from_parameters_and_project_to_2D();

	// iterate over all rings
	for (size_t r = 0; r < _coordinates3D._rings.size(); ++r)
	{
		// iterate over all points in ring
		for (size_t i = 0; i < _coordinates3D._rings[r].size(); ++i)
		{
			if (r == MIDDLE_RING) {
				if ( (i % POINTS_PER_MIDDLE_CELL) == POINTS_PER_MIDDLE_CELL / 2 ) {
					_interactionPoints.push_back(0.5*(result._rings[r][i] + result._rings[r - 1][i]));
				}
			}
		}
	}

	// iterate over points of inner ring
	for (size_t i = 0; i < POINTS_PER_LINE; ++i)
	{
		// if center point reached: save point to interaction-points-list
		if (i == POINTS_PER_LINE / 2)
		{
			_interactionPoints.push_back(result._inner_line[i]);
		}

	}

	// the last interaction point is P1
	_interactionPoints.push_back(result._outer_ring[0]);

	return result;
}



CEREAL_REGISTER_TYPE(InteractiveGrid)
