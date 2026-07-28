// Legend.qml — QML wrapper for the QmlLegend C++ backend (issue #65).
//
// Root is GP.QmlLegend so that `Legend { chart: chartView }` in host QML
// exposes QmlLegend's C++ properties (chart, items, position) directly.
// The Repeater builds one Row per series: a coloured rectangle marker
// followed by the series name.  Clicking a row toggles the series visibility
// via modelData.toggle(); hidden series are shown at reduced opacity so the
// row itself remains clickable.
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
                opacity: modelData.visible ? 1.0 : 0.4

                TapHandler {
                    onTapped: modelData.toggle()
                }

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
