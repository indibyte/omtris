#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QVariantList>
#include <QVector>

// The game's whole state: the 10×20 board, the falling piece, the queue, the
// hold slot, and the score. QML reads the flattened `grid` and calls the
// actions; every rule lives here so the tests can drive it without a display.
class Backend : public QObject {
    Q_OBJECT
    Q_PROPERTY(int width READ width CONSTANT)
    Q_PROPERTY(int height READ height CONSTANT)
    Q_PROPERTY(QVariantList grid READ grid NOTIFY gridChanged)
    Q_PROPERTY(QVariantList next READ next NOTIFY nextChanged)
    Q_PROPERTY(int hold READ hold NOTIFY holdChanged)
    Q_PROPERTY(bool canHold READ canHold NOTIFY canHoldChanged)
    Q_PROPERTY(int score READ score NOTIFY statsChanged)
    Q_PROPERTY(int lines READ lines NOTIFY statsChanged)
    Q_PROPERTY(int level READ level NOTIFY statsChanged)
    Q_PROPERTY(int best READ best NOTIFY bestChanged)
    Q_PROPERTY(bool running READ running NOTIFY stateChanged)
    Q_PROPERTY(bool paused READ paused NOTIFY stateChanged)
    Q_PROPERTY(bool gameOver READ gameOver NOTIFY stateChanged)

public:
    // A settings file keeps the best score; the default uses the app's own
    // QSettings, and tests pass a temporary path.
    explicit Backend(QObject *parent = nullptr, const QString &settingsFile = {});
    ~Backend() override;

    int width() const { return 10; }
    int height() const { return 20; }

    QVariantList grid() const { return m_grid; }
    QVariantList next() const { return m_next; }
    int hold() const { return m_hold; }
    bool canHold() const { return m_canHold; }
    int score() const { return m_score; }
    int lines() const { return m_lines; }
    int level() const { return m_level; }
    int best() const { return m_best; }
    bool running() const { return m_running; }
    bool paused() const { return m_paused; }
    bool gameOver() const { return m_gameOver; }

    Q_INVOKABLE void start();
    Q_INVOKABLE void togglePause();
    Q_INVOKABLE void move(int dx);
    Q_INVOKABLE void rotate(int direction);
    Q_INVOKABLE void softDrop();
    Q_INVOKABLE void hardDrop();
    Q_INVOKABLE void holdPiece();
    Q_INVOKABLE void tick();

    // Hooks the tests use to arrange a board or a piece; QML never needs them.
    int activeType() const { return m_type; }
    int activeX() const { return m_x; }
    int activeY() const { return m_y; }
    void setCell(int x, int y, int value);
    void setPiece(int type);

signals:
    void gridChanged();
    void nextChanged();
    void holdChanged();
    void canHoldChanged();
    void statsChanged();
    void linesCleared(int count);
    void bestChanged();
    void stateChanged();

private:
    void reset();
    void spawn();
    void refillQueue();
    void updateNext();
    void rebuildGrid();
    void lockPiece();
    void finishClear();
    void endGame();
    void applyGravity();
    void loadBest();
    void saveBest();
    bool collides(const QVector<QVector<int>> &shape, int px, int py) const;
    bool canControl() const;

    QVector<int> m_board;
    QVariantList m_grid;
    QVariantList m_next;
    QVector<int> m_queue;
    QVector<int> m_bag;
    QVector<int> m_pendingRows;
    QVector<QVector<int>> m_shape;

    int m_type = 0;
    int m_x = 0;
    int m_y = 0;
    int m_hold = 0;
    bool m_canHold = true;
    int m_score = 0;
    int m_lines = 0;
    int m_level = 1;
    int m_best = 0;
    bool m_running = false;
    bool m_paused = false;
    bool m_gameOver = false;

    QTimer m_gravity;
    QTimer m_clearTimer;
    QString m_settingsFile;
};