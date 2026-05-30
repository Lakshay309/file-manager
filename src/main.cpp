// #include <QApplication>
// #include <qt6/QtGui/qicon.h>

// #include "ui/MainWindow.hpp"

// int main(int argc, char *argv[]) {

//     QApplication app(argc, argv);
//     app.setApplicationName("File Manager");
//     app.setApplicationVersion("0.1");
//     app.setOrganizationName("Lakshay309");

//     QIcon icon(QCoreApplication::applicationDirPath() + "/../resources/icons/test2.jpg");
    
//     app.setWindowIcon(icon);

//     MainWindow window;
//     window.setWindowIcon(icon);
//     window.show();
    
//     return app.exec();
// }

#include <QApplication>
#include <QMainWindow>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QMainWindow window;
    window.resize(1200, 800);
    window.show();

    return app.exec();
}