#pragma once
#include <QWidget>
#include <QString>

class DockerManager;
class QTextEdit;
class QLineEdit;
enum class GameType;

class GameConsoleWidget : public QWidget {
    Q_OBJECT
public:
    explicit GameConsoleWidget(DockerManager *docker,
                               const QString &containerName,
                               GameType       gameType,
                               const QString &rconPass = {},
                               QWidget       *parent   = nullptr);

private slots:
    void onSendCommand();

private:
    void appendOutput(const QString &line, const QString &colorHex = "#a0f0a0");
    void handleCommand(const QString &input);

    DockerManager *m_docker;
    QString        m_container;
    GameType       m_gameType;
    QString        m_rconPass;

    QTextEdit *m_output;
    QLineEdit *m_input;
};
