import QtQuick
import QGraphPlot 0.1 as GP

//! Public QML legend type — skins the C++ QmlLegend and places the entry
//! column according to `position` (Qt::Alignment). Same pattern as
//! ChartView.qml / LineSeries.qml (C++ registered as QmlLegend, not Legend).
//!
//! Combine vertical and horizontal flags, e.g. `Qt.AlignBottom | Qt.AlignRight`.
//! Default is Qt.AlignTop (top-left). See issue #93.
GP.QmlLegend {
    id: root

    readonly property bool _alignTop: (root.position & Qt.AlignTop) !== 0
    readonly property bool _alignBottom: (root.position & Qt.AlignBottom) !== 0
    readonly property bool _alignLeft: (root.position & Qt.AlignLeft) !== 0
    readonly property bool _alignRight: (root.position & Qt.AlignRight) !== 0
    readonly property bool _alignHCenter: (root.position & Qt.AlignHCenter) !== 0
    readonly property bool _alignVCenter: (root.position & Qt.AlignVCenter) !== 0

    Column {
        id: legendColumn
        objectName: "legendColumn"
        spacing: 4
        padding: 8

        anchors.top: root._alignTop ? parent.top : undefined
        anchors.bottom: root._alignBottom ? parent.bottom : undefined
        anchors.verticalCenter: root._alignVCenter
                                 || (!root._alignTop && !root._alignBottom)
                                 ? parent.verticalCenter
                                 : undefined

        // Default AlignTop alone → top-left (previous implicit layout).
        anchors.left: root._alignLeft
                      || (!root._alignRight && !root._alignHCenter && root._alignTop
                          && !root._alignBottom && !root._alignVCenter)
                      || (!root._alignLeft && !root._alignRight && !root._alignHCenter
                          && !root._alignTop && !root._alignBottom && !root._alignVCenter)
                      ? parent.left
                      : undefined
        anchors.right: root._alignRight ? parent.right : undefined
        anchors.horizontalCenter: root._alignHCenter ? parent.horizontalCenter : undefined

        Repeater {
            model: root.items

            delegate: Row {
                id: rowDelegate
                spacing: 6
                property var legendItem: modelData
                // Dim when series is hidden (toggle keeps row visible for re-show).
                opacity: legendItem && !legendItem.visible ? 0.45 : 1.0

                Rectangle {
                    width: 16
                    height: 16
                    color: legendItem ? legendItem.color : "transparent"
                    border.color: legendItem ? legendItem.color : "gray"
                    border.width: 1

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if (legendItem) {
                                legendItem.toggle()
                            }
                        }
                    }
                }

                Text {
                    text: legendItem ? legendItem.name : ""
                    anchors.verticalCenter: parent.verticalCenter
                    font.pixelSize: 12
                    color: root.chart && root.chart.theme ? root.chart.theme.textColor : "#333333"
                }
            }
        }
    }
}
