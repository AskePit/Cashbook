#include "innodedialog.h"
#include "ui_innodedialog.h"

#include <QAbstractItemModel>

InNodeDialog::InNodeDialog(QAbstractItemModel &model, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::InNodeDialog),
    m_model(model)
{
    ui->setupUi(this);
    ui->treeView->setModel(&m_model);
    ui->treeView->expandAll();
}

InNodeDialog::~InNodeDialog()
{
    delete ui;
}

QModelIndex InNodeDialog::getIndex()
{
    return ui->treeView->currentIndex();
}
