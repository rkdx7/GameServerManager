#pragma once
#include <QWidget>
#include <QVector>
#include "VMInstance.h"

class QVBoxLayout;
class QScrollArea;
class QLabel;

class InfraPage : public QWidget {
    Q_OBJECT
public:
    explicit InfraPage(QVector<VMInstance> *vmList, QWidget *parent = nullptr);

    void refreshVMList();

signals:
    void vmListChanged();

private slots:
    void onAddCustomVM();
    void onCreateScalewayVM();
    void onAdminVM(int vmIndex);
    void onDeleteVM(int vmIndex);

private:
    QWidget *buildVMCard(const VMInstance &vm, int index);

    QVector<VMInstance> *m_vmList;
    QVBoxLayout         *m_cardsLayout = nullptr;
    QWidget             *m_cardsArea   = nullptr;
};
