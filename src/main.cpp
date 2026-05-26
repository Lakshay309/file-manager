#include <QApplication>
#include <qt6/QtGui/qicon.h>

#include "window/MainWindow.hpp"

int main(int argc, char *argv[]) {

    QApplication app(argc, argv);
    app.setApplicationName("File Manager");
    app.setApplicationVersion("0.1");
    app.setOrganizationName("Lakshay309");
    QIcon icon(QCoreApplication::applicationDirPath() + "/../resources/icons/icon.png");
    app.setWindowIcon(icon);
    
    MainWindow window;
    window.setWindowIcon(icon);
    window.show();
    
    return app.exec();
}