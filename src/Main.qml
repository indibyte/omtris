import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtMultimedia

ApplicationWindow {
    id: win

    width: 940
    height: 730
    minimumWidth: 840
    minimumHeight: 620
    visible: true
    title: qsTr("Omtris")
    color: theme.darkerBackground

    Material.theme: Material.Dark
    Material.accent: theme.accent
    Material.roundedScale: theme.radius

    property bool helpVisible: false
    property bool pausedForHelp: false

    // Losing focus should not cost a life.
    onActiveChanged: {
        if (!active && backend.running && !backend.paused && !backend.gameOver)
            backend.togglePause();
    }

    function toggleHelp() {
        if (!win.helpVisible) {
            if (backend.running && !backend.paused) {
                backend.togglePause();
                win.pausedForHelp = true;
            }
            win.helpVisible = true;
        } else {
            win.helpVisible = false;
            if (win.pausedForHelp) {
                backend.togglePause();
                win.pausedForHelp = false;
            }
        }
    }

    function primaryAction() {
        if (win.helpVisible) {
            win.toggleHelp();
            return;
        }
        if (!backend.running || backend.gameOver) {
            backend.start();
            return;
        }
        if (backend.paused) {
            backend.togglePause();
            return;
        }
        backend.hardDrop();
    }

    function dismiss() {
        if (win.helpVisible) {
            win.toggleHelp();
            return;
        }
        if (backend.running && !backend.gameOver)
            backend.togglePause();
    }

    SoundEffect {
        id: clearSound
        objectName: "clearSound"
        source: "qrc:/sounds/clear.wav"
        volume: 0.55
    }

    SoundEffect {
        id: tetrisSound
        objectName: "tetrisSound"
        source: "qrc:/sounds/tetris.wav"
        volume: 0.65
    }

    Connections {
        target: backend
        function onLinesCleared(count) {
            if (count >= 4)
                tetrisSound.play();
            else
                clearSound.play();
        }
    }

    Shortcut { sequence: "Q"; context: Qt.ApplicationShortcut; onActivated: Qt.quit() }
    Shortcut { sequence: "?"; context: Qt.ApplicationShortcut; onActivated: win.toggleHelp() }
    Shortcut { sequence: "Left"; context: Qt.ApplicationShortcut; onActivated: backend.move(-1) }
    Shortcut { sequence: "Right"; context: Qt.ApplicationShortcut; onActivated: backend.move(1) }
    Shortcut { sequence: "Down"; context: Qt.ApplicationShortcut; onActivated: backend.softDrop() }
    Shortcut { sequence: "Up"; context: Qt.ApplicationShortcut; onActivated: backend.rotate(1) }
    Shortcut { sequence: "X"; context: Qt.ApplicationShortcut; onActivated: backend.rotate(1) }
    Shortcut { sequence: "Z"; context: Qt.ApplicationShortcut; onActivated: backend.rotate(-1) }
    Shortcut { sequence: "C"; context: Qt.ApplicationShortcut; onActivated: backend.holdPiece() }
    Shortcut { sequence: "Space"; context: Qt.ApplicationShortcut; onActivated: win.primaryAction() }
    Shortcut { sequence: "Return"; context: Qt.ApplicationShortcut; onActivated: win.primaryAction() }
    Shortcut { sequence: "Enter"; context: Qt.ApplicationShortcut; onActivated: win.primaryAction() }
    Shortcut { sequence: "P"; context: Qt.ApplicationShortcut; onActivated: backend.togglePause() }
    Shortcut { sequence: "Escape"; context: Qt.ApplicationShortcut; onActivated: win.dismiss() }
    Shortcut { sequence: "R"; context: Qt.ApplicationShortcut; onActivated: backend.start() }

    Item {
        anchors.fill: parent
        LayoutMirroring.enabled: Qt.application.layoutDirection === Qt.RightToLeft
        LayoutMirroring.childrenInherit: true

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 26
            spacing: 18

            RowLayout {
                objectName: "header"
                Layout.fillWidth: true
                spacing: 16

                ColumnLayout {
                    spacing: 2

                    Text {
                        text: qsTr("OMTRIS")
                        color: theme.foreground
                        font.pixelSize: 26
                        font.weight: Font.DemiBold
                        font.letterSpacing: 4
                    }

                    Text {
                        text: qsTr("A falling-block game for Omarchy")
                        color: theme.muted
                        font.pixelSize: 13
                    }
                }

                Item {
                    Layout.fillWidth: true
                }

                Text {
                    Layout.alignment: Qt.AlignVCenter
                    text: qsTr("Best %1").arg(backend.best.toLocaleString(Qt.locale(), 'f', 0))
                    color: theme.muted
                    font.pixelSize: 13
                }

                Text {
                    Layout.alignment: Qt.AlignVCenter
                    text: backend.paused ? qsTr("Resume") : qsTr("Pause")
                    color: backend.paused ? theme.accent : theme.muted
                    font.pixelSize: 13

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onEntered: parent.color = theme.accent
                        onExited: parent.color = backend.paused ? theme.accent : theme.muted
                        onClicked: backend.togglePause()
                    }
                }

                Text {
                    Layout.alignment: Qt.AlignVCenter
                    text: qsTr("Shortcuts")
                    color: theme.muted
                    font.pixelSize: 13

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onEntered: parent.color = theme.accent
                        onExited: parent.color = theme.muted
                        onClicked: win.toggleHelp()
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: false
                Layout.fillHeight: true
                Layout.alignment: Qt.AlignHCenter
                spacing: 22

                Item {
                    objectName: "boardArea"
                    Layout.preferredWidth: 340
                    Layout.fillHeight: true

                    Board {
                        objectName: "board"
                        anchors.fill: parent
                        grid: backend.grid
                    }

                    Rectangle {
                        anchors.fill: parent
                        visible: !backend.running || backend.paused
                        color: Qt.rgba(0, 0, 0, 0.55)

                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: win.primaryAction()
                        }

                        Rectangle {
                            anchors.centerIn: parent
                            width: Math.min(parent.width - 48, 320)
                            height: stateColumn.height + 44
                            radius: Math.min(12, theme.radius)
                            color: theme.surface
                            border.color: Qt.rgba(1, 1, 1, 0.06)
                            border.width: 1

                            ColumnLayout {
                                id: stateColumn
                                anchors.centerIn: parent
                                width: parent.width - 44
                                spacing: 8

                                Text {
                                    Layout.fillWidth: true
                                    horizontalAlignment: Text.AlignHCenter
                                    text: backend.gameOver
                                          ? qsTr("Game Over")
                                          : backend.paused ? qsTr("Paused") : qsTr("Omtris")
                                    color: backend.gameOver ? theme.pieceColors[4] : theme.foreground
                                    font.pixelSize: 22
                                    font.weight: Font.DemiBold
                                }

                                Text {
                                    Layout.fillWidth: true
                                    horizontalAlignment: Text.AlignHCenter
                                    text: backend.gameOver
                                          ? qsTr("Score %1").arg(backend.score.toLocaleString(Qt.locale(), 'f', 0))
                                          : backend.paused ? qsTr("Press P to resume")
                                                           : qsTr("Press Enter to start")
                                    color: theme.muted
                                    font.pixelSize: 13
                                }

                                Text {
                                    Layout.fillWidth: true
                                    horizontalAlignment: Text.AlignHCenter
                                    visible: backend.gameOver
                                    text: qsTr("Press Enter to play again")
                                    color: theme.accent
                                    font.pixelSize: 13
                                }
                            }
                        }
                    }
                }

                ColumnLayout {
                    objectName: "sidePanel"
                    Layout.fillWidth: false
                    Layout.preferredWidth: 260
                    Layout.fillHeight: true
                    spacing: 14

                    StatCard {
                        Layout.fillWidth: true
                        label: qsTr("Score")
                        value: backend.score.toLocaleString(Qt.locale(), 'f', 0)
                        highlight: true
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 14

                        StatCard {
                            Layout.fillWidth: true
                            label: qsTr("Level")
                            value: backend.level
                        }

                        StatCard {
                            Layout.fillWidth: true
                            label: qsTr("Lines")
                            value: backend.lines
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 116
                        radius: Math.min(8, theme.radius)
                        color: theme.surface
                        border.color: Qt.rgba(1, 1, 1, 0.05)
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 13
                            spacing: 6

                            Text {
                                text: qsTr("Hold")
                                color: theme.muted
                                font.pixelSize: 12
                            }

                            Item {
                                Layout.fillWidth: true
                                Layout.fillHeight: true

                                PreviewBox {
                                    anchors.centerIn: parent
                                    piece: backend.hold
                                    opacity: backend.canHold ? 1 : 0.35
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.minimumHeight: 148
                        radius: Math.min(8, theme.radius)
                        color: theme.surface
                        border.color: Qt.rgba(1, 1, 1, 0.05)
                        border.width: 1

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 13
                            spacing: 8

                            Text {
                                text: qsTr("Next")
                                color: theme.muted
                                font.pixelSize: 12
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                spacing: 2

                                Repeater {
                                    model: backend.next

                                    delegate: Item {
                                        required property var modelData

                                        Layout.fillWidth: true
                                        Layout.fillHeight: true

                                        PreviewBox {
                                            anchors.centerIn: parent
                                            piece: modelData
                                            cell: 12
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                        text: qsTr("← → move  ·  ↑ rotate  ·  Space drop  ·  C hold  ·  P pause")
                        color: theme.muted
                        font.pixelSize: 11
                        wrapMode: Text.WordWrap
                    }
                }
            }
        }
    }

    HelpOverlay {
        open: win.helpVisible
        onClose: win.toggleHelp()
    }
}