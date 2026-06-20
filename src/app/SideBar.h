#pragma once
#include <QWidget>
#include <QVector>

class QPushButton;
class QVBoxLayout;
class QLabel;

class SideBar : public QWidget {
    Q_OBJECT
public:
    explicit SideBar(QWidget *parent = nullptr);
    void selectButton(int index);

signals:
    void pageSelected(int index);

private:
    void addCategory(QVBoxLayout *layout, const QString &name);
    void addGame(QVBoxLayout *layout, const QString &label);
    void filterGames(const QString &text);

    struct Category {
        QLabel *label = nullptr;
        QVector<QPushButton*> games;
    };

    QVector<QPushButton*> m_navButtons;
    QVector<Category> m_categories;
    int m_currentIndex = 0;
};
