#include "FrameGenRegistry.h"

#include "FsrFrameGenerator.h"

namespace ufx {

FrameGenRegistry::FrameGenRegistry() {
    gens_.push_back(std::make_unique<FsrFrameGenerator>());
    gens_.push_back(std::make_unique<XessFrameGenerator>());
    gens_.push_back(std::make_unique<CsrFrameGenerator>());
}

IFrameGenerator* FrameGenRegistry::Find(FrameGenId id) const {
    for (const auto& g : gens_)
        if (g->Caps().id == id) return g.get();
    return nullptr;
}

}
