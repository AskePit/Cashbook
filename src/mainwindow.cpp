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
    ui->walletsTree->resizeColumnToContents(1);
    ui->splitter->setStretchFactor(0, 100);
    ui->splitter->setStretchFactor(1, 50);
}

MainWindow::~MainWindow()
{
    delete ui;
}

} // namespace cashbook

void cashbook::MainWindow::on_addUserButton_clicked()
{
    m_data.owners.insertRow(m_data.owners.rowCount());
}

void cashbook::MainWindow::on_removeUserButton_clicked()
{
    auto index = ui->ownersList->currentIndex();

    if(!index.isValid()) {
        return;
    }

    int row = index.row();
    m_data.owners.removeRow(row);
}

void cashbook::MainWindow::on_upUserButton_clicked()
{
    auto index = ui->ownersList->currentIndex();

    if(!index.isValid()) {
        return;
    }

    int row = index.row();

    if(row == 0) {
        return;
    }

    m_data.owners.moveRow(QModelIndex(), row, QModelIndex(), row-1);
}

void cashbook::MainWindow::on_downUserButton_clicked()
{
    auto index = ui->ownersList->currentIndex();

    if(!index.isValid()) {
        return;
    }

    int row = index.row();

    if(row == m_data.owners.rowCount()-1) {
        return;
    }

    m_data.owners.moveRow(QModelIndex(), row, QModelIndex(), row+2);
}

void cashbook::MainWindow::on_addTransactionButton_clicked()
{
    m_data.log.insertRow(0);
}

void cashbook::MainWindow::on_removeTransactionButton_clicked()
{
    if(m_data.log.rowCount() == 0) {
        return;
    }

    m_data.log.removeRow(0);
}

void cashbook::MainWindow::on_addWalletSiblingButton_clicked()
{
    auto index = ui->walletsTree->currentIndex();

    if(!index.isValid()) {
        return;
    }

    int i = m_data.wallets.rowCount(index.parent());
    m_data.wallets.insertRow(i, index.parent());
}

void cashbook::MainWindow::on_addWalletChildButton_clicked()
{
    auto index = ui->walletsTree->currentIndex();

    if(!index.isValid()) {
        return;
    }

    int i = m_data.wallets.rowCount(index);
    m_data.wallets.insertRow(i, index);
}

void cashbook::MainWindow::on_removeWalletButton_clicked()
{
    auto index = ui->walletsTree->currentIndex();

    if(!index.isValid()) {
        return;
    }

    m_data.wallets.removeRow(index.row(), index.parent());
}

void cashbook::MainWindow::on_addInCategorySiblingButton_clicked()
{
    auto index = ui->inCategoriesTree->currentIndex();

    if(!index.isValid()) {
        return;
    }

    int i = m_data.inCategories.rowCount(index.parent());
    m_data.inCategories.insertRow(i, index.parent());
}

void cashbook::MainWindow::on_addInCategoryChildButton_clicked()
{
    auto index = ui->inCategoriesTree->currentIndex();

    if(!index.isValid()) {
        return;
    }

    int i = m_data.inCategories.rowCount(index);
    m_data.inCategories.insertRow(i, index);
}

void cashbook::MainWindow::on_removeInCategoryButton_clicked()
{
    auto index = ui->inCategoriesTree->currentIndex();

    if(!index.isValid()) {
        return;
    }

    m_data.inCategories.removeRow(index.row(), index.parent());
}

void cashbook::MainWindow::on_addOutCategorySiblingButton_clicked()
{
    auto index = ui->outCategoriesTree->currentIndex();

    if(!index.isValid()) {
        return;
    }

    int i = m_data.outCategories.rowCount(index.parent());
    m_data.outCategories.insertRow(i, index.parent());
}

void cashbook::MainWindow::on_addOutCategoryChildButton_clicked()
{
    auto index = ui->outCategoriesTree->currentIndex();

    if(!index.isValid()) {
        return;
    }

    int i = m_data.outCategories.rowCount(index);
    m_data.outCategories.insertRow(i, index);
}

void cashbook::MainWindow::on_removeOutCategoryButton_clicked()
{
    auto index = ui->outCategoriesTree->currentIndex();

    if(!index.isValid()) {
        return;
    }

    m_data.outCategories.removeRow(index.row(), index.parent());
}
