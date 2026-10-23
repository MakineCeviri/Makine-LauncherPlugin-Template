cmake_minimum_required(VERSION 3.25)
project(my-plugin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Single-config generators (Ninja, Makefiles) build unoptimised without a type.
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Link the C++ runtime into the DLL. The plugin host looks for DLLs only in the
# plugin's folder, the launcher's folder and System32, so a runtime DLL you do
# not ship would be the launcher's copy, in whatever version it has.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# nlohmann/json (header-only, MIT). Pinned release archive; the hash matches the
# one published in the v3.12.0 release notes.
include(FetchContent)
FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
    URL_HASH SHA256=42f6e95cad6ec532fd372391373363b62a14af6d771056dbfc86160e6dfff7aa
    SYSTEM)
FetchContent_MakeAvailable(json)

add_library(my-plugin SHARED plugin.cpp)
target_include_directories(my-plugin PRIVATE include)
target_link_libraries(my-plugin PRIVATE nlohmann_json::nlohmann_json)
# NOMINMAX: windows.h would otherwise turn std::min/std::max into macros on MSVC.
target_compile_definitions(my-plugin PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
set_target_properties(my-plugin PROPERTIES PREFIX "")   # my-plugin.dll, not libmy-plugin.dll
if(MINGW)
    target_link_options(my-plugin PRIVATE -static -static-libgcc -static-libstdc++)
endif()
if(MSVC)
    target_compile_options(my-plugin PRIVATE /utf-8)    # Turkish text in string literals
endif()

# build/package/ is the plugin as Makine installs it: manifest.json, and the DLL
# at the path the manifest's "entry" names.
set(PACKAGE_DIR "${CMAKE_BINARY_DIR}/package")
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/manifest.json" MANIFEST_JSON)
string(JSON PLUGIN_ENTRY GET "${MANIFEST_JSON}" entry)
configure_file(manifest.json "${PACKAGE_DIR}/manifest.json" COPYONLY)
add_custom_command(TARGET my-plugin POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:my-plugin>" "${PACKAGE_DIR}/${PLUGIN_ENTRY}"
    VERBATIM)

# cmake --build build --target pack  ->  build/<id>-<version>.mkplugin
find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
    add_custom_target(pack
        COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/tools/plugin_registry.py"
                pack "${PACKAGE_DIR}"
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        VERBATIM)
    add_dependencies(pack my-plugin)
else()
    message(STATUS "Python 3 not found: the pack target is not available")
endif()
