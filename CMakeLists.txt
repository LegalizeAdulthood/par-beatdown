cmake_minimum_required(VERSION 3.23...3.31)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")
include(misc_target)
include(set_cxx_standard)
include(target_folder)
include(vs_startup_project)

set_cxx_standard(17)

set(PAR_BEATDOWN_PROJECT_NAME "Par Beatdown" CACHE "STRING" "Default project name for IDEs")
project("${PAR_BEATDOWN_PROJECT_NAME}" CXX)

option(PAR_BEATDOWN_ENABLE_TRACKER_FILE "Enable tracker file support" ON)
option(PAR_BEATDOWN_ENABLE_AUDIO_FILE "Enable ordinary audio file support" OFF)

find_package(nlohmann_json CONFIG REQUIRED)
if(PAR_BEATDOWN_ENABLE_TRACKER_FILE)
    find_package(libopenmpt CONFIG REQUIRED)
endif()

include(CTest)

add_subdirectory(libs)
add_subdirectory(tools)

vs_startup_project(par-beatdown)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

misc_target(FILES
    "CMakePresets.json"
    "ReadMe.md")
misc_group_sources("CMake Scripts" FILES
    "cmake/misc_target.cmake"
    "cmake/set_cxx_standard.cmake"
    "cmake/target_folder.cmake"
    "cmake/vs_startup_project.cmake")
misc_group_sources("Editing" FILES
    ".clang-format"
    ".editorconfig")
misc_group_sources("Git" FILES
    ".gitignore"
    ".gitmodules")
misc_group_sources("VcPkg" FILES
    "vcpkg-configuration.json"
    "vcpkg.json"
    "vcpkg-ports/sonic-annotator/portfile.cmake"
    "vcpkg-ports/sonic-annotator/usage"
    "vcpkg-ports/sonic-annotator/vcpkg.json")
