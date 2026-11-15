#include "backend.h"

#include <QSettings>
#include <QRandomGenerator>

#include <algorithm>
#include <utility>

namespace {

constexpr int kCols = 10;
constexpr int kRows = 20;
constexpr int kNextCount = 3;

// Rotation states are matrices, rotated on demand; the matrices are square, so
// a quarter turn is a transpose plus a flip.
QVector<QVector<int>> baseShape(int type)
{
    switch (type) {
    case 1: return {{0, 0, 0, 0}, {1, 1, 1, 1}, {0, 0, 0, 0}, {0, 0, 0, 0}};
    case 2: return {{1, 1}, {1, 1}};
    case 3: return {{0, 1, 0}, {1, 1, 1}, {0, 0, 0}};
    case 4: return {{0, 1, 1}, {1, 1, 0}, {0, 0, 0}};
    case 5: return {{1, 1, 0}, {0, 1, 1}, {0, 0, 0}};
    case 6: return {{1, 0, 0}, {1, 1, 1}, {0, 0, 0}};
    case 7: return {{0, 0, 1}, {1, 1, 1}, {0, 0, 0}};
    default: return {};
    }
}

QVector<QVector<int>> turned(const QVector<QVector<int>> &shape)
{
    const int n = shape.size();
    QVector<QVector<int>> result(n, QVector<int>(n, 0));
    for (int y = 0; y < n; ++y)
        for (int x = 0; x < n; ++x)
            result[x][n - 1 - y] = shape[y][x];
    return result;
}

int indexOf(int x, int y)
{
    return y * kCols + x;
}

} // namespace

Backend::Backend(QObject *parent, const QString &settingsFile)
    : QObject(parent),
      m_board(kCols * kRows, 0),
      m_settingsFile(settingsFile) {
    m_gravity.setInterval(800);
    connect(&m_gravity, &QTimer::timeout, this, &Backend::tick);
    m_clearTimer.setSingleShot(true);
    m_clearTimer.setInterval(150);
    connect(&m_clearTimer, &QTimer::timeout, this, &Backend::finishClear);
    loadBest();
}

Backend::~Backend()
{
    m_gravity.stop();
    m_clearTimer.stop();
    // A game closed mid-run still keeps its best score.
    if (m_score > m_best) {
        m_best = m_score;
        saveBest();
    }
}

void Backend::start()
{
    reset();
    m_running = true;
    m_paused = false;
    m_gameOver = false;
    applyGravity();
    spawn();
    rebuildGrid();
    emit statsChanged();
    emit holdChanged();
    emit canHoldChanged();
    emit stateChanged();
    emit gridChanged();
    if (!m_gameOver)
        m_gravity.start();
}

void Backend::togglePause()
{
    if (!m_running || m_gameOver)
        return;
    m_paused = !m_paused;
    if (m_paused)
        m_gravity.stop();
    else
        m_gravity.start();
    emit stateChanged();
}

void Backend::move(int dx)
{
    if (!canControl())
        return;
    if (!collides(m_shape, m_x + dx, m_y)) {
        m_x += dx;
        rebuildGrid();
        emit gridChanged();
    }
}

void Backend::rotate(int direction)
{
    if (!canControl())
        return;
    QVector<QVector<int>> rotated = m_shape;
    const int turns = direction < 0 ? 3 : 1;
    for (int i = 0; i < turns; ++i)
        rotated = turned(rotated);

    // Simple wall kicks: nudge sideways, then up, until the piece fits.
    static const int kicks[5][2] = {{0, 0}, {-1, 0}, {1, 0}, {-2, 0}, {2, 0}};
    for (const auto &kick : kicks) {
        if (!collides(rotated, m_x + kick[0], m_y + kick[1])) {
            m_shape = rotated;
            m_x += kick[0];
            m_y += kick[1];
            rebuildGrid();
            emit gridChanged();
            return;
        }
    }
    if (!collides(rotated, m_x, m_y - 1)) {
        m_shape = rotated;
        --m_y;
        rebuildGrid();
        emit gridChanged();
    }
}

void Backend::softDrop()
{
    if (!canControl())
        return;
    if (!collides(m_shape, m_x, m_y + 1)) {
        ++m_y;
        ++m_score;
        emit statsChanged();
        rebuildGrid();
        emit gridChanged();
    } else {
        lockPiece();
    }
}

void Backend::hardDrop()
{
    if (!canControl())
        return;
    const int start = m_y;
    while (!collides(m_shape, m_x, m_y + 1))
        ++m_y;
    m_score += 2 * (m_y - start);
    emit statsChanged();
    lockPiece();
}

void Backend::holdPiece()
{
    if (!canControl() || !m_canHold)
        return;

    const int current = m_type;
    if (m_hold == 0) {
        m_hold = current;
        spawn();
    } else {
        const int swap = m_hold;
        m_hold = current;
        m_type = swap;
        m_shape = baseShape(swap);
        m_x = (kCols - m_shape.first().size()) / 2;
        m_y = swap == 1 ? -1 : 0;
        if (collides(m_shape, m_x, m_y))
            endGame();
    }

    m_canHold = false;
    emit holdChanged();
    emit canHoldChanged();
    rebuildGrid();
    emit gridChanged();
}

void Backend::tick()
{
    if (!canControl())
        return;
    if (!collides(m_shape, m_x, m_y + 1)) {
        ++m_y;
        rebuildGrid();
        emit gridChanged();
    } else {
        lockPiece();
    }
}

void Backend::reset()
{
    m_gravity.stop();
    m_clearTimer.stop();
    m_board.fill(0);
    m_queue.clear();
    m_bag.clear();
    m_pendingRows.clear();
    m_shape.clear();
    m_type = 0;
    m_x = 0;
    m_y = 0;
    m_hold = 0;
    m_canHold = true;
    m_score = 0;
    m_lines = 0;
    m_level = 1;
    m_grid.clear();
}

void Backend::spawn()
{
    if (m_queue.size() <= kNextCount)
        refillQueue();
    m_type = m_queue.takeFirst();
    refillQueue();
    m_shape = baseShape(m_type);
    m_x = (kCols - m_shape.first().size()) / 2;
    m_y = m_type == 1 ? -1 : 0;
    m_canHold = true;
    emit canHoldChanged();
    updateNext();
    emit nextChanged();

    if (collides(m_shape, m_x, m_y)) {
        m_type = 0;
        m_shape.clear();
        endGame();
    }
}

void Backend::refillQueue()
{
    while (m_queue.size() < kNextCount + 1) {
        if (m_bag.isEmpty()) {
            m_bag = {1, 2, 3, 4, 5, 6, 7};
            for (int i = m_bag.size() - 1; i > 0; --i)
                std::swap(m_bag[i], m_bag[QRandomGenerator::global()->bounded(i + 1)]);
        }
        m_queue.append(m_bag.takeFirst());
    }
}

void Backend::updateNext()
{
    m_next.clear();
    for (int i = 0; i < kNextCount && i < m_queue.size(); ++i)
        m_next.append(m_queue[i]);
}

bool Backend::collides(const QVector<QVector<int>> &shape, int px, int py) const
{
    for (int y = 0; y < shape.size(); ++y) {
        for (int x = 0; x < shape[y].size(); ++x) {
            if (!shape[y][x])
                continue;
            const int boardX = px + x;
            const int boardY = py + y;
            if (boardX < 0 || boardX >= kCols || boardY >= kRows)
                return true;
            if (boardY >= 0 && m_board[indexOf(boardX, boardY)] != 0)
                return true;
        }
    }
    return false;
}

bool Backend::canControl() const
{
    return m_running && !m_paused && !m_gameOver && m_type != 0;
}

void Backend::rebuildGrid()
{
    m_grid.clear();
    m_grid.reserve(kCols * kRows);

    for (int y = 0; y < kRows; ++y) {
        const bool flash = m_pendingRows.contains(y);
        for (int x = 0; x < kCols; ++x) {
            int value = m_board[indexOf(x, y)];
            if (flash && value != 0)
                value = 22;
            m_grid.append(value);
        }
    }

    if (m_type == 0 || m_gameOver)
        return;

    // Ghost first, so the falling piece paints over it when it rests.
    int ghostY = m_y;
    while (!collides(m_shape, m_x, ghostY + 1))
        ++ghostY;
    for (int y = 0; y < m_shape.size(); ++y) {
        for (int x = 0; x < m_shape[y].size(); ++x) {
            if (!m_shape[y][x])
                continue;
            const int by = ghostY + y;
            if (by < 0 || by >= kRows)
                continue;
            const int index = indexOf(m_x + x, by);
            if (m_board[index] == 0)
                m_grid[index] = 7 + m_type;
        }
    }

    for (int y = 0; y < m_shape.size(); ++y) {
        for (int x = 0; x < m_shape[y].size(); ++x) {
            if (!m_shape[y][x])
                continue;
            const int by = m_y + y;
            if (by < 0 || by >= kRows)
                continue;
            m_grid[indexOf(m_x + x, by)] = 14 + m_type;
        }
    }
}

void Backend::lockPiece()
{
    if (m_type == 0)
        return;

    bool placed = false;
    for (int y = 0; y < m_shape.size(); ++y) {
        for (int x = 0; x < m_shape[y].size(); ++x) {
            if (!m_shape[y][x])
                continue;
            const int by = m_y + y;
            if (by >= 0 && by < kRows) {
                m_board[indexOf(m_x + x, by)] = m_type;
                placed = true;
            }
        }
    }

    if (!placed) {
        m_type = 0;
        m_shape.clear();
        endGame();
        return;
    }

    m_type = 0;
    m_shape.clear();
    m_canHold = true;
    emit canHoldChanged();

    m_pendingRows.clear();
    for (int y = 0; y < kRows; ++y) {
        bool full = true;
        for (int x = 0; x < kCols; ++x) {
            if (m_board[indexOf(x, y)] == 0) {
                full = false;
                break;
            }
        }
        if (full)
            m_pendingRows.append(y);
    }

    if (!m_pendingRows.isEmpty()) {
        m_gravity.stop();
        rebuildGrid();
        emit gridChanged();
        m_clearTimer.start();
    } else {
        spawn();
        rebuildGrid();
        emit gridChanged();
        if (m_running && !m_paused && !m_gameOver)
            m_gravity.start();
    }
}

void Backend::finishClear()
{
    if (m_pendingRows.isEmpty())
        return;

    const int cleared = m_pendingRows.size();
    QVector<int> board(kCols * kRows, 0);
    int writeY = kRows - 1;
    for (int y = kRows - 1; y >= 0; --y) {
        if (m_pendingRows.contains(y))
            continue;
        for (int x = 0; x < kCols; ++x)
            board[indexOf(x, writeY)] = m_board[indexOf(x, y)];
        --writeY;
    }
    m_board = board;
    m_pendingRows.clear();

    static const int points[5] = {0, 100, 300, 500, 800};
    m_score += points[std::min(cleared, 4)] * m_level;
    m_lines += cleared;

    const int level = 1 + m_lines / 10;
    if (level != m_level) {
        m_level = level;
        applyGravity();
    }
    emit statsChanged();
    emit linesCleared(cleared);

    spawn();
    rebuildGrid();
    emit gridChanged();
    if (m_running && !m_paused && !m_gameOver)
        m_gravity.start();
}

void Backend::endGame()
{
    m_running = false;
    m_gameOver = true;
    m_paused = false;
    m_gravity.stop();
    m_clearTimer.stop();
    if (m_score > m_best) {
        m_best = m_score;
        saveBest();
        emit bestChanged();
    }
    emit stateChanged();
}

void Backend::applyGravity()
{
    int ms = 800;
    for (int i = 1; i < m_level; ++i)
        ms = static_cast<int>(ms * 0.85);
    m_gravity.setInterval(qMax(70, ms));
}

void Backend::loadBest()
{
    if (m_settingsFile.isEmpty()) {
        QSettings settings;
        m_best = settings.value("best", 0).toInt();
        return;
    }
    QSettings settings(m_settingsFile, QSettings::IniFormat);
    m_best = settings.value("best", 0).toInt();
}

void Backend::saveBest()
{
    if (m_settingsFile.isEmpty()) {
        QSettings settings;
        settings.setValue("best", m_best);
        settings.sync();
        return;
    }
    QSettings settings(m_settingsFile, QSettings::IniFormat);
    settings.setValue("best", m_best);
    settings.sync();
}

void Backend::setCell(int x, int y, int value)
{
    if (x < 0 || x >= kCols || y < 0 || y >= kRows)
        return;
    m_board[indexOf(x, y)] = value;
    rebuildGrid();
    emit gridChanged();
}

void Backend::setPiece(int type)
{
    if (type < 1 || type > 7)
        return;
    m_type = type;
    m_shape = baseShape(type);
    m_x = (kCols - m_shape.first().size()) / 2;
    m_y = type == 1 ? -1 : 0;
    rebuildGrid();
    emit gridChanged();
}