import QtQuick
import QtQuick.Window
import QGraphPlot

Window {
    id: rootWindow
    width: 800
    height: 600
    visible: true
    title: "QGraphPlot 60fps Streaming Demo"
    color: "#F8F9FA" // Sleek light gray background

    property int frameCounter: 0
    property int fps: 0
    property real lastSampleTime: 0

    onFrameSwapped: ++frameCounter

    Timer {
        interval: 1000
        running: true
        repeat: true
        onTriggered: {
            var currentTime = Date.now()
            if (rootWindow.lastSampleTime > 0) {
                var elapsedSeconds = (currentTime - rootWindow.lastSampleTime) / 1000.0
                rootWindow.fps = Math.round(rootWindow.frameCounter / elapsedSeconds)
            }
            rootWindow.lastSampleTime = currentTime
            rootWindow.frameCounter = 0
        }
    }

    ChartView {
        id: chart
        anchors.fill: parent
        anchors.margins: 30

        xMin: dataSource.xMin
        xMax: dataSource.xMax
        yMin: -1.5
        yMax: 1.5

        marginLeft: 60
        marginRight: 30
        marginTop: 40
        marginBottom: 50

        xAxis: horizAxis
        yAxis: vertAxis

        QmlAxis {
            id: horizAxis
            orientation: Qt.Horizontal
            tickCount: 11
            color: "#495057"
            gridColor: "#DEE2E6"
            showGrid: true
        }

        QmlAxis {
            id: vertAxis
            orientation: Qt.Vertical
            tickCount: 7
            color: "#495057"
            gridColor: "#DEE2E6"
            showGrid: true
        }

        LineSeries {
            id: series
            model: dataSource.model
            color: "#007ACC" // Premium deep blue
            name: "Sine Wave"
        }
    }

    Text {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 12
        text: rootWindow.fps + " FPS"
        font.pixelSize: 14
        color: "#495057"
    }
}
