#include "models.h"
#include "bookkeeping.h"

#include <QSet>
#include <functional>
#include <QComboBox>
#include <QCheckBox>
#include <QApplication>

namespace cashbook
{

const Qt::ItemFlags disabledFlag {0}; // Read-only and Disabled

//
// Common templates
//
namespace common {

template <class Model>
static inline Qt::ItemFlags flags(const Model *model, const QModelIndex &index)
{
    if (!index.isValid()) {
        return disabledFlag;
    }

    return Qt::ItemIsEditable | /*Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled |*/ model->QAbstractItemModel::flags(index);
}

static QVariant headerData(int section, Qt::Orientation orientation, int role)
{
    UNUSED(section, orientation, role);
    return QVariant();
}

namespace list {

template <class List>
static int rowCount(const List &list, const QModelIndex &parent)
{
    UNUSED(parent);
    return as<int>(list.size());
}

} // namespace list

namespace tree {

template<class Model, class DataType>
static Node<DataType> *getItem(const Model *model, const QModelIndex &index)
{
    if (index.isValid()) {
        Node<DataType> *item = as<Node<DataType>*>(index.internalPointer());
        if (item) {
            return item;
        }
    }
    return model->rootItem;
}

template<class Model>
static QModelIndex index(const Model *model, int row, int column, const QModelIndex &parent)
{
    if (!model->hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    auto *parentItem = model->getItem(parent);

    if(row >= as<int>(parentItem->childCount())) {
        return QModelIndex();
    }

    auto *childItem = parentItem->at(row);
    if (childItem) {
        return model->createIndex(row, column, childItem);
    } else {
        return QModelIndex();
    }
}

template<class Model, class DataType>
static bool insertRows(Model *model, std::function<DataType()> createData, int position, int rows, const QModelIndex &parent)
{
    Node<DataType> *parentItem = model->getItem(parent);

    model->beginInsertRows(parent, position, position + rows - 1);
    for(int i = 0; i<rows; ++i) {
        parentItem->addChildAt(createData(), position);
    }

    model->endInsertRows();

    return true;
}

template<class Model>
static QModelIndex parent(const Model *model, const QModelIndex &index)
{
    if (!index.isValid()) {
        return QModelIndex();
    }

    auto *childItem = model->getItem(index);
    auto *parentItem = childItem->parent;

    if (parentItem == model->rootItem) {
        return QModelIndex();
    }

    auto *parentParentItem = parentItem->parent;
    size_t row = 0;
    size_t n = parentParentItem->childCount();
    for( ; row < n; ++row) {
        if(parentParentItem->at(row) == parentItem) {
            break;
        }
    }

    return model->createIndex(as<int>(row), 0, parentItem);
}

template<class Model>
static bool removeRows(Model *model, int position, int rows, const QModelIndex &parent)
{
    auto *parentItem = model->getItem(parent);
    bool success = true;

    QStringList nodeIds;
    /*
     * Traverse each node to be deleted and dump all of their nested nodes as
     * nodes to be removed.
     */
    for(int i = position; i<position + rows; ++i) {
        auto node = parentItem->at(i);
        auto l = node->toList();
        for(const auto *n : l) {
            nodeIds << pathToString(n);
        }
    }
    emit model->nodesGonnaBeRemoved(nodeIds);

    model->beginRemoveRows(parent, position, position + rows - 1);
    for(int i = 0; i<rows; ++i) {
        parentItem->removeChildAt(position);
    }
    model->endRemoveRows();

    return success;
}

template<class Model>
static bool moveRow(Model *model, const QModelIndex &sourceParent, int sourceRow, const QModelIndex &destinationParent, int destinationChild)
{
    auto *srcParentItem = model->getItem(sourceParent);
    auto *dstParentItem = model->getItem(destinationParent);
    auto *srcChildItem = srcParentItem->at(sourceRow);

    bool down = sourceParent == destinationParent && sourceRow < destinationChild;

    model->beginMoveRows(sourceParent, sourceRow, sourceRow, destinationParent, destinationChild);
    srcChildItem->attachSelfAsChildAt(dstParentItem, destinationChild-as<int>(down));
    model->endMoveRows();

    return true;
}

template<class Model>
static int rowCount(const Model *model, const QModelIndex &parent)
{
    auto *parentItem = model->getItem(parent);
    return as<int>(parentItem->childCount());
}

template<class Model, class DataType>
static void constructor(Model *model)
{
    model->rootItem = new Node<DataType>;
}

template<class Model>
static void destructor(Model *model)
{
    delete model->rootItem;
}

} // namespace tree
} // namespace common

//
// Categories
//

CategoriesModel::CategoriesModel(CategoryMoneyMap &statistics, QObject *parent)
    : TreeModel(parent)
    , statistics(statistics)
{
    common::tree::constructor<CategoriesModel, Category>(this);
}

CategoriesModel::~CategoriesModel()
{
    common::tree::destructor(this);
}

int CategoriesModel::columnCount(const QModelIndex &parent) const
{
    UNUSED(parent);
    return CategoriesColumn::Count;
}

QVariant CategoriesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (role != Qt::DisplayRole && role != Qt::EditRole) {
        return QVariant();
    }

    Node<Category> *item = getItem(index);

    switch(index.column())
    {
        case CategoriesColumn::Name: return item->data;
        case CategoriesColumn::Regular:
            if(item->isLeaf()) {
                return item->data.regular;
            } else {
                return QVariant();
            }
        case CategoriesColumn::Statistics: {
            const Money &money = statistics[item];
            return money == 0 ? QVariant() : formatMoney(statistics[item]);
        }
    }

    return QVariant();
}

Qt::ItemFlags CategoriesModel::flags(const QModelIndex &index) const
{
    if(index.column() == CategoriesColumn::Statistics) {
        return disabledFlag;
    }

    return common::flags(this, index);
}

Node<Category> *CategoriesModel::getItem(const QModelIndex &index) const
{
    return common::tree::getItem<CategoriesModel, Category>(this, index);
}

QVariant CategoriesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch(section)
        {
            case CategoriesColumn::Name: return tr("Категория");
            case CategoriesColumn::Regular: return tr("Регулярная");
            case CategoriesColumn::Statistics: return tr("Оборот");
        }
    }

    return QVariant();
}

QModelIndex CategoriesModel::index(int row, int column, const QModelIndex &parent) const
{
    return common::tree::index(this, row, column, parent);
}

bool CategoriesModel::insertRows(int position, int rows, const QModelIndex &parent)
{
    auto createCategory = [](){
        return tr("Новая категория");
    };
    return common::tree::insertRows<CategoriesModel, Category>(this, createCategory, position, rows, parent);
}

QModelIndex CategoriesModel::parent(const QModelIndex &index) const
{
    return common::tree::parent(this, index);
}

bool CategoriesModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    return common::tree::removeRows(this, position, rows, parent);
}

bool CategoriesModel::moveRow(const QModelIndex &sourceParent, int sourceRow, const QModelIndex &destinationParent, int destinationChild)
{
    return common::tree::moveRow(this, sourceParent, sourceRow, destinationParent, destinationChild);
}

int CategoriesModel::rowCount(const QModelIndex &parent) const
{
    return common::tree::rowCount(this, parent);
}

bool CategoriesModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole) {
        return false;
    }

    Node<Category> *item = getItem(index);
    switch(index.column())
    {
        case CategoriesColumn::Name: item->data = value.toString(); break;
        case CategoriesColumn::Regular: item->data.regular = value.toBool(); break;
    }
    emit dataChanged(index, index);

    return true;
}

//
// Wallets
//

WalletsModel::WalletsModel(QObject *parent)
    : TreeModel(parent)
{
    common::tree::constructor<WalletsModel, Wallet>(this);
}

WalletsModel::~WalletsModel()
{
    common::tree::destructor(this);
}

int WalletsModel::columnCount(const QModelIndex &parent) const
{
    UNUSED(parent);
    return WalletColumn::Count;
}

QVariant WalletsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (role != Qt::DisplayRole && role != Qt::EditRole) {
        return QVariant();
    }

    Node<Wallet> *item = getItem(index);

    switch(index.column())
    {
        case WalletColumn::Name: return item->data.name;
        case WalletColumn::Amount:
            if(item->isLeaf()) {
                const Money &money = item->data.amount;

                if(role == Qt::DisplayRole) {
                    return formatMoney(money);
                } else { // Qt::EditRole
                    return as<double>(money);
                }
            }
    }

    return QVariant();
}

Qt::ItemFlags WalletsModel::flags(const QModelIndex &index) const
{
    return common::flags(this, index);
}

Node<Wallet> *WalletsModel::getItem(const QModelIndex &index) const
{
    return common::tree::getItem<WalletsModel, Wallet>(this, index);
}

QVariant WalletsModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch(section)
        {
            case WalletColumn::Name: return tr("Кошелек");
            case WalletColumn::Amount: return tr("Сумма");
        }
    }

    return QVariant();
}

QModelIndex WalletsModel::index(int row, int column, const QModelIndex &parent) const
{
    return common::tree::index(this, row, column, parent);
}

bool WalletsModel::insertRows(int position, int rows, const QModelIndex &parent)
{
    auto createCategory = [](){
        Wallet w;
        w.name = tr("Новый кошелек");
        return w;
    };
    return common::tree::insertRows<WalletsModel, Wallet>(this, createCategory, position, rows, parent);
}

QModelIndex WalletsModel::parent(const QModelIndex &index) const
{
    return common::tree::parent(this, index);
}

bool WalletsModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    return common::tree::removeRows(this, position, rows, parent);
}

bool WalletsModel::moveRow(const QModelIndex &sourceParent, int sourceRow, const QModelIndex &destinationParent, int destinationChild)
{
    return common::tree::moveRow(this, sourceParent, sourceRow, destinationParent, destinationChild);
}

int WalletsModel::rowCount(const QModelIndex &parent) const
{
    return common::tree::rowCount(this, parent);
}

bool WalletsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole) {
        return false;
    }

    Node<Wallet> *item = getItem(index);

    switch(index.column())
    {
        case WalletColumn::Name: item->data.name = value.toString(); break;
        case WalletColumn::Amount: item->data.amount = value.toDouble(); break;
    }

    emit dataChanged(index, index);

    return true;
}

//
// Owners
//

OwnersModel::OwnersModel(QObject *parent)
    : QAbstractListModel(parent)
{}

OwnersModel::~OwnersModel()
{}

QVariant OwnersModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (role != Qt::DisplayRole && role != Qt::EditRole) {
        return QVariant();
    }

    return owners[index.row()];
}

QVariant OwnersModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return common::headerData(section, orientation, role);
}

int OwnersModel::rowCount(const QModelIndex &parent) const
{
    return common::list::rowCount(owners, parent);
}

Qt::ItemFlags OwnersModel::flags(const QModelIndex &index) const
{
    return common::flags(this, index);
}

bool OwnersModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole) {
        return false;
    }

    if(index.column() != 0) {
        return false;
    }

    owners[index.row()] = value.toString();
    emit dataChanged(index, index);

    return true;
}

bool OwnersModel::insertRows(int position, int rows, const QModelIndex &parent)
{
    UNUSED(parent);
    beginInsertRows(parent, position, position + rows - 1);
    owners.insert(position, rows, tr("Новый пользователь"));
    endInsertRows();
    return true;
}

bool OwnersModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    UNUSED(parent);

    QStringList names;
    for(int i = position; i<position + rows; ++i) {
        names << owners[i];
    }
    emit nodesGonnaBeRemoved(names);

    beginRemoveRows(parent, position, position + rows - 1);
    owners.remove(position, rows);
    endRemoveRows();
    return true;
}

bool OwnersModel::moveRow(const QModelIndex &sourceParent, int sourceRow, const QModelIndex &destinationParent, int destinationChild)
{
    if(!beginMoveRows(sourceParent, sourceRow, sourceRow, destinationParent, destinationChild)) {
        return false;
    }

    bool down = sourceRow < destinationChild;
    int shift = as<int>(down);
    owners.move(sourceRow, destinationChild - shift);

    endMoveRows();

    return true;
}

//
// Log
//

LogModel::LogModel(Statistics &statistics, QObject *parent)
    : QAbstractTableModel(parent)
    , statistics(statistics)
{}

LogModel::~LogModel()
{}

bool LogModel::anchoreTransactions()
{
    if(!unanchored) {
        return false;
    }

    for(int i = unanchored-1; i>=0; --i) {
        Transaction &t = log[i];
        if(t.type != Transaction::Type::In && t.from.isValidPointer()) {
            Node<Wallet> *w = const_cast<Node<Wallet>*>(t.from.toPointer());
            if(w) {
                w->data.amount -= t.amount;
            }
            if(t.type == Transaction::Type::Out) {
                Month month(t.date);
                statistics.brief[month].common.spent += t.amount;

                if(!t.category.empty()) {
                    const auto &archNode = t.category.first();
                    if(archNode.isValidPointer()) {
                        const Node<Category> *category = archNode.toPointer();
                        if(category->data.regular) {
                            statistics.brief[month].regular.spent += t.amount;
                        }

                        statistics.outCategories.propagateMoney(category, t.amount);
                    }
                }
            }
        }

        if(t.type != Transaction::Type::Out && t.to.isValidPointer()) {
            Node<Wallet> *w = const_cast<Node<Wallet>*>(t.to.toPointer());
            if(w) {
                w->data.amount += t.amount;
            }
            if(t.type == Transaction::Type::In) {
                Month month(t.date);
                statistics.brief[month].common.received += t.amount;

                if(!t.category.empty()) {
                    const auto &archNode = t.category.first();
                    if(archNode.isValidPointer()) {
                        const Node<Category> *category = archNode.toPointer();
                        if(category->data.regular) {
                            statistics.brief[month].regular.received += t.amount;
                        }

                        statistics.inCategories.propagateMoney(category, t.amount);
                    }
                }
            }
        }
    }

    const int left = 0;
    const int top = 0;
    const int bottom = unanchored-1;
    const int right = LogColumn::Count-1;

    emit dataChanged(index(top, left), index(bottom, right));

    unanchored = 0;
    return true;
}

template <class T>
static QVariant archNodeData(const ArchNode<T> &archNode, int role)
{
    if(role == Qt::DisplayRole) {
        if(archNode.isValidPointer()) {
            const Node<T> *node = archNode.toPointer();
            return pathToShortString(node);
        } else {
            return archNode.toString();
        }
    } else { // Qt::EditRole
        return archNode;
    }
}

QVariant LogModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (role != Qt::DisplayRole && role != Qt::EditRole) {
        return QVariant();
    }

    const Transaction &t = log[index.row()];

    switch(index.column())
    {
        case LogColumn::Date: {
            if(role == Qt::DisplayRole) {
                return t.date.toString("dd.MM.yy");
            } else {
                return t.date;
            }
        }
        case LogColumn::Type: {
            if(role == Qt::DisplayRole) {
                return Transaction::Type::toString(t.type);
            } else {
                return as<int>(t.type);
            }
        } break;
        case LogColumn::Category: {
            if(t.category.empty()) {
                return QVariant();
            } else {
                const ArchNode<Category> &archNode = t.category.first();
                return archNodeData(archNode, role);
            }
        } break;
        case LogColumn::Money: {
            if(role == Qt::DisplayRole) {
                return formatMoney(t.amount);
            } else { // Qt::EditRole
                return as<double>(t.amount);
            }
        }
        case LogColumn::From: {
            return archNodeData(t.from, role);
        } break;
        case LogColumn::To: {
            return archNodeData(t.to, role);
        } break;
        case LogColumn::Note: return t.note;
    }

    return QVariant();
}

QVariant LogModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch(section)
        {
            case LogColumn::Date: return tr("Дата");
            case LogColumn::Type: return tr("Тип");
            case LogColumn::Category: return tr("Категория");
            case LogColumn::Money: return tr("Сумма");
            case LogColumn::From: return tr("Откуда");
            case LogColumn::To: return tr("Куда");
            case LogColumn::Note: return tr("Примечание");
        }
    }

    return QVariant();
}

int LogModel::rowCount(const QModelIndex &parent) const
{
    return common::list::rowCount(log, parent);
}

int LogModel::columnCount(const QModelIndex &parent) const
{
    UNUSED(parent);
    return LogColumn::Count;
}

static const QSet<LogColumn::t> archNodeColumns = {
    LogColumn::Category,
    LogColumn::From,
    LogColumn::To,
};

template <class DataType>
static Qt::ItemFlags archNodeFlags(const QAbstractItemModel *model, const QModelIndex &index, const ArchNode<DataType> &archNode)
{
    if(archNode.isValidPointer()) {
        return common::flags(model, index);
    } else {
        return model->QAbstractItemModel::flags(index);
    }
}

Qt::ItemFlags LogModel::flags(const QModelIndex &index) const
{
    if(index.row() > unanchored-1) {
        return disabledFlag;
    }

    if (!index.isValid()) {
        return 0;
    }

    auto column = as<LogColumn::t>(index.column());

    if(!archNodeColumns.contains(column)) {
        return common::flags(this, index);
    } else {
        const Transaction &t = log[index.row()];
        switch(column) {
            case LogColumn::Category: {
                if(t.type == Transaction::Type::Transfer) {
                    return disabledFlag;
                }

                if(!t.category.empty()) {
                    return archNodeFlags(this, index, t.category.first());
                } else {
                    return common::flags(this, index);
                }
            }

            case LogColumn::From: {
                if(t.type == Transaction::Type::In) {
                    return disabledFlag;
                }
                return archNodeFlags(this, index, t.from);
            }
            case LogColumn::To: {
                if(t.type == Transaction::Type::Out) {
                    return disabledFlag;
                }
                return archNodeFlags(this, index, t.to);
            }
        }
    }

    return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
}

bool LogModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole) {
        return false;
    }

    Transaction &t = log[index.row()];

    switch(index.column())
    {
        case LogColumn::Date: t.date = value.toDate(); break;
        case LogColumn::Type: {
            auto oldType = t.type;
            t.type = as<Transaction::Type::t>(value.toInt());
            if(t.type != oldType) {
                t.category.clear();
                t.from = t.to = as<const Node<Wallet>*>(nullptr);
                emit dataChanged(
                  createIndex(index.row(), LogColumn::Category),
                  createIndex(index.row(), LogColumn::Category),
                  {Qt::DisplayRole, Qt::EditRole}
                );
                emit dataChanged(
                  createIndex(index.row(), LogColumn::From),
                  createIndex(index.row(), LogColumn::To),
                  {Qt::DisplayRole, Qt::EditRole}
                );
            }
        } break;
        case LogColumn::Category: {
            t.category.clear();

            if(!value.isNull()) {
                t.category.push_back(value);
            }
        } break;
        case LogColumn::Money: t.amount = value.toDouble(); break;
        case LogColumn::From: t.from = value; break;
        case LogColumn::To: t.to = value; break;
        case LogColumn::Note: t.note = value.toString(); break;
    }

    emit dataChanged(index, index);

    return true;
}

bool LogModel::insertRows(int position, int rows, const QModelIndex &parent)
{
    UNUSED(parent);
    beginInsertRows(parent, position, position + rows - 1);
    Transaction t;
    t.date = QDate::currentDate();
    log.insert(position, rows, t);
    unanchored += rows;
    endInsertRows();
    return true;
}

bool LogModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    UNUSED(parent);
    beginRemoveRows(parent, position, position + rows - 1);
    log.remove(position, rows);
    unanchored -= 1;
    endRemoveRows();
    return true;
}

BoolDelegate::BoolDelegate(QObject *parent)
    : QItemDelegate(parent)
{}

BoolDelegate::~BoolDelegate()
{}

void BoolDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if(!index.data().isValid()) {
        return QItemDelegate::paint(painter, option, index);
    }

    QStyleOptionButton style;
    style.state = QStyle::State_Enabled;
    style.state |= index.data().toBool() ? QStyle::State_On : QStyle::State_Off;
    style.direction = QApplication::layoutDirection();
    style.rect = option.rect;

    QApplication::style()->drawControl(QStyle::CE_CheckBox, &style, painter);
    QApplication::style()->drawControl(QStyle::CE_CheckBox, &style, painter);
}

QWidget *BoolDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    UNUSED(option, index);

    if(!index.data().isValid()) {
        return nullptr;
    }

    return new QCheckBox(parent);
}

void BoolDelegate::setEditorData(QWidget *editor, const QModelIndex& index) const
{
    if (QCheckBox *box = qobject_cast<QCheckBox*>(editor)) {
        box->setChecked(index.data(Qt::EditRole).toBool());
    } else {
        QItemDelegate::setEditorData(editor, index);
    }
}

void BoolDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    if (QCheckBox* box = qobject_cast<QCheckBox*>(editor)) {
        model->setData(index, box->isChecked(), Qt::EditRole);
    } else {
        QItemDelegate::setModelData(editor, model, index);
    }
}


static const QSet<LogColumn::t> defaultColumns = {
    LogColumn::Date,
    LogColumn::Money,
    LogColumn::Note,
};

LogItemDelegate::LogItemDelegate(Data &data, QObject* parent)
    : QStyledItemDelegate(parent)
    , m_data(data)
{}

LogItemDelegate::~LogItemDelegate()
{}

QWidget* LogItemDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    auto column = as<LogColumn::t>(index.column());
    if (defaultColumns.contains(column)) {
        return QStyledItemDelegate::createEditor(parent, option, index);
    }

    const Transaction &record = m_data.log.log[index.row()];

    int col = index.column();

    switch(col) {
        case LogColumn::Type: {
            QComboBox* box = new QComboBox(parent);
            for(auto t : Transaction::Type::enumerate()) {
                box->addItem(Transaction::Type::toString(t), as<int>(t));
            }
            return box;
        } break;

        case LogColumn::Category: {
            if(record.type == Transaction::Type::Transfer) {
                return nullptr;
            }

            const CategoriesModel &categories =
                    record.type == Transaction::Type::In ?
                        m_data.inCategories :
                        m_data.outCategories;

            QComboBox* box = new QComboBox(parent);
            auto nodes = categories.rootItem->toList();
            for(const Node<Category> *n : nodes) {
                box->addItem(pathToString(n), ArchNode<Category>(n));
            }
            return box;

        } break;

        case LogColumn::From:
        case LogColumn::To: {
            if(col == LogColumn::From && record.type == Transaction::Type::In
            || col == LogColumn::To   && record.type == Transaction::Type::Out) {
                return nullptr;
            }

            QComboBox* box = new QComboBox(parent);
            const auto nodes = m_data.wallets.rootItem->getLeafs();
            for(const Node<Wallet> *n : nodes) {
                box->addItem(pathToString(n), ArchNode<Wallet>(n));
            }
            return box;

        } break;
    }

    // should not reach this code, but just to be sure
    return QStyledItemDelegate::createEditor(parent, option, index);
}

void LogItemDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    if (QComboBox* box = qobject_cast<QComboBox*>(editor)) {
        QVariant value = index.data(Qt::EditRole);
        int i = box->findData(value);
        if(i >= 0) {
            box->setCurrentIndex(i);
        }
    } else {
        QStyledItemDelegate::setEditorData(editor, index);
    }
}

void LogItemDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    if (QComboBox* box = qobject_cast<QComboBox*>(editor)) {
        model->setData(index, box->currentData(), Qt::EditRole);
    } else {
        QStyledItemDelegate::setModelData(editor, model, index);
    }
}

} // namespace cashbook
