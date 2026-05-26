#pragma once

#include <QTreeView>
#include <QFileSystemModel>
#include <QPushButton>
#include <QLineEdit>
#include <qabstractitemmodel.h>
#include <qlineedit.h>
#include <qobject.h>
#include <qpushbutton.h>
#include <qwidget.h>

class FileView : public QTreeView{
    public:
        // constructore parent-> null, backButton-> help use to move back, currentPath display the current path  
        explicit FileView(QWidget *parent=NULL,QPushButton * backButton=NULL,QLineEdit *currentPath = NULL);
        
        // Public Functions
        void updateCurrentPath();
        void navigateUp();
        void navigateTo(const QString& path);
        QString getCurrentPath() const;
        void createNewFolder(const QString &dirName);



    private:
        QFileSystemModel *m_model;
        QPushButton *backButton;
        QLineEdit * currentPath;

        void onItemDoubleClicked(const QModelIndex &index);
};

