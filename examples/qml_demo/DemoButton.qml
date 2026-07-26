import QtQuick
import QGraphPlot

// 데모용 최소 버튼. QtQuick.Controls 스타일 의존성 없이 테마 색상만 사용한다.
Rectangle {
    id: root

    property alias text: label.text
    property Theme theme: null
    property bool highlighted: false

    signal clicked()

    width: label.implicitWidth + 20
    height: label.implicitHeight + 12
    radius: 4
    border.width: 1
    border.color: root.theme ? root.theme.axisColor : "#495057"
    color: root.highlighted ? border.color
                            : (root.theme ? root.theme.plotAreaColor : "#FFFFFF")

    Text {
        id: label
        anchors.centerIn: parent
        font.pixelSize: root.theme ? root.theme.fontPixelSize + 1 : 12
        color: root.highlighted ? (root.theme ? root.theme.plotAreaColor : "#FFFFFF")
                                : (root.theme ? root.theme.textColor : "#495057")
    }

    MouseArea {
        anchors.fill: parent
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
