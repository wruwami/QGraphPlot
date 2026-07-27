// Legend.qml — QML wrapper for the QmlLegend C++ backend (issue #65).
//
// Root is GP.QmlLegend so that `Legend { chart: chartView }` in host QML
// exposes QmlLegend's C++ properties (chart, items, position) directly.
// The Repeater builds one Row per series: a coloured rectangle marker
// followed by the series name.  Rows whose series is currently hidden are
// made invisible; the MouseArea on the outer Item toggles visibility on click.
import QtQuick
import QGraphPlot 0.1 as GP

GP.QmlLegend {
    id: root

    Column {
        spacing: 4

        Repeater {
            model: root.items
            delegate: Row {
                spacing: 6
                visible: modelData.visible

                Rectangle {
                    width: 12
                    height: 12
                    color: modelData.color
                }

                Text {
                    text: modelData.name
                }
            }
        }
    }
}
