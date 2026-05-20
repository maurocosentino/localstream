#pragma once

#include "crow.h"
#include "db/Database.hpp"
#include <optional>

namespace localstream {

struct RangeRequest {
    long long start;
    long long end;
    long long total;
};

template<typename App>
class StreamHandler {
public:
    StreamHandler(Database& db, App& app)
        : db_(db), app_(app)
    {
        setupRoutes();
    }

private:
    Database& db_;
    App&      app_;

    void setupRoutes();

    std::optional<RangeRequest> parseRangeHeader(
        const std::string& range_header,
        long long file_size
    );

    std::string getMimeType(const std::string& format);
};

} // namespace localstream

#include "streaming/StreamHandler.ipp"