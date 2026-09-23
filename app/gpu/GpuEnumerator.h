#pragma once

#include "GpuInfo.h"

#include <memory>
#include <vector>

namespace ufx {

class IGpuEnumerator {
public:
    virtual ~IGpuEnumerator() = default;
    virtual std::vector<GpuInfo> Enumerate() = 0;
};

// Returns the DXGI-backed enumerator on Windows, and the mock on other
// platforms so unit tests still exercise the pipeline.
std::unique_ptr<IGpuEnumerator> CreateDefaultGpuEnumerator();

}
