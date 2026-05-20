#pragma once
#include "crow.h"
#include <string>

namespace localstream {

struct AuthMiddleware : crow::ILocalMiddleware {

    struct context {};

    void before_handle(crow::request& req, crow::response& res, context&)
    {
        
        // Rutas públicas que no necesitan auth
        if (req.url == "/health" || req.url == "/") {
            return;
        }

        if (req.url.rfind("/rest/", 0) == 0) return;
        // Assets del frontend tampoco
        if (req.url.rfind("/assets/", 0) == 0) {
            return;
        }

        const std::string key = req.get_header_value("X-Api-Key");

        if (key.empty() || key != api_key_) {
            res.code = 401;
            crow::json::wvalue json;
            json["error"] = "API key inválida o ausente";
            res.write(json.dump());
            res.set_header("Content-Type", "application/json");
            res.end();
        }
    }

    void after_handle(crow::request&, crow::response&, context&) {}

    void set_key(const std::string& key) { api_key_ = key; }

private:
    std::string api_key_;
};

} // namespace localstream