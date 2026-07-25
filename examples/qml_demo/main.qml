import QtQuick
import QtQuick.Window
import QGraphPlot

Window {
    id: rootWindow
    width: 800
    height: 600
    visible: true
    title: "QGraphPlot QML Demo"
    color: "#F8F9FA" // Sleek light gray background

    ChartView {
        id: chart
        anchors.fill: parent
        anchors.margins: 30

        xMin: 0.0
        xMax: 20.0
        yMin: -1.5
        yMax: 1.5

        marginLeft: 60
        marginRight: 30
        marginTop: 40
        marginBottom: 50

        xAxis: horizAxis
        yAxis: vertAxis

        Axis {
            id: horizAxis
            orientation: Qt.Horizontal
            tickCount: 11
            color: "#495057"
            gridColor: "#DEE2E6"
            showGrid: true
        }

        Axis {
            id: vertAxis
            orientation: Qt.Vertical
            tickCount: 7
            color: "#495057"
            gridColor: "#DEE2E6"
            showGrid: true
        }

        LineSeries {
            id: series
            model: chartModel
            color: "#007ACC" // Premium deep blue
            name: "Sine Wave"
        }
    }
}
