#pragma once
#include <QMainWindow>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTreeWidget>
#include <QLabel>
#include <QStackedWidget>
#include <QToolBar>
#include <QStatusBar>
#include <QMenu>
#include <QAction>
#include <qboxlayout.h>
#include "../core/Filesystem.hpp"
#include "../core/AppManager.hpp"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    // ── Core backend ──────────────────────────
    FileSystem  fileSystem_;
    AppManager  appManager_;

    // ── Top bar ───────────────────────────────
    QPushButton*  homeButton_;
    QPushButton*  upButton_;
    QPushButton*  refreshButton_;
    QLineEdit*    pathEdit_;
    QPushButton*  propertiesButton_;
    QPushButton*  themeButton_;

    // ── Toolbar actions ───────────────────────
    QToolBar*   actionToolBar_;
    QAction*    copyAction_;
    QAction*    cutAction_;
    QAction*    pasteAction_;
    QAction*    renameAction_;
    QAction*    deleteAction_;
    QAction*    favAction_;
    QAction*    gridViewAction_;
    QAction*    listViewAction_;
    QAction*    newFolderAction_;

    // ── File views ────────────────────────────
    QStackedWidget* viewStack_;
    QListWidget*    gridView_;     // icon/grid mode
    QTreeWidget*    listView_;     // details/list mode

    // ── Status bar ────────────────────────────
    QLabel*     statusLabel_;

    // ── State ─────────────────────────────────
    bool        isDarkMode_  = true;
    bool        isGridMode_  = true;
    QString     selectedPath_;       // currently selected file path

    // ── Methods ───────────────────────────────
    void buildTopBar(QVBoxLayout* mainLayout);
    void buildToolBar();
    void buildFileViews(QVBoxLayout* mainLayout);
    void buildStatusBar();
    void buildContextMenus();
    void connectSignals();

    void refreshFileView();
    void applyTheme(bool dark);
    void updateStatusBar();

    // File actions
    void onCopy();
    void onCut();
    void onPaste();
    void onRename();
    void onDelete();
    void onToggleFav();
    void onNewFolder();
    void onProperties(const QString& path = "");
    void onOpenWith(const QString& path);

    // Selection helpers
    QString     currentSelectedPath() const;
    void        setSelection(const QString& path);

    void showContextMenu(const QPoint& globalPos, bool hasSelection);
};