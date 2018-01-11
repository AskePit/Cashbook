#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDebug>

namespace cashbook
{

MainWindow::MainWindow(Data &data, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_data(data)
    , m_logDelegate(m_data)
{
    ui->setupUi(this);

    ui->logTable->setItemDelegate(&m_logDelegate);

    ui->outCategoriesTree->setModel(&m_data.outCategories);
    ui->inCategoriesTree->setModel(&m_data.inCategories);
    ui->walletsTree->setModel(&m_data.wallets);
    ui->ownersList->setModel(&m_data.owners);
    ui->logTable->setModel(&m_data.log);

    ui->walletsTree->expandAll();
    ui->walletsTree->resizeColumnToContents(0);
}

MainWindow::~MainWindow()
{
    delete ui;
}

} // namespace cashbook

void cashbook::MainWindow::on_inCategoriesTree_clicked(const QModelIndex &index)
{
    ui->statusbar->showMessage(pathToString(m_data.inCategories.getItem(index)));
}

void cashbook::MainWindow::on_outCategoriesTree_clicked(const QModelIndex &index)
{
    ui->statusbar->showMessage(pathToString(m_data.outCategories.getItem(index)));
}

void cashbook::MainWindow::on_walletsTree_clicked(const QModelIndex &index)
{
    ui->statusbar->showMessage(pathToString(m_data.wallets.getItem(index)));
}
