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

static void recalculateHeight(QTableView *view) {
    int height = qMax(view->verticalHeader()->length() + view->horizontalHeader()->height(), 115);
    view->setFixedHeight(height);
}

static void setSplitterStretching(QSplitter *splitter, int x1, int x2) {
    splitter->setStretchFactor(0, x1);
    splitter->setStretchFactor(1, x2);
}

void ViewModelMap::operator =(std::initializer_list<ViewModel> list) {
    std::vector<ViewModel>::operator =(list);
}

void ViewModelMap::connectModels() {
    for(ViewModel &vm : *this) {
        vm.view->setModel(vm.model);

        if(vm.contextMenu) {
            QObject::connect(vm.view, &QAbstractItemView::customContextMenuRequested, vm.contextMenu);
        }

        if(vm.delegate) {
            vm.view->setItemDelegate(vm.delegate);
        }
    }
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

    typedef void (MainWindow::*ContextMenuFunc)(const QPoint &);

    using namespace std::placeholders;
    auto bindToThis = [this](ContextMenuFunc f){
        return std::bind(f, this, _1);
    };

    vm = {
        {
            /* .view =        */ ui->outCategoriesTree,
            /* .model =       */ &m_data.outCategoriesModel,
            /* .delegate =    */ &m_modelsDelegate,
            /* .contextMenu = */ bindToThis(&MainWindow::showOutCategoryMenu),
        },
        {
            /* .view =        */ ui->inCategoriesTree,
            /* .model =       */ &m_data.inCategoriesModel,
            /* .delegate =    */ &m_modelsDelegate,
            /* .contextMenu = */ bindToThis(&MainWindow::showInCategoryMenu),
        },
        {
            /* .view =        */ ui->walletsTree,
            /* .model =       */ &m_data.walletsModel,
            /* .delegate =    */ nullptr,
            /* .contextMenu = */ nullptr,
        },
        {
            /* .view =        */ ui->ownersList,
            /* .model =       */ &m_data.ownersModel,
            /* .delegate =    */ nullptr,
            /* .contextMenu = */ nullptr,
        },
        {
            /* .view =        */ ui->logTable,
            /* .model =       */ &m_data.logModel,
            /* .delegate =    */ &m_modelsDelegate,
            /* .contextMenu = */ bindToThis(&MainWindow::showLogMenu),
        },
        {
            /* .view =        */ ui->shortPlansTable,
            /* .model =       */ &m_data.plans[PlanTerm::Short],
            /* .delegate =    */ &m_modelsDelegate,
            /* .contextMenu = */ bindToThis(&MainWindow::showShortPlansMenu),
        },
        {
            /* .view =        */ ui->middlePlansTable,
            /* .model =       */ &m_data.plans[PlanTerm::Middle],
            /* .delegate =    */ &m_modelsDelegate,
            /* .contextMenu = */ bindToThis(&MainWindow::showMiddlePlansMenu),
        },
        {
            /* .view =        */ ui->longPlansTable,
            /* .model =       */ &m_data.plans[PlanTerm::Long],
            /* .delegate =    */ &m_modelsDelegate,
            /* .contextMenu = */ bindToThis(&MainWindow::showLongPlansMenu),
        },
        {
            /* .view =        */ ui->activeTasksTable,
            /* .model =       */ &m_data.tasks[TaskStatus::Active],
            /* .delegate =    */ &m_modelsDelegate,
            /* .contextMenu = */ nullptr,
        },
        {
            /* .view =        */ ui->completedTasksTable,
            /* .model =       */ &m_data.tasks[TaskStatus::Completed],
            /* .delegate =    */ &m_modelsDelegate,
            /* .contextMenu = */ nullptr,
        },
    };

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

    vm.connectModels();

    connect(&m_data.walletsModel, &WalletsModel::recalculated, ui->walletsTree, &QTreeView::expandAll);
    connect(&m_data.logModel, &TreeModel::dataChanged, this, &MainWindow::updateUnanchoredSum);

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

    connect(&m_data.plans[PlanTerm::Short], &QAbstractItemModel::rowsInserted, [this](){ recalculateHeight(ui->shortPlansTable); });
    connect(&m_data.plans[PlanTerm::Short], &QAbstractItemModel::rowsRemoved, [this](){ recalculateHeight(ui->shortPlansTable); });
    connect(&m_data.plans[PlanTerm::Middle], &QAbstractItemModel::rowsInserted, [this](){ recalculateHeight(ui->middlePlansTable); });
    connect(&m_data.plans[PlanTerm::Middle], &QAbstractItemModel::rowsRemoved, [this](){ recalculateHeight(ui->middlePlansTable); });
    connect(&m_data.plans[PlanTerm::Long], &QAbstractItemModel::rowsInserted, [this](){ recalculateHeight(ui->longPlansTable); });
    connect(&m_data.plans[PlanTerm::Long], &QAbstractItemModel::rowsRemoved, [this](){ recalculateHeight(ui->longPlansTable); });
    connect(&m_data.tasks[TaskStatus::Active], &QAbstractItemModel::rowsInserted, [this](){ recalculateHeight(ui->activeTasksTable); });
    connect(&m_data.tasks[TaskStatus::Active], &QAbstractItemModel::rowsRemoved, [this](){ recalculateHeight(ui->activeTasksTable); });
    connect(&m_data.tasks[TaskStatus::Completed], &QAbstractItemModel::rowsInserted, [this](){ recalculateHeight(ui->completedTasksTable); });
    connect(&m_data.tasks[TaskStatus::Completed], &QAbstractItemModel::rowsRemoved, [this](){ recalculateHeight(ui->completedTasksTable); });
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
        m_data.inCategoriesModel.update();
        m_data.outCategoriesModel.update();
    });

    connect(ui->dateTo, &QDateEdit::dateChanged, [this](const QDate &to) {
        m_data.loadCategoriesStatistics(m_data.statistics.categoriesFrom, to);
        m_data.inCategoriesModel.update();
        m_data.outCategoriesModel.update();
    });

    connect(&m_data, &Data::categoriesStatisticsUpdated, [this]() {
        const auto *inRoot = m_data.inCategoriesModel.rootItem;
        const auto *outRoot = m_data.outCategoriesModel.rootItem;

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
    m_data.logModel.insertRow(0);
    showUnanchoredSum();
}

void cashbook::MainWindow::on_removeTransactionButton_clicked()
{
    QModelIndex index = ui->logTable->currentIndex();
    if(index.isValid()) {
        m_data.logModel.removeRow(index.row());
    }

    updateUnanchoredSum();
}

void cashbook::MainWindow::on_copyTransactionButton_clicked()
{
    m_data.logModel.copyTop();

    updateUnanchoredSum();
}

void cashbook::MainWindow::on_anchoreTransactionsButton_clicked()
{
    bool did = m_data.anchoreTransactions();
    did;

    hideUnanchoredSum();
}


//
// Wallets
//
void cashbook::MainWindow::on_addWalletSiblingButton_clicked() {
    addSiblingNode(*ui->walletsTree, m_data.walletsModel);
}
void cashbook::MainWindow::on_addWalletChildButton_clicked() {
    addChildNode(*ui->walletsTree, m_data.walletsModel);
}
void cashbook::MainWindow::on_removeWalletButton_clicked() {
    removeNode(*ui->walletsTree, m_data.walletsModel);
}
void cashbook::MainWindow::on_upWalletButton_clicked() {
    upNode(*ui->walletsTree, m_data.walletsModel);
}
void cashbook::MainWindow::on_downWalletButton_clicked()
{
    downNode(*ui->walletsTree, m_data.walletsModel);
}
void cashbook::MainWindow::on_outWalletButton_clicked() {
    outNode(*ui->walletsTree, m_data.walletsModel);
}
void cashbook::MainWindow::on_inWalletButton_clicked() {
    inNode(*ui->walletsTree, m_data.walletsModel);
}

//
// In categories
//
void cashbook::MainWindow::on_addInCategorySiblingButton_clicked() {
    addSiblingNode(*ui->inCategoriesTree, m_data.inCategoriesModel);
}
void cashbook::MainWindow::on_addInCategoryChildButton_clicked() {
    addChildNode(*ui->inCategoriesTree, m_data.inCategoriesModel);
}
void cashbook::MainWindow::on_removeInCategoryButton_clicked() {
    removeNode(*ui->inCategoriesTree, m_data.inCategoriesModel);
}
void cashbook::MainWindow::on_upInCategoryButton_clicked() {
    upNode(*ui->inCategoriesTree, m_data.inCategoriesModel);
}
void cashbook::MainWindow::on_downInCategoryButton_clicked() {
    downNode(*ui->inCategoriesTree, m_data.inCategoriesModel);
}
void cashbook::MainWindow::on_outInCategoryButton_clicked() {
    outNode(*ui->inCategoriesTree, m_data.inCategoriesModel);
}
void cashbook::MainWindow::on_inInCategoryButton_clicked() {
    inNode(*ui->inCategoriesTree, m_data.inCategoriesModel);
}

//
// Out categories
//
void cashbook::MainWindow::on_addOutCategorySiblingButton_clicked() {
    addSiblingNode(*ui->outCategoriesTree, m_data.outCategoriesModel);
}
void cashbook::MainWindow::on_addOutCategoryChildButton_clicked() {
    addChildNode(*ui->outCategoriesTree, m_data.outCategoriesModel);
}
void cashbook::MainWindow::on_removeOutCategoryButton_clicked() {
    removeNode(*ui->outCategoriesTree, m_data.outCategoriesModel);
}
void cashbook::MainWindow::on_upOutCategoryButton_clicked() {
    upNode(*ui->outCategoriesTree, m_data.outCategoriesModel);
}
void cashbook::MainWindow::on_downOutCategoryButton_clicked() {
    downNode(*ui->outCategoriesTree, m_data.outCategoriesModel);
}
void cashbook::MainWindow::on_outOutCategoryButton_clicked() {
    outNode(*ui->outCategoriesTree, m_data.outCategoriesModel);
}
void cashbook::MainWindow::on_inOutCategoryButton_clicked() {
    inNode(*ui->outCategoriesTree, m_data.outCategoriesModel);
}

//
// Users
//
void cashbook::MainWindow::on_addUserButton_clicked() {
    addListItem(m_data.ownersModel);
}
void cashbook::MainWindow::on_removeUserButton_clicked() {
    removeListItem(*ui->ownersList, m_data.ownersModel);
}
void cashbook::MainWindow::on_upUserButton_clicked() {
    upListItem(*ui->ownersList, m_data.ownersModel);
}
void cashbook::MainWindow::on_downUserButton_clicked() {
    downListItem(*ui->ownersList, m_data.ownersModel);
}

//
// Plans
//
void cashbook::MainWindow::on_addShortPlanButton_clicked() {
    addListItem(m_data.plans[PlanTerm::Short]);
}
void cashbook::MainWindow::on_removeShortPlanButton_clicked() {
    removeListItem(*ui->shortPlansTable, m_data.plans[PlanTerm::Short]);
}
void cashbook::MainWindow::on_upShortPlanButton_clicked() {
    upListItem(*ui->shortPlansTable, m_data.plans[PlanTerm::Short]);
}
void cashbook::MainWindow::on_downShortPlanButton_clicked() {
    downListItem(*ui->shortPlansTable, m_data.plans[PlanTerm::Short]);
}

void cashbook::MainWindow::on_addMiddlePlanButton_clicked() {
    addListItem(m_data.plans[PlanTerm::Middle]);
}
void cashbook::MainWindow::on_removeMiddlePlanButton_clicked() {
    removeListItem(*ui->middlePlansTable, m_data.plans[PlanTerm::Middle]);
}
void cashbook::MainWindow::on_upMiddlePlanButton_clicked() {
    upListItem(*ui->middlePlansTable, m_data.plans[PlanTerm::Middle]);
}
void cashbook::MainWindow::on_downMiddlePlanButton_clicked() {
    downListItem(*ui->middlePlansTable, m_data.plans[PlanTerm::Middle]);
}

void cashbook::MainWindow::on_addLongPlanButton_clicked() {
    addListItem(m_data.plans[PlanTerm::Long]);
}
void cashbook::MainWindow::on_removeLongPlanButton_clicked() {
    removeListItem(*ui->longPlansTable, m_data.plans[PlanTerm::Long]);
}
void cashbook::MainWindow::on_upLongPlanButton_clicked() {
    upListItem(*ui->longPlansTable, m_data.plans[PlanTerm::Long]);
}
void cashbook::MainWindow::on_downLongPlanButton_clicked() {
    downListItem(*ui->longPlansTable, m_data.plans[PlanTerm::Long]);
}

void cashbook::MainWindow::on_addActiveTaskButton_clicked() {
    addListItem(m_data.tasks[TaskStatus::Active]);
}
void cashbook::MainWindow::on_removeActiveTaskButton_clicked() {
    removeListItem(*ui->activeTasksTable, m_data.tasks[TaskStatus::Active]);
}
void cashbook::MainWindow::on_upActiveTaskButton_clicked() {
    upListItem(*ui->activeTasksTable, m_data.tasks[TaskStatus::Active]);
}
void cashbook::MainWindow::on_downActiveTaskButton_clicked() {
    downListItem(*ui->activeTasksTable, m_data.tasks[TaskStatus::Active]);
}

void cashbook::MainWindow::on_removeCompletedTaskButton_clicked() {
    removeListItem(*ui->completedTasksTable, m_data.tasks[TaskStatus::Completed]);
}

void cashbook::MainWindow::on_actionSave_triggered()
{
    /*if(!m_changed) {
        return;
    }*/

    saveFile();
    m_data.resetChanged();
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

    if(m_data.isChanged()) {
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
    showCategoryContextMenu(m_data.inCategoriesModel, ui->inCategoriesTree, ui->actionInStatement, point);
}

void cashbook::MainWindow::showOutCategoryMenu(const QPoint& point)
{
    showCategoryContextMenu(m_data.outCategoriesModel, ui->outCategoriesTree, ui->actionOutStatement, point);
}

void cashbook::MainWindow::showCategoryStatement(Transaction::Type::t type)
{
    bool in = type == Transaction::Type::In;

    QTreeView *categoryTree = in ? ui->inCategoriesTree : ui->outCategoriesTree;
    CategoriesModel &categoryModel = in ? m_data.inCategoriesModel : m_data.outCategoriesModel;

    QModelIndex index = categoryTree->currentIndex();
    Node<Category> *node = categoryModel.getItem(index);
    if(!node) {
        return;
    }

    FilteredLogModel *filterModel = new FilteredLogModel(ui->dateFrom->date(), ui->dateTo->date(), type, node, categoryTree);
    filterModel->setSourceModel(&m_data.logModel);
    QTableView *table = new QTableView();
    table->setWindowFlags(Qt::WindowCloseButtonHint | Qt::Tool);
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
    if(m_data.logModel.unanchored) {
        showUnanchoredSum();
    } else {
        hideUnanchoredSum();
    }
}

void cashbook::MainWindow::showUnanchoredSum()
{
    Money sum;
    for(int i = 0; i<m_data.logModel.unanchored; ++i) {
        sum += m_data.logModel.log[i].amount;
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

void cashbook::MainWindow::showLogMenu(const QPoint& point)
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

void cashbook::MainWindow::showShortPlansMenu(const QPoint& point)
{
    QPoint globalPos = ui->shortPlansTable->mapToGlobal(point);
    showPlansContextMenu(globalPos);
}

void cashbook::MainWindow::showMiddlePlansMenu(const QPoint& point)
{
    QPoint globalPos = ui->middlePlansTable->mapToGlobal(point);
    showPlansContextMenu(globalPos);
}

void cashbook::MainWindow::showLongPlansMenu(const QPoint& point)
{
    QPoint globalPos = ui->longPlansTable->mapToGlobal(point);
    showPlansContextMenu(globalPos);
}

void cashbook::MainWindow::showPlansContextMenu(const QPoint& point)
{
    QTableView *table = qobject_cast<QTableView *>( childAt(point)->parent() );
    if(!table) {
        return;
    }

    PlanTerm::t termFrom = (table == ui->shortPlansTable)  ? PlanTerm::Short :
                           (table == ui->middlePlansTable) ? PlanTerm::Middle :
                           (table == ui->longPlansTable)   ? PlanTerm::Long : PlanTerm::Count;

    if(termFrom == PlanTerm::Count) {
        return;
    }

    PlansModel &model = m_data.plans[termFrom];
    auto index = table->indexAt(table->mapFromGlobal(point));

    if(!index.isValid()) {
        return;
    }

    QMenu menu(table);

    /*
     * Idea is to use a pull of connections, which would contain one connection
     * per action. Each time connection should be different beacause of
     * different `from` and `to` tables. It means that it is simpler and safer
     * to disconnect connection each time it executes it's slot and reconnect
     * it next time with new lambda.
     */
    static std::shared_ptr<QMetaObject::Connection> connections[PlanTerm::Count];

    auto connectionAction = [&, this](PlanTerm::t term) {
        QAction *action = (term == PlanTerm::Short)  ? ui->actionMoveToShortPlans :
                          (term == PlanTerm::Middle) ? ui->actionMoveToMiddlePlans : ui->actionMoveToLongPlans;

        menu.addAction(action);

        connections[term] = std::make_shared<QMetaObject::Connection>();
        *connections[term] = connect(action, &QAction::triggered, [this, &model, &index, term](){
            for(auto term : PlanTerm::enumerate()) {
                if(connections[term]) QObject::disconnect(*connections[term]);
            }

            const Plan &plan = model.plans[index.row()];
            m_data.plans[term].insertPlan(plan);
            model.removeRow(index.row());
        });
    };

    if (termFrom != PlanTerm::Short) {
        connectionAction(PlanTerm::Short);
    }
    if (termFrom != PlanTerm::Middle) {
        connectionAction(PlanTerm::Middle);
    }
    if (termFrom != PlanTerm::Long) {
        connectionAction(PlanTerm::Long);
    }
    menu.exec(point);
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

    const Transaction &t = m_data.logModel.log[m_noteContextIndex.row()];

    QString note = getTextDialog(tr("Примечание"), tr("Примечание"), t.note, this);
    if(!note.isNull()) {
        m_data.logModel.updateNote(m_noteContextIndex.row(), note);
    }

    m_noteContextIndex = QModelIndex();
}
