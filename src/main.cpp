#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("GameServerManager");
    app.setOrganizationName("GameServerManager");

    auto *main = new MainWindow;
    main->show();

    return app.exec();
}
