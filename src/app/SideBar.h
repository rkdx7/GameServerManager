#pragma once
#include <QWidget>
#include <QVector>

class QPushButton;
class QVBoxLayout;
class QLabel;
class QLineEdit;
class QScrollArea;
class QFrame;

class SideBar : public QWidget {
    Q_OBJECT
public:
    explicit SideBar(QWidget *parent = nullptr);
    void selectButton(int index);

    // Collapse/expand the menu toward the left. The state is persisted in
    // QSettings ("ui/sidebarCollapsed") so it is remembered across the app.
    void setCollapsed(bool collapsed, bool persist = true);
    bool isCollapsed() const { return m_collapsed; }

signals:
    void pageSelected(int index);
    void collapsedChanged(bool collapsed);

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

    QLabel       *m_logo       = nullptr;
    QFrame       *m_logoSep    = nullptr;
    QLineEdit    *m_search     = nullptr;
    QScrollArea  *m_scrollArea = nullptr;
    QPushButton  *m_toggleBtn  = nullptr;
    bool          m_collapsed  = false;
};
