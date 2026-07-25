# ═══════════════════════════════════════════════════════════════
# QGraphPlot examples — qml_demo
# ═══════════════════════════════════════════════════════════════

# executable 타겟 선언
qt_add_executable(qml_demo
    main.cpp
)

# 링킹 설정 (qml_frontend 모듈 및 QtQuick 추가)
target_link_libraries(qml_demo
    PRIVATE
        qml_frontend
        Qt6::Quick
)

# 메인 QML 파일을 리소스로 패키징하기 위해 QML 모듈화 처리
qt_add_qml_module(qml_demo
    URI qml_demo
    VERSION 1.0
    QML_FILES
        main.qml
)
