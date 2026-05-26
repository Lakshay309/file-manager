#include "MainWindow.hpp"
#include "../config/AppConfig.hpp"
#include <QWidget>
#include <QSplitter>
#include <qicon.h>
#include <qlabel.h>
#include <qwidget.h>
#include<QVBoxLayout>

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
    content->setMinimumWidth(600);
    auto *contentLayout = new QVBoxLayout(content);

    // pathBar
    auto *pathBar = new QLabel("home");
    pathBar->setFixedHeight(40);
    pathBar->setStyleSheet("background:#503030; padding:5px;");
    contentLayout->addWidget(pathBar);


    // fileView
    auto *fileView = new QWidget();
    fileView->setStyleSheet("background:#303030;");
    contentLayout->addWidget(fileView);


    content->setStyleSheet("background:#505050;");
    splitter->addWidget(content);


    splitter->setSizes({
        static_cast<int>(config::DefaultWindowSize.width() * 0.2),
        static_cast<int>(config::DefaultWindowSize.width() * 0.8)
    });

    splitter->setCollapsible(0, false);
    splitter->setChildrenCollapsible(false);


    setWindowIcon(QIcon("../../resources/icons/icon.png"));
}
