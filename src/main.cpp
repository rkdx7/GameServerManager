#include <QApplication>
#include "LoginWindow.h"
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("GameServerManager");
    app.setOrganizationName("GameServerManager");

    auto *login   = new LoginWindow;
    MainWindow *main = nullptr;

    QObject::connect(login, &LoginWindow::loginSucceeded, [&]() {
        login->hide();
        main = new MainWindow;
        QObject::connect(main, &MainWindow::loggedOut, [&]() {
            main->close();
            main->deleteLater();
            main = nullptr;
            login->show();
        });
        main->show();
    });

    login->show();
    return app.exec();
}
