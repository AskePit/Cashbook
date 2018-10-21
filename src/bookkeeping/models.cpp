#include "models.h"
#include "bookkeeping.h"
#include "widgets.h"

#include <QSet>
#include <functional>
#include <stack>
#include <QComboBox>
#include <QCheckBox>
#include <QDateEdit>
#include <QApplication>
#include <QPainter>
#include <QDebug>

#include <unordered_map>
#include <askelib_qt/askelib/std/opt.h>

namespace cashbook
{

//
// Common templates
//
namespace common {

template <class Model>
static inline Qt::ItemFlags flags(const Model *model, const QModelIndex &index)
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    return Qt::ItemIsEditable | /*Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled |*/ model->QAbstractItemModel::flags(index);
}

static QVariant headerData(int section, Qt::Orientation orientation, int role)
{
    UNUSED(section);
    UNUSED(orientation);
    UNUSED(role);
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

template<class Model, class DataType>
QModelIndex itemIndex(const Model *model, const Node<DataType> *item)
{
    std::stack<const Node<DataType> *> path;


    while(item != model->rootItem) {
        path.push(item);
        item = item->parent;
    }

    QModelIndex index;
    while(!path.empty()) {
        for(int i = 0; i<model->rowCount(index); ++i) {
            QModelIndex tmp = model->index(i, 0, index);
            if(tmp.internalPointer() == path.top()) {
                index = tmp;
                path.pop();
                break;
            }
        }
    }

    return index;
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

namespace colors {
    // Foregrounds
    static const QColor normal {Qt::black};
    static const QColor inactive {120, 120, 120};
    static const QColor readonly {100, 100, 100};
    static const QColor profit {34, 120, 53};
    static const QColor loses {220, 47, 67};

    // Backgrounds
    static const QColor incorrect {250, 77, 97};
    static const QColor noField {252, 252, 252};
}


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

    if(index.column() == CategoriesColumn::Statistics && role == Qt::ForegroundRole) {
        return colors::readonly;
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
        return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    }

    return common::flags(this, index);
}

Node<Category> *CategoriesModel::getItem(const QModelIndex &index) const
{
    return common::tree::getItem<CategoriesModel, Category>(this, index);
}

QModelIndex CategoriesModel::itemIndex(const Node<Category> *item) const
{
    return common::tree::itemIndex<CategoriesModel, Category>(this, item);
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
    return changeFilter( common::tree::insertRows<CategoriesModel, Category>(this, createCategory, position, rows, parent) );
}

QModelIndex CategoriesModel::parent(const QModelIndex &index) const
{
    return common::tree::parent(this, index);
}

bool CategoriesModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    return changeFilter( common::tree::removeRows(this, position, rows, parent) );
}

bool CategoriesModel::moveRow(const QModelIndex &sourceParent, int sourceRow, const QModelIndex &destinationParent, int destinationChild)
{
    return changeFilter( common::tree::moveRow(this, sourceParent, sourceRow, destinationParent, destinationChild));
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
    setChanged();

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

QModelIndex WalletsModel::itemIndex(const Node<Wallet> *item) const
{
    return common::tree::itemIndex<WalletsModel, Wallet>(this, item);
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
    return changeFilter( common::tree::insertRows<WalletsModel, Wallet>(this, createCategory, position, rows, parent) );
}

QModelIndex WalletsModel::parent(const QModelIndex &index) const
{
    return common::tree::parent(this, index);
}

bool WalletsModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    return changeFilter( common::tree::removeRows(this, position, rows, parent) );
}

bool WalletsModel::moveRow(const QModelIndex &sourceParent, int sourceRow, const QModelIndex &destinationParent, int destinationChild)
{
    return changeFilter( common::tree::moveRow(this, sourceParent, sourceRow, destinationParent, destinationChild) );
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
    setChanged();

    return true;
}

//
// Owners
//

OwnersModel::OwnersModel(QObject *parent)
    : TableModel(parent)
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

int OwnersModel::columnCount(const QModelIndex &parent) const
{
    UNUSED(parent);
    return WalletColumn::Count;
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
    setChanged();

    return true;
}

bool OwnersModel::insertRows(int position, int rows, const QModelIndex &parent)
{
    UNUSED(parent);
    beginInsertRows(parent, position, position + rows - 1);
    owners.insert(position, rows, tr("Новый пользователь"));
    endInsertRows();
    setChanged();
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
    setChanged();
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
    setChanged();

    return true;
}

//
// Log
//

LogModel::LogModel(Statistics &statistics, QObject *parent)
    : TableModel(parent)
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

                const auto &archNode = t.category;
                if(archNode.isValidPointer()) {
                    const Node<Category> *category = archNode.toPointer();
                    if(category && category->data.regular) {
                        statistics.brief[month].regular.spent += t.amount;
                    }

                    statistics.outCategories.propagateMoney(category, t.amount);
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

                const auto &archNode = t.category;
                if(archNode.isValidPointer()) {
                    const Node<Category> *category = archNode.toPointer();
                    if(category && category->data.regular) {
                        statistics.brief[month].regular.received += t.amount;
                    }

                    statistics.inCategories.propagateMoney(category, t.amount);
                }
            }
        }
    }

    const int left = 0;
    const int top = 0;
    const int bottom = unanchored-1;
    const int right = LogColumn::Count-1;

    emit dataChanged(index(top, left), index(bottom, right));
    setChanged();

    unanchored = 0;
    return true;
}

bool LogModel::copyTop()
{
    if(unanchored == 0 || log.empty()) {
        return false;
    }

    if(!insertRow(0)) {
        return false;
    }

    log[0] = log[1];
    emit dataChanged(index(0, LogColumn::Start), index(0, LogColumn::Count), {Qt::DisplayRole});
    setChanged();
    return true;
}

template <class T>
static bool isErrorNode(const ArchNode<T> &node)
{
    if(node.isValidPointer()) {
        if(node.toPointer() == nullptr) {
            return true;
        }
    } else {
        if(node.toString().isEmpty()) {
            return true;
        }
    }

    return false;
}

bool LogModel::canAnchore() const
{
    for(int i = 0; i<unanchored; ++i) {
        const Transaction &t = log[i];

        if(t.type == Transaction::Type::In || t.type == Transaction::Type::Out) {
           if(isErrorNode(t.category)) {
               return false;
           }
        }

        if(t.type != Transaction::Type::Out) {
            if(isErrorNode(t.to)) {
                return false;
            }
        }

        if(t.type != Transaction::Type::In) {
            if(isErrorNode(t.from)) {
                return false;
            }
        }
    }

    return true;
}

void LogModel::appendTransactions(const std::vector<Transaction> &transactions)
{
    if(transactions.empty()) {
        return;
    }
    QDate date = transactions.front().date;
    changedMonths.insert(Month(date));
    setChanged();

    for(const auto &t : transactions) {
        log.push_front(t);
    }

    unanchored += static_cast<int>(transactions.size());

    emit dataChanged(index(0, LogColumn::Start), index(static_cast<int>(transactions.size()-1), LogColumn::Count), {Qt::DisplayRole});
}

template <class T>
static QVariant archNodeData(const ArchNode<T> &archNode, int role)
{
    if(role == Qt::DisplayRole) {
        return archNodeToShortString(archNode);
    } else if(role == Qt::EditRole) {
        return archNode;
    } else if(role == Qt::BackgroundRole) {
        bool incorrect = false;
        if(archNode.isValidPointer()) {
            const Node<T> *node = archNode.toPointer();
            incorrect = !node;
        } else {
            incorrect = archNode.toString().isEmpty();
        }

        return incorrect ? colors::incorrect : QVariant();
    }

    return QVariant();
}

QVariant LogModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if(role == Qt::ForegroundRole) {
        if(index.row() >= unanchored) {
            return colors::inactive;
        } else {
            return colors::normal;
        }
    }

    if (role != Qt::DisplayRole && role != Qt::EditRole && role != Qt::BackgroundRole) {
        return QVariant();
    }

    const Transaction &t = log[index.row()];
    int column = index.column();

    if(column == LogColumn::Date) {
        if(role == Qt::DisplayRole) {
            return t.date.toString("dd.MM.yy");
        } else if(role == Qt::EditRole) {
            return t.date;
        }

    } else if(column == LogColumn::Type) {
        if(role == Qt::DisplayRole) {
            return Transaction::Type::toString(t.type);
        } else if(role == Qt::EditRole) {
            return QVariant::fromValue<Transaction::Type::t>(t.type);
        }

    } else if(column == LogColumn::Category) {
        if(t.type != Transaction::Type::Transfer) {
            return archNodeData(t.category, role);
        } else {
            if(role == Qt::BackgroundRole) {
                return colors::noField;
            }
        }

    } else if(column == LogColumn::Money) {
        if(role == Qt::DisplayRole) {
            return formatMoney(t.amount);
        } else if(role == Qt::EditRole) {
            return as<double>(t.amount);
        }

    } else if(column == LogColumn::From) {
        if(t.type != Transaction::Type::In) {
            return archNodeData(t.from, role);
        } else {
            if(role == Qt::BackgroundRole) {
                return colors::noField;
            }
        }

    } else if(column == LogColumn::To) {
        if(t.type != Transaction::Type::Out) {
            return archNodeData(t.to, role);
        } else {
            if(role == Qt::BackgroundRole) {
                return colors::noField;
            }
        }

    } else if(column == LogColumn::Note) {
        return t.note;
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
        return Qt::NoItemFlags;
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
                    return Qt::NoItemFlags;
                }

                return archNodeFlags(this, index, t.category);
            }

            case LogColumn::From: {
                if(t.type == Transaction::Type::In) {
                    return Qt::NoItemFlags;
                }
                return archNodeFlags(this, index, t.from);
            }
            case LogColumn::To: {
                if(t.type == Transaction::Type::Out) {
                    return Qt::NoItemFlags;
                }
                return archNodeFlags(this, index, t.to);
            }
            default: break;
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
            t.type = value.value<Transaction::Type::t>();
            if(t.type != oldType) {
                t.category.setNull();
                t.from.setNull();
                t.to.setNull();
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
            if(!value.isNull()) {
                t.category = value;
            }
        } break;
        case LogColumn::Money: t.amount = value.toDouble(); break;
        case LogColumn::From: t.from = value; break;
        case LogColumn::To: t.to = value; break;
        case LogColumn::Note: t.note = value.toString(); break;
    }

    emit dataChanged(index, index);
    changedMonths.insert(Month(t.date));
    setChanged();

    return true;
}

bool LogModel::insertRows(int position, int rows, const QModelIndex &parent)
{
    UNUSED(parent);
    beginInsertRows(parent, position, position + rows - 1);
    Transaction t;
    t.date = today;
    log.insert(position, rows, t);
    unanchored += rows;
    changedMonths.insert(Month(t.date));
    endInsertRows();

    setChanged();
    return true;
}

bool LogModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    // always assume that rows == 1

    UNUSED(parent);
    beginRemoveRows(parent, position, position + rows - 1);

    changedMonths.insert(Month(log[position].date));
    log.remove(position, rows);
    unanchored -= 1;

    endRemoveRows();
    setChanged();
    return true;
}

void LogModel::updateNote(int row, const QString &note)
{
    Transaction &t = log[row];
    t.note = note;

    changedMonths.insert(Month(t.date));
    QModelIndex i = index(row, LogColumn::Note);
    emit dataChanged(i, i, {Qt::DisplayRole});
    setChanged();
}

static bool isNodeBelongsTo(const Node<Category> *node, const Node<Category> *parent) {
    while(node) {
        if(node == parent) {
            return true;
        }
        node = node->parent;
    }

    return false;
}

void LogModel::updateTask(Task &task) const
{
    task.spent = 0;
    task.rest = 0;

    if(log.empty()) {
        task.rest = task.amount;
        return;
    }

    const QDate &logBegin = log.at(log.size()-1).date;
    const QDate &logEnd = log.at(0).date;

    if(task.to < logBegin || task.from > logEnd) {
        task.rest = task.amount;
        return;
    }

    size_t i = 0;
    while(i < log.size()) {
        const Transaction &t = log[i++];

        if(task.type != t.type) {
            continue;
        }

        if(t.date > task.to) {
            continue;
        }

        if(t.date < task.from) {
            break;
        }

        const ArchNode<Category> &trArchNode = t.category;
        const ArchNode<Category> &taskArchNode = task.category;
        if(trArchNode.isValidPointer() && taskArchNode.isValidPointer()) {
            const Node<Category> *trNode = trArchNode.toPointer();
            const Node<Category> *taskNode = taskArchNode.toPointer();
            if(isNodeBelongsTo(trNode, taskNode)) {
                task.spent += t.amount;
            }
        }
    }

    task.rest = task.amount - task.spent;
}

struct Balance
{
    int sum {0}; // wallet's balance
    std::vector<ArchNode<Wallet>> ins; // pointers to wallets that received money from this wallet
};

void LogModel::normalizeData()
{
    if(log.empty()) {
        return;
    }

    auto history = log.getStdVector(); // get log (from now till eldest record) in reverse manner (from eldest record till now)

    QDate date = history.front().date;
    std::vector<Transaction> res; // our result
    std::vector<std::reference_wrapper<const Transaction>> day; // temp container for every day

    res.reserve(history.size());

    const auto processDay = [this, &day, &date, &res]() {
        std::vector<Transaction> normalizedDay;
        normalizedDay.reserve(day.size());

        // 1. Merge same Categories for same Transaction Type that has same Note (or have no any)
        for(int i = 0; i<day.size(); ++i) {
            Transaction iRec = day[i];

            for(int j = i+1; j<day.size(); ++j) {
                const Transaction& jRec = day[j];

                bool merge = iRec.type == jRec.type
                          && iRec.category == jRec.category
                          && iRec.from == jRec.from
                          && iRec.to == jRec.to
                          && iRec.note == jRec.note;

                if(merge) {
                    qDebug() << QString("Merge: %1, \'%2\' %3 merges with %4").arg(
                        iRec.date.toString(),
                        (iRec.category.toPointer() ? iRec.category.toPointer()->data : "TRANSFER"),
                        formatMoney(iRec.amount, false),
                        formatMoney(jRec.amount, false)
                    ).toStdString().c_str();

                    iRec.amount += jRec.amount;
                    std::swap(*std::next(day.begin(), j), day.back());
                    day.pop_back();
                    changedMonths.insert(Month(iRec.date));
                    setChanged();
                    --j;
                }
            }

            normalizedDay.emplace_back(std::move(iRec));
        }

        // 2. Merge Transer Transactions like: from A->B->C to A->C for same amounts of money
        {
            // collect all transfer transactions for a day
            std::vector<std::reference_wrapper<const Transaction>> transfers;
            for(const auto& t : normalizedDay) {
                if(t.type == Transaction::Type::Transfer) {
                    transfers.push_back(t);
                }
            }

            // calculate wallets' balances for all transactions
            std::unordered_map<ArchNode<Wallet>, Balance> balances;

            for(const Transaction& t : transfers) {
                balances[t.from].sum -= t.amount.as_cents();
                balances[t.to].sum += t.amount.as_cents();
                balances[t.from].ins.push_back(t.to);
            }

            // container for optimized transfer trnsactions
            std::vector<Transaction> newTransfers;

            // if a pair of wallets refers to only one transaction, remove it from balances and just move to a result container unchanged
            {
                auto it = transfers.begin();
                while(it != transfers.end()) {
                    const Transaction& t = *it;
                    if(balances[t.from].sum + balances[t.to].sum == 0) {
                        newTransfers.push_back(t);
                        balances.erase(t.from);
                        balances.erase(t.to);

                        std::swap(*it, transfers.back());
                        transfers.pop_back();
                    } else {
                        ++it;
                    }
                }
            }

            // if remained transfers have a picture of graph with paths no longer than 1, discard it as nonoptimizable
            if(!balances.empty())
            {
                // we avoided recursive check because it's enough to check 2nd nesting level to deside that graph needs normalization
                bool allShortPaths = true;
                for(const auto& p : balances) {
                    const Balance& balance = p.second;
                    for(const ArchNode<Wallet>& in : balance.ins) {
                        if(!balances[in].ins.empty()) {
                            allShortPaths = false;
                            break;
                        }
                    }
                }

                if(allShortPaths) {
                    goto skipTransferNormalization;
                }
            }

            // remove all zeros in balance
            auto it = balances.begin();
            while(it != balances.end()) {
                if(it->second.sum == 0) {
                    it = balances.erase(it);
                } else {
                    ++it;
                }
            }

            // no balances - no optimization
            if(balances.empty()) {
                goto skipTransferNormalization; // one of rare places, where it is reasonable, i think
            }

            changedMonths.insert(Month(date));
            setChanged();

            // spread balance records between outs and ins depending on sign of wallets' balance
            std::vector<std::pair<ArchNode<Wallet>, int>> outs;
            std::vector<std::pair<ArchNode<Wallet>, int>> ins;

            for(const auto acc : balances) {
                if(acc.second.sum > 0) {
                    ins.emplace_back(std::make_pair(acc.first, acc.second.sum));
                } else {
                    outs.emplace_back(std::make_pair(acc.first, -acc.second.sum));
                }
            }

            // sort ins and outs
            const auto less = [](const std::pair<ArchNode<Wallet>, int>& p1, const std::pair<ArchNode<Wallet>, int>& p2) -> bool {
                return p1.second < p2.second;
            };

            std::sort(outs.begin(), outs.end(), less);
            std::sort(ins.begin(), ins.end(), less);

            // resolve transport task
            std::vector<std::vector<aske::opt<int>>> tt(outs.size());
            for(auto& vec : tt) {
                vec.resize(ins.size());
            }

            for(int o = 0; o<tt.size(); ++o) {
                auto& oRow = tt[o];
                for(int i = 0; i<oRow.size(); ++i) {
                    auto& cell = oRow[i];

                    if(cell) {
                        continue;
                    }

                    auto& out = outs[o].second;
                    auto& in = ins[i].second;

                    if(out < in) {
                        cell = out;
                        in -= out;
                        out = 0;
                        for(auto& c : oRow) {
                            if(!c) {
                                c = 0;
                            }
                        }
                    } else {
                        cell = in;
                        out -= in;
                        in = 0;
                        for(int n = 0; n<outs.size(); ++n) {
                            auto& c = tt[n][i];
                            if(!c) {
                                c = 0;
                            }
                        }
                    }
                }
            }

            // create new transfer transactions from result of transport task in `tt`
            for(int o = 0; o<tt.size(); ++o) {
                auto& oRow = tt[o];
                for(int i = 0; i<oRow.size(); ++i) {
                    auto& cell = oRow[i];

                    if(!cell || cell.get() == 0) {
                        continue;
                    }

                    auto& out = outs[o].first;
                    auto& in = ins[i].first;

                    Transaction t;
                    t.type = Transaction::Type::Transfer;
                    t.date = date;
                    t.from = out;
                    t.to = in;
                    t.amount = Money(static_cast<intmax_t>(cell.get()));

                    newTransfers.push_back(t);
                }
            }

            // remove all transfers from normalizedDay
            qDebug() << QString("Before transfer optimization for '%1' day:").arg(date.toString()).toStdString().c_str();
            {
                auto it = normalizedDay.begin();
                while(it != normalizedDay.end()) {
                    if(it->type == Transaction::Type::Transfer) {
                        qDebug() << '\t' << archNodeToShortString(it->from).toStdString().c_str() << "->" << archNodeToShortString(it->to).toStdString().c_str() << formatMoney(it->amount, false).toStdString().c_str();
                        std::swap(*it, normalizedDay.back());
                        normalizedDay.pop_back();
                    } else {
                        ++it;
                    }
                }
            }

            // add new transfers to normalizedDay
            qDebug() << "After transfer optimization:";
            for(const auto& t : newTransfers) {
                qDebug() << '\t' << archNodeToShortString(t.from).toStdString().c_str() << "->" << archNodeToShortString(t.to).toStdString().c_str() << formatMoney(t.amount, false).toStdString().c_str();
            }
            normalizedDay.insert(normalizedDay.end(), newTransfers.begin(), newTransfers.end());
            qDebug() << "";
        }

        skipTransferNormalization:

        // Add normalized day to res
        res.insert(res.end(), normalizedDay.begin(), normalizedDay.end());
    };

    for(const auto &record : history)
    {
        if(record.date != date) {
            // process previous day
            processDay();
            date = record.date;
            day.clear();
        }

        day.push_back(record);
    }
    processDay(); // process last day

    log.setStdVector(res);

    beginResetModel();
    endResetModel();
}

FilteredLogModel::FilteredLogModel(const QDate &from, const QDate &to, Transaction::Type::t type, const Node<Category> *category, QObject *parent)
    : QSortFilterProxyModel(parent)
    , m_from(from)
    , m_to(to)
    , m_type(type)
    , m_category(category)
{}

bool FilteredLogModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    UNUSED(sourceParent);

    LogModel *model = qobject_cast<LogModel*>(sourceModel());

    const Transaction &t = model->log[sourceRow];
    if(t.type != m_type) {
        return false;
    }

    if(t.date < m_from || t.date > m_to) {
        return false;
    }

    if(!t.category.isValidPointer()) {
        return false;
    }

    const Node<Category> *node = t.category.toPointer();

    while(node) {
        if(node == m_category) {
            return true;
        }

        node = node->parent;
    }

    return false;
}

//
// Plans
//
PlansModel::PlansModel(QObject *parent)
    : TableModel(parent)
{}

PlansModel::~PlansModel()
{}

QVariant PlansModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if(role == Qt::ForegroundRole) {
        return colors::normal;
    }

    if (role != Qt::DisplayRole && role != Qt::EditRole) {
        return QVariant();
    }

    const Plan &item = plans[index.row()];

    switch(index.column())
    {
        case PlansColumn::Name: return item.name;
        case PlansColumn::Type: {
            if(role == Qt::DisplayRole) {
                return Transaction::Type::toString(item.type);
            } else {
                return QVariant::fromValue<Transaction::Type::t>(item.type);
            }
        } break;
        case PlansColumn::Category: {
            return archNodeData(item.category, role);
        } break;
        case PlansColumn::Money: {
            if(role == Qt::DisplayRole) {
                return formatMoney(item.amount);
            } else { // Qt::EditRole
                return as<double>(item.amount);
            }
        } break;
    }

    return QVariant();
}

QVariant PlansModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch(section)
        {
            case PlansColumn::Name: return tr("Название");
            case PlansColumn::Type: return tr("Тип");
            case PlansColumn::Category: return tr("Категория");
            case PlansColumn::Money: return tr("Сумма");
        }
    }

    return QVariant();
}

int PlansModel::rowCount(const QModelIndex &parent) const
{
    return common::list::rowCount(plans, parent);
}

int PlansModel::columnCount(const QModelIndex &parent) const
{
    UNUSED(parent);
    return PlansColumn::Count;
}

static const QSet<PlansColumn::t> planArchNodeColumns = {
    PlansColumn::Category,
};

Qt::ItemFlags PlansModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return 0;
    }

    auto column = as<PlansColumn::t>(index.column());

    if(!planArchNodeColumns.contains(column)) {
        return common::flags(this, index);
    } else {
        const Plan &item = plans[index.row()];
        switch(column) {
            case PlansColumn::Category: {
                if(item.type == Transaction::Type::Transfer) {
                    return Qt::NoItemFlags;
                }

                return archNodeFlags(this, index, item.category);
            }
            default: break;
        }
    }

    return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
}

bool PlansModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole) {
        return false;
    }

    Plan &item = plans[index.row()];

    switch(index.column())
    {
        case PlansColumn::Name: item.name = value.toString(); break;
        case PlansColumn::Type: {
            auto oldType = item.type;
            item.type = value.value<Transaction::Type::t>();
            if(item.type != oldType) {
                item.category.setNull();
                emit dataChanged(
                  createIndex(index.row(), PlansColumn::Category),
                  createIndex(index.row(), PlansColumn::Category),
                  {Qt::DisplayRole, Qt::EditRole}
                );
            }
        } break;
        case PlansColumn::Category: {
            if(!value.isNull()) {
                item.category = value;
            }
        } break;
        case PlansColumn::Money: item.amount = value.toDouble(); break;
    }

    emit dataChanged(index, index);
    setChanged();

    return true;
}

bool PlansModel::insertRows(int position, int rows, const QModelIndex &parent)
{
    // always assume that rows == 1

    UNUSED(parent);
    beginInsertRows(parent, position, position + rows - 1);
    Plan item;
    plans.insert(position, item);
    endInsertRows();
    setChanged();
    return true;
}

bool PlansModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    // always assume that rows == 1

    UNUSED(parent);
    beginRemoveRows(parent, position, position + rows - 1);
    plans.removeAt(position);
    endRemoveRows();
    setChanged();
    return true;
}

bool PlansModel::moveRow(const QModelIndex &sourceParent, int sourceRow, const QModelIndex &destinationParent, int destinationChild)
{
    if(!beginMoveRows(sourceParent, sourceRow, sourceRow, destinationParent, destinationChild)) {
        return false;
    }

    bool down = sourceRow < destinationChild;
    int shift = as<int>(down);
    plans.move(sourceRow, destinationChild - shift);

    endMoveRows();
    setChanged();

    return true;
}

void PlansModel::insertPlan(const Plan &plan)
{
    int size = plans.size();

    insertRow(size);
    plans[size] = plan;

    emit dataChanged(index(size, PlansColumn::Count), index(size, PlansColumn::Count));
    setChanged();
}

//
// Tasks
//
TasksModel::TasksModel(const LogModel &log, QObject *parent)
    : TableModel(parent)
    , m_log(log)
{}

TasksModel::~TasksModel()
{}

QVariant TasksModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    int column = index.column();

    if(role == Qt::ForegroundRole) {
        if(column == TasksColumn::Spent || column == TasksColumn::Rest) {
            return colors::readonly;
        } else {
            return colors::normal;
        }
    }

    if (role != Qt::DisplayRole && role != Qt::EditRole) {
        return QVariant();
    }

    const Task &item = tasks[index.row()];

    switch(column)
    {
        case TasksColumn::Type: {
            if(role == Qt::DisplayRole) {
                return Transaction::Type::toString(item.type);
            } else {
                return QVariant::fromValue<Transaction::Type::t>(item.type);
            }
        } break;
        case TasksColumn::Category: {
            return archNodeData(item.category, role);
        } break;
        case TasksColumn::From: {
            if(role == Qt::DisplayRole) {
                return item.from.toString("dd.MM.yy");
            } else {
                return item.from;
            }
        } break;
        case TasksColumn::To: {
            if(role == Qt::DisplayRole) {
                return item.to.toString("dd.MM.yy");
            } else {
                return item.to;
            }
        } break;
        case TasksColumn::Money: {
            if(role == Qt::DisplayRole) {
                return formatMoney(item.amount);
            } else { // Qt::EditRole
                return as<double>(item.amount);
            }
        } break;
        case TasksColumn::Spent: {
            if(role == Qt::DisplayRole) {
                return formatMoney(item.spent);
            } else { // Qt::EditRole
                return as<double>(item.spent);
            }
        } break;
        case TasksColumn::Rest: {
            if(role == Qt::DisplayRole) {
                return formatMoney(item.rest);
            } else { // Qt::EditRole
                return as<double>(item.rest);
            }
        } break;
    }

    return QVariant();
}

QVariant TasksModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch(section)
        {
            case TasksColumn::Type: return tr("Тип");
            case TasksColumn::Category: return tr("Категория");
            case TasksColumn::From: return tr("Начало");
            case TasksColumn::To: return tr("Завершение");
            case TasksColumn::Money: return tr("Заложенная сумма");
            case TasksColumn::Spent: return tr("Потрачено");
            case TasksColumn::Rest: return tr("Осталось");
        }
    }

    return QVariant();
}

int TasksModel::rowCount(const QModelIndex &parent) const
{
    return common::list::rowCount(tasks, parent);
}

int TasksModel::columnCount(const QModelIndex &parent) const
{
    UNUSED(parent);
    return TasksColumn::Count;
}

static const QSet<TasksColumn::t> taskNodeColumns = {
    TasksColumn::Category,
};

Qt::ItemFlags TasksModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return 0;
    }

    auto column = as<TasksColumn::t>(index.column());

    if(column == TasksColumn::Spent || column == TasksColumn::Rest) {
        return Qt::NoItemFlags;
    }

    if(!taskNodeColumns.contains(column)) {
        return common::flags(this, index);
    } else {
        const Task &item = tasks[index.row()];
        switch(column) {
            case TasksColumn::Category: {
                if(item.type == Transaction::Type::Transfer) {
                    return Qt::NoItemFlags;
                }

                return archNodeFlags(this, index, item.category);
            }
            default: break;
        }
    }

    return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
}

bool TasksModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole) {
        return false;
    }

    Task &task = tasks[index.row()];
    Task origin(task);

    switch(index.column())
    {
        case TasksColumn::Type: {
            auto oldType = task.type;
            task.type = value.value<Transaction::Type::t>();
            if(task.type != oldType) {
                task.category.setNull();
                emit dataChanged(
                  createIndex(index.row(), TasksColumn::Category),
                  createIndex(index.row(), TasksColumn::Category),
                  {Qt::DisplayRole, Qt::EditRole}
                );
            }
        } break;
        case TasksColumn::Category: {
            if(!value.isNull()) {
                task.category = value;
            }
        } break;
        case TasksColumn::From: task.from = value.toDate(); break;
        case TasksColumn::To: task.to = value.toDate(); break;
        case TasksColumn::Money: task.amount = value.toDouble(); break;
        case TasksColumn::Spent: task.spent = value.toDouble(); break;
        case TasksColumn::Rest: task.rest = value.toDouble(); break;
    }

    emit dataChanged(index, index);
    setChanged();

    if(origin.category != task.category
    || origin.from != task.from
    || origin.to != task.to
    || origin.amount != task.amount) {
        m_log.updateTask(task);
        emit dataChanged(
          createIndex(index.row(), TasksColumn::Spent),
          createIndex(index.row(), TasksColumn::Rest),
          {Qt::DisplayRole, Qt::EditRole}
        );
    }

    return true;
}

bool TasksModel::insertRows(int position, int rows, const QModelIndex &parent)
{
    // always assume that rows == 1

    UNUSED(parent);
    beginInsertRows(parent, position, position + rows - 1);
    Task item;
    item.from = today;
    item.to = today;
    tasks.insert(position, item);
    endInsertRows();
    setChanged();
    return true;
}

bool TasksModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    // always assume that rows == 1

    UNUSED(parent);
    beginRemoveRows(parent, position, position + rows - 1);
    tasks.removeAt(position);
    endRemoveRows();
    setChanged();
    return true;
}

bool TasksModel::moveRow(const QModelIndex &sourceParent, int sourceRow, const QModelIndex &destinationParent, int destinationChild)
{
    if(!beginMoveRows(sourceParent, sourceRow, sourceRow, destinationParent, destinationChild)) {
        return false;
    }

    bool down = sourceRow < destinationChild;
    int shift = as<int>(down);
    tasks.move(sourceRow, destinationChild - shift);

    endMoveRows();
    setChanged();

    return true;
}

//
// Brief
//
BriefModel::BriefModel(BriefStatistics &brief, QObject *parent)
    : TableModel(parent)
    , brief(brief)
{}

BriefModel::~BriefModel()
{}

int BriefModel::rowCount(const QModelIndex &parent) const
{
    UNUSED(parent);
    return as<int>(brief.size())*BriefRow::Count;
}

int BriefModel::columnCount(const QModelIndex &parent) const
{
    UNUSED(parent);
    return BriefColumn::Count;
}


Qt::ItemFlags BriefModel::flags(const QModelIndex &index) const
{
    UNUSED(index);
    return 0;
}

QVariant BriefModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    int col = index.column();
    int realRow = index.row()/BriefRow::Count;
    BriefRow::t rowType = as<BriefRow::t>(index.row()%BriefRow::Count);

    const auto &monthIt = std::next(brief.begin(), realRow);
    const auto &month = monthIt->first;
    const auto &record = monthIt->second;

    bool header = index.row() == BriefRow::Space2*1;

    if(role == Qt::DisplayRole) {
        switch(col)
        {
            default: return QVariant();

            case BriefColumn::Date: {
                if(rowType != BriefRow::Space1 && rowType != BriefRow::Space2) {
                    return month.toString();
                }
            } break;
            case BriefColumn::Common: {
                const auto &common = record.common;
                switch(rowType) {
                    default: return QVariant();
                    case BriefRow::Received: return "▲ " + formatMoney(common.received);
                    case BriefRow::Spent: return "▼ " + formatMoney(common.spent);
                }
            } break;
            case BriefColumn::Balance: {
                if(header) {
                    return tr("Баланс");
                }

                if(rowType == BriefRow::Space1 || rowType == BriefRow::Space2) {
                    return QVariant();
                }

                const auto &common = record.common;
                return formatMoney(common.received - common.spent);
            } break;
            case BriefColumn::Regular: {
                if(header) {
                    return tr("Регулярные");
                }

                const auto &regular = record.regular;

                switch(rowType) {
                    default: return QVariant();
                    case BriefRow::Received: return formatMoney(regular.received);
                    case BriefRow::Spent: return formatMoney(regular.spent);
                }
            } break;
            case BriefColumn::Nonregular: {
                if(header) {
                    return tr("Нерегулярные");
                }

                const auto &common = record.common;
                const auto &regular = record.regular;
                switch(rowType) {
                    default: return QVariant();
                    case BriefRow::Received: return formatMoney(common.received - regular.received);
                    case BriefRow::Spent: return formatMoney(common.spent - regular.spent);
                }
            }
        }
    }

    if(role == Qt::ForegroundRole) {
        if(col == BriefColumn::Common) {
            return rowType == BriefRow::Received ? colors::profit : colors::loses;
        } else {
            return colors::normal;
        }
    }

    if(role == Qt::FontRole) {
        if(header) {
            QFont f("Segoe UI", 8);
            f.setBold(true);
            return f;
        }

        if(col == BriefColumn::Common) {
            QFont f("Segoe UI", 9);
            f.setBold(true);
            return f;
        } else {
            return QFont("Segoe UI", 9);
        }
    }

    if(role == Qt::TextAlignmentRole) {
        if(col == BriefColumn::Date) {
            return QVariant(Qt::AlignHCenter | Qt::AlignVCenter);
        } else {
            return QVariant(Qt::AlignLeft | Qt::AlignVCenter);
        }
    }

    return QVariant();
}

QVariant BriefModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch(section)
        {
            case BriefColumn::Date: return tr("Месяц");
            case BriefColumn::Common: return tr("Всего");
            case BriefColumn::Balance: return tr("Баланс");
            case BriefColumn::Regular: return tr("Регулярные");
            case BriefColumn::Nonregular: return tr("Нерегулярные");
        }
    }

    return QVariant();
}

static int categoryNodeType = qRegisterMetaType<const Node<Category> *>();
static int walletNodeType = qRegisterMetaType<const Node<Wallet> *>();
static int transactionTypeType = qRegisterMetaType<Transaction::Type::t>();

ModelsDelegate::ModelsDelegate(Data &data, QObject* parent)
    : QItemDelegate(parent)
    , m_data(data)
{}

ModelsDelegate::~ModelsDelegate()
{}

void ModelsDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if(index.data().type() != QVariant::Bool) {
        QItemDelegate::paint(painter, option, index);
        return;
    }

    QStyleOptionButton style;
    style.state = QStyle::State_Enabled;
    style.state |= index.data().toBool() ? QStyle::State_On : QStyle::State_Off;
    style.direction = QApplication::layoutDirection();
    style.rect = option.rect;

    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect, option.palette.highlight());
    }
    QApplication::style()->drawControl(QStyle::CE_CheckBox, &style, painter);
}

QWidget* ModelsDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QVariant data = index.data(Qt::EditRole);
    QVariant::Type type = data.type();

    if(type != QVariant::Date && type != QVariant::Bool && type != QVariant::UserType) {
        return QItemDelegate::createEditor(parent, option, index);
    }

    if(type == QVariant::Bool) {
        return new QCheckBox(parent);
    }

    if(type == QVariant::Date) {
        QDateEdit *edit = new QDateEdit(parent);
        edit->setCalendarPopup(true);

        bool log = !!qobject_cast<const LogModel*>(index.model());
        if(log) {
            QDate min;
            QDate max {today};
            if(as<int>(m_data.logModel.log.size()) > index.row()+1) {
                min = m_data.logModel.log[index.row()+1].date;
            }

            if(min == max) {
                return nullptr;
            }

            edit->setMinimumDate(min);
            edit->setMaximumDate(max);
        }

        return edit;
    }

    // custom types
    int customType = data.userType();
    if(customType == transactionTypeType) {
        QComboBox *box = new QComboBox(parent);
        QVector<Transaction::Type::t> v {Transaction::Type::In, Transaction::Type::Out};

        bool tasks = !!qobject_cast<const TasksModel*>(index.model());
        if(!tasks) {
            v << Transaction::Type::Transfer;
        }

        for(auto t : v) {
            box->addItem(Transaction::Type::toString(t), as<int>(t));
        }
        return box;
    }

    if(customType == categoryNodeType) {
        int column;
        if(qobject_cast<const LogModel*>(index.model())) {
            column = LogColumn::Type;
        } else if(qobject_cast<const PlansModel*>(index.model())) {
            column = PlansColumn::Type;
        } else if(qobject_cast<const TasksModel*>(index.model())) {
            column = TasksColumn::Type;
        }

        QModelIndex typeIndex = index.model()->index(index.row(), column);
        QVariant data = typeIndex.data(Qt::EditRole);
        Transaction::Type::t type = as<Transaction::Type::t>(data.toInt());
        if(type == Transaction::Type::Transfer) {
            return nullptr;
        }

        CategoriesModel &categories =
                type == Transaction::Type::In ?
                    m_data.inCategoriesModel :
                    m_data.outCategoriesModel;

        NodeButton<Category> *button = new NodeButton<Category>(categories, parent);
        return button;
    }

    if(customType == walletNodeType) {
        // assume that only LogModel uses wallets tree editor
        QModelIndex typeIndex = index.model()->index(index.row(), LogColumn::Type);
        QVariant data = typeIndex.data(Qt::EditRole);
        Transaction::Type::t type = as<Transaction::Type::t>(data.toInt());

        int col = index.column();

        if((col == LogColumn::From && type == Transaction::Type::In)
        || (col == LogColumn::To   && type == Transaction::Type::Out)) {
            return nullptr;
        }

        NodeButton<Wallet> *button = new NodeButton<Wallet>(m_data.walletsModel, parent);
        return button;
    }

    // should not reach this code, but just to be sure
    return QItemDelegate::createEditor(parent, option, index);
}

void ModelsDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    // bool case
    if (QCheckBox *box = qobject_cast<QCheckBox*>(editor)) {
        box->setChecked(index.data(Qt::EditRole).toBool());
        return;
    }

    // Date case
    if (QDateEdit *edit = qobject_cast<QDateEdit*>(editor)) {
        QDate date = index.data(Qt::EditRole).toDate();
        edit->setDate(date);
        return;
    }

    // Transaction type case
    if (QComboBox *box = qobject_cast<QComboBox*>(editor)) {
        QVariant value = index.data(Qt::EditRole);
        int i = box->findData(value);
        if(i >= 0) {
            box->setCurrentIndex(i);
        }
        return;
    }

    // trees case
    NodeButton<Category> *cat = dynamic_cast<NodeButton<Category> *>(editor);
    NodeButton<Wallet> *wal = dynamic_cast<NodeButton<Wallet> *>(editor);

    QVariant value = index.data(Qt::EditRole);

    // Categories tree
    if(cat) {
       const ArchNode<Category> archNode = value;
       if(!archNode.isValidPointer()) {
           QItemDelegate::setEditorData(editor, index);
           return;
       }

       const Node<Category> *node = archNode.toPointer();
       cat->setNode(node);
       return;

    // Wallets tree
    } else if(wal) {
        ArchNode<Wallet> archNode = value;
        if(!archNode.isValidPointer()) {
            QItemDelegate::setEditorData(editor, index);
            return;
        }

        const Node<Wallet> *node = archNode.toPointer();
        wal->setNode(node);
        return;
    }

    QItemDelegate::setEditorData(editor, index);
}

void ModelsDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    // bool case
    if (QCheckBox* box = qobject_cast<QCheckBox*>(editor)) {
        model->setData(index, box->isChecked(), Qt::EditRole);
        return;
    }

    // Date case
    if (QDateEdit *edit = qobject_cast<QDateEdit*>(editor)) {
        model->setData(index, edit->date(), Qt::EditRole);
        return;
    }

    // Transaction type case
    if (QComboBox* box = qobject_cast<QComboBox*>(editor)) {
        model->setData(index, box->currentData(), Qt::EditRole);
        return;
    }

    // trees case
    NodeButton<Category> *cat = dynamic_cast<NodeButton<Category> *>(editor);
    NodeButton<Wallet> *wal = dynamic_cast<NodeButton<Wallet> *>(editor);

    // Categories tree
    if(cat) {
       model->setData(index, ArchNode<Category>(cat->node()), Qt::EditRole);
       return;

    // Wallets tree
    } else if(wal) {
        model->setData(index, ArchNode<Wallet>(wal->node()), Qt::EditRole);
        return;
    }

    QItemDelegate::setModelData(editor, model, index);
}

bool ModelsDelegate::eventFilter(QObject *object, QEvent *event)
{
    NodeButton<Category> *cat = dynamic_cast<NodeButton<Category> *>(object);
    NodeButton<Wallet> *wal = dynamic_cast<NodeButton<Wallet> *>(object);

    NodeButtonState state {cat ? cat->state() : wal ? wal->state() : NodeButtonState::Folded};

    if(state == NodeButtonState::Expanded && event->type() == QEvent::FocusOut) { // for expaned case we do not want to close editor
        return true;
    }

    return QItemDelegate::eventFilter(object, event);
}

void CategoriesViewEventFilter::setViews(QTreeView *in, QTreeView *out)
{
    m_in = in;
    m_out = out;
}

bool CategoriesViewEventFilter::eventFilter(QObject *watched, QEvent *event)
{
    if(event->type() != QEvent::MouseButtonPress) {
        return QObject::eventFilter(watched, event);
    }

    QMouseEvent *e { dynamic_cast<QMouseEvent *>(event) };
    if(!e) {
        return QObject::eventFilter(watched, event);
    }

    QWidget *w = qobject_cast<QWidget *>(watched);

    if(w && (w == m_in->viewport() || w == m_out->viewport())) {
        QTreeView *view = w == m_in->viewport() ? m_in : m_out;

        QPoint pos = e->pos();
        QModelIndex index = view->indexAt(pos);
        if(index.column() == CategoriesColumn::Regular) {
            view->edit(index);
        }
    }

    return QObject::eventFilter(watched, event);
}

} // namespace cashbook
