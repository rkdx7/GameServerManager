#pragma once
#include <QDialog>
#include "VMInstance.h"

class ScalewayProvider;
class QLineEdit;
class QComboBox;
class QLabel;
class QPushButton;
class QTabWidget;
class QProgressBar;

class CreateVMDialog : public QDialog {
    Q_OBJECT
public:
    explicit CreateVMDialog(QWidget *parent = nullptr);

    VMInstance createdVM() const { return m_result; }

private slots:
    void onCreate();
    void onBrowseKey();

private:
    QWidget *buildPrerequisitesTab();
    QWidget *buildConfigTab();

    QLineEdit  *m_apiToken;
    QLineEdit  *m_projectId;
    QComboBox  *m_region;
    QComboBox  *m_instanceType;
    QLineEdit  *m_sshKeyPath;
    QLineEdit  *m_vmName;
    QLabel     *m_statusLabel;
    QPushButton *m_createBtn;

    ScalewayProvider *m_provider = nullptr;
    VMInstance        m_result;
};
