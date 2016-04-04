#pragma once

#include <map>
#include <memory>
#include <set>
#include <vector>

#include "biotracker/serialization/SerializationData.h"
#include "biotracker/serialization/TrackedObject.h"

#include "pipeline/common/Grid.h"

/* shift decoded bits so that bit index 0 is the first bit on the "left side"
 * of the white semicircle */
#define SHIFT_DECODED_BITS

class PipelineGrid;
namespace pipeline {
class Tag;
class TagCandidate;
}

namespace {
typedef std::vector<pipeline::Tag> taglist_t;
typedef std::shared_ptr<PipelineGrid> GroundTruthGridSPtr;
typedef std::reference_wrapper<const pipeline::Tag> PipelineTagRef;
typedef std::reference_wrapper<const pipeline::TagCandidate> PipelineTagCandidateRef;
typedef std::reference_wrapper<const PipelineGrid> PipelineGridRef;
}

namespace GroundTruth {
struct LocalizerEvaluationResults       {
    std::set<GroundTruthGridSPtr>                 taggedGridsOnFrame;
    std::set<PipelineTagRef>                      falsePositives;
    std::set<PipelineTagRef>                      truePositives;
    std::set<GroundTruthGridSPtr>                 falseNegatives;
    std::map<PipelineTagRef, GroundTruthGridSPtr> gridByTag;
};

struct EllipseFitterEvaluationResults {
    std::set<GroundTruthGridSPtr> taggedGridsOnFrame;
    std::set<PipelineTagRef> falsePositives;

    //mapping of pipeline tag to its best ellipse (only if it matches ground truth ellipse)
    std::vector<std::pair<PipelineTagRef, PipelineTagCandidateRef>> truePositives;
    std::set<GroundTruthGridSPtr> falseNegatives;
};

struct GridFitterEvaluationResults {
    std::vector<PipelineGridRef>  truePositives;
    std::vector<PipelineGridRef>  falsePositives;
    std::set<GroundTruthGridSPtr> falseNegatives;
};

struct DecoderEvaluationResults {
    struct result_t {
        cv::Rect            boundingBox;
        int                 decodedTagId;
        std::string         decodedTagIdStr;
        Grid::idarray_t     groundTruthTagId;
        std::string         groundTruthTagIdStr;
        int                 hammingDistance;
        PipelineGridRef     pipelineGrid;

        result_t(const PipelineGridRef grid) : pipelineGrid(grid) {}
    };

    boost::optional<double> getAverageHammingDistanceNormalized() const {
        if (!evaluationResults.empty()) {
            double sum = 0.;
            for (const result_t &result : evaluationResults) {
                sum += (static_cast<double>(result.hammingDistance) / Grid::NUM_MIDDLE_CELLS);
            }
            return (sum / evaluationResults.size());
        } else {
            return boost::optional<double>();
        }
    }

    std::vector<result_t> evaluationResults;
};
}

class GroundTruthEvaluation {
  public:
    explicit GroundTruthEvaluation(BioTracker::Core::Serialization::Data &&groundTruthData);

    void evaluateLocalizer(const int currentFrameNumber, taglist_t const &taglist);
    void evaluateEllipseFitter(taglist_t const &taglist);
    void evaluateGridFitter();
    void evaluateDecoder();

    void reset();

    GroundTruth::LocalizerEvaluationResults const &getLocalizerResults() const {
        return _localizerResults;
    }
    GroundTruth::EllipseFitterEvaluationResults const &getEllipsefitterResults() const {
        return _ellipsefitterResults;
    }
    GroundTruth::GridFitterEvaluationResults const &getGridfitterResults() const {
        return _gridfitterResults;
    }
    GroundTruth::DecoderEvaluationResults const &getDecoderResults() const {
        return _decoderResults;
    }

  private:
    BioTracker::Core::Serialization::Data _groundTruthData;

    GroundTruth::LocalizerEvaluationResults     _localizerResults;
    GroundTruth::EllipseFitterEvaluationResults _ellipsefitterResults;
    GroundTruth::GridFitterEvaluationResults    _gridfitterResults;
    GroundTruth::DecoderEvaluationResults       _decoderResults;

    typedef std::pair<double, PipelineTagCandidateRef> gridcomparison_t;

    gridcomparison_t compareGrids(const pipeline::Tag &detectedTag, const GroundTruthGridSPtr &grid) const;
};
