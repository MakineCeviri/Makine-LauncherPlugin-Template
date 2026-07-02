// SPDX-License-Identifier: AGPL-3.0-only
// Copyright (c) 2026 Makine Çeviri. Commons Clause applies — see LICENSE.

/**
 * Makine Launcher Plugin Template
 *
 * All plugins must export the 5 required C ABI functions.
 * Optional exports for settings and OCR are shown below.
 *
 * Build: cmake -B build -G Ninja && cmake --build build
 * Pack:  python makine-pack.py ./build/release/
 *
 * See: https://github.com/MakineCeviri/Makine-Launcher/blob/main/core/include/makine/plugin/plugin_api.h
 */

#include <makine/plugin/plugin_api.h>

static bool s_ready = false;
static char s_error[256] = "";

// ═══════════════════════════════════════════════════════════════════════
// Required Exports (all 5 must be implemented)
// ═══════════════════════════════════════════════════════════════════════

extern "C" __declspec(dllexport)
MakinePluginInfo makine_get_info(void)
{
    return {
        "com.github-username.my-plugin",  // id — must match manifest.json
        "My Makine Plugin",               // name
        "0.1.0",                           // version
        MAKINE_PLUGIN_API_VERSION          // apiVersion
    };
}

extern "C" __declspec(dllexport)
MakineError makine_initialize(const char* dataPath)
{
    // dataPath = AppData/Local/Makine/plugin-data/<your-plugin-id>/
    // Use this directory to store caches, configs, etc.
    (void)dataPath;

    // TODO: Initialize your plugin here
    s_ready = true;
    return MAKINE_OK;
}

extern "C" __declspec(dllexport)
void makine_shutdown(void)
{
    // TODO: Clean up resources here
    s_ready = false;
}

extern "C" __declspec(dllexport)
bool makine_is_ready(void)
{
    return s_ready;
}

extern "C" __declspec(dllexport)
const char* makine_get_last_error(void)
{
    return s_error;
}

// ═══════════════════════════════════════════════════════════════════════
// Optional: Settings Exports
// If your manifest.json has a "settings" array, implement these so
// Launcher can read/write your plugin's configuration from the UI.
// ═══════════════════════════════════════════════════════════════════════

/*
extern "C" __declspec(dllexport)
const char* makine_get_setting(const char* key)
{
    // Return the current value for the given setting key.
    // Keys match the "key" field in your manifest.json "settings" array.
    // Return nullptr if key is unknown.
    (void)key;
    return nullptr;
}

extern "C" __declspec(dllexport)
void makine_set_setting(const char* key, const char* value)
{
    // Persist the new value for the given setting key.
    // Called when the user changes a setting in Launcher UI.
    (void)key;
    (void)value;
}
*/

// ═══════════════════════════════════════════════════════════════════════
// Optional: OCR + Translate Exports
// For plugins that provide screen capture OCR and translation.
// Category should be "accessibility" in manifest.json.
// ═══════════════════════════════════════════════════════════════════════

/*
extern "C" __declspec(dllexport)
const char* makine_capture_ocr_translate(void* hwnd, int x, int y, int w, int h)
{
    // Capture the screen region (x, y, w, h), run OCR, translate, return result.
    // hwnd is reserved (currently nullptr).
    // Return the translated text, or nullptr on failure.
    (void)hwnd; (void)x; (void)y; (void)w; (void)h;
    return nullptr;
}

extern "C" __declspec(dllexport)
const char* makine_get_last_ocr_text(void)
{
    // Return the raw OCR text from the last capture (before translation).
    return nullptr;
}
*/
