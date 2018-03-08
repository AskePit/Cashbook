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

template <class View>
static void extendColumn (View *view, int column, int pad) {
    view->setColumnWidth(column, view->columnWidth(column) + pad);
}

template <class View>
static void resizeContentsWithPadding(View *view, int columns, int pad) {
    view->resizeColumnsToContents();
    for(int i = 0; i<columns; ++i) {
        extendColumn(view, i, pad);
    }
}

template <class View>
static void resizeCellWithPadding(View *view, int column, int pad) {
    view->resizeColumnToContents(column);
    extendColumn(view, column, pad);
}

static void hideAll(QWidgetList l) {
    for(QWidget *w : l) {
        w->hide();
    }
}

static void showAll(QWidgetList l) {
    for(QWidget *w : l) {
        w->show();
    }
}

static void setItemDelegate(QItemDelegate &delegate, QList<QAbstractItemView *> l) {
    for(QAbstractItemView *v : l) {
        v->setItemDelegate(&delegate);
    }
}

static void setItemDelegateForColumn(QItemDelegate &delegate, int column, QList<QAbstractItemView *> l) {
    for(QAbstractItemView *v : l) {
        v->setItemDelegateForColumn(column, &delegate);
    }
}

static void recalculateHeight(QTableView *view) {
    int height = qMax(view->verticalHeader()->length() + view->horizontalHeader()->height(), 115);
    view->setFixedHeight(height);
}

static void setSplitterStretching(QSplitter *splitter, int x1, int x2) {
    splitter->setStretchFactor(0, x1);
    splitter->setStretchFactor(1, x2);
}

MainWindow::MainWindow(Data &data, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_data(data)
    , m_modelsDelegate(m_data)
{
    preLoadSetup();
    loadFile();
    postLoadSetup();

    showMaximized();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::preLoadSetup()
{
    ui->setupUi(this);

    ui->menuLayout->setContentsMargins(0, 0, 0, 0);
    ui->stackedWidget->setContentsMargins(0, 0, 0, 0);

    hideUnanchoredSum();

    const int stretch1 = 100;
    const int stretch2 = 30;
    setSplitterStretching(ui->logSplitter, stretch1, stretch2);
    setSplitterStretching(ui->briefSplitter, stretch1, stretch2);
    setSplitterStretching(ui->ownersSplitter, stretch1, stretch2);

    ui->logTable->setColumnWidth(LogColumn::Date, 55);
    ui->logTable->setColumnWidth(LogColumn::Type, 70);
    ui->logTable->setColumnWidth(LogColumn::Category, 200);
    ui->logTable->setColumnWidth(LogColumn::Money, 65);
    ui->logTable->setColumnWidth(LogColumn::From, 145);
    ui->logTable->setColumnWidth(LogColumn::To, 145);

    ui->dateTo->setMaximumDate(today); // should be today day
    ui->thisMonthButton->setText(months[today.month()-1]);
    ui->thisYearButton->setText(QString::number(today.year()));

    m_categoriesEventFilter.setViews(ui->inCategoriesTree, ui->outCategoriesTree);
    ui->inCategoriesTree->viewport()->installEventFilter(&m_categoriesEventFilter);
    ui->outCategoriesTree->viewport()->installEventFilter(&m_categoriesEventFilter);

    setItemDelegate(m_modelsDelegate, {
        ui->logTable,
        ui->shortPlansTable,
        ui->middlePlansTable,
        ui->longPlansTable,
        ui->activeTasksTable,
        ui->completedTasksTable,
    });

    setItemDelegateForColumn(m_modelsDelegate, CategoriesColumn::Regular, {
        ui->inCategoriesTree,
        ui->outCategoriesTree,
    });

    ui->outCategoriesTree->setModel(&m_data.outCategories);
    ui->inCategoriesTree->setModel(&m_data.inCategories);
    ui->walletsTree->setModel(&m_data.wallets);
    ui->ownersList->setModel(&m_data.owners);
    ui->logTable->setModel(&m_data.log);

    ui->shortPlansTable->setModel(&m_data.plans.shortPlans);
    ui->middlePlansTable->setModel(&m_data.plans.middlePlans);
    ui->longPlansTable->setModel(&m_data.plans.longPlans);
    ui->activeTasksTable->setModel(&m_data.tasks.active);
    ui->completedTasksTable->setModel(&m_data.tasks.completed);

    connect(ui->inCategoriesTree, &QTreeView::customContextMenuRequested, this, &MainWindow::showInCategoryMenu);
    connect(ui->outCategoriesTree, &QTreeView::customContextMenuRequested, this, &MainWindow::showOutCategoryMenu);
    connect(ui->logTable, &QTableView::customContextMenuRequested, this, &MainWindow::showLogContextMenu);

    connect(&m_data.wallets, &WalletsModel::recalculated, ui->walletsTree, &QTreeView::expandAll);
    connect(&m_data.log, &TreeModel::dataChanged, this, &MainWindow::updateUnanchoredSum);

    ui->shortPlansBar->installEventFilter(&m_clickFilter);
    ui->middlePlansBar->installEventFilter(&m_clickFilter);
    ui->longPlansBar->installEventFilter(&m_clickFilter);

    connect(&m_clickFilter, &ClickFilter::mouseClicked, [this](QWidget *w){
        if(w == ui->shortPlansBar) {
            showShortPlans(true);
            showMiddlePlans(false);
            showLongPlans(false);
        } else if(w == ui->middlePlansBar) {
            showShortPlans(false);
            showMiddlePlans(true);
            showLongPlans(false);
        } else if(w == ui->longPlansBar) {
            showShortPlans(false);
            showMiddlePlans(false);
            showLongPlans(true);
        }
    });

    emit m_clickFilter.mouseClicked(ui->shortPlansBar);

    connect(&m_data.plans.shortPlans, &QAbstractItemModel::rowsInserted, [this](){ recalculateHeight(ui->shortPlansTable); });
    connect(&m_data.plans.shortPlans, &QAbstractItemModel::rowsRemoved, [this](){ recalculateHeight(ui->shortPlansTable); });
    connect(&m_data.plans.middlePlans, &QAbstractItemModel::rowsInserted, [this](){ recalculateHeight(ui->middlePlansTable); });
    connect(&m_data.plans.middlePlans, &QAbstractItemModel::rowsRemoved, [this](){ recalculateHeight(ui->middlePlansTable); });
    connect(&m_data.plans.longPlans, &QAbstractItemModel::rowsInserted, [this](){ recalculateHeight(ui->longPlansTable); });
    connect(&m_data.plans.longPlans, &QAbstractItemModel::rowsRemoved, [this](){ recalculateHeight(ui->longPlansTable); });
    connect(&m_data.tasks.active, &QAbstractItemModel::rowsInserted, [this](){ recalculateHeight(ui->activeTasksTable); });
    connect(&m_data.tasks.active, &QAbstractItemModel::rowsRemoved, [this](){ recalculateHeight(ui->activeTasksTable); });
    connect(&m_data.tasks.completed, &QAbstractItemModel::rowsInserted, [this](){ recalculateHeight(ui->completedTasksTable); });
    connect(&m_data.tasks.completed, &QAbstractItemModel::rowsRemoved, [this](){ recalculateHeight(ui->completedTasksTable); });
}

void MainWindow::loadFile()
{
    cashbook::load(m_data);
}

void MainWindow::postLoadSetup()
{
    ui->dateFrom->setDate(m_data.statistics.categoriesFrom);

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

    connect(&m_data, &Data::categoriesStatisticsUpdated, [this]() {
        const auto *inRoot = m_data.inCategories.rootItem;
        const auto *outRoot = m_data.outCategories.rootItem;

        const Money &in = m_data.statistics.inCategories[inRoot];
        const Money &out = m_data.statistics.outCategories[outRoot];

        ui->inTotalLabel->setText(formatMoney(in));
        ui->outTotalLabel->setText(formatMoney(out));
    });

    ui->dateTo->setDate(m_data.statistics.categoriesTo);

    int pad = 17;
    resizeContentsWithPadding(ui->logTable, LogColumn::Count, pad);
    resizeContentsWithPadding(ui->shortPlansTable, PlansColumn::Count, pad);
    resizeContentsWithPadding(ui->middlePlansTable, PlansColumn::Count, pad);
    resizeContentsWithPadding(ui->longPlansTable, PlansColumn::Count, pad);
    resizeContentsWithPadding(ui->activeTasksTable, TasksColumn::Count, pad);
    resizeContentsWithPadding(ui->completedTasksTable, TasksColumn::Count, pad);

    ui->walletsTree->resizeColumnToContents(WalletColumn::Name);

    pad = 25;
    resizeCellWithPadding(ui->inCategoriesTree, CategoriesColumn::Name, pad);
    resizeCellWithPadding(ui->outCategoriesTree, CategoriesColumn::Name, pad);

    updateUnanchoredSum();

    ui->briefTable->setModel(&m_data.briefModel);
    for(int row = 0; row<ui->briefTable->model()->rowCount(); row += BriefRow::Count) {
        ui->briefTable->setSpan(row + BriefRow::Received, BriefColumn::Date, 2, 1);
        ui->briefTable->setSpan(row + BriefRow::Received, BriefColumn::Balance, 2, 1);
    }

    resizeContentsWithPadding(ui->briefTable, BriefColumn::Count, 30);

    recalculateHeight(ui->shortPlansTable);
    recalculateHeight(ui->middlePlansTable);
    recalculateHeight(ui->longPlansTable);
    recalculateHeight(ui->activeTasksTable);
    recalculateHeight(ui->completedTasksTable);
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
// Common list templates
//

template <class ListModel>
static bool addListItem(ListModel &model)
{
    return model.insertRow(model.rowCount());
}

template <class ListModel>
static bool removeListItem(QAbstractItemView &view, ListModel &model)
{
    auto index = view.currentIndex();

    if(!index.isValid()) {
        return false;
    }

    int row = index.row();
    return model.removeRow(row);
}

template <class ListModel>
static bool upListItem(QAbstractItemView &view, ListModel &model)
{
    auto index = view.currentIndex();

    if(!index.isValid()) {
        return false;
    }

    int row = index.row();

    if(row == 0) {
        return false;
    }

    return model.moveRow(QModelIndex(), row, QModelIndex(), row-1);
}

template <class ListModel>
static bool downListItem(QAbstractItemView &view, ListModel &model)
{
    auto index = view.currentIndex();

    if(!index.isValid()) {
        return false;
    }

    int row = index.row();

    if(row == model.rowCount()-1) {
        return false;
    }

    return model.moveRow(QModelIndex(), row, QModelIndex(), row+2);
}

//
// Transactions
//
void cashbook::MainWindow::on_addTransactionButton_clicked()
{
    m_changed |= m_data.log.insertRow(0);

    showUnanchoredSum();
}

void cashbook::MainWindow::on_removeTransactionButton_clicked()
{
    QModelIndex index = ui->logTable->currentIndex();
    if(index.isValid()) {
        m_data.log.removeRow(index.row());
    }

    updateUnanchoredSum();
}

void cashbook::MainWindow::on_anchoreTransactionsButton_clicked()
{
    bool did = m_data.anchoreTransactions();
    m_changed |= did;

    hideUnanchoredSum();
}


//
// Wallets
//
void cashbook::MainWindow::on_addWalletSiblingButton_clicked() {
    m_changed |= addSiblingNode(*ui->walletsTree, m_data.wallets);
}
void cashbook::MainWindow::on_addWalletChildButton_clicked() {
    m_changed |= addChildNode(*ui->walletsTree, m_data.wallets);
}
void cashbook::MainWindow::on_removeWalletButton_clicked() {
    m_changed |= removeNode(*ui->walletsTree, m_data.wallets);
}
void cashbook::MainWindow::on_upWalletButton_clicked() {
    m_changed |= upNode(*ui->walletsTree, m_data.wallets);
}
void cashbook::MainWindow::on_downWalletButton_clicked()
{
    m_changed |= downNode(*ui->walletsTree, m_data.wallets);
}
void cashbook::MainWindow::on_outWalletButton_clicked() {
    m_changed |= outNode(*ui->walletsTree, m_data.wallets);
}
void cashbook::MainWindow::on_inWalletButton_clicked() {
    m_changed |= inNode(*ui->walletsTree, m_data.wallets);
}

//
// In categories
//
void cashbook::MainWindow::on_addInCategorySiblingButton_clicked() {
    m_changed |= addSiblingNode(*ui->inCategoriesTree, m_data.inCategories);
}
void cashbook::MainWindow::on_addInCategoryChildButton_clicked() {
    m_changed |= addChildNode(*ui->inCategoriesTree, m_data.inCategories);
}
void cashbook::MainWindow::on_removeInCategoryButton_clicked() {
    m_changed |= removeNode(*ui->inCategoriesTree, m_data.inCategories);
}
void cashbook::MainWindow::on_upInCategoryButton_clicked() {
    m_changed |= upNode(*ui->inCategoriesTree, m_data.inCategories);
}
void cashbook::MainWindow::on_downInCategoryButton_clicked() {
    m_changed |= downNode(*ui->inCategoriesTree, m_data.inCategories);
}
void cashbook::MainWindow::on_outInCategoryButton_clicked() {
    m_changed |= outNode(*ui->inCategoriesTree, m_data.inCategories);
}
void cashbook::MainWindow::on_inInCategoryButton_clicked() {
    m_changed |= inNode(*ui->inCategoriesTree, m_data.inCategories);
}

//
// Out categories
//
void cashbook::MainWindow::on_addOutCategorySiblingButton_clicked() {
    m_changed |= addSiblingNode(*ui->outCategoriesTree, m_data.outCategories);
}
void cashbook::MainWindow::on_addOutCategoryChildButton_clicked() {
    m_changed |= addChildNode(*ui->outCategoriesTree, m_data.outCategories);
}
void cashbook::MainWindow::on_removeOutCategoryButton_clicked() {
    m_changed |= removeNode(*ui->outCategoriesTree, m_data.outCategories);
}
void cashbook::MainWindow::on_upOutCategoryButton_clicked() {
    m_changed |= upNode(*ui->outCategoriesTree, m_data.outCategories);
}
void cashbook::MainWindow::on_downOutCategoryButton_clicked() {
    m_changed |= downNode(*ui->outCategoriesTree, m_data.outCategories);
}
void cashbook::MainWindow::on_outOutCategoryButton_clicked() {
    m_changed |= outNode(*ui->outCategoriesTree, m_data.outCategories);
}
void cashbook::MainWindow::on_inOutCategoryButton_clicked() {
    m_changed |= inNode(*ui->outCategoriesTree, m_data.outCategories);
}

//
// Users
//
void cashbook::MainWindow::on_addUserButton_clicked() {
    m_changed |= addListItem(m_data.owners);
}
void cashbook::MainWindow::on_removeUserButton_clicked() {
    m_changed |= removeListItem(*ui->ownersList, m_data.owners);
}
void cashbook::MainWindow::on_upUserButton_clicked() {
    m_changed |= upListItem(*ui->ownersList, m_data.owners);
}
void cashbook::MainWindow::on_downUserButton_clicked() {
    m_changed |= downListItem(*ui->ownersList, m_data.owners);
}

//
// Plans
//
void cashbook::MainWindow::on_addShortPlanButton_clicked() {
    m_changed |= addListItem(m_data.plans.shortPlans);
}
void cashbook::MainWindow::on_removeShortPlanButton_clicked() {
    m_changed |= removeListItem(*ui->shortPlansTable, m_data.plans.shortPlans);
}
void cashbook::MainWindow::on_upShortPlanButton_clicked() {
    m_changed |= upListItem(*ui->shortPlansTable, m_data.plans.shortPlans);
}
void cashbook::MainWindow::on_downShortPlanButton_clicked() {
    m_changed |= downListItem(*ui->shortPlansTable, m_data.plans.shortPlans);
}

void cashbook::MainWindow::on_addMiddlePlanButton_clicked() {
    m_changed |= addListItem(m_data.plans.middlePlans);
}
void cashbook::MainWindow::on_removeMiddlePlanButton_clicked() {
    m_changed |= removeListItem(*ui->middlePlansTable, m_data.plans.middlePlans);
}
void cashbook::MainWindow::on_upMiddlePlanButton_clicked() {
    m_changed |= upListItem(*ui->middlePlansTable, m_data.plans.middlePlans);
}
void cashbook::MainWindow::on_downMiddlePlanButton_clicked() {
    m_changed |= downListItem(*ui->middlePlansTable, m_data.plans.middlePlans);
}

void cashbook::MainWindow::on_addLongPlanButton_clicked() {
    m_changed |= addListItem(m_data.plans.longPlans);
}
void cashbook::MainWindow::on_removeLongPlanButton_clicked() {
    m_changed |= removeListItem(*ui->longPlansTable, m_data.plans.longPlans);
}
void cashbook::MainWindow::on_upLongPlanButton_clicked() {
    m_changed |= upListItem(*ui->longPlansTable, m_data.plans.longPlans);
}
void cashbook::MainWindow::on_downLongPlanButton_clicked() {
    m_changed |= downListItem(*ui->longPlansTable, m_data.plans.longPlans);
}

void cashbook::MainWindow::on_addActiveTaskButton_clicked() {
    m_changed |= addListItem(m_data.tasks.active);
}
void cashbook::MainWindow::on_removeActiveTaskButton_clicked() {
    m_changed |= removeListItem(*ui->activeTasksTable, m_data.tasks.active);
}
void cashbook::MainWindow::on_upActiveTaskButton_clicked() {
    m_changed |= upListItem(*ui->activeTasksTable, m_data.tasks.active);
}
void cashbook::MainWindow::on_downActiveTaskButton_clicked() {
    m_changed |= downListItem(*ui->activeTasksTable, m_data.tasks.active);
}

void cashbook::MainWindow::on_removeCompletedTaskButton_clicked() {
    m_changed |= removeListItem(*ui->completedTasksTable, m_data.tasks.completed);
}

void cashbook::MainWindow::on_actionSave_triggered()
{
    /*if(!m_changed) {
        return;
    }*/

    saveFile();
    m_changed = false;
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
        Plans,
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

void cashbook::MainWindow::on_plansButton_clicked()
{
    ui->stackedWidget->setCurrentIndex(Tab::Plans);
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

void cashbook::MainWindow::showCategoryStatement(Transaction::Type::t type)
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

void cashbook::MainWindow::updateUnanchoredSum()
{
    if(m_data.log.unanchored) {
        showUnanchoredSum();
    } else {
        hideUnanchoredSum();
    }
}

void cashbook::MainWindow::showUnanchoredSum()
{
    Money sum;
    for(int i = 0; i<m_data.log.unanchored; ++i) {
        sum += m_data.log.log[i].amount;
    }
    ui->unanchoredSumLabel->setText(formatMoney(sum));

    ui->unanchoredLabel->show();
    ui->unanchoredSumLabel->show();
}

void cashbook::MainWindow::hideUnanchoredSum()
{
    ui->unanchoredLabel->hide();
    ui->unanchoredSumLabel->hide();
}

void cashbook::MainWindow::showShortPlans(bool show)
{
    static const QWidgetList l {
        ui->shortPlansTable,
        ui->addShortPlanButton,
        ui->removeShortPlanButton,
        ui->upShortPlanButton,
        ui->downShortPlanButton,
    };

    show ? showAll(l) : hideAll(l);
}

void cashbook::MainWindow::showMiddlePlans(bool show)
{
    static const QWidgetList l {
        ui->middlePlansTable,
        ui->addMiddlePlanButton,
        ui->removeMiddlePlanButton,
        ui->upMiddlePlanButton,
        ui->downMiddlePlanButton,
    };

    show ? showAll(l) : hideAll(l);
}

void cashbook::MainWindow::showLongPlans(bool show)
{
    static const QWidgetList l {
        ui->longPlansTable,
        ui->addLongPlanButton,
        ui->removeLongPlanButton,
        ui->upLongPlanButton,
        ui->downLongPlanButton,
    };

    show ? showAll(l) : hideAll(l);
}


void cashbook::MainWindow::showActiveTasks(bool show)
{
    static const QWidgetList l {
        ui->activeTasksTable,
        ui->addActiveTaskButton,
        ui->removeActiveTaskButton,
        ui->upActiveTaskButton,
        ui->downActiveTaskButton,
    };

    show ? showAll(l) : hideAll(l);
}

void cashbook::MainWindow::showCompletedTasks(bool show)
{
    static const QWidgetList l {
        ui->completedTasksTable,
    };

    show ? showAll(l) : hideAll(l);
}


void cashbook::MainWindow::on_actionInStatement_triggered()
{
    showCategoryStatement(Transaction::Type::In);
}

void cashbook::MainWindow::on_actionOutStatement_triggered()
{
    showCategoryStatement(Transaction::Type::Out);
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
