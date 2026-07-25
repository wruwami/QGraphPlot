# ═══════════════════════════════════════════════════════════════
# QGraphPlot examples — widget_demo
# ═══════════════════════════════════════════════════════════════

qt_add_executable(widget_demo
    main.cpp
)

target_link_libraries(widget_demo
    PRIVATE
        widget_frontend
        Qt6::Widgets
)
