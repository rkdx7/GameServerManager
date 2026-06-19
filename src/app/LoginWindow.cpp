#include "LoginWindow.h"

#include <QFrame>
#include <QLabel>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QGraphicsDropShadowEffect>
#include <QPainter>
#include <QLinearGradient>

LoginWindow::LoginWindow(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Connexion");
    setFixedSize(440, 540);
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog);
    setAttribute(Qt::WA_TranslucentBackground);

    // --- Carte blanche ---
    auto *card = new QFrame(this);
    card->setFixedSize(380, 468);
    card->setAttribute(Qt::WA_StyledBackground, true);
    card->setStyleSheet(R"(
        QFrame {
            background-color: #ffffff;
            border-radius: 20px;
        }
    )");

    auto *shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(24);
    shadow->setColor(QColor(0, 0, 0, 110));
    shadow->setOffset(0, 8);
    card->setGraphicsEffect(shadow);

    // --- Bouton fermer ---
    auto *btnClose = new QPushButton("✕", card);
    btnClose->setFixedSize(32, 32);
    btnClose->move(card->width() - 44, 12);
    btnClose->setCursor(Qt::PointingHandCursor);
    btnClose->setStyleSheet(R"(
        QPushButton {
            background: transparent;
            color: #9ca3af;
            border: none;
            font-size: 14px;
            font-weight: bold;
            border-radius: 16px;
        }
        QPushButton:hover {
            color: #ef4444;
            background: #fee2e2;
        }
    )");
    connect(btnClose, &QPushButton::clicked, this, &QDialog::reject);

    // --- Icône ---
    auto *icon = new QLabel("\U0001F512", card);
    icon->setAlignment(Qt::AlignCenter);
    icon->setStyleSheet("font-size: 48px; background: transparent;");

    // --- Titre ---
    auto *title = new QLabel("Bienvenue !", card);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet(
        "font-size: 26px; font-weight: 700; color: #1e1b4b; background: transparent;");

    // --- Sous-titre ---
    auto *subtitle = new QLabel("Connectez-vous à votre compte", card);
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setStyleSheet(
        "font-size: 13px; color: #6b7280; background: transparent;");

    const QString labelStyle =
        "font-size: 13px; font-weight: 600; color: #374151; background: transparent;";
    const QString inputStyle = R"(
        QLineEdit {
            border: 2px solid #e5e7eb;
            border-radius: 10px;
            padding: 0px 14px;
            font-size: 14px;
            color: #111827;
            background: #f9fafb;
        }
        QLineEdit:focus {
            border-color: #6366f1;
            background: #fafaff;
        }
    )";

    auto *lbUser = new QLabel("Identifiant", card);
    lbUser->setStyleSheet(labelStyle);
    m_user = new QLineEdit(card);
    m_user->setPlaceholderText("Entrez votre identifiant");
    m_user->setFixedHeight(48);
    m_user->setStyleSheet(inputStyle);

    auto *lbPass = new QLabel("Mot de passe", card);
    lbPass->setStyleSheet(labelStyle);
    m_password = new QLineEdit(card);
    m_password->setEchoMode(QLineEdit::Password);
    m_password->setPlaceholderText("Entrez votre mot de passe");
    m_password->setFixedHeight(48);
    m_password->setStyleSheet(inputStyle);

    m_btnLogin = new QPushButton("Se connecter", card);
    m_btnLogin->setFixedHeight(50);
    m_btnLogin->setCursor(Qt::PointingHandCursor);
    m_btnLogin->setStyleSheet(R"(
        QPushButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #6366f1, stop:1 #8b5cf6);
            color: white;
            border: none;
            border-radius: 10px;
            font-size: 15px;
            font-weight: 700;
        }
        QPushButton:hover {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #4f46e5, stop:1 #7c3aed);
        }
        QPushButton:pressed {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #4338ca, stop:1 #6d28d9);
        }
    )");

    auto *line = new QFrame(card);
    line->setFrameShape(QFrame::HLine);
    line->setStyleSheet("color: #e5e7eb; background: transparent;");

    auto *hint = new QLabel("admin / 1234", card);
    hint->setAlignment(Qt::AlignCenter);
    hint->setStyleSheet("font-size: 11px; color: #d1d5db; background: transparent;");

    // --- Layout de la carte ---
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(40, 32, 40, 28);
    cardLayout->setSpacing(0);
    cardLayout->addWidget(icon);
    cardLayout->addSpacing(6);
    cardLayout->addWidget(title);
    cardLayout->addSpacing(4);
    cardLayout->addWidget(subtitle);
    cardLayout->addSpacing(24);
    cardLayout->addWidget(lbUser);
    cardLayout->addSpacing(6);
    cardLayout->addWidget(m_user);
    cardLayout->addSpacing(14);
    cardLayout->addWidget(lbPass);
    cardLayout->addSpacing(6);
    cardLayout->addWidget(m_password);
    cardLayout->addSpacing(24);
    cardLayout->addWidget(m_btnLogin);
    cardLayout->addSpacing(16);
    cardLayout->addWidget(line);
    cardLayout->addSpacing(8);
    cardLayout->addWidget(hint);

    // --- Layout principal ---
    auto *root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->addWidget(card, 0, Qt::AlignCenter);

    connect(m_btnLogin, &QPushButton::clicked, this, &LoginWindow::onLogin);
    connect(m_password, &QLineEdit::returnPressed, this, &LoginWindow::onLogin);
}

void LoginWindow::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QLinearGradient gradient(0, 0, width(), height());
    gradient.setColorAt(0.0, QColor("#1e1b4b"));
    gradient.setColorAt(0.5, QColor("#312e81"));
    gradient.setColorAt(1.0, QColor("#1e1b4b"));

    painter.setBrush(gradient);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(rect(), 24, 24);
}

void LoginWindow::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton) {
        m_dragging = true;
        m_dragPos = e->globalPosition().toPoint() - frameGeometry().topLeft();
    }
    QDialog::mousePressEvent(e);
}

void LoginWindow::mouseMoveEvent(QMouseEvent *e)
{
    if (m_dragging && (e->buttons() & Qt::LeftButton))
        move(e->globalPosition().toPoint() - m_dragPos);
    QDialog::mouseMoveEvent(e);
}

void LoginWindow::mouseReleaseEvent(QMouseEvent *e)
{
    if (e->button() == Qt::LeftButton)
        m_dragging = false;
    QDialog::mouseReleaseEvent(e);
}

void LoginWindow::onLogin()
{
    if (m_user->text() == "admin" && m_password->text() == "1234") {
        emit loginSucceeded();
    } else {
        QMessageBox::warning(this, "Erreur", "Identifiants incorrects.");
        m_password->clear();
        m_password->setFocus();
    }
}
