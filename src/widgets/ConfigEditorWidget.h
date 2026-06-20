#pragma once
#include <QWidget>
#include <QString>

class DockerManager;
class QPlainTextEdit;
class QLabel;
class QPushButton;

// Editor for a single configuration file inside a running container. Lets the
// user view, edit and save a server's main config file at any time *after*
// installation. Shared by every game dashboard that has a known config path.
class ConfigEditorWidget : public QWidget {
    Q_OBJECT
public:
    explicit ConfigEditorWidget(DockerManager *docker,
                                const QString &containerName,
                                const QString &filePath,
                                QWidget       *parent = nullptr);

public slots:
    void load();          // (re)load file content from the container
    void ensureLoaded();  // load once, on first tab activation

private slots:
    void save();
    void saveAndRestart();

private:
    void doSave(bool restart);
    void setStatus(const QString &msg, bool error = false);

    DockerManager *m_docker;
    QString        m_container;
    QString        m_filePath;
    bool           m_busy       = false;
    bool           m_loadedOnce = false;

    QPlainTextEdit *m_editor;
    QPushButton    *m_saveBtn;
    QPushButton    *m_saveRestartBtn;
    QPushButton    *m_reloadBtn;
    QLabel         *m_status;
};
