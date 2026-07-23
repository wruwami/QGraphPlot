# ═══════════════════════════════════════════════════════════════
# QGraphPlotCore — shared C++ core library
#
# This target is consumed by both the QML and Widget frontends
# (QGraphPlot_MVP_Plan.md § Architecture).
# ═══════════════════════════════════════════════════════════════

# ── Build-time option: shared vs static ───────────────────────
# Default: SHARED (matches QCustomPlot / Qt Charts distribution model).
# Override with -DQGRAPHPLOT_BUILD_SHARED=OFF to get a static lib.
option(QGRAPHPLOT_BUILD_SHARED "Build QGraphPlotCore as a shared library" ON)

set(_core_lib_type STATIC)
if(QGRAPHPLOT_BUILD_SHARED)
    set(_core_lib_type SHARED)
endif()

# ── Generate version header (VERSIONING.md §3) ───────────────
# Single source of truth for the version is the root CMakeLists.txt.
# Do NOT hard-code version numbers in headers.
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/qgraphplot_version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/qgraphplot/qgraphplot_version.h
    @ONLY
)

# ── Library target ────────────────────────────────────────────
# qt_add_library handles Qt-specific concerns (MOC, Android, etc.)
# automatically and is preferred over add_library for Qt 6 projects.
# Headers with Q_OBJECT must be listed so AUTOMOC picks them up.
set(_core_sources
    qgraphplot_global.h
    model/QAbstractSeriesModel.h
    model/QRingBufferSeriesModel.h
    model/QRingBufferSeriesModel.cpp
    transform/QCoordinateTransform.h
    transform/QCoordinateTransform.cpp
    transform/QScaleEngine.h
    transform/QScaleEngine.cpp
    series/QAbstractSeries.h
    series/QAbstractSeries.cpp
)

set(_core_public_headers
    qgraphplot_global.h
    model/QAbstractSeriesModel.h
    model/QRingBufferSeriesModel.h
    transform/QCoordinateTransform.h
    transform/QScaleEngine.h
    series/QAbstractSeries.h
)

qt_add_library(QGraphPlotCore ${_core_lib_type}
    ${_core_sources}
)
set_target_properties(QGraphPlotCore PROPERTIES AUTOMOC ON)

# ── Private compile definition: BUILDING_LIB ─────────────────
# Inside this target QGRAPHPLOT_EXPORT expands to Q_DECL_EXPORT.
target_compile_definitions(QGraphPlotCore PRIVATE QGRAPHPLOT_BUILDING_LIB)

# ── Public include directories ────────────────────────────────
# 1. Source dir  : for qgraphplot_global.h and model/ headers
# 2. Binary dir  : for the generated qgraphplot_version.h
target_include_directories(QGraphPlotCore
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include/qgraphplot>
    INTERFACE
        $<INSTALL_INTERFACE:include>
)

# ── Qt dependencies ───────────────────────────────────────────
# Gui is needed for QPointF / QRectF used by the model layer.
target_link_libraries(QGraphPlotCore PUBLIC Qt6::Core Qt6::Gui)

# ── C++ standard (inherit from root, but make explicit) ───────
target_compile_features(QGraphPlotCore PUBLIC cxx_std_17)

# ── Library naming on Windows (debug suffix) ─────────────────
if(WIN32 AND QGRAPHPLOT_BUILD_SHARED)
    set_target_properties(QGraphPlotCore PROPERTIES
        DEBUG_POSTFIX d
    )
endif()

# ── Alias target for in-tree consumers (QML/Widget frontends) ─
# Frontends link against QGraphPlot::Core regardless of whether the
# package was found via add_subdirectory() or find_package().
if(NOT TARGET QGraphPlot::Core)
    add_library(QGraphPlot::Core ALIAS QGraphPlotCore)
endif()

message(STATUS "  QGraphPlotCore: ${_core_lib_type} library")
