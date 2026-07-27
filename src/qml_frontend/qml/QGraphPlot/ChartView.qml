import QtQuick
import QGraphPlot 0.1 as GP

GP.QmlChartView {
    id: root

    // X 축 및 Y 축 인스턴스 바인딩 (QML 단에서 라벨을 그리기 위함)
    property GP.QmlAxis xAxis: null
    property GP.QmlAxis yAxis: null

    // 라벨 스타일. theme 가 지정되면 테마 값을, 아니면 기존 기본값을 사용한다.
    readonly property color labelColor: root.theme ? root.theme.textColor : "#666666"
    readonly property int labelPixelSize: root.theme ? root.theme.fontPixelSize : 11
    readonly property string labelFontFamily: root.theme ? root.theme.fontFamily : ""

    // 차트 타이틀 렌더링
    Text {
        visible: root.title !== ""
        text: root.title
        x: root.width / 2 - width / 2
        y: root.marginTop / 2 - height / 2
        font.pixelSize: root.labelPixelSize + 2
        font.bold: true
        font.family: root.labelFontFamily !== "" ? root.labelFontFamily : Qt.application.font.family
        color: root.labelColor
    }

    // X 축 (가로축) 라벨 렌더링
    Repeater {
        model: root.xAxis ? root.xAxis.ticks : []
        delegate: Text {
            text: modelData.label
            x: root.mapToPixel(modelData.position, 0.0).x - width / 2
            y: root.height - root.marginBottom + 8
            font.pixelSize: root.labelPixelSize
            font.family: root.labelFontFamily !== "" ? root.labelFontFamily : Qt.application.font.family
            color: root.labelColor
        }
    }

    // Y 축 (세로축) 라벨 렌더링
    Repeater {
        model: root.yAxis ? root.yAxis.ticks : []
        delegate: Text {
            text: modelData.label
            x: root.marginLeft - width - 8
            y: root.mapToPixel(0.0, modelData.position).y - height / 2
            font.pixelSize: root.labelPixelSize
            font.family: root.labelFontFamily !== "" ? root.labelFontFamily : Qt.application.font.family
            color: root.labelColor
        }
    }
}
