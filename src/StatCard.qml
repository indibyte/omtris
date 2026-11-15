import QtQuick

Rectangle {
    id: root

    property string label: ""
    property string value: ""
    property bool highlight: false

    implicitHeight: 74
    radius: Math.min(8, theme.radius)
    color: theme.surface
    border.color: Qt.rgba(1, 1, 1, 0.05)
    border.width: 1

    Column {
        anchors.fill: parent
        anchors.margins: 13
        spacing: 3

        Text {
            width: parent.width
            text: root.label
            color: theme.muted
            font.pixelSize: 12
            elide: Text.ElideRight
        }

        Text {
            width: parent.width
            text: root.value
            color: root.highlight ? theme.accent : theme.foreground
            font.pixelSize: 25
            font.weight: Font.DemiBold
            elide: Text.ElideRight
        }
    }
}