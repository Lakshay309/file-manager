#include "MainWindow.hpp"
#include "../config/AppConfig.hpp"
#include <QWidget>
#include <QSplitter>
#include <qwidget.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    
    // size of the window
    setMinimumSize(config::MinWindowSize);
    resize(config::DefaultWindowSize);


    //  dividing the screen into 2 side-bar and content
    auto *splitter = new QSplitter(this);
    setCentralWidget(splitter);

    // side-bar
    auto *sidebar = new QWidget();
    sidebar->setStyleSheet("background:#303030;");
    sidebar->setMinimumWidth(200);
    splitter->addWidget(sidebar);


    // main content part
    auto *content = new QWidget();
    content->setStyleSheet("background:#505050;");
    splitter->addWidget(content);

    splitter->setSizes({
        static_cast<int>(config::DefaultWindowSize.width() * 0.2),
        static_cast<int>(config::DefaultWindowSize.width() * 0.8)
    });

    splitter->setCollapsible(0, false);
}
