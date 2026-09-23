#pragma once

#include "GpuEnumerator.h"

namespace ufx {

class MockGpuEnumerator : public IGpuEnumerator {
public:
    explicit MockGpuEnumerator(std::vector<GpuInfo> gpus) : gpus_(std::move(gpus)) {}
    std::vector<GpuInfo> Enumerate() override { return gpus_; }

    static MockGpuEnumerator Default();

private:
    std::vector<GpuInfo> gpus_;
};

}
