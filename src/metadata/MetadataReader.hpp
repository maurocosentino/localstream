#pragma once

#include <optional>
#include <string>
#include "models.hpp"

namespace localstream {

class MetadataReader {
public:
    std::optional<TrackMetadata> read(const std::string& file_path);
};

} // namespace localstream