# ═══════════════════════════════════════════════════════════════
# QGraphPlot examples — qml_demo
# ═══════════════════════════════════════════════════════════════

# executable 타겟 선언.
# streamingdatasource.{h,cpp}는 아래 qt_add_qml_module()의 SOURCES로만 추가한다
# (qml_frontend와 동일한 패턴). 여기서 같이 나열하면 target_sources()가 두 번
# 호출되어 중복 등록된다.
qt_add_executable(qml_demo
    main.cpp
    StreamingDataSource.h
    StreamingDataSource.cpp
)

# qt_add_qml_module()의 메타타입 생성(qt6_extract_metatypes)이 요구하는 AUTOMOC 활성화
set_target_properties(qml_demo PROPERTIES AUTOMOC ON)

# 링킹 설정 (qml_frontend 모듈 및 QtQuick 추가)
target_link_libraries(qml_demo
    PRIVATE
        qml_frontend
        Qt6::Quick
)

# 메인 QML 파일을 리소스로 패키징하기 위해 QML 모듈화 처리.
# streamingdatasource.h는 QML_ELEMENT로 등록되므로 SOURCES에도 나열해야
# main.qml에서 import 없이 StreamingDataSource {}로 바로 사용할 수 있다.
qt_add_qml_module(qml_demo
    URI qml_demo
    VERSION 1.0
    SOURCES
        streamingdatasource.h
        streamingdatasource.cpp
    QML_FILES
        main.qml
)
