#pragma once

#include "crow.h"
#include "db/Database.hpp"
#include "api/AuthMiddleware.hpp"
#include <optional>

namespace localstream {

struct RangeRequest {
    long long start;
    long long end;
    long long total;
};

class StreamHandler {
public:
    StreamHandler(Database& db, crow::App<AuthMiddleware>& app);

private:
    Database&                  db_;
    crow::App<AuthMiddleware>& app_;

    void setupRoutes();

    std::optional<RangeRequest> parseRangeHeader(
        const std::string& range_header,
        long long file_size
    );

    std::string getMimeType(const std::string& format);
};

} // namespace localstream