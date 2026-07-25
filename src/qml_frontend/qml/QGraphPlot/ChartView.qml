import QtQuick
import QGraphPlot 0.1 as GP

GP.QmlChartView {
    id: root

    // X축 및 Y축 인스턴스 바인딩 (QML 단에서 라벨을 그리기 위함)
    property GP.QmlAxis xAxis: null
    property GP.QmlAxis yAxis: null

    // X축(가로축) 라벨 렌더링
    Repeater {
        model: root.xAxis ? root.xAxis.ticks : []
        delegate: Text {
            text: modelData.label
            x: root.mapToPixel(modelData.position, 0.0).x - width / 2
            y: root.height - root.marginBottom + 8
            font.pixelSize: 11
            color: "#666666"
        }
    }

    // Y축(세로축) 라벨 렌더링
    Repeater {
        model: root.yAxis ? root.yAxis.ticks : []
        delegate: Text {
            text: modelData.label
            x: root.marginLeft - width - 8
            y: root.mapToPixel(0.0, modelData.position).y - height / 2
            font.pixelSize: 11
            color: "#666666"
        }
    }
}
