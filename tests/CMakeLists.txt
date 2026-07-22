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

# ── tst_ringbuffer ────────────────────────────────────────────
qt_add_test(tst_ringbuffer
    SOURCES tst_ringbuffer.cpp
    LIBRARIES ${_test_libs}
)

# ── tst_coordinate ────────────────────────────────────────────
qt_add_test(tst_coordinate
    SOURCES tst_coordinate.cpp
    LIBRARIES ${_test_libs}
)

# ── tst_scaleengine ───────────────────────────────────────────
qt_add_test(tst_scaleengine
    SOURCES tst_scaleengine.cpp
    LIBRARIES ${_test_libs}
)

# The test source includes a private .moc include at the bottom which
# expects AUTOMOC to have processed tst_ringbuffer.cpp. qt_add_test
# enables AUTOMOC by default for the target, so no extra setup is needed.
