#pragma once

namespace ufx {

struct Dx11Caps {
    bool available = false;
    unsigned featureLevel = 0;
    bool tearingSupported = false;
};

Dx11Caps ProbeDx11();

}
