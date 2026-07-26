# ═══════════════════════════════════════════════════════════════
# QGraphPlot qml_frontend — QML QuickItem and Scene Graph views
# ═══════════════════════════════════════════════════════════════

# QML 모듈 생성. URI는 QGraphPlot으로 설정하여 QML에서 import QGraphPlot이 가능하게 함.
# C++ 소스 파일과 QML 파일들을 함께 묶어 모듈을 정의합니다.
set(CMAKE_AUTOMOC ON)

qt_add_qml_module(qml_frontend
    URI QGraphPlot
    VERSION 0.1
    DEPENDENCIES
        QtQuick
    SOURCES
        QmlChartView.h QmlChartView.cpp
        QmlLineSeries.h QmlLineSeries.cpp
        QmlScatterSeries.h QmlScatterSeries.cpp
        QmlAxis.h QmlAxis.cpp
        QmlTheme.h
    QML_FILES
        qml/QGraphPlot/ChartView.qml
        qml/QGraphPlot/LineSeries.qml
        qml/QGraphPlot/ScatterSeries.qml
)

# QML 모듈이 QtQuick과 공유 C++ 코어(QGraphPlotCore)를 참조하도록 링크 설정
target_link_libraries(qml_frontend
    PUBLIC
        QGraphPlot::Core
    PRIVATE
        Qt6::Quick
        Qt6::Qml
)

# 인클루드 경로 설정 (core 소스 위치 참조를 위해)
target_include_directories(qml_frontend
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Windows 빌드용 AUTOMOC 활성화
set_target_properties(qml_frontend PROPERTIES AUTOMOC ON)

if(NOT TARGET QGraphPlot::Qml)
    add_library(QGraphPlot::Qml ALIAS qml_frontend)
endif()

install(TARGETS qml_frontend
    EXPORT QGraphPlotTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
