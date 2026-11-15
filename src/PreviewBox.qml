import QtQuick

Item {
    id: root

    property int piece: 0
    property int cell: 11
    property color pieceColor: root.piece > 0 ? theme.pieceColors[(root.piece - 1) % 7] : "transparent"

    // The spawn orientation of each piece, trimmed to its filled bounds so the
    // preview is centered whatever the shape.
    readonly property var shape: {
        const matrices = {
            1: [[0, 0, 0, 0], [1, 1, 1, 1], [0, 0, 0, 0], [0, 0, 0, 0]],
            2: [[1, 1], [1, 1]],
            3: [[0, 1, 0], [1, 1, 1]],
            4: [[0, 1, 1], [1, 1, 0]],
            5: [[1, 1, 0], [0, 1, 1]],
            6: [[1, 0, 0], [1, 1, 1]],
            7: [[0, 0, 1], [1, 1, 1]]
        };
        const matrix = matrices[root.piece];
        if (!matrix)
            return { cells: [], cols: 0, rows: 0 };

        let minX = 99, minY = 99, maxX = -1, maxY = -1;
        for (let y = 0; y < matrix.length; ++y)
            for (let x = 0; x < matrix[y].length; ++x)
                if (matrix[y][x]) {
                    minX = Math.min(minX, x);
                    maxX = Math.max(maxX, x);
                    minY = Math.min(minY, y);
                    maxY = Math.max(maxY, y);
                }

        if (maxX < 0)
            return { cells: [], cols: 0, rows: 0 };

        const cells = [];
        for (let y = 0; y < matrix.length; ++y)
            for (let x = 0; x < matrix[y].length; ++x)
                if (matrix[y][x])
                    cells.push([x - minX, y - minY]);

        return { cells: cells, cols: maxX - minX + 1, rows: maxY - minY + 1 };
    }

    Repeater {
        model: root.shape.cells

        delegate: Rectangle {
            required property var modelData

            width: root.cell - 1
            height: root.cell - 1
            radius: Math.min(3, theme.radius)
            color: root.pieceColor
            x: (root.width - root.shape.cols * root.cell) / 2 + modelData[0] * root.cell
            y: (root.height - root.shape.rows * root.cell) / 2 + modelData[1] * root.cell
        }
    }

    Text {
        anchors.centerIn: parent
        visible: root.piece === 0
        text: "—"
        color: theme.muted
        font.pixelSize: 16
    }
}