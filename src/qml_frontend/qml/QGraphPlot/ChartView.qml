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

    // ── Zoom / pan / rubberband interaction (issue #64) ──────────
    // Saved range for double-click reset (mirrors WidgetChartView state).
    property double _savedXMin: 0.0
    property double _savedXMax: 10.0
    property double _savedYMin: 0.0
    property double _savedYMax: 10.0
    property bool _rangeSaved: false

    function _saveRange() {
        if (!_rangeSaved) {
            _savedXMin = root.xMin;
            _savedXMax = root.xMax;
            _savedYMin = root.yMin;
            _savedYMax = root.yMax;
            _rangeSaved = true;
        }
    }

    // Wheel → zoom centered at cursor
    WheelHandler {
        id: wheelZoom
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        onWheel: (event) => {
            const plotL = root.marginLeft;
            const plotR = root.width  - root.marginRight;
            const plotT = root.marginTop;
            const plotB = root.height - root.marginBottom;
            const plotW = plotR - plotL;
            const plotH = plotB - plotT;
            if (plotW <= 0 || plotH <= 0) return;

            const px = event.x;
            const py = event.y;
            if (px < plotL || px > plotR || py < plotT || py > plotB) return;

            const degrees = event.angleDelta.y / 8.0;
            const factor = Math.pow(0.9, degrees / 15.0);

            const dataX = root.xMin + (px - plotL) / plotW * (root.xMax - root.xMin);
            const dataY = root.yMax - (py - plotT) / plotH * (root.yMax - root.yMin);

            root._saveRange();
            if (root.zoomXEnabled && root.xMax !== root.xMin) {
                const spanX = root.xMax - root.xMin;
                const newSpanX = spanX * factor;
                const tX = (dataX - root.xMin) / spanX;
                root.setXRange(dataX - tX * newSpanX, dataX + (1.0 - tX) * newSpanX);
            }
            if (root.zoomYEnabled && root.yMax !== root.yMin) {
                const spanY = root.yMax - root.yMin;
                const newSpanY = spanY * factor;
                const tY = (dataY - root.yMin) / spanY;
                root.setYRange(dataY - tY * newSpanY, dataY + (1.0 - tY) * newSpanY);
            }
        }
    }

    // Left-drag → pan (translate both axes relative to press position)
    DragHandler {
        id: panHandler
        acceptedButtons: Qt.LeftButton
        target: null

        property double startXMin: 0
        property double startXMax: 10
        property double startYMin: 0
        property double startYMax: 10

        onActiveChanged: {
            if (active) {
                startXMin = root.xMin;
                startXMax = root.xMax;
                startYMin = root.yMin;
                startYMax = root.yMax;
                root._saveRange();
            }
        }
        onActiveTranslationChanged: {
            if (!active) return;
            const plotW = root.width  - root.marginLeft - root.marginRight;
            const plotH = root.height - root.marginTop  - root.marginBottom;
            if (plotW <= 0 || plotH <= 0) return;
            const spanX = startXMax - startXMin;
            const spanY = startYMax - startYMin;
            if (spanX === 0 || spanY === 0) return;
            const dx = -activeTranslation.x / plotW * spanX;
            const dy =  activeTranslation.y / plotH * spanY;
            if (root.zoomXEnabled) root.setXRange(startXMin + dx, startXMax + dx);
            if (root.zoomYEnabled) root.setYRange(startYMin + dy, startYMax + dy);
        }
    }

    // Right-drag → rubberband area zoom
    DragHandler {
        id: rubberbandHandler
        acceptedButtons: Qt.RightButton
        target: null

        property real rbX1: 0
        property real rbY1: 0
        property real rbX2: 0
        property real rbY2: 0

        onActiveChanged: {
            if (active) {
                rbX1 = centroid.pressPosition.x;
                rbY1 = centroid.pressPosition.y;
                rbX2 = centroid.position.x;
                rbY2 = centroid.position.y;
            } else {
                const w = Math.abs(rbX2 - rbX1);
                const h = Math.abs(rbY2 - rbY1);
                if (w > 4 && h > 4) {
                    const plotL = root.marginLeft;
                    const plotR = root.width  - root.marginRight;
                    const plotT = root.marginTop;
                    const plotB = root.height - root.marginBottom;
                    const plotW = plotR - plotL;
                    const plotH = plotB - plotT;
                    if (plotW <= 0 || plotH <= 0) return;

                    const bx1 = Math.max(Math.min(rbX1, rbX2), plotL);
                    const bx2 = Math.min(Math.max(rbX1, rbX2), plotR);
                    const by1 = Math.max(Math.min(rbY1, rbY2), plotT);
                    const by2 = Math.min(Math.max(rbY1, rbY2), plotB);
                    if (bx2 <= bx1 || by2 <= by1) return;

                    const xSpan = root.xMax - root.xMin;
                    const ySpan = root.yMax - root.yMin;
                    if (xSpan === 0 || ySpan === 0) return;
                    const newXMin = root.xMin + (bx1 - plotL) / plotW * xSpan;
                    const newXMax = root.xMin + (bx2 - plotL) / plotW * xSpan;
                    const newYMax = root.yMax - (by1 - plotT) / plotH * ySpan;
                    const newYMin = root.yMax - (by2 - plotT) / plotH * ySpan;

                    root._saveRange();
                    if (root.zoomXEnabled) root.setXRange(newXMin, newXMax);
                    if (root.zoomYEnabled) root.setYRange(newYMin, newYMax);
                }
            }
        }
        onCentroidChanged: {
            if (active) {
                rbX2 = centroid.position.x;
                rbY2 = centroid.position.y;
            }
        }
    }

    // Rubberband rectangle shown while right-drag is active
    Rectangle {
        id: rubberbandRect
        visible: rubberbandHandler.active
        x: Math.min(rubberbandHandler.rbX1, rubberbandHandler.rbX2)
        y: Math.min(rubberbandHandler.rbY1, rubberbandHandler.rbY2)
        width:  Math.abs(rubberbandHandler.rbX2 - rubberbandHandler.rbX1)
        height: Math.abs(rubberbandHandler.rbY2 - rubberbandHandler.rbY1)
        color: Qt.rgba(0, 0.47, 1, 0.15)
        border.color: Qt.rgba(0, 0.47, 1, 0.8)
        border.width: 1
    }

    //! Restores the viewport that was saved before the first zoom/pan gesture.
    //! No-op when no snapshot is held.
    function resetZoom() {
        if (!_rangeSaved) return;
        setXRange(_savedXMin, _savedXMax);
        setYRange(_savedYMin, _savedYMax);
        _rangeSaved = false;
    }

    // Double-click → restore pre-zoom range
    TapHandler {
        acceptedButtons: Qt.LeftButton
        onDoubleTapped: root.resetZoom()
    }
}
