#pragma once
#include <QWidget>
#include <QString>

class DockerManager;
class QPlainTextEdit;
class QComboBox;
class QCheckBox;
class QTimer;

// Read-only viewer for a container's logs (`docker logs`). Shared by every game
// dashboard so log viewing is available for all server types by default.
class GameLogsWidget : public QWidget {
    Q_OBJECT
public:
    explicit GameLogsWidget(DockerManager *docker,
                            const QString &containerName,
                            QWidget       *parent = nullptr);

public slots:
    void refresh();

private:
    void setNote(const QString &text);

    DockerManager *m_docker;
    QString        m_container;
    bool           m_loading = false;

    QPlainTextEdit *m_output;
    QComboBox      *m_tailCombo;
    QCheckBox      *m_autoRefresh;
    QTimer         *m_timer;
};
