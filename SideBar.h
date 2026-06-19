#pragma once
#include <QWidget>
#include <QVector>

class QPushButton;
class QVBoxLayout;

class SideBar : public QWidget {
    Q_OBJECT
public:
    explicit SideBar(QWidget *parent = nullptr);
    void selectButton(int index);

signals:
    void pageSelected(int index);
    void logoutRequested();

private:
    void addCategory(QVBoxLayout *layout, const QString &name);
    void addGame(QVBoxLayout *layout, const QString &label);

    QVector<QPushButton*> m_navButtons;
    int m_currentIndex = 0;
};
