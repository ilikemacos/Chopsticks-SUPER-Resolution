#pragma once

#include "IUpscaler.h"

#include <memory>
#include <vector>

namespace ufx {

// Owns one instance of every known upscaler adapter and answers UI queries.
class UpscalerRegistry {
public:
    UpscalerRegistry();

    const std::vector<std::unique_ptr<IUpscaler>>& All() const { return upscalers_; }
    IUpscaler* Find(UpscalerId id) const;

    // Convenience: which upscalers are available on this GPU for this API.
    struct Entry {
        IUpscaler* upscaler;
        Availability availability;
    };
    std::vector<Entry> Evaluate(const GpuInfo& gpu, GraphicsApi api) const;

private:
    std::vector<std::unique_ptr<IUpscaler>> upscalers_;
};

const char* UpscalerIdName(UpscalerId id);
bool ParseUpscalerId(std::string_view s, UpscalerId& out);
const char* GraphicsApiName(GraphicsApi api);
bool ParseGraphicsApi(std::string_view s, GraphicsApi& out);

}
