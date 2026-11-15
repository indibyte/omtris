import QtQuick

Item {
    id: root

    property var grid: []
    property int cols: 10
    property int rows: 20
    property color boardColor: theme.darkerBackground
    property color accent: theme.accent
    property var colors: theme.pieceColors
    property int radius: Math.min(3, theme.radius)

    readonly property int cell: Math.max(
               6, Math.floor(Math.min((root.height - 48) / root.rows, (root.width - 48) / root.cols)))
    readonly property real pixelWidth: root.cell * root.cols
    readonly property real pixelHeight: root.cell * root.rows

    function cellColor(value) {
        if (value >= 15)
            return root.colors[(value - 15) % 7];
        if (value >= 8)
            return root.accent;
        if (value === 22)
            return "#ffffff";
        return root.colors[(value - 1) % 7];
    }

    Rectangle {
        objectName: "boardFrame"
        anchors.centerIn: parent
        width: root.pixelWidth + 20
        height: root.pixelHeight + 20
        radius: Math.min(12, theme.radius)
        color: theme.surface
        border.color: Qt.rgba(1, 1, 1, 0.06)
        border.width: 1

        Canvas {
            id: canvas
            objectName: "boardCanvas"
            anchors.centerIn: parent
            width: root.pixelWidth
            height: root.pixelHeight

            onWidthChanged: requestPaint()
            onHeightChanged: requestPaint()

            onPaint: {
                const ctx = getContext("2d");
                ctx.reset();

                const cell = root.cell;
                const pad = Math.max(1, Math.floor(cell * 0.07));
                const radius = root.radius;

                // The empty well, with a quiet grid under the blocks.
                ctx.fillStyle = root.boardColor;
                ctx.fillRect(0, 0, width, height);
                ctx.strokeStyle = Qt.rgba(1, 1, 1, 0.035);
                ctx.lineWidth = 1;
                ctx.beginPath();
                for (let x = 1; x < root.cols; ++x) {
                    ctx.moveTo(x * cell + 0.5, 0);
                    ctx.lineTo(x * cell + 0.5, height);
                }
                for (let y = 1; y < root.rows; ++y) {
                    ctx.moveTo(0, y * cell + 0.5);
                    ctx.lineTo(width, y * cell + 0.5);
                }
                ctx.stroke();

                for (let index = 0; index < root.grid.length; ++index) {
                    const value = root.grid[index];
                    if (!value)
                        continue;

                    const x = (index % root.cols) * cell;
                    const y = Math.floor(index / root.cols) * cell;
                    const ghost = value >= 8 && value < 15;

                    if (ghost) {
                        ctx.globalAlpha = 0.22;
                        ctx.fillStyle = root.accent;
                        canvas.roundRect(ctx, x + pad, y + pad, cell - 2 * pad, cell - 2 * pad, radius);
                        ctx.fill();
                        ctx.globalAlpha = 0.6;
                        ctx.lineWidth = 1;
                        ctx.strokeStyle = root.accent;
                        ctx.stroke();
                        ctx.globalAlpha = 1;
                    } else {
                        ctx.fillStyle = root.cellColor(value);
                        canvas.roundRect(ctx, x + pad, y + pad, cell - 2 * pad, cell - 2 * pad, radius);
                        ctx.fill();

                        // A light top edge gives each block a little body.
                        ctx.fillStyle = Qt.rgba(1, 1, 1, value === 22 ? 0.4 : 0.14);
                        const inset = pad + Math.min(radius, (cell - 2 * pad) / 2);
                        ctx.fillRect(x + inset, y + pad, cell - 2 * inset, 1);
                    }
                }
            }

            function roundRect(context, x, y, w, h, r) {
                const radius = Math.max(0, Math.min(r, w / 2, h / 2));
                context.beginPath();
                context.moveTo(x + radius, y);
                context.arcTo(x + w, y, x + w, y + h, radius);
                context.arcTo(x + w, y + h, x, y + h, radius);
                context.arcTo(x, y + h, x, y, radius);
                context.arcTo(x, y, x + w, y, radius);
                context.closePath();
            }
        }
    }

    Connections {
        target: backend
        function onGridChanged() {
            canvas.requestPaint();
        }
    }

    Connections {
        target: theme
        function onChanged() {
            canvas.requestPaint();
        }
    }
}