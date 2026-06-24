#pragma once
#include <QDialog>
#include "VMInstance.h"

class DockerManager;
class QLineEdit;
class QSpinBox;
class QLabel;

class AddCustomVMDialog : public QDialog {
    Q_OBJECT
public:
    explicit AddCustomVMDialog(QWidget *parent = nullptr);

    VMInstance result() const;

private slots:
    void onTestConnection();
    void onBrowseKey();

private:
    QLineEdit *m_name;
    QLineEdit *m_host;
    QSpinBox  *m_port;
    QLineEdit *m_user;
    QLineEdit *m_keyPath;
    QLineEdit *m_sudoPass;
    QLabel    *m_statusLabel;
};
