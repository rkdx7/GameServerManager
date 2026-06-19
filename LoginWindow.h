#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QPoint>
#include <QMouseEvent>

class LoginWindow : public QDialog
{
    Q_OBJECT

public:
    explicit LoginWindow(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

signals:
    void loginSucceeded();

private slots:
    void onLogin();

private:
    QLineEdit   *m_user;
    QLineEdit   *m_password;
    QPushButton *m_btnLogin;
    QPoint       m_dragPos;
    bool         m_dragging = false;
};
