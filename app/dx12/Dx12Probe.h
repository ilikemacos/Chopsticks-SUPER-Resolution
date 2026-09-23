#pragma once

namespace ufx {

struct Dx12Caps {
    bool available = false;
    unsigned featureLevel = 0;
    bool meshShaders = false;
    bool sm66 = false;         // Shader Model 6.6
    bool waveIntrinsics = false;
    bool enhancedBarriers = false;
};

Dx12Caps ProbeDx12();

}
