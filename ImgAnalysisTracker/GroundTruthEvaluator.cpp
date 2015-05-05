#include "GroundTruthEvaluator.h"

#include "pipeline/util/Util.h"
#include "pipeline/datastructure/PipelineGrid.h"
#include "pipeline/datastructure/PipelineGrid.impl.h"
#include "pipeline/datastructure/Tag.h"
#include "pipeline/datastructure/TagCandidate.h"

#include "legacy/Grid3D.h"

GroundTruthEvaluation::GroundTruthEvaluation(Serialization::Data &&groundTruthData)
    : _groundTruthData(std::move(groundTruthData))
{
}

void GroundTruthEvaluation::evaluateLocalizer(const int currentFrameNumber, taglist_t const& taglist)
{
	GroundTruth::LocalizerEvaluationResults& results = _localizerResults;

	// iterate over ground truth data (typed Grid3D), convert them to PipelineGrids
	// and store them in the std::set taggedGridsOnFrame
	for (TrackedObject const& object : _groundTruthData.getTrackedObjects())
	{
		const std::shared_ptr<Grid3D> grid3d = object.maybeGet<Grid3D>(currentFrameNumber);
		if (!grid3d)
			continue;
		// convert to PipelineGrid
		const auto grid = std::make_shared<PipelineGrid>(
		            grid3d->getCenter(), grid3d->getPixelRadius(),
		            grid3d->getZRotation(), grid3d->getYRotation(),
		            grid3d->getXRotation());
		grid->setIdArray(grid3d->getIdArray());

		// insert in set
		if (grid)
			results.taggedGridsOnFrame.insert(grid);
	}

	// Detect False Negatives
	for (const std::shared_ptr<PipelineGrid>& grid : results.taggedGridsOnFrame)
	{
		// ROI of ground truth
		const cv::Rect gridBox = grid->getBoundingBox();

		bool inGroundTruth = false;
		for (const pipeline::Tag& tag : taglist)
		{
			// ROI of pipeline tag
			const cv::Rect& tagBox = tag.getBox();

			// target property is complete containment
			if (tagBox.contains(gridBox.tl())
			    && tagBox.contains(gridBox.br()))
			{
				inGroundTruth = true;
				break;
			}
		}

		// mark current ground truth box as missing detection (false negative)
		if (!inGroundTruth)
			results.falseNegatives.insert(grid);
	}

	// Detect False Positives
	// ground truth grids
	std::set<std::shared_ptr<PipelineGrid>> notYetDetectedGrids = results.taggedGridsOnFrame;

	// iterate over all pipeline tags
	for (const pipeline::Tag& tag : taglist)
	{
		// ROI of pipeline tag
		const cv::Rect& tagBox = tag.getBox();

		bool inGroundTruth = false;

		// iterate over all ground truth grids
		for (const std::shared_ptr<PipelineGrid>& grid : notYetDetectedGrids)
		{
			// ROI of ground truth grid
			const cv::Rect gridBox = grid->getBoundingBox();

			if (tagBox.contains(gridBox.tl())
			    && tagBox.contains(gridBox.br()))
			{
				// match!
				inGroundTruth = true;

				// store pipeline tag in extra set
				results.truePositives.insert(tag);

				// store mapping from roi -> grid
				results.gridByTag[tag] = grid;

				// this modifies the container in a range based for loop, which may invalidate
				// the iterators. This is not problem in this specific case because we exit the loop
				// right away. (beware if that is going to be changed)
				notYetDetectedGrids.erase(grid);
				break;
			}
		}

		// if no match was found, the pipeline-reported roi is a false detection
		if (!inGroundTruth)
			results.falsePositives.insert(tag);
	}
}

void GroundTruthEvaluation::evaluateEllipseFitter(taglist_t const& taglist)
{
	// ToDo
	static const double threshold = 100.;

	GroundTruth::EllipseFitterEvaluationResults& results = _ellipsefitterResults;

	// copy ground truth grids from localizer results struct
	results.taggedGridsOnFrame = _localizerResults.taggedGridsOnFrame;

	// iterate over pipeline ROIs
	for (const pipeline::Tag& tag : taglist)
	{
		// find ground truth grid associated to current ROI
		auto it = _localizerResults.gridByTag.find(tag);

		// if valid iterator (and grid)
		if (it != _localizerResults.gridByTag.end())
		{
			// get ground truth grid
			const std::shared_ptr<PipelineGrid>& grid = (*it).second;

			// if the pipeline ROI has ellipses
			if (!tag.getCandidatesConst().empty())
			{
				// calculate similarity score (or distance measure?)
				// and store it in std::pair with the
				// ROI's ellipse
				auto scoreCandidatePair = compareGrids(tag, grid);

				// get the score
				const double score = scoreCandidatePair.first;

				// get the ellipse
				const pipeline::TagCandidate& candidate = scoreCandidatePair.second;

				// store the ROI-ellipse pair if match (why that way?)
				if (score <= threshold)
					results.truePositives.push_back( { tag, candidate });
				else // store as false detection
					results.falsePositives.insert(tag);
			}
			// if no ellipses are found by pipeline,
			// this tag (ROI) is counted as a missing detection
			else
				results.falseNegatives.insert(grid);

		}
		// there is no ground truth data for the specified ROI
		// --> also count it as a false detection
		else
			if (tag.getCandidatesConst().size())
			{
				results.falsePositives.insert(tag);
			}
	}

	// mark all ground truth grids that the localizer wasn't able to find
	// as missing detections on the ellipsefitter layer as well
	for (const std::shared_ptr<PipelineGrid>& grid : _localizerResults.falseNegatives)
	{
		results.falseNegatives.insert(grid);
	}
}

void GroundTruthEvaluation::evaluateGridFitter()
{
	// iterate over all correctly found ROI / ellipse pairs
	for (auto const& candidateByGrid : _ellipsefitterResults.truePositives)
	{
		// get ROI
		const pipeline::Tag& tag = candidateByGrid.first;

		// get ellipse
		const pipeline::TagCandidate& bestCandidate = candidateByGrid.second;

		// use the ROI (tag) to lookup the groundtruth grid (the map was build in evaluateLocalizer)
		// TODO: Workaround, remove this!
		assert(_localizerResults.gridByTag.count(tag));
		if (!_localizerResults.gridByTag.count(tag)) continue;

		const std::shared_ptr<PipelineGrid> groundTruthGrid = _localizerResults.gridByTag.at(tag);

		// GridFitter should always return a Pipeline for a TagCandidate
		assert(!bestCandidate.getGridsConst().empty());
		bool foundMatch = false;
		boost::optional<std::pair<double, std::reference_wrapper<const PipelineGrid>>> bestFoundGrid;
		for (const PipelineGrid& foundGrid : bestCandidate.getGridsConst())
		{
			const double score = groundTruthGrid->compare(foundGrid);

			// only continue if score is better than the score of the best found grid as of yet
			if (bestFoundGrid) {
				if (score <= bestFoundGrid.get().first) {
					continue;
				}
			}

			bestFoundGrid = { score, foundGrid };

			// compare ground truth grid to pipeline grid
			if (groundTruthGrid->compare(foundGrid) > 0.4)
			{
				_gridfitterResults.truePositives.push_back(foundGrid);
				foundMatch = true;
				break;
			}
		}

		// if no candidate has a acceptable score -> false positive
		assert(bestFoundGrid);
		if (!foundMatch) {
			_gridfitterResults.falsePositives.push_back(bestFoundGrid.get().second);
		}
	}

	// as long as GridFitter always returns a PipelineGrid for each candidate, false negatives for
	// GridFitter should be same as for the EllipseFitter
	_gridfitterResults.falseNegatives.insert(
	            _ellipsefitterResults.falseNegatives.begin(),
	            _ellipsefitterResults.falseNegatives.end());
}

void GroundTruthEvaluation::evaluateDecoder()
{
	GroundTruth::DecoderEvaluationResults results;

	for (const auto gridTagPair : _localizerResults.gridByTag) {

		pipeline::Tag const& tag = gridTagPair.first;
		GroundTruthGridSPtr const& groundTruthGrid = gridTagPair.second;
		if (tag.getCandidatesConst().empty()) continue;

		boost::optional<GroundTruth::DecoderEvaluationResults::result_t> bestResult;
		for (pipeline::TagCandidate const& gridCandidate : tag.getCandidatesConst()) {
			assert(!gridCandidate.getDecodings().empty());
			assert(gridCandidate.getDecodings().size() == gridCandidate.getGridsConst().size());
			if (gridCandidate.getDecodings().empty()) continue;

			for (size_t idx = 0; idx < gridCandidate.getDecodings().size(); ++idx) {
				pipeline::decoding_t decoding = gridCandidate.getDecodings()[idx];
				const PipelineGridRef pipelineGrid  = gridCandidate.getGridsConst()[idx];

				GroundTruth::DecoderEvaluationResults::result_t result(pipelineGrid);

				result.boundingBox     = groundTruthGrid->getBoundingBox();

#ifdef SHIFT_DECODED_BITS
				Util::rotateBitset(decoding, 3);
#endif

				result.decodedTagId    = decoding.to_ulong();
				result.decodedTagIdStr = decoding.to_string();

				result.groundTruthTagId = groundTruthGrid->getIdArray();

#ifdef SHIFT_DECODED_BITS
				std::rotate(result.groundTruthTagId.begin(),
				            result.groundTruthTagId.begin() + Grid::NUM_MIDDLE_CELLS - 3,
				            result.groundTruthTagId.end());
#endif

				std::stringstream groundTruthIdStr;
				for (int64_t idx = Grid::NUM_MIDDLE_CELLS - 1; idx >= 0; --idx) {
					groundTruthIdStr << result.groundTruthTagId[idx];
				}
				result.groundTruthTagIdStr = groundTruthIdStr.str();

				// hamming distance calculation
				result.hammingDistance = 0;
				for (size_t idx = 0; idx < Grid::NUM_MIDDLE_CELLS; ++idx) {
					if ((result.groundTruthTagId[idx] != decoding[idx]) &&
					    (!boost::indeterminate(result.groundTruthTagId[idx])))
					{
						++result.hammingDistance;
					}
				}

				if (!bestResult) {
					bestResult = result;
				} else {
					if (bestResult.get().hammingDistance > result.hammingDistance) {
						bestResult = result;
					}
				}
			}
		}

		if (bestResult) {
			results.evaluationResults.push_back(bestResult.get());
		}
	}

	_decoderResults = std::move(results);
}

GroundTruthEvaluation::gridcomparison_t GroundTruthEvaluation::compareGrids(const pipeline::Tag& detectedTag, const GroundTruthGridSPtr& grid) const
{
	assert(!detectedTag.getCandidatesConst().empty());
	auto deviation =
	        [](cv::Point2i const& cen, cv::Size const& axis, double angle, cv::Point const& point)
	{
		const double sina = std::sin(angle);
		const double cosa = std::cos(angle);
		const int x = point.x;
		const int y = point.y;
		const int a = axis.width;
		const int b = axis.height;

		double res = std::abs((x * x) / (a * a) + (y * y) / (b * b) - (cosa * cosa) - (sina * sina)) + cv::norm(cen - point);

		return res;
	};

	const std::vector<cv::Point> outerPoints = grid->getOuterRingEdgeCoordinates();
	boost::optional<PipelineTagCandidateRef> bestCandidate;
	double bestDeviation = std::numeric_limits<double>::max();

	for (pipeline::TagCandidate const& candidate : detectedTag.getCandidatesConst()) {
		double sumDeviation = 0.;
		const pipeline::Ellipse& ellipse = candidate.getEllipse();

		for (cv::Point const& point : outerPoints) {
			cv::Point rel_point = cv::Point(point.x -detectedTag.getBox().x,point.y -detectedTag.getBox().y);
			sumDeviation += deviation(ellipse.getCen(), ellipse.getAxis(),
			                          ellipse.getAngle(), rel_point);
		}
		const double deviation = sumDeviation / outerPoints.size();

		if (deviation < bestDeviation) {
			bestDeviation = deviation;
			bestCandidate = candidate;
		}
	}
	assert(bestCandidate);
	return {bestDeviation, bestCandidate.get()};
}



void GroundTruthEvaluation::reset()
{
	_localizerResults     = GroundTruth::LocalizerEvaluationResults();
	_ellipsefitterResults = GroundTruth::EllipseFitterEvaluationResults();
	_gridfitterResults    = GroundTruth::GridFitterEvaluationResults();
	_decoderResults       = GroundTruth::DecoderEvaluationResults();
}
