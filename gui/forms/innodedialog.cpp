#include "innodedialog.h"
#include "ui_innodedialog.h"

#include "bookkeeping/models.h"

#include <QAbstractItemModel>

namespace cashbook {

InNodeDialog::InNodeDialog(QAbstractItemModel &model, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::InNodeDialog),
    m_model(model)
{
    ui->setupUi(this);
    ui->treeView->setModel(&m_model);
    ui->treeView->setEditTriggers(QTreeView::NoEditTriggers);
    for(int i = CategoriesColumn::Name+1; i<CategoriesColumn::Count; ++i) {
        ui->treeView->hideColumn(i);
    }
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

} // namespace cashbook
