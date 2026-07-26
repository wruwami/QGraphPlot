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
    series/QLineSeries.h
    series/QLineSeries.cpp
    series/QScatterSeries.h
    series/QScatterSeries.cpp
    theme/QGraphPlotTheme.h
    theme/QGraphPlotTheme.cpp
)

set(_core_public_headers
    qgraphplot_global.h
    model/QAbstractSeriesModel.h
    model/QRingBufferSeriesModel.h
    transform/QCoordinateTransform.h
    transform/QScaleEngine.h
    series/QAbstractSeries.h
    series/QLineSeries.h
    series/QScatterSeries.h
    theme/QGraphPlotTheme.h
)

qt_add_library(QGraphPlotCore ${_core_lib_type}
    ${_core_sources}
)
set_target_properties(QGraphPlotCore PROPERTIES AUTOMOC ON)

# Emit the metatypes JSON for this target so the QML module can re-export core
# types (QGraphPlotTheme) with QML_FOREIGN — qmltyperegistrar resolves foreign
# types only from the metatypes of the libraries it links against.
qt_extract_metatypes(QGraphPlotCore)

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
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
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

# ── Installation rules ───────────────────────────────────────
install(TARGETS QGraphPlotCore
    EXPORT QGraphPlotTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install public headers preserving directory structure
install(FILES qgraphplot_global.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qgraphplot
)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/qgraphplot/qgraphplot_version.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qgraphplot
)
install(FILES model/QAbstractSeriesModel.h model/QRingBufferSeriesModel.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qgraphplot/model
)
install(FILES series/QAbstractSeries.h series/QLineSeries.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qgraphplot/series
)
install(FILES transform/QCoordinateTransform.h transform/QScaleEngine.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qgraphplot/transform
)
install(FILES theme/QGraphPlotTheme.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/qgraphplot/theme
)

message(STATUS "  QGraphPlotCore: ${_core_lib_type} library")
