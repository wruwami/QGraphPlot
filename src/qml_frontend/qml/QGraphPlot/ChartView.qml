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
        y: Math.max(0, root.marginTop / 2 - height / 2)
        font.pixelSize: root.labelPixelSize + 2
        font.bold: true
        font.family: root.labelFontFamily !== "" ? root.labelFontFamily : Qt.application.font.family
        color: root.labelColor
    }

    // X-axis title rendering (centered below the plot area)
    Text {
        id: xAxisTitle
        visible: root.xAxis !== null && root.xAxis.title !== ""
        text: root.xAxis ? root.xAxis.title : ""
        x: root.marginLeft + (root.width - root.marginLeft - root.marginRight) / 2 - width / 2
        y: root.height - root.marginBottom / 2 - height / 2
        font.pixelSize: root.labelPixelSize
        font.bold: true
        font.family: root.labelFontFamily !== "" ? root.labelFontFamily : Qt.application.font.family
        color: root.labelColor
    }

    // Y-axis title rendering (rotated, centered beside the plot area)
    Text {
        id: yAxisTitle
        visible: root.yAxis !== null && root.yAxis.title !== ""
        text: root.yAxis ? root.yAxis.title : ""
        rotation: -90
        x: root.marginLeft / 2 - width / 2
        y: root.marginTop + (root.height - root.marginTop - root.marginBottom) / 2 - height / 2
        font.pixelSize: root.labelPixelSize
        font.bold: true
        font.family: root.labelFontFamily !== "" ? root.labelFontFamily : Qt.application.font.family
        color: root.labelColor
    }

    // Ensure margins account for axis title dimensions
    Component.onCompleted: {
        updateMarginsForTitles();
    }

    Connections {
        target: root.xAxis
        function onTitleChanged() { updateMarginsForTitles(); }
    }

    Connections {
        target: root.yAxis
        function onTitleChanged() { updateMarginsForTitles(); }
    }

    function updateMarginsForTitles() {
        // Reserve space for X-axis: tick labels (≈20px) + spacing (8px) + title height + spacing (4px)
        const xTitleSpace = xAxisTitle.visible ? xAxisTitle.implicitHeight + 4 : 0;
        const minBottomMargin = 20 + 8 + xTitleSpace;
        if (root.marginBottom < minBottomMargin) {
            root.marginBottom = minBottomMargin;
        }

        // Reserve space for Y-axis: tick labels (≈40px) + spacing (8px) + title width (rotated, so height is used) + spacing (4px)
        const yTitleSpace = yAxisTitle.visible ? yAxisTitle.implicitHeight + 4 : 0;
        const minLeftMargin = 40 + 8 + yTitleSpace;
        if (root.marginLeft < minLeftMargin) {
            root.marginLeft = minLeftMargin;
        }
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
