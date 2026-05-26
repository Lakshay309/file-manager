#include "FileView.hpp"
#include <qabstractitemmodel.h>
#include <qdir.h>
#include <qfilesystemmodel.h>
#include <qobject.h>
#include <qpushbutton.h>
#include <qtreeview.h>

FileView::FileView(QWidget *parent,QPushButton * backButton,QLineEdit *currentPath):QTreeView(parent){
    
    m_model = new QFileSystemModel(parent);
    
    // copying the calues
    this->backButton=backButton;
    this->currentPath=currentPath;

    m_model->setRootPath(QDir::homePath());

    setModel(m_model);

    setRootIndex(
        m_model->index(QDir::homePath())
    );

    connect(
        this,
        &QTreeView::doubleClicked,
        this,
        &FileView::onItemDoubleClicked
    );

    connect(
        this->backButton,
        &QPushButton::clicked,
        this,
        &FileView::navigateUp
    );   
}

void FileView::onItemDoubleClicked(const QModelIndex &index){
    // for folder in future
    if(!m_model->isDir(index)) {
        return;
    }
    setRootIndex(index);
    updateCurrentPath();
}

void FileView::navigateUp(){
    QString current = m_model->filePath(rootIndex());
    QDir dir(current);
    if (dir.cdUp()){
        navigateTo(dir.absolutePath());
    }
}

void FileView::navigateTo(const QString& path){
    QModelIndex index = m_model->index(path);
    setRootIndex(index);
    updateCurrentPath();
}

// updating path in  currentPath
void FileView::updateCurrentPath(){
    QString current =m_model->filePath(rootIndex());
    QDir dir(current);
    currentPath->setText(current);
}

void FileView::createNewFolder(const QString &dirName){
    if(dirName.isEmpty()) 
        return;

    QString current =m_model->filePath(rootIndex());
    QDir dir(current);
    
    dir.mkdir(dirName);
}
