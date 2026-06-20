#pragma once
#include <QDialog>

class DockerManager;
class QButtonGroup;
class QStackedWidget;
class QLineEdit;
class QComboBox;
class QLabel;
class QPushButton;
class QNetworkAccessManager;
class QNetworkReply;
class QTimer;

class ImagePickerDialog : public QDialog {
    Q_OBJECT
public:
    explicit ImagePickerDialog(DockerManager *docker,
                                const QString &currentImage,
                                QWidget *parent = nullptr);

    QString selectedImage()   const;
    bool    requiresLogin()   const;
    QString loginRegistry()   const;
    QString loginUsername()   const;
    QString loginPassword()   const;

private slots:
    void onSourceChanged(int id);
    void loadLocalImages();
    void updatePreview();
    void scheduleHubTagReload();
    void reloadHubTags();

private:
    QWidget *buildHubPage();
    QWidget *buildLocalPage();
    QWidget *buildPrivatePage();

    // repository part typed on the Hub page, normalised for the Docker Hub API
    QString  hubRepository() const;

    DockerManager  *m_docker;
    QString         m_currentImage;

    QButtonGroup   *m_sourceGroup  = nullptr;
    QStackedWidget *m_srcStack     = nullptr;

    // Hub page
    QLineEdit      *m_hubImage     = nullptr;   // repository (without tag)
    QComboBox      *m_hubTag       = nullptr;   // tag, auto-filled from the registry
    QLabel         *m_hubTagStatus = nullptr;

    QNetworkAccessManager *m_net        = nullptr;
    QNetworkReply         *m_tagReply   = nullptr;
    QTimer                *m_tagDebounce = nullptr;

    // Local page
    QComboBox      *m_localCombo   = nullptr;
    QLabel         *m_localStatus  = nullptr;
    bool            m_localLoaded  = false;

    // Private registry page
    QLineEdit      *m_privRegistry = nullptr;
    QLineEdit      *m_privImage    = nullptr;
    QLineEdit      *m_privUser     = nullptr;
    QLineEdit      *m_privPass     = nullptr;

    QLabel         *m_previewLabel = nullptr;
    QPushButton    *m_applyBtn     = nullptr;
};
