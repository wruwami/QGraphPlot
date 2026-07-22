# ═══════════════════════════════════════════════════════════════
# QGraphPlot tests
#
# Uses QtTest (Qt's built-in test framework) and CTest for orchestration.
# Each test binary is registered via qt_add_test which wires up CTest
# automatically.
# ═══════════════════════════════════════════════════════════════

# Tests link against the core library target directly (build interface),
# not via the installed package, so they always exercise the in-tree code.
set(_test_libs QGraphPlot::Core)

# ── TestRingBuffer ────────────────────────────────────────────
qt_add_test(TestRingBuffer
    SOURCES TestRingBuffer.cpp
    LIBRARIES ${_test_libs}
)

# The test source includes a private .moc include at the bottom which
# expects AUTOMOC to have processed TestRingBuffer.cpp. qt_add_test
# enables AUTOMOC by default for the target, so no extra setup is needed.
