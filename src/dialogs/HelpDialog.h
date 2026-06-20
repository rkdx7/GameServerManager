#pragma once
#include <QDialog>

// Centre d'aide : documentation produit dans un style kitsch et fun.
// Ouvert depuis l'icône « 📖 » du menu fixe en bas de la barre latérale.
class HelpDialog : public QDialog {
    Q_OBJECT
public:
    explicit HelpDialog(QWidget *parent = nullptr);

    // URL publique du dépôt GitHub du produit (réutilisée par la barre latérale).
    static const char *kRepoUrl;
};
