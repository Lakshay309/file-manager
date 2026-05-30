#pragma once

#include <QMainWindow>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>

#include "../core/Filesystem.hpp"
#include "../core/AppManager.hpp"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    FileSystem fileSystem_;
    AppManager appManager_;

    QLineEdit* pathEdit_;
    QListWidget* fileView_;

    QPushButton* homeButton_;
    QPushButton* upButton_;
    QPushButton* refreshButton_;
    QPushButton* propertiesButton_;

    void refreshFileView();
};