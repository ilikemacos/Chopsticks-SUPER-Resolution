#pragma once

#include "GameProfile.h"
#include "core/AppPaths.h"
#include "core/Result.h"

#include <string>
#include <vector>

namespace ufx {

// Loads and saves game profiles as one JSON file per profile under
// AppPaths::profiles. Profile file names are derived from a slug of the name.
class ProfileService {
public:
    explicit ProfileService(AppPaths paths);

    Result<std::vector<GameProfile>> LoadAll() const;
    Result<void> Save(const GameProfile& profile) const;
    Result<void> Remove(const std::string& name) const;

    // Import/export a single profile to an arbitrary path.
    Result<GameProfile> Import(const std::filesystem::path& file) const;
    Result<void> Export(const GameProfile& profile, const std::filesystem::path& file) const;

    static std::string Slug(const std::string& name);

private:
    std::filesystem::path FileFor(const std::string& name) const;
    AppPaths paths_;
};

}
