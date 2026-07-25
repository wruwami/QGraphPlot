# ═══════════════════════════════════════════════════════════════
# QGraphPlot widget_frontend — QWidget front-end stub
# ═══════════════════════════════════════════════════════════════

set(CMAKE_AUTOMOC ON)

qt_add_library(widget_frontend STATIC
    WidgetChartView.h
    WidgetChartView.cpp
)

set_target_properties(widget_frontend PROPERTIES AUTOMOC ON)

target_link_libraries(widget_frontend
    PUBLIC
        QGraphPlot::Core
        Qt6::Widgets
)

target_include_directories(widget_frontend
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

if(NOT TARGET QGraphPlot::Widget)
    add_library(QGraphPlot::Widget ALIAS widget_frontend)
endif()
