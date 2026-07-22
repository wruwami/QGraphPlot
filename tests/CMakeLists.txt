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

# ── tst_ringbuffer ────────────────────────────────────────────
qt_add_test(tst_ringbuffer
    SOURCES tst_ringbuffer.cpp
    LIBRARIES ${_test_libs}
)

# The test source includes a private .moc include at the bottom which
# expects AUTOMOC to have processed tst_ringbuffer.cpp. qt_add_test
# enables AUTOMOC by default for the target, so no extra setup is needed.
