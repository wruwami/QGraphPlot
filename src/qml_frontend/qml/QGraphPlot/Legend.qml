import QtQuick
import QGraphPlot 0.1 as GP

GP.Legend {
    id: root

    Column {
        spacing: 4
        padding: 8

        Repeater {
            model: root.items

            delegate: Row {
                id: rowDelegate
                spacing: 6
                property var legendItem: modelData
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
