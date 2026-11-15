#include <QDir>
#include <QSignalSpy>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include "backend.h"
#include "theme.h"
#include "translator.h"

class OmtrisTests : public QObject {
    Q_OBJECT

private slots:
    void startIsClean()
    {
        QTemporaryDir dir;
        Backend backend(nullptr, dir.filePath("settings.conf"));
        QCOMPARE(backend.grid().size(), 0);
        QVERIFY(!backend.running());

        backend.start();
        QVERIFY(backend.running());
        QVERIFY(!backend.paused());
        QVERIFY(!backend.gameOver());
        QCOMPARE(backend.score(), 0);
        QCOMPARE(backend.grid().size(), 200);
        QCOMPARE(backend.next().size(), 3);
    }

    void gridHasOneActivePiece()
    {
        QTemporaryDir dir;
        Backend backend(nullptr, dir.filePath("settings.conf"));
        backend.start();

        int active = 0;
        for (const auto &value : backend.grid())
            if (value.toInt() >= 15)
                ++active;
        QCOMPARE(active, 4);
    }

    void movesWithinTheWalls()
    {
        QTemporaryDir dir;
        Backend backend(nullptr, dir.filePath("settings.conf"));
        backend.start();

        const int before = backend.activeX();
        backend.move(-1);
        QCOMPARE(backend.activeX(), before - 1);

        for (int i = 0; i < 20; ++i)
            backend.move(-1);
        QCOMPARE(backend.activeX(), 0);
    }

    void rotatesAndKeepsFourCells()
    {
        QTemporaryDir dir;
        Backend backend(nullptr, dir.filePath("settings.conf"));
        backend.start();
        backend.setPiece(1);
        backend.softDrop();

        backend.rotate(1);
        int active = 0;
        for (const auto &value : backend.grid())
            if (value.toInt() >= 15)
                ++active;
        QCOMPARE(active, 4);

        backend.rotate(-1);
        active = 0;
        for (const auto &value : backend.grid())
            if (value.toInt() >= 15)
                ++active;
        QCOMPARE(active, 4);
    }

    void softDropScores()
    {
        QTemporaryDir dir;
        Backend backend(nullptr, dir.filePath("settings.conf"));
        backend.start();

        const int y = backend.activeY();
        backend.softDrop();
        QCOMPARE(backend.activeY(), y + 1);
        QCOMPARE(backend.score(), 1);
    }

    void hardDropLocksAndSpawns()
    {
        QTemporaryDir dir;
        Backend backend(nullptr, dir.filePath("settings.conf"));
        backend.start();

        backend.hardDrop();
        QVERIFY(backend.score() > 0);

        int locked = 0, active = 0;
        for (const auto &value : backend.grid()) {
            const int cell = value.toInt();
            if (cell >= 1 && cell <= 7)
                ++locked;
            if (cell >= 15)
                ++active;
        }
        QCOMPARE(locked, 4);
        QCOMPARE(active, 4);
        QVERIFY(backend.next().size() == 3);
    }

    void clearsLines()
    {
        QTemporaryDir dir;
        Backend backend(nullptr, dir.filePath("settings.conf"));
        backend.start();

        // Leave exactly the four columns the I piece falls into.
        for (int x = 0; x < 10; ++x)
            if (x < 3 || x > 6)
                backend.setCell(x, 19, 2);
        backend.setPiece(1);
        QSignalSpy clears(&backend, &Backend::linesCleared);
        backend.hardDrop();

        QTRY_COMPARE(backend.lines(), 1);
        QVERIFY(backend.score() >= 100);
        QCOMPARE(backend.grid().size(), 200);
        QCOMPARE(clears.count(), 1);
        QCOMPARE(clears.at(0).at(0).toInt(), 1);
    }

    void clearsFourLines()
    {
        QTemporaryDir dir;
        Backend backend(nullptr, dir.filePath("settings.conf"));
        backend.start();

        for (int y = 16; y < 20; ++y)
            for (int x = 0; x < 10; ++x)
                if (x != 3)
                    backend.setCell(x, y, 2);
        backend.setPiece(1);
        backend.rotate(1);
        backend.move(-2);
        QSignalSpy clears(&backend, &Backend::linesCleared);
        backend.hardDrop();

        QTRY_COMPARE(backend.lines(), 4);
        QCOMPARE(clears.count(), 1);
        QCOMPARE(clears.at(0).at(0).toInt(), 4);
    }

    void holdsOncePerDrop()
    {
        QTemporaryDir dir;
        Backend backend(nullptr, dir.filePath("settings.conf"));
        backend.start();

        const int first = backend.activeType();
        QVERIFY(first >= 1 && first <= 7);
        backend.holdPiece();
        QCOMPARE(backend.hold(), first);
        QVERIFY(!backend.canHold());

        const int second = backend.activeType();
        backend.holdPiece();
        QCOMPARE(backend.activeType(), second);
    }

    void pausesGravity()
    {
        QTemporaryDir dir;
        Backend backend(nullptr, dir.filePath("settings.conf"));
        backend.start();

        backend.togglePause();
        QVERIFY(backend.paused());
        const int y = backend.activeY();
        backend.tick();
        QCOMPARE(backend.activeY(), y);

        backend.togglePause();
        QVERIFY(!backend.paused());
    }

    void gameOverWhenBlocked()
    {
        QTemporaryDir dir;
        Backend backend(nullptr, dir.filePath("settings.conf"));
        backend.start();
        backend.hardDrop();

        // Block the spawn area, leaving one hole so locking there does not
        // clear a line; the next spawn then has nowhere to appear.
        for (int y = 0; y < 2; ++y)
            for (int x = 0; x < 10; ++x)
                if (x != 9)
                    backend.setCell(x, y, 2);
        backend.tick();

        QTRY_VERIFY(backend.gameOver());
        QVERIFY(!backend.running());
        QVERIFY(!backend.paused());
    }

    void bestSurvivesRestart()
    {
        QTemporaryDir dir;
        const QString settings = dir.filePath("settings.conf");
        Backend backend(nullptr, settings);
        backend.start();
        backend.hardDrop();
        QVERIFY(backend.score() > 0);

        for (int y = 0; y < 2; ++y)
            for (int x = 0; x < 10; ++x)
                if (x != 9)
                    backend.setCell(x, y, 2);
        backend.tick();
        QTRY_VERIFY(backend.gameOver());
        QVERIFY(backend.best() > 0);

        const int best = backend.best();
        Backend again(nullptr, settings);
        QCOMPARE(again.best(), best);
    }

    void themeFallsBack()
    {
        QTemporaryDir dir;
        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QString("#FFD60A"));
        QCOMPARE(theme.accentForeground(), QString("black"));
        QCOMPARE(theme.background(), QString("#181d20"));
        QCOMPARE(theme.surface(), QString("#21272c"));
        QCOMPARE(theme.pieceColors().size(), 7);
    }

    void themeReadsTheColorsFile()
    {
        QTemporaryDir dir;
        QDir(dir.path()).mkpath("theme");
        QVERIFY(write(dir.path() + "/theme/colors.toml",
                      "mode = \"dark\"\n"
                      "accent = \"#112233\"\n"
                      "background = \"#101010\"\n"
                      "dark_background = \"#181818\"\n"
                      "foreground = \"#eeeeee\"\n"
                      "cyan = \"#00ffff\"\n"
                      "red = \"#ff0000\"\n"));

        Theme theme(dir.path());
        QCOMPARE(theme.accent(), QString("#112233"));
        QCOMPARE(theme.accentForeground(), QString("white"));
        QCOMPARE(theme.background(), QString("#101010"));
        QCOMPARE(theme.surface(), QString("#181818"));
        QCOMPARE(theme.foreground(), QString("#eeeeee"));
        QCOMPARE(theme.pieceColors().at(0), QString("#00ffff"));
        QCOMPARE(theme.pieceColors().at(4), QString("#ff0000"));
    }

    void themeFollowsHyprlandRounding()
    {
        QTemporaryDir dir;
        const QString response = dir.filePath("response.json");
        const QString command = dir.filePath("hyprctl");
        auto write = [](const QString &path, const QByteArray &data) {
            QFile file(path);
            return file.open(QIODevice::WriteOnly) && file.write(data) == data.size();
        };
        QVERIFY(write(command, "#!/bin/sh\ncat '" + response.toUtf8() + "'\n"));
        QVERIFY(QFile::setPermissions(command, QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        QVERIFY(write(response, "{\"int\":0}"));

        Theme theme(dir.path(), nullptr, command);
        QCOMPARE(theme.radius(), 0);
        QVERIFY(write(response, "{\"int\":6}"));
        QTRY_COMPARE(theme.radius(), 6);
    }

    void translatorFallsBackToEnglish()
    {
        QTemporaryDir dir;
        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.language(), QString("en"));
        QVERIFY(translator.translate("Main", "Save").isNull());
    }

    void translatorFollowsTheLanguageFile()
    {
        QTemporaryDir dir;
        QVERIFY(write(dir.path() + "/language", "pt-BR\n"));
        QVERIFY(write(dir.path() + "/pt.json", R"({"Save": "Guardar", "Open": "Abrir"})"));
        QVERIFY(write(dir.path() + "/pt-BR.json", R"({"Save": "Salvar"})"));

        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.language(), QString("pt-BR"));
        QCOMPARE(translator.translate("Main", "Save"), QString("Salvar"));
        QCOMPARE(translator.translate("Main", "Open"), QString("Abrir"));
        QVERIFY(translator.translate("Main", "Quit").isNull());
    }

    void translatorPicksPluralForms()
    {
        QTemporaryDir dir;
        QVERIFY(write(dir.path() + "/language", "ru\n"));
        QVERIFY(write(dir.path() + "/en.json",
                      R"({"%n clips": {"one": "%n clip", "other": "%n clips"}})"));
        QVERIFY(write(dir.path() + "/ru.json",
                      R"({"%n clips": {"one": "%n клип", "few": "%n клипа", "many": "%n клипов", "other": "%n клипа"}})"));

        Translator translator(dir.path() + "/language", dir.path());
        QCOMPARE(translator.translate("Main", "%n clips", nullptr, 21), QString("%n клип"));
        QCOMPARE(translator.translate("Main", "%n clips", nullptr, 22), QString("%n клипа"));
        QCOMPARE(translator.translate("Main", "%n clips", nullptr, 25), QString("%n клипов"));
        QCOMPARE(Translator::pluralCategory("ar", 11), QString("many"));
        QCOMPARE(Translator::pluralCategory("ja", 1), QString("other"));
    }

private:
    static bool write(const QString &path, const QByteArray &contents)
    {
        QFile file(path);
        return file.open(QIODevice::WriteOnly) && file.write(contents) == contents.size();
    }
};

QTEST_MAIN(OmtrisTests)
#include "omtris_tests.moc"