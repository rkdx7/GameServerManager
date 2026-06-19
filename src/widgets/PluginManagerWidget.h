#pragma once
#include <QWidget>
#include <QString>

class DockerManager;
class QListWidget;
class QListWidgetItem;
class QPlainTextEdit;
class QLabel;
class QPushButton;
class QComboBox;

class PluginManagerWidget : public QWidget {
    Q_OBJECT
public:
    explicit PluginManagerWidget(DockerManager *docker,
                                  const QString  &containerName,
                                  QWidget        *parent = nullptr);
    void refresh();

private slots:
    void onPluginSelected(QListWidgetItem *current, QListWidgetItem *prev);
    void onConfigFileSelected(int index);
    void onUpload();
    void onDeletePlugin();
    void onSaveConfig();

private:
    static QString guessConfigDir(const QString &jarName);
    void           populateConfigs(const QString &pluginDir);
    void           loadConfigFile(const QString &fullPath);
    void           setStatus(const QString &msg, bool error = false);

    DockerManager  *m_docker;
    QString         m_containerName;
    QString         m_currentConfigPath;

    QListWidget    *m_jarList;
    QPushButton    *m_deleteBtn;

    QLabel         *m_configTitle;
    QComboBox      *m_configCombo;
    QPlainTextEdit *m_configEditor;
    QPushButton    *m_saveBtn;
    QLabel         *m_statusLabel;
    QWidget        *m_rightPanel;
};
