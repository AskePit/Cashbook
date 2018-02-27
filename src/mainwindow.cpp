#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <std/fs.h>
#include "innodedialog.h"
#include "bookkeeping/bookkeeping.h"
#include "serialization.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QHeaderView>
#include <QInputDialog>

namespace cashbook
{

static const QStringList months = {
    QObject::tr("Январь"),
    QObject::tr("Февраль"),
    QObject::tr("Март"),
    QObject::tr("Апрель"),
    QObject::tr("Май"),
    QObject::tr("Июнь"),
    QObject::tr("Июль"),
    QObject::tr("Август"),
    QObject::tr("Сентябрь"),
    QObject::tr("Октябрь"),
    QObject::tr("Ноябрь"),
    QObject::tr("Декабрь"),
};

MainWindow::MainWindow(Data &data, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_data(data)
    , m_logDelegate(m_data)
{
    ui->setupUi(this);

    ui->menuLayout->setContentsMargins(0, 0, 0, 0);
    ui->stackedWidget->setContentsMargins(0, 0, 0, 0);

    ui->logTable->setItemDelegate(&m_logDelegate);
    ui->inCategoriesTree->setItemDelegateForColumn(CategoriesColumn::Regular, &m_boolDelegate);
    ui->outCategoriesTree->setItemDelegateForColumn(CategoriesColumn::Regular, &m_boolDelegate);

    connect(ui->inCategoriesTree, &QTreeView::customContextMenuRequested, this, &MainWindow::showInCategoryMenu);
    connect(ui->outCategoriesTree, &QTreeView::customContextMenuRequested, this, &MainWindow::showOutCategoryMenu);
    connect(ui->logTable, &QTableView::customContextMenuRequested, this, &MainWindow::showLogContextMenu);

    ui->logSplitter->setStretchFactor(0, 100);
    ui->logSplitter->setStretchFactor(1, 35);

    ui->logTable->setColumnWidth(LogColumn::Date, 55);
    ui->logTable->setColumnWidth(LogColumn::Type, 70);
    ui->logTable->setColumnWidth(LogColumn::Category, 200);
    ui->logTable->setColumnWidth(LogColumn::Money, 65);
    ui->logTable->setColumnWidth(LogColumn::From, 145);
    ui->logTable->setColumnWidth(LogColumn::To, 145);

    m_categoriesEventFilter.setViews(ui->inCategoriesTree, ui->outCategoriesTree);
    ui->inCategoriesTree->viewport()->installEventFilter(&m_categoriesEventFilter);
    ui->outCategoriesTree->viewport()->installEventFilter(&m_categoriesEventFilter);

    connect(&m_data.wallets, &WalletsModel::recalculated, ui->walletsTree, &QTreeView::expandAll);

    showMaximized();

    ui->dateTo->setMaximumDate(m_data.statistics.categoriesTo); // should be today day

    connect(ui->dateFrom, &QDateEdit::dateChanged, [this](const QDate &from) {
        m_data.loadCategoriesStatistics(from, m_data.statistics.categoriesTo);
        m_data.inCategories.update();
        m_data.outCategories.update();
    });

    connect(ui->dateTo, &QDateEdit::dateChanged, [this](const QDate &to) {
        m_data.loadCategoriesStatistics(m_data.statistics.categoriesFrom, to);
        m_data.inCategories.update();
        m_data.outCategories.update();
    });

    ui->thisMonthButton->setText(months[today.month()-1]);
    ui->thisYearButton->setText(QString::number(today.year()));

    connect(&m_data, &Data::categoriesStatisticsUpdated, [this]() {
        const auto *inRoot = m_data.inCategories.rootItem;
        const auto *outRoot = m_data.outCategories.rootItem;

        const Money &in = m_data.statistics.inCategories[inRoot];
        const Money &out = m_data.statistics.outCategories[outRoot];

        ui->inTotalLabel->setText(formatMoney(in));
        ui->outTotalLabel->setText(formatMoney(out));
    });

    ui->outCategoriesTree->setModel(&m_data.outCategories);
    ui->inCategoriesTree->setModel(&m_data.inCategories);
    ui->walletsTree->setModel(&m_data.wallets);
    ui->ownersList->setModel(&m_data.owners);
    ui->logTable->setModel(&m_data.log);

    loadFile();

    ui->briefTable->setModel(&m_data.briefModel);
    for(int row = 0; row<ui->briefTable->model()->rowCount(); row += BriefRow::Count) {
        ui->briefTable->setSpan(row + BriefRow::Received, BriefColumn::Date, 2, 1);
    }

    QHeaderView *headerView = ui->briefTable->horizontalHeader();
    headerView->setSectionResizeMode(QHeaderView::Stretch);
}

MainWindow::~MainWindow()
{
    delete ui;
}

template <class View>
void extendColumn (View *view, int column, int pad) {
    view->setColumnWidth(column, view->columnWidth(column) + pad);
}

void MainWindow::loadFile()
{
    cashbook::load(m_data);

    ui->dateFrom->setDate(m_data.statistics.categoriesFrom);
    ui->dateTo->setDate(m_data.statistics.categoriesTo);

    ui->logTable->resizeColumnsToContents();

    int pad = 17;

    for(int i = 0; i<LogColumn::Count; ++i) {
        extendColumn(ui->logTable, i, pad);
    }

    ui->walletsTree->resizeColumnToContents(WalletColumn::Name);
    ui->inCategoriesTree->resizeColumnToContents(CategoriesColumn::Name);
    ui->outCategoriesTree->resizeColumnToContents(CategoriesColumn::Name);

    pad = 25;
    extendColumn(ui->inCategoriesTree, CategoriesColumn::Name, pad);
    extendColumn(ui->outCategoriesTree, CategoriesColumn::Name, pad);
}

void MainWindow::saveFile()
{
    cashbook::save(m_data);
}

} // namespace cashbook

//
// Common tree templates
//
template <class TreeModel>
static bool addSiblingNode(QTreeView &view, TreeModel &model)
{
    auto index = view.currentIndex();

    if(!index.isValid()) {
        if(model.rootItem->childCount() == 0) {
            model.insertRow(0, QModelIndex());
        }
        return true;
    }

    int i = model.rowCount(index.parent());
    model.insertRow(i, index.parent());
    return true;
}

template <class TreeModel>
static bool addChildNode(QTreeView &view, TreeModel &model)
{
    auto index = view.currentIndex();

    if(!index.isValid()) {
        if(model.rootItem->childCount() == 0) {
            model.insertRow(0, QModelIndex());
        }
        return true;
    }

    int i = model.rowCount(index);
    model.insertRow(i, index);
    return true;
}

template <class TreeModel>
static bool removeNode(QTreeView &view, TreeModel &model)
{
    auto index = view.currentIndex();

    if(!index.isValid()) {
        return false;
    }
    model.removeRow(index.row(), index.parent());
    return true;
}

template <class TreeModel>
static bool upNode(QTreeView &view, TreeModel &model)
{
    auto index = view.currentIndex();

    if(!index.isValid()) {
        return false;
    }

    int row = index.row();

    if(row == 0) {
        return false;
    }

    model.moveRow(index.parent(), row, index.parent(), row-1);
    return true;
}

template <class TreeModel>
static bool downNode(QTreeView &view, TreeModel &model)
{
    auto index = view.currentIndex();

    if(!index.isValid()) {
        return false;
    }

    int row = index.row();

    if(row == model.rowCount(index.parent())-1) {
        return false;
    }

    model.moveRow(index.parent(), row, index.parent(), row+2);
    return true;
}

template <class TreeModel>
static bool outNode(QTreeView &view, TreeModel &model)
{
    auto index = view.currentIndex();

    if(!index.isValid()) {
        return false;
    }

    int row = index.row();

    const auto parent = index.parent();

    if(!parent.isValid()) {
        return false;
    }

    const auto parentParent = parent.parent();
    model.moveRow(parent, row, parentParent, model.rowCount(parentParent));
    return true;
}

template <class TreeModel>
static bool inNode(QTreeView &view, TreeModel &model)
{
    auto index = view.currentIndex();

    if(!index.isValid()) {
        return false;
    }

    int row = index.row();
    auto srcParent = index.parent();

    cashbook::InNodeDialog d(model);
    if(d.exec()) {
        auto dstParent = d.getIndex();

        if(index == dstParent) {
            return false;
        }

        model.moveRow(srcParent, row, dstParent, model.rowCount(dstParent));
        return true;
    } else {
        return false;
    }
}

//
// Users
//
void cashbook::MainWindow::on_addUserButton_clicked()
{
    m_changed |= m_data.owners.insertRow(m_data.owners.rowCount());
}

void cashbook::MainWindow::on_removeUserButton_clicked()
{
    auto index = ui->ownersList->currentIndex();

    if(!index.isValid()) {
        return;
    }

    int row = index.row();
    m_changed |= m_data.owners.removeRow(row);
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

    m_changed |= m_data.owners.moveRow(QModelIndex(), row, QModelIndex(), row-1);
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

    m_changed |= m_data.owners.moveRow(QModelIndex(), row, QModelIndex(), row+2);
}

//
// Transactions
//
void cashbook::MainWindow::on_addTransactionButton_clicked()
{
    m_changed |= m_data.log.insertRow(0);
}

void cashbook::MainWindow::on_removeTransactionButton_clicked()
{
    QModelIndex index = ui->logTable->currentIndex();
    if(index.isValid()) {
        m_data.log.removeRow(index.row());
    }

}

void cashbook::MainWindow::on_anchoreTransactionsButton_clicked()
{
    bool did = m_data.anchoreTransactions();
    m_changed |= did;
}


//
// Wallets
//
void cashbook::MainWindow::on_addWalletSiblingButton_clicked()
{
    m_changed |= addSiblingNode(*ui->walletsTree, m_data.wallets);
}

void cashbook::MainWindow::on_addWalletChildButton_clicked()
{
    m_changed |= addChildNode(*ui->walletsTree, m_data.wallets);
}

void cashbook::MainWindow::on_removeWalletButton_clicked()
{
    m_changed |= removeNode(*ui->walletsTree, m_data.wallets);
}

void cashbook::MainWindow::on_upWalletButton_clicked()
{
    m_changed |= upNode(*ui->walletsTree, m_data.wallets);
}

void cashbook::MainWindow::on_downWalletButton_clicked()
{
    m_changed |= downNode(*ui->walletsTree, m_data.wallets);
}

void cashbook::MainWindow::on_outWalletButton_clicked()
{
    m_changed |= outNode(*ui->walletsTree, m_data.wallets);
}

void cashbook::MainWindow::on_inWalletButton_clicked()
{
    m_changed |= inNode(*ui->walletsTree, m_data.wallets);
}

//
// In categories
//
void cashbook::MainWindow::on_addInCategorySiblingButton_clicked()
{
    m_changed |= addSiblingNode(*ui->inCategoriesTree, m_data.inCategories);
}

void cashbook::MainWindow::on_addInCategoryChildButton_clicked()
{
    m_changed |= addChildNode(*ui->inCategoriesTree, m_data.inCategories);
}

void cashbook::MainWindow::on_removeInCategoryButton_clicked()
{
    m_changed |= removeNode(*ui->inCategoriesTree, m_data.inCategories);
}

void cashbook::MainWindow::on_upInCategoryButton_clicked()
{
    m_changed |= upNode(*ui->inCategoriesTree, m_data.inCategories);
}

void cashbook::MainWindow::on_downInCategoryButton_clicked()
{
    m_changed |= downNode(*ui->inCategoriesTree, m_data.inCategories);
}

void cashbook::MainWindow::on_outInCategoryButton_clicked()
{
    m_changed |= outNode(*ui->inCategoriesTree, m_data.inCategories);
}

void cashbook::MainWindow::on_inInCategoryButton_clicked()
{
    m_changed |= inNode(*ui->inCategoriesTree, m_data.inCategories);
}

//
// Out categories
//
void cashbook::MainWindow::on_addOutCategorySiblingButton_clicked()
{
    m_changed |= addSiblingNode(*ui->outCategoriesTree, m_data.outCategories);
}

void cashbook::MainWindow::on_addOutCategoryChildButton_clicked()
{
    m_changed |= addChildNode(*ui->outCategoriesTree, m_data.outCategories);
}

void cashbook::MainWindow::on_removeOutCategoryButton_clicked()
{
    m_changed |= removeNode(*ui->outCategoriesTree, m_data.outCategories);
}

void cashbook::MainWindow::on_upOutCategoryButton_clicked()
{
    m_changed |= upNode(*ui->outCategoriesTree, m_data.outCategories);
}

void cashbook::MainWindow::on_downOutCategoryButton_clicked()
{
    m_changed |= downNode(*ui->outCategoriesTree, m_data.outCategories);
}

void cashbook::MainWindow::on_outOutCategoryButton_clicked()
{
    m_changed |= outNode(*ui->outCategoriesTree, m_data.outCategories);
}

void cashbook::MainWindow::on_inOutCategoryButton_clicked()
{
    m_changed |= inNode(*ui->outCategoriesTree, m_data.outCategories);
}

void cashbook::MainWindow::on_actionSave_triggered()
{
    /*if(!m_changed) {
        return;
    }*/

    /*QDir().mkpath("backup");
    aske::copyFileForced(defaultBackupFile2, defaultBackupFile3);
    aske::copyFileForced(defaultBackupFile1, defaultBackupFile2);
    aske::copyFileForced(defaultDataFile, defaultBackupFile1);*/

    saveFile();
    m_changed = false;
}

void cashbook::MainWindow::on_actionOpen_triggered()
{
    QString filename =
                QFileDialog::getOpenFileName(this, tr("Открыть файл"), "", tr("Json файлы (*.json)"));

    loadFile();
}

int callQuestionDialog(const QString &message, QWidget *parent)
{
    QMessageBox msgBox {parent};
    msgBox.setText(message);

    msgBox.setStandardButtons(QMessageBox::Ok | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::Cancel);
    return msgBox.exec();
}

void cashbook::MainWindow::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);

    if(m_changed) {
        int ret { callQuestionDialog(tr("Сохранить изменения?"), this) };
        if(ret == QMessageBox::Ok) {
            on_actionSave_triggered();
        }
    }
}

class Tab {
public:
    enum t {
        Main = 0,
        Categories,
        Users
    };
};

void cashbook::MainWindow::on_mainButton_clicked()
{
    ui->stackedWidget->setCurrentIndex(Tab::Main);
}

void cashbook::MainWindow::on_categoriesButton_clicked()
{
    ui->stackedWidget->setCurrentIndex(Tab::Categories);
}

void cashbook::MainWindow::on_usersButton_clicked()
{
    ui->stackedWidget->setCurrentIndex(Tab::Users);
}

void cashbook::MainWindow::on_thisMonthButton_clicked()
{
    QDate start(today.year(), today.month(), 1);

    ui->dateFrom->setDate(start);
    ui->dateTo->setDate(today);
}

void cashbook::MainWindow::on_thisYearButton_clicked()
{
    QDate start(today.year(), 1, 1);

    ui->dateFrom->setDate(start);
    ui->dateTo->setDate(today);
}

void cashbook::MainWindow::on_monthButton_clicked()
{
    QDate start {today.addDays(-30)};

    ui->dateFrom->setDate(start);
    ui->dateTo->setDate(today);
}

void cashbook::MainWindow::on_yearButton_clicked()
{
    QDate start {today.addYears(-1)};

    ui->dateFrom->setDate(start);
    ui->dateTo->setDate(today);
}

static void showCategoryContextMenu(const cashbook::CategoriesModel &model, QTreeView *view, QAction *action, const QPoint &point)
{
    QPoint globalPos = view->mapToGlobal(point);
    auto index = view->indexAt(point);

    const Node<cashbook::Category> *node = model.getItem(index);
    bool ok = false;
    if(node) {
        ok = model.statistics[node].as_cents() != 0;
    }

    if(!ok) {
        return;
    }

    QMenu menu(view);
    menu.addAction(action);
    menu.exec(globalPos);
}

void cashbook::MainWindow::showInCategoryMenu(const QPoint& point)
{
    showCategoryContextMenu(m_data.inCategories, ui->inCategoriesTree, ui->actionInStatement, point);
}

void cashbook::MainWindow::showOutCategoryMenu(const QPoint& point)
{
    showCategoryContextMenu(m_data.outCategories, ui->outCategoriesTree, ui->actionOutStatement, point);
}

void cashbook::MainWindow::onActionStatementTriggered(Transaction::Type::t type)
{
    bool in = type == Transaction::Type::In;

    QTreeView *categoryTree = in ? ui->inCategoriesTree : ui->outCategoriesTree;
    CategoriesModel &categoryModel = in ? m_data.inCategories : m_data.outCategories;

    QModelIndex index = categoryTree->currentIndex();
    Node<Category> *node = categoryModel.getItem(index);
    if(!node) {
        return;
    }

    FilteredLogModel *filterModel = new FilteredLogModel(ui->dateFrom->date(), ui->dateTo->date(), type, node, categoryTree);
    filterModel->setSourceModel(&m_data.log);
    QTableView *table = new QTableView();
    table->setWindowFlags(Qt::WindowCloseButtonHint);
    table->setAttribute(Qt::WA_DeleteOnClose);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->verticalHeader()->hide();
    table->verticalHeader()->setDefaultSectionSize(23);
    table->setModel(filterModel);
    table->setGeometry(QRect(categoryTree->mapToGlobal(categoryTree->pos()), categoryTree->size()));
    table->show();
    table->resizeColumnsToContents();
    table->hideColumn(LogColumn::Type);
    table->hideColumn(in ? LogColumn::From : LogColumn::To);
}

void cashbook::MainWindow::on_actionInStatement_triggered()
{
    onActionStatementTriggered(Transaction::Type::In);
}

void cashbook::MainWindow::on_actionOutStatement_triggered()
{
    onActionStatementTriggered(Transaction::Type::Out);
}

void cashbook::MainWindow::showLogContextMenu(const QPoint& point)
{
    QPoint globalPos = ui->logTable->mapToGlobal(point);
    auto index = ui->logTable->indexAt(point);

    if(index.column() != LogColumn::Note) {
        return;
    }

    m_noteContextIndex = index;
    QMenu menu(ui->logTable);
    menu.addAction(ui->actionEditNote);
    menu.exec(globalPos);
}

static QString getTextDialog(const QString &title, const QString &message, const QString &text, QWidget *parent)
{
    bool ok = false;
    QString answer = QInputDialog::getText(parent, title, message, QLineEdit::Normal, text, &ok);
    return ok ? answer : QString::null;
}

void cashbook::MainWindow::on_actionEditNote_triggered()
{
    if(!m_noteContextIndex.isValid()) {
        return;
    }

    const Transaction &t = m_data.log.log[m_noteContextIndex.row()];

    QString note = getTextDialog(tr("Примечание"), tr("Примечание"), t.note, this);
    if(!note.isNull()) {
        m_data.log.updateNote(m_noteContextIndex.row(), note);
    }
}
