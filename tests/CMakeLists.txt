# ═══════════════════════════════════════════════════════════════
# QGraphPlot tests
#
# Uses QtTest (Qt's built-in test framework) and CTest for orchestration.
# Each test binary is registered via qt_add_test which wires up CTest
# automatically.
# ═══════════════════════════════════════════════════════════════

# Tests link against the core library target directly (build interface),
# not via the installed package, so they always exercise the in-tree code.
if(NOT COMMAND qt_add_test)
    macro(qt_add_test name)
        cmake_parse_arguments(ARG "" "" "SOURCES;LIBRARIES" ${ARGN})
        add_executable(${name} ${ARG_SOURCES})
        target_link_libraries(${name} PRIVATE Qt6::Test ${ARG_LIBRARIES})
        set_target_properties(${name} PROPERTIES AUTOMOC ON)
        add_test(NAME ${name} COMMAND ${name})
    endmacro()
endif()

set(_test_libs QGraphPlot::Core)

# ── TestRingBuffer ────────────────────────────────────────────
qt_add_test(TestRingBuffer
    SOURCES TestRingBuffer.cpp
    LIBRARIES ${_test_libs}
)

# ── TestCoordinate ────────────────────────────────────────────
# Regression: dropped by a merge conflict resolution; restored here.
qt_add_test(TestCoordinate
    SOURCES TestCoordinate.cpp
    LIBRARIES ${_test_libs}
)

# ── TestScaleEngine ───────────────────────────────────────────
# Regression: dropped by a merge conflict resolution; restored here.
qt_add_test(TestScaleEngine
    SOURCES TestScaleEngine.cpp
    LIBRARIES ${_test_libs}
)

# ── TestAutoScaler ────────────────────────────────────────────
# Shared axis auto-range helper (issue #63).
qt_add_test(TestAutoScaler
    SOURCES TestAutoScaler.cpp
    LIBRARIES ${_test_libs}
)

# ── TestAbstractSeries ────────────────────────────────────────
# Regression: dropped by a merge conflict resolution; restored here.
qt_add_test(TestAbstractSeries
    SOURCES TestAbstractSeries.cpp
    LIBRARIES ${_test_libs}
)

# ── TestStaticModel ───────────────────────────────────────────
qt_add_test(TestStaticModel
    SOURCES TestStaticModel.cpp
    LIBRARIES ${_test_libs}
)

# ── TestTheme ─────────────────────────────────────────────────
qt_add_test(TestTheme
    SOURCES TestTheme.cpp
    LIBRARIES ${_test_libs}
)

# ── TestQmlChartView ──────────────────────────────────────────
# Only built when the QML frontend itself is (QGRAPHPLOT_BUILD_QML_FRONTEND).
if(TARGET qml_frontend)
    qt_add_test(TestQmlChartView
        SOURCES TestQmlChartView.cpp
        LIBRARIES qml_frontend Qt6::Quick Qt6::Qml
    )
endif()

# ── TestQmlAxis ───────────────────────────────────────────────
# Only built when the QML frontend itself is (QGRAPHPLOT_BUILD_QML_FRONTEND).
if(TARGET qml_frontend)
    qt_add_test(TestQmlAxis
        SOURCES TestQmlAxis.cpp
        LIBRARIES qml_frontend Qt6::Quick Qt6::Qml
    )
endif()

# ── TestQmlLegend ─────────────────────────────────────────────
# Legend widget auto-populated from a chart's series (issue #65). Only
# built when the QML frontend itself is (QGRAPHPLOT_BUILD_QML_FRONTEND).
if(TARGET qml_frontend)
    qt_add_test(TestQmlLegend
        SOURCES TestQmlLegend.cpp
        LIBRARIES qml_frontend Qt6::Quick Qt6::Qml
    )
endif()

# ── TestWidgetChartView ───────────────────────────────────────
# Only built when the Widget frontend itself is (QGRAPHPLOT_BUILD_WIDGET_FRONTEND).
if(TARGET widget_frontend)
    qt_add_test(TestWidgetChartView
        SOURCES TestWidgetChartView.cpp
        LIBRARIES widget_frontend Qt6::Widgets
    )
endif()

# The test sources include a private .moc include at the bottom which
# expects AUTOMOC to have processed the corresponding .cpp. qt_add_test
# enables AUTOMOC by default for the target, so no extra setup is needed.
