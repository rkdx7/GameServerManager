#pragma once
#include <QWidget>
#include <QPoint>
#include <QVector>
#include "VMInstance.h"

class QStackedWidget;
class DockerManager;
class InfraPage;
class DeploymentTargetSelector;
class QPaintEvent;
class QMouseEvent;

class MainWindow : public QWidget {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

signals:
    void loggedOut();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void onVMListChanged();

private:
    QStackedWidget  *m_stack      = nullptr;
    DockerManager   *m_docker     = nullptr;
    InfraPage       *m_infraPage  = nullptr;
    QPoint           m_dragPos;
    bool             m_dragging   = false;

    QVector<VMInstance>             m_vmList;
    QVector<DeploymentTargetSelector *> m_targetSelectors;
};
