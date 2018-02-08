#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "innodedialog.h"
#include "bookkeeping.h"
#include "serialization.h"

#include <QFileDialog>
#include <QFileInfo>

namespace cashbook
{

static const QString defaultDataFile {"cashbook.json"};

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
    ui->inCategoriesTree->expandAll();
    ui->outCategoriesTree->expandAll();

    ui->splitter->setStretchFactor(0, 100);
    ui->splitter->setStretchFactor(1, 50);

    ui->logTable->setColumnWidth(LogColumn::Date, 55);
    ui->logTable->setColumnWidth(LogColumn::Type, 70);
    ui->logTable->setColumnWidth(LogColumn::Category, 200);
    ui->logTable->setColumnWidth(LogColumn::Money, 65);
    ui->logTable->setColumnWidth(LogColumn::From, 145);
    ui->logTable->setColumnWidth(LogColumn::To, 145);

    connect(&m_data.wallets, &WalletsModel::recalculated, ui->walletsTree, &QTreeView::expandAll);

    showMaximized();

    QFileInfo f(defaultDataFile);
    if(f.exists()) {
        loadFile(defaultDataFile);
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::loadBriefStatistics()
{
    const QString title {tr("Краткая статистика")};
    const QString titleFont {"\"Segoe UI\""};
    const QString titleAlign {"center"};
    const QString titleColor {"black"};
    const QString titleFontSize {"11pt"};
    const QString titleWeight {"normal"};

    const QString border {"0"};
    const QString textPadding {"0"};

    const QString dateFont {"\"Segoe UI\""};
    const QString dateFontSize {"9pt"};
    const QString dateColor {"black"};
    const QString dateWeight {""};

    const QString moneyFont {"\"Segoe UI\""};
    const QString moneyFontSize {"9pt"};
    const QString moneyOutColor {"#be3922"};
    const QString moneyInColor {"green"};
    const QString moneyWeight {"bold"};

    const QString dateMoneySpacing {"10"};

    QString text = "<div style='height:1px; font-size:14px;'>&nbsp;</div>"
                   "<p style='text-align: "+titleAlign+"; color: "+titleColor+"; font-family: "+titleFont+"; font-size: "+titleFontSize+"; font-weight: "+titleWeight+";'>"+title+"</p>"
                   "<br/>"
                   "<table align='center' border='"+border+"' width=100% style='font-family: "+dateFont+"; font-size: "+dateFontSize+"; font-weight: "+dateWeight+";'>";

    for(const auto &record : m_data.briefStatistics) {
        text += "<tr>"
                    "<td valign='middle' style='text-align: right; padding: 0 0 0 "+textPadding+"; color: "+dateColor+"'>"+record.first.toString()+"</td>"
                    "<td width='"+dateMoneySpacing+"'></td>"
                    "<td style='text-align: left;'><table border='"+border+"' style='font-family: "+moneyFont+"; font-weight: "+moneyWeight+"; font-size: "+moneyFontSize+";'>"
                        "<tr><td style='text-align: left; color: "+moneyInColor+"; padding: 0 "+textPadding+" 0 0'>▲ "+formatMoney(record.second.received)+"</td></tr>"
                        "<tr><td style='text-align: left; color: "+moneyOutColor+"; padding: 0 "+textPadding+" 0 0'>▼ "+formatMoney(record.second.spent)+"</td></tr>"
                    "</table></td>"
                "</tr>"
                "<tr><td><br/></td></tr>";
    }

    text += "</table>";

    ui->statisticsField->setText(text);
}

void MainWindow::loadFile(const QString &filename)
{
    if(filename.isEmpty()) {
        return;
    }

    cashbook::load(m_data, filename);
    loadBriefStatistics();

    ui->walletsTree->resizeColumnToContents(WalletColumn::Name);
}

void MainWindow::saveFile(const QString &filename)
{
    if(filename.isEmpty()) {
        return;
    }

    cashbook::save(m_data, filename);
}

} // namespace cashbook

//
// Common tree templates
//
template <class TreeModel>
static void addSiblingNode(QTreeView &view, TreeModel &model)
{
    auto index = view.currentIndex();

    if(!index.isValid()) {
        if(model.rootItem->childCount() == 0) {
            model.insertRow(0, QModelIndex());
        }
        return;
    }

    int i = model.rowCount(index.parent());
    model.insertRow(i, index.parent());
}

template <class TreeModel>
static void addChildNode(QTreeView &view, TreeModel &model)
{
    auto index = view.currentIndex();

    if(!index.isValid()) {
        if(model.rootItem->childCount() == 0) {
            model.insertRow(0, QModelIndex());
        }
        return;
    }

    int i = model.rowCount(index);
    model.insertRow(i, index);
}

template <class TreeModel>
static void removeNode(QTreeView &view, TreeModel &model)
{
    auto index = view.currentIndex();

    if(!index.isValid()) {
        return;
    }
    model.removeRow(index.row(), index.parent());
}

template <class TreeModel>
static void upNode(QTreeView &view, TreeModel &model)
{
    auto index = view.currentIndex();

    if(!index.isValid()) {
        return;
    }

    int row = index.row();

    if(row == 0) {
        return;
    }

    model.moveRow(index.parent(), row, index.parent(), row-1);
}

template <class TreeModel>
static void downNode(QTreeView &view, TreeModel &model)
{
    auto index = view.currentIndex();

    if(!index.isValid()) {
        return;
    }

    int row = index.row();

    if(row == model.rowCount(index.parent())-1) {
        return;
    }

    model.moveRow(index.parent(), row, index.parent(), row+2);
}

template <class TreeModel>
static void outNode(QTreeView &view, TreeModel &model)
{
    auto index = view.currentIndex();

    if(!index.isValid()) {
        return;
    }

    int row = index.row();

    const auto parent = index.parent();

    if(!parent.isValid()) {
        return;
    }

    const auto parentParent = parent.parent();

    model.moveRow(parent, row, parentParent, model.rowCount(parentParent));
}

template <class TreeModel>
static void inNode(QTreeView &view, TreeModel &model)
{
    auto index = view.currentIndex();

    if(!index.isValid()) {
        return;
    }

    int row = index.row();
    auto srcParent = index.parent();

    InNodeDialog d(model);
    if(d.exec()) {
        auto dstParent = d.getIndex();

        if(index == dstParent) {
            return;
        }

        model.moveRow(srcParent, row, dstParent, model.rowCount(dstParent));
    }
}

//
// Users
//
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

//
// Transactions
//
void cashbook::MainWindow::on_addTransactionButton_clicked()
{
    m_data.log.insertRow(0);
}

void cashbook::MainWindow::on_anchoreTransactionsButton_clicked()
{
    m_data.anchoreTransactions();
}


//
// Wallets
//
void cashbook::MainWindow::on_addWalletSiblingButton_clicked()
{
    addSiblingNode(*ui->walletsTree, m_data.wallets);
}

void cashbook::MainWindow::on_addWalletChildButton_clicked()
{
    addChildNode(*ui->walletsTree, m_data.wallets);
}

void cashbook::MainWindow::on_removeWalletButton_clicked()
{
    removeNode(*ui->walletsTree, m_data.wallets);
}

void cashbook::MainWindow::on_upWalletButton_clicked()
{
    upNode(*ui->walletsTree, m_data.wallets);
}

void cashbook::MainWindow::on_downWalletButton_clicked()
{
    downNode(*ui->walletsTree, m_data.wallets);
}

void cashbook::MainWindow::on_outWalletButton_clicked()
{
    outNode(*ui->walletsTree, m_data.wallets);
}

void cashbook::MainWindow::on_inWalletButton_clicked()
{
    inNode(*ui->walletsTree, m_data.wallets);
}

//
// In categories
//
void cashbook::MainWindow::on_addInCategorySiblingButton_clicked()
{
    addSiblingNode(*ui->inCategoriesTree, m_data.inCategories);
}

void cashbook::MainWindow::on_addInCategoryChildButton_clicked()
{
    addChildNode(*ui->inCategoriesTree, m_data.inCategories);
}

void cashbook::MainWindow::on_removeInCategoryButton_clicked()
{
    removeNode(*ui->inCategoriesTree, m_data.inCategories);
}

void cashbook::MainWindow::on_upInCategoryButton_clicked()
{
    upNode(*ui->inCategoriesTree, m_data.inCategories);
}

void cashbook::MainWindow::on_downInCategoryButton_clicked()
{
    downNode(*ui->inCategoriesTree, m_data.inCategories);
}

void cashbook::MainWindow::on_outInCategoryButton_clicked()
{
    outNode(*ui->inCategoriesTree, m_data.inCategories);
}

void cashbook::MainWindow::on_inInCategoryButton_clicked()
{
    inNode(*ui->inCategoriesTree, m_data.inCategories);
}

//
// Out categories
//
void cashbook::MainWindow::on_addOutCategorySiblingButton_clicked()
{
    addSiblingNode(*ui->outCategoriesTree, m_data.outCategories);
}

void cashbook::MainWindow::on_addOutCategoryChildButton_clicked()
{
    addChildNode(*ui->outCategoriesTree, m_data.outCategories);
}

void cashbook::MainWindow::on_removeOutCategoryButton_clicked()
{
    removeNode(*ui->outCategoriesTree, m_data.outCategories);
}

void cashbook::MainWindow::on_upOutCategoryButton_clicked()
{
    upNode(*ui->outCategoriesTree, m_data.outCategories);
}

void cashbook::MainWindow::on_downOutCategoryButton_clicked()
{
    downNode(*ui->outCategoriesTree, m_data.outCategories);
}

void cashbook::MainWindow::on_outOutCategoryButton_clicked()
{
    outNode(*ui->outCategoriesTree, m_data.outCategories);
}

void cashbook::MainWindow::on_inOutCategoryButton_clicked()
{
    inNode(*ui->outCategoriesTree, m_data.outCategories);
}

void cashbook::MainWindow::on_actionSave_triggered()
{
    /*QString filename =
                QFileDialog::getSaveFileName(this, tr("Сохранить файл"), "", tr("Json файлы (*.json)"));*/

    saveFile(defaultDataFile);
}

void cashbook::MainWindow::on_actionOpen_triggered()
{
    QString filename =
                QFileDialog::getOpenFileName(this, tr("Открыть файл"), "", tr("Json файлы (*.json)"));

    loadFile(filename);
}
