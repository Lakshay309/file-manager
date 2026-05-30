#include "MainWindow.hpp"
#include "../config/AppConfig.hpp"

#include <QApplication>
#include <QFrame>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSplitter>
#include <QListWidgetItem>
#include <QTreeWidgetItem>
#include <QIcon>
#include <QInputDialog>
#include <QMessageBox>
#include <QMenu>
#include <QAction>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QStackedWidget>
#include <QHeaderView>
#include <QDesktopServices>
#include <QUrl>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QDateTime>
#include <filesystem>
#include <QScrollArea>
#include <QWidgetAction>

// ─────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setMinimumSize(config::MinWindowSize);
    resize(config::DefaultWindowSize);
    setWindowTitle("File Manager");
    setWindowIcon(QIcon("../../resources/icons/icons8.png"));

    auto *central = new QWidget(this);
    setCentralWidget(central);

    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(8);

    buildTopBar(mainLayout);
    buildToolBar();
    buildFileViews(mainLayout);
    buildStatusBar();
    connectSignals();

    applyTheme(true);   // start in dark mode
    refreshFileView();
}

// ─────────────────────────────────────────────
//  Build: Top Bar
// ─────────────────────────────────────────────

void MainWindow::buildTopBar(QVBoxLayout* mainLayout) {
    auto *topPanel = new QFrame;
    topPanel->setObjectName("topPanel");

    auto *topLayout = new QHBoxLayout(topPanel);
    topLayout->setContentsMargins(12, 8, 12, 8);
    topLayout->setSpacing(8);

    homeButton_       = new QPushButton("⌂");
    upButton_         = new QPushButton("↑");
    refreshButton_    = new QPushButton("↻");
    propertiesButton_ = new QPushButton("ℹ");
    themeButton_      = new QPushButton("🌙");

    for (auto* btn : {homeButton_, upButton_, refreshButton_, propertiesButton_, themeButton_}) {
        btn->setFixedSize(36, 36);
        btn->setObjectName("iconButton");
    }

    pathEdit_ = new QLineEdit;
    pathEdit_->setPlaceholderText("Enter path...");
    pathEdit_->setObjectName("pathEdit");

    topLayout->addWidget(homeButton_);
    topLayout->addWidget(upButton_);
    topLayout->addWidget(refreshButton_);
    topLayout->addWidget(pathEdit_, 1);
    topLayout->addWidget(propertiesButton_);
    topLayout->addWidget(themeButton_);

    mainLayout->addWidget(topPanel);
}

// ─────────────────────────────────────────────
//  Build: Action Toolbar
// ─────────────────────────────────────────────

void MainWindow::buildToolBar() {
    actionToolBar_ = addToolBar("Actions");
    actionToolBar_->setMovable(false);
    actionToolBar_->setObjectName("actionToolBar");
    actionToolBar_->setIconSize(QSize(16, 16));

    newFolderAction_ = actionToolBar_->addAction("📁 New Folder");
    actionToolBar_->addSeparator();
    copyAction_      = actionToolBar_->addAction("⎘ Copy");
    cutAction_       = actionToolBar_->addAction("✂ Cut");
    pasteAction_     = actionToolBar_->addAction("⎙ Paste");
    actionToolBar_->addSeparator();
    renameAction_    = actionToolBar_->addAction("✎ Rename");
    deleteAction_    = actionToolBar_->addAction("🗑 Delete");
    actionToolBar_->addSeparator();
    favAction_       = actionToolBar_->addAction("☆ Favourite");
    actionToolBar_->addSeparator();

    // View toggle on the right side
    auto* spacer = new QWidget;
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    actionToolBar_->addWidget(spacer);

    gridViewAction_ = actionToolBar_->addAction("⊞ Grid");
    listViewAction_ = actionToolBar_->addAction("☰ List");

    gridViewAction_->setCheckable(true);
    listViewAction_->setCheckable(true);
    gridViewAction_->setChecked(true);

    // Disable file actions until something is selected
    copyAction_->setEnabled(false);
    cutAction_->setEnabled(false);
    pasteAction_->setEnabled(false);
    renameAction_->setEnabled(false);
    deleteAction_->setEnabled(false);
    favAction_->setEnabled(false);
}

// ─────────────────────────────────────────────
//  Build: File Views (grid + list stacked)
// ─────────────────────────────────────────────

void MainWindow::buildFileViews(QVBoxLayout* mainLayout) {
    viewStack_ = new QStackedWidget;

    // Grid view — icon tiles
    gridView_ = new QListWidget;
    gridView_->setObjectName("gridView");
    gridView_->setViewMode(QListWidget::IconMode);
    gridView_->setIconSize(QSize(48, 48));
    gridView_->setGridSize(QSize(110, 90));
    gridView_->setResizeMode(QListWidget::Adjust);
    gridView_->setMovement(QListWidget::Static);
    gridView_->setSpacing(6);
    gridView_->setContextMenuPolicy(Qt::CustomContextMenu);

    // List view — rows with columns
    listView_ = new QTreeWidget;
    listView_->setObjectName("listView");
    listView_->setColumnCount(4);
    listView_->setHeaderLabels({"Name", "Type", "Size", "Fav"});
    listView_->header()->setSectionResizeMode(0, QHeaderView::Stretch);
    listView_->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    listView_->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    listView_->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    listView_->setRootIsDecorated(false);
    listView_->setAlternatingRowColors(true);
    listView_->setContextMenuPolicy(Qt::CustomContextMenu);

    viewStack_->addWidget(gridView_);   // index 0 = grid
    viewStack_->addWidget(listView_);   // index 1 = list
    viewStack_->setCurrentIndex(0);

    mainLayout->addWidget(viewStack_, 1);
}

// ─────────────────────────────────────────────
//  Build: Status Bar
// ─────────────────────────────────────────────

void MainWindow::buildStatusBar() {
    statusLabel_ = new QLabel("Ready");
    statusBar()->addWidget(statusLabel_);
    statusBar()->setObjectName("statusBar");
}

// ─────────────────────────────────────────────
//  Connect All Signals
// ─────────────────────────────────────────────

void MainWindow::connectSignals() {

    // ── Top bar buttons ───────────────────────
    connect(homeButton_, &QPushButton::clicked, this, [this]() {
        fileSystem_.toHome();
        refreshFileView();
    });

    connect(upButton_, &QPushButton::clicked, this, [this]() {
        fileSystem_.toParent();
        refreshFileView();
    });

    connect(refreshButton_, &QPushButton::clicked, this, [this]() {
        refreshFileView();
    });

    connect(propertiesButton_, &QPushButton::clicked, this, [this]() {
        onProperties(selectedPath_);
    });

    connect(themeButton_, &QPushButton::clicked, this, [this]() {
        isDarkMode_ = !isDarkMode_;
        applyTheme(isDarkMode_);
    });

    connect(pathEdit_, &QLineEdit::returnPressed, this, [this]() {
        fileSystem_.updatePath(pathEdit_->text().toStdString());
        refreshFileView();
    });

    // ── Toolbar actions ───────────────────────
    connect(newFolderAction_, &QAction::triggered, this, &MainWindow::onNewFolder);
    connect(copyAction_,      &QAction::triggered, this, &MainWindow::onCopy);
    connect(cutAction_,       &QAction::triggered, this, &MainWindow::onCut);
    connect(pasteAction_,     &QAction::triggered, this, &MainWindow::onPaste);
    connect(renameAction_,    &QAction::triggered, this, &MainWindow::onRename);
    connect(deleteAction_,    &QAction::triggered, this, &MainWindow::onDelete);
    connect(favAction_,       &QAction::triggered, this, &MainWindow::onToggleFav);

    connect(gridViewAction_, &QAction::triggered, this, [this]() {
        isGridMode_ = true;
        viewStack_->setCurrentIndex(0);
        gridViewAction_->setChecked(true);
        listViewAction_->setChecked(false);
    });

    connect(listViewAction_, &QAction::triggered, this, [this]() {
        isGridMode_ = false;
        viewStack_->setCurrentIndex(1);
        gridViewAction_->setChecked(false);
        listViewAction_->setChecked(true);
    });

    // ── Grid view interactions ────────────────
    connect(gridView_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem* item) {
        QString path = item->data(Qt::UserRole).toString();
        if (std::filesystem::is_directory(path.toStdString())) {
            fileSystem_.updatePath(path.toStdString());
            refreshFileView();
        }
    });

    connect(gridView_, &QListWidget::itemClicked, this, [this](QListWidgetItem* item) {
        setSelection(item->data(Qt::UserRole).toString());
    });

    connect(gridView_, &QListWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        auto* item = gridView_->itemAt(pos);
        if (item) setSelection(item->data(Qt::UserRole).toString());
        showContextMenu(gridView_->mapToGlobal(pos), item != nullptr);
    });

    // ── List view interactions ────────────────
    connect(listView_, &QTreeWidget::itemDoubleClicked, this, [this](QTreeWidgetItem* item) {
        QString path = item->data(0, Qt::UserRole).toString();
        if (std::filesystem::is_directory(path.toStdString())) {
            fileSystem_.updatePath(path.toStdString());
            refreshFileView();
        }
    });

    connect(listView_, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem* item) {
        setSelection(item->data(0, Qt::UserRole).toString());
    });

    connect(listView_, &QTreeWidget::customContextMenuRequested, this, [this](const QPoint& pos) {
        auto* item = listView_->itemAt(pos);
        if (item) setSelection(item->data(0, Qt::UserRole).toString());
        showContextMenu(listView_->mapToGlobal(pos), item != nullptr);
    });
}

// ─────────────────────────────────────────────
//  Show Context Menu
// ─────────────────────────────────────────────

void MainWindow::showContextMenu(const QPoint& globalPos, bool hasSelection) {
    QMenu menu(this);
    menu.setObjectName("contextMenu");

    if (hasSelection) {
        menu.addAction("⎘  Copy",       this, &MainWindow::onCopy);
        menu.addAction("✂  Cut",        this, &MainWindow::onCut);
        menu.addSeparator();
        menu.addAction("✎  Rename",     this, &MainWindow::onRename);
        menu.addAction("🗑  Delete",    this, &MainWindow::onDelete);
        menu.addSeparator();
        menu.addAction("☆  Toggle Favourite", this, &MainWindow::onToggleFav);
        menu.addSeparator();

        
        menu.addSeparator();
        menu.addAction("ℹ  Properties", this, [this]() { onProperties(selectedPath_); });
    }
    // Open With submenu
    auto* openWithMenu = menu.addMenu("Open with...");
    std::filesystem::path targetForOpen = selectedPath_.isEmpty() ? fileSystem_.getCurrentFilePath() : std::filesystem::path(selectedPath_.toStdString());  

    // TODO: good for now 
    auto appsForFile = appManager_.getAllApps();
    // getAppForFile(
    //     targetForOpen
    // );
    if (appsForFile.empty()) {
        openWithMenu->addAction("No apps found")->setEnabled(false);
    } else {
        // Container widget that goes inside the menu
        auto* container = new QWidget;
        auto* vbox = new QVBoxLayout(container);
        vbox->setContentsMargins(4, 4, 4, 4);
        vbox->setSpacing(2);

        for (const auto& app : appsForFile) {
            QString appName = QString::fromStdString(app.name);
            QString appPath = QString::fromStdString(app.execPath.string());

            auto* btn = new QPushButton(appName);
            btn->setFlat(true);
            btn->setCursor(Qt::PointingHandCursor);
            btn->setStyleSheet(R"(
                QPushButton {
                    text-align: left;
                    padding: 5px 12px;
                    border-radius: 4px;
                    background: transparent;
                }
                QPushButton:hover { background-color: #4a90d9; color: white; }
            )");

            connect(btn, &QPushButton::clicked, this, [this, appPath, &menu]() {
                menu.close();
                onOpenWith(appPath);
            });

            vbox->addWidget(btn);
        }

        // Scroll area wrapping the button list
        auto* scroll = new QScrollArea;
        scroll->setWidget(container);
        scroll->setWidgetResizable(true);
        scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        scroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        scroll->setFrameShape(QFrame::NoFrame);

        // Cap height at 8 items worth, expand if fewer
        int itemH = 31;
        int capH  = 8 * itemH;
        int realH = appsForFile.size() * itemH + 8;
        scroll->setFixedSize(220, std::min(realH, capH));

        auto* widgetAction = new QWidgetAction(openWithMenu);
        widgetAction->setDefaultWidget(scroll);
        openWithMenu->addAction(widgetAction);
            }

            menu.addAction("⎙  Paste",       this, &MainWindow::onPaste);
            menu.addSeparator();
            menu.addAction("📁  New Folder",  this, &MainWindow::onNewFolder);
            menu.addAction("↻  Refresh",     this, &MainWindow::refreshFileView);

            menu.exec(globalPos);
        }

// ─────────────────────────────────────────────
//  Refresh File View
// ─────────────────────────────────────────────

void MainWindow::refreshFileView() {
    gridView_->clear();
    listView_->clear();
    selectedPath_.clear();

    pathEdit_->setText(
        QString::fromStdString(fileSystem_.getCurrentFilePath().string())
    );

    auto files = fileSystem_.getAllFileInCurrentDir();

    for (const auto& file : files) {
        QString name  = QString::fromStdString(file.name);
        QString path  = QString::fromStdString(file.path.string());
        QString type  = QString::fromStdString(file.type);
        QString fav   = file.isFav ? "★" : "";
        QString icon  = file.isDir ? "📁" : "📄";

        // Size formatting
        QString sizeStr;
        if (file.isDir) {
            sizeStr = "—";
        } else if (file.size < 1024) {
            sizeStr = QString::number(file.size) + " B";
        } else if (file.size < 1024 * 1024) {
            sizeStr = QString::number(file.size / 1024) + " KB";
        } else {
            sizeStr = QString::number(file.size / (1024 * 1024)) + " MB";
        }

        // ── Grid item ──
        auto* gridItem = new QListWidgetItem(icon + "\n" + name);
        gridItem->setData(Qt::UserRole, path);
        gridItem->setToolTip(
            QString("Name: %1\nType: %2\nSize: %3%4")
                .arg(name, type, sizeStr, file.isFav ? "\n★ Favourite" : "")
        );
        gridView_->addItem(gridItem);

        // ── List item ──
        auto* listItem = new QTreeWidgetItem({icon + " " + name, type, sizeStr, fav});
        listItem->setData(0, Qt::UserRole, path);
        listItem->setToolTip(0, path);
        listView_->addTopLevelItem(listItem);
    }

    updateStatusBar();

    // Enable/disable paste based on clipboard state
    pasteAction_->setEnabled(true);

    // Disable file-specific actions since nothing selected
    copyAction_->setEnabled(false);
    cutAction_->setEnabled(false);
    renameAction_->setEnabled(false);
    deleteAction_->setEnabled(false);
    favAction_->setEnabled(false);
}

// ─────────────────────────────────────────────
//  Selection
// ─────────────────────────────────────────────

void MainWindow::setSelection(const QString& path) {
    selectedPath_ = path;
    bool hasFile = !path.isEmpty();
    copyAction_->setEnabled(hasFile);
    cutAction_->setEnabled(hasFile);
    renameAction_->setEnabled(hasFile);
    deleteAction_->setEnabled(hasFile);
    favAction_->setEnabled(hasFile);
    updateStatusBar();
}

QString MainWindow::currentSelectedPath() const {
    return selectedPath_;
}

// ─────────────────────────────────────────────
//  File Actions
// ─────────────────────────────────────────────

void MainWindow::onCopy() {
    if (selectedPath_.isEmpty()) return;
    std::filesystem::path p(selectedPath_.toStdString());
    fileSystem_.copy(p.filename().string());
    statusLabel_->setText("Copied: " + QString::fromStdString(p.filename().string()));
}

void MainWindow::onCut() {
    if (selectedPath_.isEmpty()) return;
    std::filesystem::path p(selectedPath_.toStdString());
    fileSystem_.cut(p.filename().string());
    statusLabel_->setText("Cut: " + QString::fromStdString(p.filename().string()));
}

void MainWindow::onPaste() {
    bool ok = fileSystem_.paste();
    if (ok) {
        refreshFileView();
        statusLabel_->setText("Pasted successfully.");
    } else {
        statusLabel_->setText("Paste failed — file already exists or nothing to paste.");
    }
}

void MainWindow::onRename() {
    if (selectedPath_.isEmpty()) return;
    std::filesystem::path p(selectedPath_.toStdString());
    QString oldName = QString::fromStdString(p.filename().string());

    bool ok;
    QString newName = QInputDialog::getText(
        this, "Rename", "New name:", QLineEdit::Normal, oldName, &ok
    );

    if (ok && !newName.isEmpty() && newName != oldName) {
        bool result = fileSystem_.renamePath(oldName.toStdString(), newName.toStdString());
        if (result) {
            refreshFileView();
            statusLabel_->setText("Renamed to: " + newName);
        } else {
            QMessageBox::warning(this, "Rename Failed", "Could not rename the file.");
        }
    }
}

void MainWindow::onDelete() {
    if (selectedPath_.isEmpty()) return;
    std::filesystem::path p(selectedPath_.toStdString());
    QString name = QString::fromStdString(p.filename().string());

    auto reply = QMessageBox::question(
        this, "Delete",
        "Are you sure you want to delete \"" + name + "\"?",
        QMessageBox::Yes | QMessageBox::No
    );

    if (reply == QMessageBox::Yes) {
        bool result = fileSystem_.deletePath(name.toStdString());
        if (result) {
            selectedPath_.clear();
            refreshFileView();
            statusLabel_->setText("Deleted: " + name);
        } else {
            QMessageBox::warning(this, "Delete Failed", "Could not delete the file.");
        }
    }
}

void MainWindow::onToggleFav() {
    if (selectedPath_.isEmpty()) return;
    std::filesystem::path p(selectedPath_.toStdString());
    bool result = fileSystem_.ToggleFav(p);
    refreshFileView();
    statusLabel_->setText(result ? "★ Favourite updated." : "Could not update favourite.");
}

void MainWindow::onNewFolder() {
    bool ok;
    QString name = QInputDialog::getText(
        this, "New Folder", "Folder name:", QLineEdit::Normal, "New Folder", &ok
    );
    if (ok && !name.isEmpty()) {
        bool result = fileSystem_.createDirectory(name.toStdString());
        if (result) {
            refreshFileView();
            statusLabel_->setText("Created folder: " + name);
        } else {
            QMessageBox::warning(this, "Error", "Could not create folder.");
        }
    }
}

void MainWindow::onProperties(const QString& path) {
    QString targetPath = path.isEmpty() ? selectedPath_ : path;
    if (targetPath.isEmpty()) return;

    auto props = fileSystem_.getProperties(
        std::filesystem::path(targetPath.toStdString())
    );

    // Format last modified time
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
        props.lastModified - std::filesystem::file_time_type::clock::now()
        + std::chrono::system_clock::now()
    );
    std::time_t tt = std::chrono::system_clock::to_time_t(sctp);
    QString lastMod = QString(std::ctime(&tt)).trimmed();

    QDialog dialog(this);
    dialog.setWindowTitle("Properties — " + QString::fromStdString(props.name));
    dialog.setMinimumWidth(360);

    auto* layout = new QFormLayout(&dialog);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(10);

    layout->addRow("Name:",          new QLabel(QString::fromStdString(props.name)));
    layout->addRow("Path:",          new QLabel(QString::fromStdString(props.path.string())));
    layout->addRow("Parent:",        new QLabel(QString::fromStdString(props.parentPath.string())));
    layout->addRow("Last Modified:", new QLabel(lastMod));

    // Size for files
    if (!std::filesystem::is_directory(props.path)) {
        auto size = std::filesystem::file_size(props.path);
        QString sizeStr;
        if (size < 1024)             sizeStr = QString::number(size) + " bytes";
        else if (size < 1024*1024)   sizeStr = QString::number(size/1024) + " KB";
        else                         sizeStr = QString::number(size/(1024*1024)) + " MB";
        layout->addRow("Size:", new QLabel(sizeStr));
    }

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    layout->addRow(buttons);

    dialog.exec();
}

void MainWindow::onOpenWith(const QString& appPath) {
    if (selectedPath_.isEmpty() || appPath.isEmpty()) return;
    fileSystem_.openWith(
        std::filesystem::path(appPath.toStdString()),
        std::filesystem::path(selectedPath_.toStdString())
    );
}

// ─────────────────────────────────────────────
//  Status Bar
// ─────────────────────────────────────────────

void MainWindow::updateStatusBar() {
    auto files = fileSystem_.getAllFileInCurrentDir();
    int  total = files.size();
    int  dirs  = 0, favs = 0;
    for (const auto& f : files) {
        if (f.isDir)  dirs++;
        if (f.isFav)  favs++;
    }

    QString sel = selectedPath_.isEmpty()
        ? ""
        : "  |  Selected: " + QString::fromStdString(
            std::filesystem::path(selectedPath_.toStdString()).filename().string());

    statusLabel_->setText(
        QString("%1 items  (%2 folders)  |  %3 favourites%4")
            .arg(total).arg(dirs).arg(favs).arg(sel)
    );
}

// ─────────────────────────────────────────────
//  Theme
// ─────────────────────────────────────────────

void MainWindow::applyTheme(bool dark) {
    themeButton_->setText(dark ? "🌙" : "☀");

    if (dark) {
        setStyleSheet(R"(
            QMainWindow, QWidget {
                background-color: #1a1a1a;
                color: #e0e0e0;
                font-family: 'Segoe UI', sans-serif;
                font-size: 13px;
            }
            #topPanel {
                background-color: #242424;
                border-radius: 8px;
            }
            #iconButton {
                background-color: #2e2e2e;
                color: #e0e0e0;
                border: 1px solid #3a3a3a;
                border-radius: 6px;
                font-size: 16px;
            }
            #iconButton:hover { background-color: #3a3a3a; }
            #iconButton:pressed { background-color: #4a90d9; }
            #pathEdit {
                background-color: #2e2e2e;
                color: #e0e0e0;
                border: 1px solid #3a3a3a;
                border-radius: 6px;
                padding: 6px 10px;
            }
            #pathEdit:focus { border-color: #4a90d9; }
            #actionToolBar {
                background-color: #1e1e1e;
                border-bottom: 1px solid #2e2e2e;
                spacing: 4px;
                padding: 2px 6px;
            }
            QToolBar QToolButton {
                background-color: transparent;
                color: #c0c0c0;
                border: none;
                border-radius: 5px;
                padding: 4px 8px;
                font-size: 12px;
            }
            QToolBar QToolButton:hover { background-color: #2e2e2e; color: white; }
            QToolBar QToolButton:checked { background-color: #4a90d9; color: white; }
            QToolBar QToolButton:disabled { color: #555; }
            QToolBar::separator { background: #333; width: 1px; margin: 4px 4px; }
            #gridView {
                background-color: #1e1e1e;
                border: none;
                color: #e0e0e0;
            }
            #gridView::item {
                background-color: #2a2a2a;
                border-radius: 8px;
                padding: 6px;
                margin: 3px;
                text-align: center;
            }
            #gridView::item:hover { background-color: #333; }
            #gridView::item:selected { background-color: #4a90d9; color: white; }
            #listView {
                background-color: #1e1e1e;
                border: none;
                color: #e0e0e0;
                alternate-background-color: #232323;
            }
            #listView::item:hover { background-color: #2e2e2e; }
            #listView::item:selected { background-color: #4a90d9; color: white; }
            QHeaderView::section {
                background-color: #242424;
                color: #888;
                border: none;
                border-bottom: 1px solid #333;
                padding: 4px 8px;
                font-size: 11px;
                text-transform: uppercase;
                letter-spacing: 0.05em;
            }
            QStatusBar {
                background-color: #1a1a1a;
                color: #666;
                font-size: 11px;
                border-top: 1px solid #2a2a2a;
            }
            QMenu {
                background-color: #2a2a2a;
                color: #e0e0e0;
                border: 1px solid #3a3a3a;
                border-radius: 6px;
                padding: 4px;
            }
            QMenu::item { padding: 6px 20px; border-radius: 4px; }
            QMenu::item:selected { background-color: #4a90d9; }
            QMenu::separator { background: #3a3a3a; height: 1px; margin: 3px 8px; }
            QScrollBar:vertical {
                background: #1e1e1e; width: 8px; border-radius: 4px;
            }
            QScrollBar::handle:vertical {
                background: #3a3a3a; border-radius: 4px; min-height: 20px;
            }
            QScrollBar::handle:vertical:hover { background: #4a4a4a; }
            QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
        )");
    } else {
        setStyleSheet(R"(
            QMainWindow, QWidget {
                background-color: #f5f5f5;
                color: #1a1a1a;
                font-family: 'Segoe UI', sans-serif;
                font-size: 13px;
            }
            #topPanel {
                background-color: white;
                border-radius: 8px;
                border: 1px solid #e0e0e0;
            }
            #iconButton {
                background-color: #f0f0f0;
                color: #1a1a1a;
                border: 1px solid #ddd;
                border-radius: 6px;
                font-size: 16px;
            }
            #iconButton:hover { background-color: #e5e5e5; }
            #iconButton:pressed { background-color: #4a90d9; color: white; }
            #pathEdit {
                background-color: white;
                color: #1a1a1a;
                border: 1px solid #ddd;
                border-radius: 6px;
                padding: 6px 10px;
            }
            #pathEdit:focus { border-color: #4a90d9; }
            #actionToolBar {
                background-color: #ececec;
                border-bottom: 1px solid #ddd;
                spacing: 4px;
                padding: 2px 6px;
            }
            QToolBar QToolButton {
                background-color: transparent;
                color: #444;
                border: none;
                border-radius: 5px;
                padding: 4px 8px;
                font-size: 12px;
            }
            QToolBar QToolButton:hover { background-color: #ddd; color: #111; }
            QToolBar QToolButton:checked { background-color: #4a90d9; color: white; }
            QToolBar QToolButton:disabled { color: #bbb; }
            QToolBar::separator { background: #ccc; width: 1px; margin: 4px 4px; }
            #gridView {
                background-color: #f5f5f5;
                border: none;
                color: #1a1a1a;
            }
            #gridView::item {
                background-color: white;
                border-radius: 8px;
                padding: 6px;
                margin: 3px;
                border: 1px solid #e8e8e8;
            }
            #gridView::item:hover { background-color: #eef4ff; border-color: #c0d8ff; }
            #gridView::item:selected { background-color: #4a90d9; color: white; }
            #listView {
                background-color: white;
                border: none;
                color: #1a1a1a;
                alternate-background-color: #fafafa;
            }
            #listView::item:hover { background-color: #eef4ff; }
            #listView::item:selected { background-color: #4a90d9; color: white; }
            QHeaderView::section {
                background-color: #f0f0f0;
                color: #666;
                border: none;
                border-bottom: 1px solid #ddd;
                padding: 4px 8px;
                font-size: 11px;
            }
            QStatusBar {
                background-color: #ececec;
                color: #888;
                font-size: 11px;
                border-top: 1px solid #ddd;
            }
            QMenu {
                background-color: white;
                color: #1a1a1a;
                border: 1px solid #ddd;
                border-radius: 6px;
                padding: 4px;
            }
            QMenu::item { padding: 6px 20px; border-radius: 4px; }
            QMenu::item:selected { background-color: #4a90d9; color: white; }
            QMenu::separator { background: #e0e0e0; height: 1px; margin: 3px 8px; }
            QScrollBar:vertical {
                background: #f0f0f0; width: 8px; border-radius: 4px;
            }
            QScrollBar::handle:vertical {
                background: #ccc; border-radius: 4px; min-height: 20px;
            }
            QScrollBar::handle:vertical:hover { background: #bbb; }
            QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
        )");
    }
}