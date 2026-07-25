# ═══════════════════════════════════════════════════════════════
# QGraphPlot examples
# ═══════════════════════════════════════════════════════════════

# QML 데모 디렉토리를 빌드에 포함합니다. (QML 프론트엔드가 비활성화되면 건너뜁니다.)
if(QGRAPHPLOT_BUILD_QML_FRONTEND)
    add_subdirectory(qml_demo)
endif()
