#include <biotracker/Registry.h>

#include "BeesBookImgAnalysisTracker.h"

extern "C" {
    void registerTracker() {
        BioTracker::Core::Registry::getInstance().registerTrackerType<BeesBookImgAnalysisTracker>(
                    "BeesBook ImgAnalysis");
    }
}
