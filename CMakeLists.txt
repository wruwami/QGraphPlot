# ═══════════════════════════════════════════════════════════════
# QGraphPlot — High-performance Qt chart library
#
# AI.md §4.2 / VERSIONING.md §3:
#   프로젝트 버전은 이 파일의 project(... VERSION x.y.z) 한 곳에만 명시.
#   다른 파일(헤더/리소스/README)에 버전을 하드코딩하지 않는다.
# ═══════════════════════════════════════════════════════════════
cmake_minimum_required(VERSION 3.16)

project(QGraphPlot
    VERSION 0.1.0
    DESCRIPTION "High-performance Qt chart library (QWidget + QML)"
    HOMEPAGE_URL "https://github.com/wruwami/QGraphPlot"
    LANGUAGES CXX
)

# ── 버전 변수 (하위 디렉토리에서 참조) ──────────────────────
# project()가 자동으로 생성: PROJECT_VERSION, PROJECT_VERSION_MAJOR 등.
# 헤더용 configure_file()은 src/qchart/CMakeLists.txt에서 수행.

# ── C++ 표준 (AI.md §1.3: Qt 6.7+ 기준) ──────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ── 정책 / 기본 옵션 ────────────────────────────────────────
set(CMAKE_CMAKE_USE_DEFAULT_FILESYSTEM ON)  # CMake 3.18+ (안전하게 fallback)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Position Independent Code (공유 라이브러리용)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# ── 빌드 옵션 (README.md의 CMake 옵션 표와 일치) ───────────
option(QGRAPHPLOT_BUILD_CORE           "Build QGraphPlot core library"        ON)
option(QGRAPHPLOT_BUILD_WIDGET_FRONTEND "Build QWidget frontend"              ON)
option(QGRAPHPLOT_BUILD_QML_FRONTEND   "Build QML frontend"                    ON)
option(QGRAPHPLOT_BUILD_EXAMPLES       "Build examples"                        ON)
option(QGRAPHPLOT_BUILD_TESTS          "Build unit tests"                      ON)
option(QGRAPHPLOT_BUILD_COVERAGE       "Enable code coverage (GCC/Clang only)" OFF)
option(QGRAPHPLOT_BUILD_DOCS           "Build API documentation (Doxygen)"     OFF)

# ── 컴파일러 경고 (엄격) ────────────────────────────────────
if(MSVC)
    add_compile_options(/W4 /permissive- /Wv:19.29 /Zc:__cplusplus /utf-8)
    # /W4: 최고 경고 수준
    # /permissive-: 표준 준수 모드
    # /Zc:__cplusplus: __cplusplus 매크로 올바르게 설정
    # /utf-8: 소스/실행 문자셋 UTF-8
else()
    add_compile_options(
        -Wall -Wextra -Wpedantic
        -Wshadow -Wnon-virtual-dtor -Wold-style-cast
        -Wcast-align -Wconversion -Wsign-conversion
        -Wnull-dereference -Wdouble-promotion -Wformat=2
        -Wimplicit-fallthrough -Wno-unused-parameter
    )
endif()

# ── Qt 찾기 (AI.md §1.3: Qt 6.7+) ────────────────────────────
set(QT_COMPONENTS Core Gui Widgets)
if(QGRAPHPLOT_BUILD_QML_FRONTEND)
    list(APPEND QT_COMPONENTS Qml Quick QuickControls2)
endif()
if(QGRAPHPLOT_BUILD_TESTS)
    list(APPEND QT_COMPONENTS Test)
endif()
find_package(Qt6 6.7 REQUIRED COMPONENTS ${QT_COMPONENTS})

# ── 커버리지 플래그 ────────────────────────────────────────
if(QGRAPHPLOT_BUILD_COVERAGE)
    if(NOT (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang"))
        message(FATAL_ERROR "Coverage requires GCC or Clang. Set QGRAPHPLOT_BUILD_COVERAGE=OFF.")
    endif()
    message(STATUS "Coverage enabled: ${CMAKE_CXX_COMPILER_ID}")
    # -fprofile-update=atomic: gcov's default counter increments are not
    # thread-safe. Multi-threaded tests (e.g. TestRingBuffer's
    # ThreadSafety::Enabled stress test) can otherwise race on the same
    # counter and produce corrupted (negative) counts that make
    # lcov/geninfo fail outright instead of just under-reporting coverage.
    add_compile_options(--coverage -O0 -g -fprofile-update=atomic)
    add_link_options(--coverage)
endif()

# ── compile_commands.json (cppcheck/IDE용) ─────────────────
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ── 출력 디렉토리 단순화 ────────────────────────────────────
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ── 하위 디렉토리 ────────────────────────────────────────────
# Phase 0 구현에 따라 순차 활성화.
#
if(QGRAPHPLOT_BUILD_CORE)
    add_subdirectory(src/core)
endif()
if(QGRAPHPLOT_BUILD_QML_FRONTEND)
    if(NOT QGRAPHPLOT_BUILD_CORE)
        message(FATAL_ERROR "QGRAPHPLOT_BUILD_QML_FRONTEND requires QGRAPHPLOT_BUILD_CORE=ON (qml_frontend links QGraphPlot::Core)")
    endif()
    add_subdirectory(src/qml_frontend)
endif()
if(QGRAPHPLOT_BUILD_WIDGET_FRONTEND)
    if(NOT QGRAPHPLOT_BUILD_CORE)
        message(FATAL_ERROR "QGRAPHPLOT_BUILD_WIDGET_FRONTEND requires QGRAPHPLOT_BUILD_CORE=ON (widget_frontend links QGraphPlot::Core)")
    endif()
    add_subdirectory(src/widget_frontend)
endif()
if(QGRAPHPLOT_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
if(QGRAPHPLOT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ── 설치 규칙 (Phase 1+ 적용 예정) ──────────────────────────
# include(GNUInstallDirs)
# include(CMakePackageConfigHelpers)
# (라이브러리가 구현된 뒤 추가)

message(STATUS "")
message(STATUS "═══ QGraphPlot ${PROJECT_VERSION} ═══")
message(STATUS "  Qt version:         ${Qt6_VERSION}")
message(STATUS "  C++ standard:       C++${CMAKE_CXX_STANDARD}")
message(STATUS "  Build type:         ${CMAKE_BUILD_TYPE}")
message(STATUS "  Core:               ${QGRAPHPLOT_BUILD_CORE}")
message(STATUS "  QWidget frontend:   ${QGRAPHPLOT_BUILD_WIDGET_FRONTEND}")
message(STATUS "  QML frontend:       ${QGRAPHPLOT_BUILD_QML_FRONTEND}")
message(STATUS "  Examples:           ${QGRAPHPLOT_BUILD_EXAMPLES}")
message(STATUS "  Tests:              ${QGRAPHPLOT_BUILD_TESTS}")
message(STATUS "  Coverage:           ${QGRAPHPLOT_BUILD_COVERAGE}")
message(STATUS "")
