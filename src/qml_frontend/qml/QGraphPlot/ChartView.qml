import QtQuick
import QGraphPlot 0.1 as GP

GP.QmlChartView {
    id: root

    // X-axis and Y-axis instance bindings (for rendering labels in QML)
    property GP.QmlAxis xAxis: null
    property GP.QmlAxis yAxis: null

    // Label style. Uses theme values when a theme is set, otherwise falls back to defaults.
    readonly property color labelColor: root.theme ? root.theme.textColor : "#666666"
    readonly property int labelPixelSize: root.theme ? root.theme.fontPixelSize : 11
    readonly property string labelFontFamily: root.theme ? root.theme.fontFamily : ""

    // Chart title rendering
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

    // X-axis (horizontal) label rendering
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

    // Y-axis (vertical) label rendering
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
