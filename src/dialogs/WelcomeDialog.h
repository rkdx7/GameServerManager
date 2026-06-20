#pragma once
#include <QDialog>

class QLabel;

// Tutoriel de bienvenue affiché au tout premier lancement : explique qu'il faut
// installer Docker et déroule un mini tuto, dans un style volontairement kitsch.
// Affiche-le via WelcomeDialog::showIfFirstLaunch(parent).
class WelcomeDialog : public QDialog {
    Q_OBJECT
public:
    explicit WelcomeDialog(QWidget *parent = nullptr);

    // Affiche le tuto une seule fois (drapeau persistant via QSettings).
    static void showIfFirstLaunch(QWidget *parent);

private slots:
    void checkDocker();

private:
    QLabel *m_status = nullptr;
};
