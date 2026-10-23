// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 Makine Çeviri. Commons Clause applies — see LICENSE.

/**
 * Makine plugin template (ABI 2).
 *
 * Shows the pieces most plugins need:
 *  - settings read from the init JSON and kept current through settings.changed
 *  - an action ("hello") that shows a notification through ui.notify
 *  - a worker thread, started in initialize() and joined in shutdown(), that keeps
 *    the plugin's status line (ui.setStatus) up to date
 *  - a file under dataDir written through the wide Win32 API
 *
 * Build: cmake -B build -G Ninja && cmake --build build      -> build/package/
 * Try:   python tools/plugin_dev.py build/package --host <makine-plugin-host.exe> --invoke hello
 * Guide: https://github.com/MakineCeviri/Makine-Launcher/blob/dev/docs/plugins/README.md
 */

#include <makine/plugin/sdk.hpp>

#include <nlohmann/json.hpp>

#include <windows.h>

#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <string_view>
#include <thread>

using json = nlohmann::json;
using makine::plugin::Result;

namespace {

/// UTF-8 to UTF-16 for the wide Win32 API. Paths in JSON are UTF-8; narrow APIs
/// (fopen, std::ifstream with a char path) read them in the ANSI code page and
/// fail for a user name such as "Şükrü".
std::wstring widen(std::string_view utf8)
{
    if (utf8.empty()) return {};
    const int n = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    std::wstring out(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), out.data(), n);
    return out;
}

/// Appends `line` to the file at `path`, creating it if needed.
bool appendLine(const std::wstring& path, const std::string& line)
{
    const HANDLE file = CreateFileW(path.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ, nullptr,
                                    OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    const std::string data = line + "\r\n";
    DWORD written = 0;
    const bool ok = WriteFile(file, data.data(), static_cast<DWORD>(data.size()), &written, nullptr)
                    && written == data.size();
    CloseHandle(file);
    return ok;
}

class TemplatePlugin : public makine::plugin::Plugin {
public:
    // initialize, shutdown, invoke and onEvent run on one host thread, one at a
    // time. invoke has 30 s (an action 120 s) before Makine gives up on it, so
    // long or periodic work belongs on a thread of your own. An exception thrown
    // in these four (a JSON parse error, say) is caught by sdk.hpp; one thrown on
    // your own thread ends the process.

    MakineStatus initialize(std::string_view initJson) override
    {
        const json init = json::parse(initJson);

        // dataDir: this plugin's own folder; it survives updates.
        m_dataDir = widen(init.value("dataDir", std::string()));
        if (m_dataDir.empty()) {
            host().error("init JSON has no dataDir");
            return MAKINE_E_INVALID_ARGUMENT;
        }

        // settings: every key from manifest.json with its saved value or default.
        const json settings = init.value("settings", json::object());
        for (const auto& [key, value] : settings.items())
            applySetting(key, value);

        m_worker = std::thread(&TemplatePlugin::statusLoop, this);
        host().info("Hazır.");
        return MAKINE_OK;
    }

    void shutdown() override
    {
        // Makine allows shutdown 3 s, then ends the process. Join every thread you
        // started: after shutdown returns, host() is gone and this object deleted.
        {
            std::lock_guard lock(m_mutex);
            m_stop = true;
        }
        m_wake.notify_all();
        if (m_worker.joinable()) m_worker.join();
    }

    Result invoke(std::string_view method, std::string_view /*paramsJson*/) override
    {
        // Actions from manifest.json arrive under their id.
        if (method == "hello") return hello();
        return Result::notFound(method);
    }

    void onEvent(std::string_view event, std::string_view payloadJson) override
    {
        // Only the events listed in manifest.json's contributes.events arrive.
        if (event == "settings.changed") {
            const json payload = json::parse(payloadJson);
            applySetting(payload.at("key").get<std::string>(), payload.at("value"));
        }
    }

private:
    Result hello()
    {
        std::string greeting;
        int count = 0;
        {
            std::lock_guard lock(m_mutex);
            greeting = m_greeting;
            count = ++m_helloCount;
            m_refresh = true;   // show the new count on the status line now
        }
        m_wake.notify_all();

        const std::string message = greeting + " (" + std::to_string(count) + ". selam)";
        if (!appendLine(m_dataDir + L"\\hello.log", message))
            host().warn("hello.log yazılamadı.");

        // host().call blocks until Makine answers; ui.notify answers at once.
        const json notice = {{"title", "Örnek eklenti"}, {"message", message}, {"kind", "success"}};
        const Result shown = host().call("ui.notify", notice.dump());
        if (!shown.succeeded()) return shown;
        return Result::ok(json{{"count", count}}.dump());
    }

    // Makine checks values against manifest.json, so each key has its declared type.
    void applySetting(const std::string& key, const json& value)
    {
        {
            std::lock_guard lock(m_mutex);
            if (key == "greeting") m_greeting = value.get<std::string>();
            else if (key == "showUptime") m_showUptime = value.get<bool>();
            else if (key == "interval") m_interval = std::chrono::seconds(std::max(1, value.get<int>()));
            m_refresh = true;   // the worker applies a change now, not after the old interval
        }
        m_wake.notify_all();
    }

    // Worker thread. host() is thread-safe, and a call made here does not hold
    // up invoke() or onEvent() on the host thread.
    void statusLoop()
    {
        const auto started = std::chrono::steady_clock::now();
        std::unique_lock lock(m_mutex);
        while (!m_stop) {
            std::string text = "Selam sayısı: " + std::to_string(m_helloCount);
            if (m_showUptime) {
                const auto minutes = std::chrono::duration_cast<std::chrono::minutes>(
                    std::chrono::steady_clock::now() - started).count();
                text += " · çalışma süresi: " + std::to_string(minutes) + " dk";
            }
            m_refresh = false;
            lock.unlock();   // don't keep the host thread waiting while Makine answers
            host().call("ui.setStatus", json{{"text", text}}.dump());
            lock.lock();
            m_wake.wait_for(lock, m_interval, [this] { return m_stop || m_refresh; });
        }
    }

    std::wstring m_dataDir;
    std::thread m_worker;

    // Shared between the host thread and the worker; guarded by m_mutex.
    std::mutex m_mutex;
    std::condition_variable m_wake;
    std::string m_greeting = "Merhaba!";
    bool m_showUptime = true;
    std::chrono::seconds m_interval{60};
    int m_helloCount = 0;
    bool m_refresh = false;
    bool m_stop = false;
};

} // namespace

MAKINE_PLUGIN(TemplatePlugin)
