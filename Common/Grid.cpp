#include "Grid.h"

#include "utility/CvHelper.h"

const double Grid::INNER_RING_RADIUS  = 0.4;
const double Grid::MIDDLE_RING_RADIUS = 0.8;
const double Grid::OUTER_RING_RADIUS  = 1.0;
const double Grid::BULGE_FACTOR       = 0.4;
const double Grid::FOCAL_LENGTH       = 2.0;

const Grid::coordinates3D_t Grid::_coordinates3D = Grid::generate_3D_base_coordinates();

Grid::Grid(cv::Point2i center, double radius, double angle_z, double angle_y, double angle_x)
	: _coordinates2D(NUM_CELLS)
	, _center(center)
	, _radius(radius * FOCAL_LENGTH)
	, _angle_z(angle_z)
	, _angle_y(angle_y)
	, _angle_x(angle_x)
{
	prepare_visualization_data();
}

void Grid::setXRotation(double angle)
{
	_angle_x = angle;
	prepare_visualization_data();
}

void Grid::setYRotation(double angle)
{
	_angle_y = angle;
	prepare_visualization_data();
}

void Grid::setZRotation(double angle)
{
	_angle_z = angle;
	prepare_visualization_data();
}

void Grid::setCenter(cv::Point c)
{
	_center = c;
}

void Grid::setRadius(double radius)
{
	_radius = radius;
	prepare_visualization_data();
}

cv::Rect Grid::getBoundingBox() const
{
	return cv::Rect(_boundingBox.tl() + _center,
					_boundingBox.size());
}

Grid::coordinates3D_t Grid::generate_3D_base_coordinates() {

	typedef coordinates3D_t::value_type value_type;
	typedef coordinates3D_t::point_type point_type;

	coordinates3D_t result;

	// generate x,y coordinates
	for(size_t i = 0; i < POINTS_PER_RING; ++i)
	{
		// angle of a unit vector
		const value_type angle = i * 2.0 * CV_PI / static_cast<double>(POINTS_PER_RING);
		// unit vector
		const point_type p(
		  std::cos(angle),
		  std::sin(angle),
		  0.0
		);

		// scale unit vector to obtain three concentric rings in the plane (z = 0)
		result._inner_ring[i]  = p * INNER_RING_RADIUS;
		result._middle_ring[i] = p * MIDDLE_RING_RADIUS;
		result._outer_ring[i]  = p * OUTER_RING_RADIUS;
	}

	// span a line from one to the other side of the inner ring
	const double radiusInPoints = static_cast<double>(POINTS_PER_LINE / 2);
	for (size_t i = 0; i < POINTS_PER_LINE; ++i)
	{
		// distance of the point to center (sign is irrelevant in next line, so save the "abs()")
		const double y = (radiusInPoints - i) / radiusInPoints * INNER_RING_RADIUS;
		// the farther away, the deeper (away from the camera)
		const double z = - std::cos(BULGE_FACTOR * y);
		// save new coordinate
		result._inner_line[i] = cv::Point3d(0, y, z);
	}

	// generate z coordinates for the three rings
	{
		// all points on each ring have the same radius, thus should have the same z-value
		const value_type z_inner_ring  = - std::cos(BULGE_FACTOR * INNER_RING_RADIUS);
		const value_type z_middle_ring = - std::cos(BULGE_FACTOR * MIDDLE_RING_RADIUS);
		const value_type z_outer_ring  = - std::cos(BULGE_FACTOR * OUTER_RING_RADIUS);

		const double z_line_sum = std::accumulate(result._inner_line.begin(), result._inner_line.end(), 0.,
			[](double res, const cv::Point3d& val) { return (res + val.z); });

		// TODO: maybe reverse changes for ground truth data compatibility
		// mean z of all points in the ring (discard the few points on the line)
		const value_type mean = (z_inner_ring + z_middle_ring + z_outer_ring + z_line_sum) /
				(3.0 + POINTS_PER_LINE);

		// subtract mean, otherwise rotation will be eccentric
		for(size_t i = 0; i < POINTS_PER_RING; ++i)
		{
			result._inner_ring[i].z  = z_inner_ring  - z_outer_ring;
			result._middle_ring[i].z = z_middle_ring - z_outer_ring;
			result._outer_ring[i].z  = z_outer_ring  - z_outer_ring;
		}
		for (size_t i = 0; i < POINTS_PER_LINE; ++i)
		{
			result._inner_line[i].z -= z_outer_ring;
		}
	}

	return result;
}

Grid::coordinates2D_t Grid::generate_3D_coordinates_from_parameters_and_project_to_2D()
{
	// output variable
	coordinates2D_t result;

	const auto rotationMatrix = CvHelper::rotationMatrix(_angle_z, _angle_y, _angle_x);

	int minx = INT_MAX, miny = INT_MAX;
	int maxx = INT_MIN, maxy = INT_MIN;

	// iterate over all rings
	for (size_t r = 0; r < _coordinates3D._rings.size(); ++r)
	{
		// iterate over all points in ring
		for (size_t i = 0; i < _coordinates3D._rings[r].size(); ++i)
		{
			// rotate point (aka vector)
			const cv::Point3d p = rotationMatrix * _coordinates3D._rings[r][i];

			// project onto image plane
			const cv::Point2i projectedPoint(static_cast<int>(round((p.x / (p.z + FOCAL_LENGTH))  * _radius)),
											 static_cast<int>(round((p.y / (p.z + FOCAL_LENGTH)) * _radius)));

			// determine outer points of bounding box
			minx = std::min(minx, projectedPoint.x);
			miny = std::min(miny, projectedPoint.y);
			maxx = std::max(maxx, projectedPoint.x);
			maxy = std::max(maxy, projectedPoint.y);

			result._rings[r][i] = std::move(projectedPoint);
		}
	}

	// iterate over points of inner ring
	for (size_t i = 0; i < POINTS_PER_LINE; ++i)
	{
		// rotate point (aka vector)
		const cv::Point3d p = rotationMatrix * (_coordinates3D._inner_line[i]);

		// project onto image plane
		const cv::Point   p2(static_cast<int>(round((p.x / (p.z + FOCAL_LENGTH)) * _radius)),
							 static_cast<int>(round((p.y / (p.z + FOCAL_LENGTH)) * _radius)));

        minx = std::min(minx, p2.x);
        miny = std::min(miny, p2.y);
        maxx = std::max(maxx, p2.x);
        maxy = std::max(maxy, p2.y);

		result._inner_line[i] = p2;
	}

	_boundingBox = cv::Rect(minx, miny, maxx - minx, maxy - miny);

	return result;
}

void Grid::prepare_visualization_data()
{
	// apply rotations and scaling (the basic parameters)
	const auto points_2d = generate_3D_coordinates_from_parameters_and_project_to_2D();

	// outer ring
	{
		auto &vec = _coordinates2D[INDEX_OUTER_WHITE_RING];
		vec.clear();

		vec.insert(vec.end(), points_2d._outer_ring.cbegin(), points_2d._outer_ring.cend());
		vec.push_back(points_2d._outer_ring[0]); // add first point to close circle
	}

	// inner ring: white half
	{
		auto &vec = _coordinates2D[INDEX_INNER_WHITE_SEMICIRCLE];
		vec.clear();

		const size_t index_270_deg_begin = POINTS_PER_RING * 3 / 4;
		const size_t index_90_deg_end    = POINTS_PER_RING * 1 / 4 + 1;

		vec.insert(vec.end(), points_2d._inner_ring.cbegin() + index_270_deg_begin, points_2d._inner_ring.cend());
		vec.insert(vec.end(), points_2d._inner_ring.cbegin(), points_2d._inner_ring.cbegin() + index_90_deg_end);
	}

	// black semicircle plus curved center line
	{
		auto &vec = _coordinates2D[INDEX_INNER_BLACK_SEMICIRCLE];
		vec.clear();

		const size_t index_90_deg_begin = POINTS_PER_RING * 1 / 4;
		const size_t index_270_deg_end  = POINTS_PER_RING * 3 / 4 + 1;

		vec.insert(vec.end(), points_2d._inner_ring.cbegin() + index_90_deg_begin, points_2d._inner_ring.cbegin() + index_270_deg_end);
		vec.insert(vec.end(), points_2d._inner_line.crbegin(), points_2d._inner_line.crend());
	}

	// cells
	{
		for (size_t i = 0; i < NUM_MIDDLE_CELLS; ++i)
		{
			auto &vec = _coordinates2D[INDEX_MIDDLE_CELLS_BEGIN + i];
			vec.clear();

			const size_t index_begin = POINTS_PER_MIDDLE_CELL * i;
			const size_t index_end  =  POINTS_PER_MIDDLE_CELL * (i + 1);
			const size_t index_rbegin = POINTS_PER_RING - index_end;
			const size_t index_rend  =  POINTS_PER_RING - index_begin;
			const size_t index_end_elem  =  index_end < POINTS_PER_RING ? index_end : 0;

			vec.insert(vec.end(), points_2d._middle_ring.cbegin() + index_begin, points_2d._middle_ring.cbegin() + index_end);

			vec.push_back(points_2d._middle_ring[index_end_elem]);
			vec.push_back(points_2d._inner_ring[index_end_elem]);

			vec.insert(vec.end(), points_2d._inner_ring.rbegin() + index_rbegin, points_2d._inner_ring.rbegin() + index_rend);
		}
	}
}
