import QtQuick
import QtQuick.Window
import QGraphPlot

Window {
    id: rootWindow
    width: 900
    height: 600
    visible: true
    title: "QGraphPlot QML Demo — 60fps Streaming"
    color: chartTheme.backgroundColor

    // ── 테마 (Phase 0.9) ─────────────────────────────────────────
    // 코어의 QGraphPlotTheme를 그대로 사용한다 (Widget 프론트엔드와 동일 타입).
    Theme {
        id: chartTheme
    }

    // ── FPS 측정 (Phase 0.7 DoD: 프레임 드랍 가시화) ──────────────
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

    ChartView {
        id: chart
        anchors.fill: parent
        anchors.margins: 30

        theme: chartTheme

        xMin: dataSource.xMin
        xMax: dataSource.xMax
        yMin: -1.2
        yMax: 1.2

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
            color: chartTheme.axisColor
            gridColor: chartTheme.gridColor
            lineWidth: chartTheme.axisWidth
            gridWidth: chartTheme.gridWidth
            showGrid: true
        }

        QmlAxis {
            id: vertAxis
            orientation: Qt.Vertical
            tickCount: 5
            color: chartTheme.axisColor
            gridColor: chartTheme.gridColor
            lineWidth: chartTheme.axisWidth
            gridWidth: chartTheme.gridWidth
            showGrid: true
        }

        LineSeries {
            id: series
            model: dataSource.model
            color: chartTheme.seriesColor(0)
            lineWidth: chartTheme.seriesLineWidth * rootWindow.lineWidthScale
            name: "Streaming Signal"
        }
    }

    // 라인 두께 슬라이더 대신 버튼으로 배수를 토글 (QtQuick.Controls 의존성 회피)
    property real lineWidthScale: 1.0
    property bool dashed: false

    Row {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 12
        spacing: 8

        Repeater {
            model: [
                { label: "Light", preset: Theme.Light },
                { label: "Dark", preset: Theme.Dark },
                { label: "Scientific", preset: Theme.Scientific }
            ]

            delegate: DemoButton {
                text: modelData.label
                highlighted: chartTheme.preset === modelData.preset
                theme: chartTheme
                onClicked: chartTheme.applyPreset(modelData.preset)
            }
        }

        DemoButton {
            text: "Width ×" + rootWindow.lineWidthScale.toFixed(1)
            theme: chartTheme
            onClicked: rootWindow.lineWidthScale =
                       rootWindow.lineWidthScale >= 3.0 ? 1.0 : rootWindow.lineWidthScale + 1.0
        }

        DemoButton {
            text: rootWindow.dashed ? "Dashed" : "Solid"
            theme: chartTheme
            onClicked: {
                rootWindow.dashed = !rootWindow.dashed
                series.dashPattern = rootWindow.dashed ? [6, 4] : []
            }
        }
    }

    Rectangle {
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 12
        width: fpsLabel.implicitWidth + 16
        height: fpsLabel.implicitHeight + 10
        radius: 4
        color: Qt.rgba(chartTheme.axisColor.r, chartTheme.axisColor.g, chartTheme.axisColor.b, 0.8)

        Text {
            id: fpsLabel
            anchors.centerIn: parent
            text: "FPS: " + rootWindow.fps + "  |  Points: " + dataSource.pointCount
            color: chartTheme.backgroundColor
            font.pixelSize: 14
            font.bold: true
        }
    }
}
