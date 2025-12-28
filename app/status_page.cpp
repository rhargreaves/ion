#include "status_page.h"

#include <sstream>
#include <string>

#include "http_response.h"
#include "version.h"

std::string_view StatusPage::get_platform() {
#ifdef __apple_build_version__
    return "macOS (AppleClang)";
#elif defined(__linux__)
    return "Linux";
#else
    return "Unknown";
#endif
}

void StatusPage::add_status_page(ion::Router& router) {
    router.add_route("/_ion/status", "GET", [](const auto&) {
        const auto platform = get_platform();

        std::stringstream html;
        html << R"(
<!DOCTYPE html>
<html>
<head>
    <title>ion server info</title>
    <style>
        body { font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
               background: #121212; color: #e0e0e0; line-height: 1.6; padding: 2rem; max-width: 800px; margin: auto; }
        .card { background: #1e1e1e; border-radius: 8px; padding: 1.5rem; border: 1px solid #333; box-shadow: 0 4px 6px rgba(0,0,0,0.3); }
        h1 { color: #f9d423; margin-top: 0; }
        .stat-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 1rem; }
        .stat-item { border-bottom: 1px solid #333; padding: 0.5rem 0; }
        .label { font-weight: bold; color: #888; display: block; font-size: 0.8rem; text-transform: uppercase; }
        .value { font-family: "SFMono-Regular", Consolas, "Liberation Mono", Menlo, monospace; color: #00d4ff; }
        .footer { margin-top: 2rem; font-size: 0.8rem; color: #666; text-align: center; }
    </style>
</head>
<body>
    <div class="card">
        <h1>ion server status ⚡️</h1>
        <div class="stat-grid">
            <div class="stat-item">
                <span class="label">Version</span>
                <span class="value">)"
             << ion::BUILD_VERSION << R"(</span>
            </div>
            <div class="stat-item">
                <span class="label">Status</span>
                <span class="value">Running</span>
            </div>
            <div class="stat-item">
                <span class="label">Platform</span>
                <span class="value">)"
             << platform << R"(</span>
            </div>
            <div class="stat-item">
                <span class="label">C++ Standard</span>
                <span class="value">)"
             << __cplusplus << R"(</span>
            </div>
        </div>
    </div>
    <div class="footer">
        Powered by ion • The light-weight HTTP/2 server
    </div>
</body>
</html>
)";

        const auto body_str = html.view();
        return ion::HttpResponse{
            .status_code = 200,
            .body = std::vector<uint8_t>{body_str.begin(), body_str.end()},
            .headers = {{"content-type", "text/html; charset=utf-8"}},
        };
    });
}
