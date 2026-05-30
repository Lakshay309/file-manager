#include "MainWindow.hpp"
#include "../config/AppConfig.hpp"

#include <QFrame>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QListWidgetItem>
#include <QIcon>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setMinimumSize(config::MinWindowSize);
    resize(config::DefaultWindowSize);

    setWindowTitle("File Manager");
    setWindowIcon(QIcon("../../resources/icons/icons8.png"));

    auto *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    auto *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(12);

    auto *splitter = new QSplitter(Qt::Vertical);


    // TOP PANEL

    auto *topPanel = new QFrame;
    topPanel->setObjectName("topPanel");

    auto *topLayout = new QHBoxLayout(topPanel);
    topLayout->setContentsMargins(16, 16, 16, 16);
    topLayout->setSpacing(10);

    homeButton_ = new QPushButton("Home");
    upButton_ = new QPushButton("Up");
    refreshButton_ = new QPushButton("Refresh");

    pathEdit_ = new QLineEdit;
    pathEdit_->setPlaceholderText("Enter Path");

    propertiesButton_ = new QPushButton("Properties");

    auto *themeButton = new QPushButton("🌙");
    themeButton->setFixedWidth(60);

    topLayout->addWidget(homeButton_);
    topLayout->addWidget(upButton_);
    topLayout->addWidget(refreshButton_);
    topLayout->addWidget(pathEdit_, 1);
    topLayout->addWidget(propertiesButton_);
    topLayout->addWidget(themeButton);

    // BOTTOM PANEL

    auto *bottomPanel = new QFrame;
    bottomPanel->setObjectName("bottomPanel");

    auto *bottomLayout = new QVBoxLayout(bottomPanel);
    bottomLayout->setContentsMargins(12, 12, 12, 12);

    fileView_ = new QListWidget;
    fileView_->setSelectionMode(QAbstractItemView::SingleSelection);

    bottomLayout->addWidget(fileView_);

    
    // SPLITTER

    splitter->addWidget(topPanel);
    splitter->addWidget(bottomPanel);

    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 4);

    mainLayout->addWidget(splitter);

    
    // THEMES

    QString darkTheme = R"(
        QMainWindow {
            background-color: #1E1E1E;
        }

        #topPanel {
            background-color: #2A2A2A;
            border-radius: 10px;
        }

        #bottomPanel {
            background-color: #2A2A2A;
            border-radius: 10px;
        }

        QPushButton {
            background-color: #3A3A3A;
            color: white;
            border: 1px solid #505050;
            border-radius: 6px;
            padding: 8px;
        }

        QPushButton:hover {
            background-color: #4A4A4A;
        }

        QLineEdit {
            background-color: #333333;
            color: white;
            border: 1px solid #555555;
            border-radius: 6px;
            padding: 8px;
        }

        QListWidget {
            background-color: transparent;
            color: white;
            border: none;
        }
    )";

    QString lightTheme = R"(
        QMainWindow {
            background-color: #F0F0F0;
        }

        #topPanel {
            background-color: white;
            border-radius: 10px;
        }

        #bottomPanel {
            background-color: white;
            border-radius: 10px;
        }

        QPushButton {
            background-color: #E5E5E5;
            color: black;
            border: 1px solid #CFCFCF;
            border-radius: 6px;
            padding: 8px;
        }

        QPushButton:hover {
            background-color: #DCDCDC;
        }

        QLineEdit {
            background-color: white;
            color: black;
            border: 1px solid #CFCFCF;
            border-radius: 6px;
            padding: 8px;
        }

        QListWidget {
            background-color: transparent;
            color: black;
            border: none;
        }
    )";

    setStyleSheet(darkTheme);

    auto *isDarkMode = new bool(true);

    connect(themeButton,
            &QPushButton::clicked,
            this,
            [this, isDarkMode, darkTheme, lightTheme, themeButton]()
    {
        *isDarkMode = !(*isDarkMode);

        if (*isDarkMode)
        {
            setStyleSheet(darkTheme);
            themeButton->setText("🌙");
        }
        else
        {
            setStyleSheet(lightTheme);
            themeButton->setText("☀");
        }
    });

    
    // BUTTON CONNECTIONS

    connect(homeButton_,
            &QPushButton::clicked,
            this,
            [this]()
    {
        fileSystem_.toHome();
        refreshFileView();
    });

    connect(upButton_,
            &QPushButton::clicked,
            this,
            [this]()
    {
        fileSystem_.toParent();
        refreshFileView();
    });

    connect(refreshButton_,
            &QPushButton::clicked,
            this,
            [this]()
    {
        refreshFileView();
    });

    connect(pathEdit_,
            &QLineEdit::returnPressed,
            this,
            [this]()
    {
        fileSystem_.updatePath(
            pathEdit_->text().toStdString()
        );

        refreshFileView();
    });

    connect(fileView_,
            &QListWidget::itemDoubleClicked,
            this,
            [this](QListWidgetItem *item)
    {
        std::filesystem::path path =
            item->data(Qt::UserRole)
                .toString()
                .toStdString();

        auto files =
            fileSystem_.getAllFileInCurrentDir();

        for (const auto &file : files)
        {
            if (file.path == path)
            {
                if (file.isDir)
                {
                    fileSystem_.updatePath(path);
                    refreshFileView();
                }

                break;
            }
        }
    });

    refreshFileView();
}

void MainWindow::refreshFileView()
{
    fileView_->clear();

    pathEdit_->setText(
        QString::fromStdString(
            fileSystem_.getCurrentFilePath().string()
        )
    );

    auto files = fileSystem_.getAllFileInCurrentDir();

    for (const auto &file : files)
    {
        QString displayName;

        if (file.isDir)
            displayName = "📁 " + QString::fromStdString(file.name);
        else
            displayName = "📄 " + QString::fromStdString(file.name);

        auto *item = new QListWidgetItem(displayName);

        item->setData(
            Qt::UserRole,
            QString::fromStdString(file.path.string())
        );

        fileView_->addItem(item);
    }
}