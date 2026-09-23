#pragma once

#include "GpuEnumerator.h"

namespace ufx {

class DxgiGpuEnumerator : public IGpuEnumerator {
public:
    std::vector<GpuInfo> Enumerate() override;
};

}
