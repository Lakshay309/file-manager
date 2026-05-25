#include <QApplication>

#include "window/MainWindow.hpp"

int main(int argc, char *argv[]) {

    QApplication app(argc, argv);
    app.setApplicationName("File Manager");
    app.setApplicationVersion("0.1");
    app.setOrganizationName("Lakshay309");
    
    MainWindow window;
    
    window.show();
    
    return app.exec();
}