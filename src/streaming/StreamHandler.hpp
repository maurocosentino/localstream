#pragma once

#include "crow.h"
#include "db/Database.hpp"

namespace localstream {

struct RangeRequest {
    long long start;
    long long end;
    long long total;
};

class StreamHandler {
public:
    StreamHandler(Database& db, crow::SimpleApp& app);

private:
    Database&        db_;
    crow::SimpleApp& app_;

    void setupRoutes();

    // Parsea el header "Range: bytes=X-Y"
    // Retorna nullopt si no hay Range header (pedido completo)
    std::optional<RangeRequest> parseRangeHeader(
        const std::string& range_header,
        long long file_size
    );

    // Determina el MIME type según el formato
    std::string getMimeType(const std::string& format);
};

} // namespace localstream