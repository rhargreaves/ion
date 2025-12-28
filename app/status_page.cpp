#include "status_page.h"

#include <spdlog/spdlog.h>

#include <iomanip>
#include <sstream>
#include <string>

#include "http_response.h"
#include "server_stats.h"
#include "version.h"

std::string StatusPage::format_duration(uint64_t total_seconds) {
    const auto days = total_seconds / 86400;
    total_seconds %= 86400;
    const auto hours = total_seconds / 3600;
    total_seconds %= 3600;
    const auto minutes = total_seconds / 60;
    const auto seconds = total_seconds % 60;

    std::stringstream ss;
    if (days > 0)
        ss << days << "d ";
    if (hours > 0 || days > 0)
        ss << hours << "h ";
    if (minutes > 0 || hours > 0 || days > 0)
        ss << minutes << "m ";
    ss << seconds << "s";

    return ss.str();
}

void StatusPage::add_status_page(ion::Router& router) {
    router.add_middleware(ServerStats::middleware());

    auto& stats = ServerStats::instance();
    spdlog::info("instance id: {}", stats.server_id);

    router.add_route("/_ion/status", "GET", [&stats](const auto&) {
        const auto now = std::chrono::steady_clock::now();
        const auto uptime_sec =
            std::chrono::duration_cast<std::chrono::seconds>(now - stats.start_time).count();
        const std::string uptime = format_duration(uptime_sec);

        const uint64_t total = stats.total_requests;
        const double avg_lat = total > 0 ? (double)stats.total_duration_us / total / 1000.0 : 0.0;

        std::stringstream html;
        html << R"html(
<!DOCTYPE html>
<html>
<head>
    <title>ion server info</title>
    <style>
   :root {
            --bg: #0d1117;
            --card-bg: #161b22;
            --border: #30363d;
            --accent: #f9d423; /* Ion Gold */
            --cyan: #00d4ff;
            --text: #c9d1d9;
            --text-dim: #8b949e;
            --success: #238636;
        }

        body {
            font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, sans-serif;
            background: var(--bg);
            color: var(--text);
            line-height: 1.6;
            padding: 2rem;
            max-width: 800px;
            margin: auto;
        }

        .card {
            background: var(--card-bg);
            border-radius: 12px;
            padding: 2rem;
            border: 1px solid var(--border);
            box-shadow: 0 10px 30px rgba(0,0,0,0.5);
            position: relative;
            overflow: hidden;
        }

        /* Subtle "lightning" decorative element */
        .card::after {
            content: "⚡️";
            position: absolute;
            top: -10px;
            right: 10px;
            font-size: 5rem;
            opacity: 0.05;
            pointer-events: none;
        }

        h1 {
            color: var(--accent);
            margin-top: 0;
            font-size: 1.5rem;
            letter-spacing: -0.5px;
            display: flex;
            align-items: center;
            gap: 10px;
        }

        h3 {
            font-size: 0.9rem;
            text-transform: uppercase;
            letter-spacing: 1px;
            color: var(--text-dim);
            margin-top: 2rem;
            border-bottom: 1px solid var(--border);
            padding-bottom: 0.5rem;
        }

        .stat-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 1.5rem;
        }

        .stat-item {
            display: flex;
            flex-direction: column;
        }

        .label {
            font-weight: 600;
            color: var(--text-dim);
            font-size: 0.75rem;
            text-transform: uppercase;
            margin-bottom: 4px;
        }

        .value {
            font-family: "SFMono-Regular", Consolas, monospace;
            color: var(--cyan);
            font-size: 1.25rem;
            font-weight: 500;
            white-space: nowrap;
        }

        .code-list {
            display: flex;
            flex-wrap: wrap;
            gap: 0.75rem;
            margin-top: 1rem;
        }

        .code-badge {
            background: rgba(0, 212, 255, 0.05);
            padding: 0.5rem 1rem;
            border-radius: 6px;
            border: 1px solid var(--border);
            display: flex;
            align-items: center;
            transition: transform 0.2s;
        }

        .code-badge:hover {
            transform: translateY(-2px);
            border-color: var(--cyan);
        }

        .code-num {
            font-weight: bold;
            color: var(--accent);
            margin-right: 10px;
            padding-right: 10px;
            border-right: 1px solid var(--border);
        }

        .footer {
            margin-top: 3rem;
            font-size: 0.75rem;
            color: var(--text-dim);
            text-align: center;
        }

        .pulse {
            width: 8px;
            height: 8px;
            background: var(--success);
            border-radius: 50%;
            display: inline-block;
            box-shadow: 0 0 8px var(--success);
            animation: blink 2s infinite;
        }

        @ keyframes blink {
            0% { opacity: 1; }
            50% { opacity: 0.3; }
            100% { opacity: 1; }
        }
    </style>
</head>
<body>
    <div class="card">
         <h1>
            <span class="pulse"></span>
            ion server status
            <span style='color: var(--accent)'>⚡ ️️</span>
        </h1>
        <div class="stat-grid">
            <div class="stat-item"><span class="label">Uptime</span><span class="value">)html"
             << uptime << R"html(</span></div>
            <div class="stat-item"><span class="label">Requests</span><span class="value">)html"
             << stats.total_requests << R"html(</span></div>
            <div class="stat-item"><span class="label">Avg Latency</span><span class="value">)html"
             << std::fixed << std::setprecision(2) << avg_lat << R"html(ms</span></div>
        </div>

        <h3>HTTP Status Codes</h3>
        <div class="code-list">)html";

        if (stats.status_codes.empty()) {
            html << "<em>No requests yet</em>";
        } else {
            for (auto const& [code, count] : stats.status_codes) {
                html << "<div class='code-badge'><span class='code-num'>" << code << "</span>"
                     << count << "</div>";
            }
        }

        html << R"html(
        </div>
    </div>
    <div class="footer">
        powered by <a href="https://github.com/rhargreaves/ion">ion</a> v)html"
             << ion::BUILD_VERSION << R"html( • instance id: )html" << stats.server_id << R"html(
    </div>
</body>
</html>
)html";

        const auto body_str = html.view();
        return ion::HttpResponse{
            .status_code = 200,
            .body = std::vector<uint8_t>{body_str.begin(), body_str.end()},
            .headers = {{"content-type", "text/html; charset=utf-8"},
                        {"cache-control", "no-store"}},
        };
    });
}
