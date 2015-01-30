#ifndef InteractiveGridH_
#define InteractiveGridH_

#include <vector>                  // std::vector
#include <array>                   // std::array
#include <opencv2/opencv.hpp>      // cv::Mat, cv::Point3_
#include <boost/logic/tribool.hpp> // boost::tribool

#include "source/tracking/algorithm/BeesBook/Common/Grid.h"
#include "source/tracking/serialization/ObjectModel.h"

class InteractiveGrid : public Grid, public ObjectModel
{
public:
	// default constructor, required for serialization
	explicit InteractiveGrid();
	explicit InteractiveGrid(cv::Point2i center, double radius, double angle_z, double angle_y, double angle_x);
	virtual ~InteractiveGrid() override;

	/**
	 * draws 2D projection of 3D-mesh on image
	 */
	void	draw(cv::Mat &img, const bool isActive) const;

	void	zRotateTowardsPointInPlane(cv::Point p);
	void	xyRotateIntoPlane(float angle_y, float angle_x);

	int		getKeyPointIndex(cv::Point p) const;

	void	toggleIdBit(size_t cell_id, bool indeterminate);
	cv::Scalar tribool2Color(const boost::logic::tribool &tribool) const;

	void	toggleTransparency();

	double	getPixelRadius() const { return _radius / FOCAL_LENGTH; }

	double	getWorldRadius() const { return _radius; }
	void	setWorldRadius(const double radius);

	boost::tribool hasBeenBitToggled() const { return _bitsTouched; }
	void setBeenBitToggled(const boost::logic::tribool::value_t toggled) { _bitsTouched.value = toggled; }

	bool    isSettable() const { return _isSettable; }
	void    setSettable(const bool settable) { _isSettable = settable; }

	float getTransparency() const { return _transparency; }
	void  setTransparency(float transparency);

	/**
	 * Axis-aligned minimum bounding box of the grid
	 */
	cv::Rect getBoundingBox() const;

	/**
	 * Axis-aligned minimum bounding box of the grid centered at (0, 0)
	 */
	cv::Rect getOriginBoundingBox() const {
		return _boundingBox;
	}

	std::vector<cv::Point> const& getOuterRingPoints() const { return _coordinates2D[OUTER_RING]; }

private:
	virtual coordinates2D_t generate_3D_coordinates_from_parameters_and_project_to_2D() override;

	void draw(cv::Mat &img, const cv::Point& center, const bool isActive) const;

	std::vector<cv::Point>              _interactionPoints; // 2D coordinates of interaction points (center of grid, grid cell centers, etc)
	float                               _transparency;      // weight in drawing mixture
	boost::tribool                      _bitsTouched;       // if at least one bit was set, this is true, after copy & paste indeterminate
	bool                                _isSettable;        // if tag can be recognized by a human

	// generate serialization functions
	friend class cereal::access;
	template <class Archive>
	void save(Archive& ar) const
	{
		ar(CEREAL_NVP(_center),
		   CEREAL_NVP(_radius),
		   CEREAL_NVP(_angle_z),
		   CEREAL_NVP(_angle_y),
		   CEREAL_NVP(_angle_x),
		   CEREAL_NVP(_ID),
		   CEREAL_NVP(_bitsTouched),
		   CEREAL_NVP(_isSettable));
	}

	template<class Archive>
	void load(Archive& ar)
	{
		ar(CEREAL_NVP(_center),
		   CEREAL_NVP(_radius),
		   CEREAL_NVP(_angle_z),
		   CEREAL_NVP(_angle_y),
		   CEREAL_NVP(_angle_x),
		   CEREAL_NVP(_ID),
		   CEREAL_NVP(_bitsTouched),
		   CEREAL_NVP(_isSettable));

		prepare_visualization_data();
	}
};

#endif
