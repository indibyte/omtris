import QtQuick
import QtQuick.Layouts

Item {
    id: root

    property bool open: false
    signal close()

    visible: root.open
    anchors.fill: parent

    readonly property var shortcuts: [
        { keys: qsTr("← →"), action: qsTr("Move") },
        { keys: qsTr("↓"), action: qsTr("Soft drop") },
        { keys: qsTr("↑ / X"), action: qsTr("Rotate") },
        { keys: qsTr("Z"), action: qsTr("Rotate back") },
        { keys: qsTr("C"), action: qsTr("Hold piece") },
        { keys: qsTr("Space"), action: qsTr("Hard drop") },
        { keys: qsTr("P"), action: qsTr("Pause") },
        { keys: qsTr("R"), action: qsTr("Restart") },
        { keys: qsTr("Q"), action: qsTr("Quit") },
        { keys: qsTr("?"), action: qsTr("Help") }
    ]

    Rectangle {
        anchors.fill: parent
        color: Qt.rgba(0, 0, 0, 0.6)

        MouseArea {
            anchors.fill: parent
            onClicked: root.close()
        }
    }

    Rectangle {
        anchors.centerIn: parent
        width: Math.min(root.width - 60, 400)
        height: content.height + 56
        radius: Math.min(14, theme.radius)
        color: theme.surface
        border.color: Qt.rgba(1, 1, 1, 0.07)
        border.width: 1

        MouseArea {
            anchors.fill: parent
            onPressed: mouse.accepted = true
        }

        ColumnLayout {
            id: content
            anchors.centerIn: parent
            width: parent.width - 48
            spacing: 10

            Text {
                Layout.fillWidth: true
                text: qsTr("Shortcuts")
                color: theme.foreground
                font.pixelSize: 18
                font.weight: Font.DemiBold
            }

            Repeater {
                model: root.shortcuts

                delegate: RowLayout {
                    required property var modelData

                    Layout.fillWidth: true
                    spacing: 12

                    Text {
                        Layout.preferredWidth: 72
                        text: modelData.keys
                        color: theme.accent
                        font.pixelSize: 13
                        font.family: "monospace"
                    }

                    Text {
                        Layout.fillWidth: true
                        text: modelData.action
                        color: theme.foreground
                        font.pixelSize: 13
                        elide: Text.ElideRight
                    }
                }
            }

            Text {
                Layout.fillWidth: true
                Layout.topMargin: 4
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("Press ? to close")
                color: theme.muted
                font.pixelSize: 12
            }
        }
    }
}