// Theme watching follows Omacut (MIT, David Heinemeier Hansson).
#include "theme.h"

#include <QColor>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>

#include <cmath>

namespace {

const QString kDefaultAccent = "#FFD60A";
const QStringList kDefaultPieces = {"#83c092", "#dbbc7f", "#d699b6", "#a7c080",
                                    "#e67e80", "#7fbbb3", "#e09d7f"};

QString pick(const QHash<QString, QString> &colors, const QString &key, const QString &fallback)
{
    return colors.value(key, fallback);
}

} // namespace

Theme::Theme(const QString &directory, QObject *parent, const QString &hyprctl)
    : QObject(parent),
      m_directory(directory.isEmpty() ? QDir::homePath() + "/.local/state/omarchy/current" : directory) {
    m_debounce.setSingleShot(true);
    m_debounce.setInterval(80);
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this] { m_debounce.start(); });
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this] { m_debounce.start(); });
    connect(&m_debounce, &QTimer::timeout, this, &Theme::reload);

    // Corners follow Hyprland's effective rounding, which themes and personal
    // overrides can both set, so ask hyprctl rather than reading its config.
    // Without Hyprland the corners stay sharp, like Omarchy's default.
    m_hyprctl = !hyprctl.isEmpty() ? hyprctl
              : directory.isEmpty() && !qEnvironmentVariableIsEmpty("HYPRLAND_INSTANCE_SIGNATURE")
                  ? QStandardPaths::findExecutable("hyprctl") : QString();
    m_roundingTimeout.setSingleShot(true);
    m_roundingTimeout.setInterval(500);
    connect(&m_roundingTimeout, &QTimer::timeout, &m_roundingProcess, &QProcess::kill);
    connect(&m_roundingProcess, &QProcess::errorOccurred, this, [this] { m_roundingTimeout.stop(); });
    connect(&m_roundingProcess, &QProcess::finished, this, [this](int code, QProcess::ExitStatus status) {
        m_roundingTimeout.stop();
        const auto value = QJsonDocument::fromJson(m_roundingProcess.readAllStandardOutput()).object().value("int");
        if (code != 0 || status != QProcess::ExitStatus::NormalExit || !value.isDouble() || value.toInt(-1) < 0)
            return;
        if (value.toInt() != m_radius) {
            m_radius = value.toInt();
            emit changed();
        }
    });
    m_roundingPoll.setInterval(1000);
    connect(&m_roundingPoll, &QTimer::timeout, this, &Theme::refreshRounding);
    if (!m_hyprctl.isEmpty())
        m_roundingPoll.start();
    reload();
}

Theme::~Theme()
{
    if (m_roundingProcess.state() != QProcess::NotRunning) {
        m_roundingProcess.kill();
        m_roundingProcess.waitForFinished(1000);
    }
}

void Theme::refreshRounding()
{
    if (m_hyprctl.isEmpty() || m_roundingProcess.state() != QProcess::NotRunning)
        return;
    m_roundingProcess.start(m_hyprctl, {"-j", "getoption", "decoration:rounding"});
    m_roundingTimeout.start();
}

// colors.toml is a flat list of key = "value" lines; only hex colors matter
// here, so anything else (mode, comments) is skipped.
QHash<QString, QString> Theme::readColors(const QString &path)
{
    QHash<QString, QString> colors;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
        return colors;

    static const QRegularExpression hex("^#[0-9a-fA-F]{6}$");
    const auto lines = QString::fromUtf8(file.readAll()).split(QLatin1Char('\n'));
    for (const auto &line : lines) {
        const int equals = line.indexOf(QLatin1Char('='));
        if (equals < 0)
            continue;
        const QString key = line.left(equals).trimmed();
        QString value = line.mid(equals + 1).trimmed();
        if (value.size() >= 2 && value.startsWith(QLatin1Char('"')) && value.endsWith(QLatin1Char('"')))
            value = value.mid(1, value.size() - 2);
        if (!key.isEmpty() && hex.match(value).hasMatch())
            colors.insert(key, value);
    }
    return colors;
}

QString Theme::accent() const
{
    return pick(m_colors, "accent", kDefaultAccent);
}

// Black or white, whichever reads on the accent (WCAG relative luminance).
QString Theme::accentForeground() const
{
    const QColor color(accent());
    auto linear = [](double v) { return v <= .04045 ? v / 12.92 : std::pow((v + .055) / 1.055, 2.4); };
    const double luminance = .2126 * linear(color.redF()) + .7152 * linear(color.greenF())
                             + .0722 * linear(color.blueF());
    return luminance > .179 ? "black" : "white";
}

QString Theme::background() const
{
    return pick(m_colors, "background", "#181d20");
}

QString Theme::darkerBackground() const
{
    return pick(m_colors, "darker_background", pick(m_colors, "background", "#181d20"));
}

QString Theme::surface() const
{
    return pick(m_colors, "dark_background", pick(m_colors, "background", "#21272c"));
}

QString Theme::surfaceAlt() const
{
    return pick(m_colors, "lighter_background", pick(m_colors, "dark_background", "#343f44"));
}

QString Theme::foreground() const
{
    return pick(m_colors, "foreground", "#d3c6aa");
}

// Secondary text on a dark window: the theme's light foreground, not its
// dark one, which is meant for text on light surfaces.
QString Theme::muted() const
{
    return pick(m_colors, "light_foreground", pick(m_colors, "dark_foreground",
                                                    pick(m_colors, "foreground", "#9da9a0")));
}

// I, O, T, S, Z, J, L take the theme's own palette, so the blocks belong to
// whatever theme is on.
QStringList Theme::pieceColors() const
{
    static const char *keys[] = {"cyan", "yellow", "magenta", "green", "red", "blue", "orange"};
    QStringList colors;
    colors.reserve(7);
    for (const char *key : keys) {
        const QString fallback = kDefaultPieces.at(colors.size());
        colors.append(pick(m_colors, QLatin1String(key), fallback));
    }
    return colors;
}

void Theme::reload()
{
    refreshRounding();
    const auto paths = m_watcher.files() + m_watcher.directories();
    if (!paths.isEmpty())
        m_watcher.removePaths(paths);

    // A theme switch replaces symlinks and files, so watch the parents too and
    // re-arm after every change. Watching the nearest existing ancestor means
    // installing Omarchy later works without restarting the app.
    QString ancestor = m_directory;
    while (!QFileInfo::exists(ancestor) && ancestor != "/")
        ancestor = QFileInfo(ancestor).absolutePath();
    QStringList candidates{ancestor, QFileInfo(ancestor).absolutePath(), m_directory,
                           m_directory + "/theme", m_directory + "/theme/colors.toml"};
    candidates.removeDuplicates();
    for (const auto &path : candidates)
        if (QFileInfo::exists(path))
            m_watcher.addPath(path);

    const QHash<QString, QString> colors = readColors(m_directory + "/theme/colors.toml");
    if (colors != m_colors) {
        m_colors = colors;
        emit changed();
    }
}