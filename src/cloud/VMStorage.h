#pragma once
#include <QVector>
#include "VMInstance.h"

class VMStorage {
public:
    static void               save(const QVector<VMInstance> &vms);
    static QVector<VMInstance> load();
};
