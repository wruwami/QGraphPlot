import QtQuick
import QtQuick.Window
import QGraphPlot

Window {
    id: rootWindow
    width: 900
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
    // ── 실시간 데이터 스트리밍 (Phase 0.7) ────────────────────────
    // 매 프레임(16ms) 1K 포인트를 QRingBufferSeriesModel::appendRange()로 밀어넣어
    // 60K 포인트 용량의 링 버퍼를 지속적으로 스트리밍한다.
    StreamingDataSource {
        id: dataSource
    }

    Timer {
        interval: 16
        running: true
        repeat: true
        onTriggered: dataSource.generateFrame()
    }
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
            tickCount: 6
            color: "#495057"
            gridColor: "#DEE2E6"
            showGrid: true
        }

        QmlAxis {
            id: vertAxis
            orientation: Qt.Vertical
            tickCount: 5
            color: "#495057"
            gridColor: "#DEE2E6"
            showGrid: true
        }

        LineSeries {
            id: series
            model: dataSource.model
            color: "#007ACC" // Premium deep blue
            name: "Streaming Signal"
        }
    }

    Rectangle {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 12
        width: fpsLabel.implicitWidth + 16
        height: fpsLabel.implicitHeight + 10
        radius: 4
        color: "#CC212529"

        Text {
            id: fpsLabel
            anchors.centerIn: parent
            text: "FPS: " + rootWindow.fps + "  |  Points: " + dataSource.pointCount
            color: "white"
            font.pixelSize: 14
            font.bold: true
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
