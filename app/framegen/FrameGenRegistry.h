#pragma once

#include "IFrameGenerator.h"

#include <memory>
#include <vector>

namespace ufx {

class FrameGenRegistry {
public:
    FrameGenRegistry();
    const std::vector<std::unique_ptr<IFrameGenerator>>& All() const { return gens_; }
    IFrameGenerator* Find(FrameGenId id) const;

private:
    std::vector<std::unique_ptr<IFrameGenerator>> gens_;
};

}
