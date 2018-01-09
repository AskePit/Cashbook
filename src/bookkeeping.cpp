#include "bookkeeping.h"
#include "types.h"

namespace cashbook
{

CategoriesModel::CategoriesModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    rootItem = new Node<Category>;
}

CategoriesModel::~CategoriesModel()
{
    delete rootItem;
}

int CategoriesModel::columnCount(const QModelIndex &parent) const
{
    UNUSED(parent);
    return 1;
}

QVariant CategoriesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    Node<Category> *item = getItem(index);

    return item->data;
}

Qt::ItemFlags CategoriesModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    return Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | QAbstractItemModel::flags(index);
}

Node<Category> *CategoriesModel::getItem(const QModelIndex &index) const
{
    if (index.isValid()) {
        Node<Category> *item = as<Node<Category>*>(index.internalPointer());
        if (item)
            return item;
    }
    return rootItem;
}

QVariant CategoriesModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    UNUSED(section, orientation, role);
    return QVariant();
}

QModelIndex CategoriesModel::index(int row, int column, const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() != 0)
        return QModelIndex();

    Node<Category> *parentItem = getItem(parent);

    if(row >= parentItem->childCount()) {
        return QModelIndex();
    }

    Node<Category> *childItem = parentItem->at(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

bool CategoriesModel::insertRows(int position, int rows, const QModelIndex &parent)
{
    Node<Category> *parentItem = getItem(parent);

    beginInsertRows(parent, position, position + rows - 1);
    for(int i = 0; i<rows; ++i) {
        parentItem->addChildAt("", position);
    }

    endInsertRows();

    return true;
}

QModelIndex CategoriesModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    Node<Category> *childItem = getItem(index);
    Node<Category> *parentItem = childItem->parent;

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(as<int>(parentItem->childCount()), 0, parentItem);
}

bool CategoriesModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    Node<Category> *parentItem = getItem(parent);
    bool success = true;

    beginRemoveRows(parent, position, position + rows - 1);
    for(int i = 0; i<rows; ++i) {
        parentItem->removeChildAt(position);
    }
    endRemoveRows();

    return success;
}

int CategoriesModel::rowCount(const QModelIndex &parent) const
{
    Node<Category> *parentItem = getItem(parent);

    return as<int>(parentItem->childCount());
}

bool CategoriesModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole)
        return false;

    Node<Category> *item = getItem(index);
    item->data = value.toString();
    emit dataChanged(index, index);

    return true;
}

class WalletColumn
{
public:
    enum t {
        Name = 0,
        Amount = 1,

        Count
    };
};

WalletsModel::WalletsModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    rootItem = new Node<Wallet>;
}

WalletsModel::~WalletsModel()
{
    delete rootItem;
}

int WalletsModel::columnCount(const QModelIndex &parent) const
{
    UNUSED(parent);
    return WalletColumn::Count;
}

QVariant WalletsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    Node<Wallet> *item = getItem(index);

    switch(index.column())
    {
        case WalletColumn::Name: return item->data.name;
        case WalletColumn::Amount:
            if(item->isLeaf()) {
                return as<double>(item->data.amount);
            }
    }

    return QVariant();
}

Qt::ItemFlags WalletsModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    return Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | QAbstractItemModel::flags(index);
}

Node<Wallet> *WalletsModel::getItem(const QModelIndex &index) const
{
    if (index.isValid()) {
        Node<Wallet> *item = as<Node<Wallet>*>(index.internalPointer());
        if (item)
            return item;
    }
    return rootItem;
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
    if (parent.isValid() && parent.column() > WalletColumn::Amount)
        return QModelIndex();

    Node<Wallet> *parentItem = getItem(parent);

    if(row >= parentItem->childCount()) {
        return QModelIndex();
    }

    Node<Wallet> *childItem = parentItem->at(row);
    if (childItem)
        return createIndex(row, column, childItem);
    else
        return QModelIndex();
}

bool WalletsModel::insertRows(int position, int rows, const QModelIndex &parent)
{
    Node<Wallet> *parentItem = getItem(parent);

    beginInsertRows(parent, position, position + rows - 1);
    for(int i = 0; i<rows; ++i) {
        parentItem->addChildAt(Wallet(), position);
    }

    endInsertRows();

    return true;
}

QModelIndex WalletsModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    Node<Wallet> *childItem = getItem(index);
    Node<Wallet> *parentItem = childItem->parent;

    if (parentItem == rootItem)
        return QModelIndex();

    return createIndex(as<int>(parentItem->childCount()), 0, parentItem);
}

bool WalletsModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    Node<Wallet> *parentItem = getItem(parent);
    bool success = true;

    beginRemoveRows(parent, position, position + rows - 1);
    for(int i = 0; i<rows; ++i) {
        parentItem->removeChildAt(position);
    }
    endRemoveRows();

    return success;
}

int WalletsModel::rowCount(const QModelIndex &parent) const
{
    Node<Wallet> *parentItem = getItem(parent);

    return as<int>(parentItem->childCount());
}

bool WalletsModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole)
        return false;

    Node<Wallet> *item = getItem(index);

    switch(index.column())
    {
        case WalletColumn::Name: item->data.name = value.toString();
        case WalletColumn::Amount: item->data.amount = value.toDouble();
    }

    emit dataChanged(index, index);

    return true;
}

OwnersModel::OwnersModel(QObject *parent)
    : QAbstractListModel(parent)
{}

OwnersModel::~OwnersModel()
{}

QVariant OwnersModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    return owners[index.row()];
}

QVariant OwnersModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    UNUSED(section, orientation, role);
    return QVariant();
}

int OwnersModel::rowCount(const QModelIndex &parent) const
{
    UNUSED(parent);
    return owners.size();
}

Qt::ItemFlags OwnersModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    return Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | QAbstractItemModel::flags(index);
}

bool OwnersModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole)
        return false;

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
    owners.insert(position, rows, IdableString());
    return true;
}

bool OwnersModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    UNUSED(parent);
    owners.remove(position, rows);
    return true;
}

class LogColumn
{
public:
    enum t {
        Date = 0,
        Type,
        Category,
        Money,
        From,
        To,
        Note,

        Count
    };
};

LogModel::LogModel(QObject *parent)
    : QAbstractTableModel(parent)
{}

LogModel::~LogModel()
{}

template <class T>
static inline QVariant getVariantPointer(T pointer)
{
    QVariant v;
    v.setValue<T>(pointer);
    return v;
}

QVariant LogModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    if (role != Qt::DisplayRole && role != Qt::EditRole)
        return QVariant();

    const Transaction &t = log[index.row()];

    switch(index.column())
    {
        case LogColumn::Date: return t.date;
        case LogColumn::Type: return t.type;
        case LogColumn::Category: {
            if(t.category.empty()) {
                return QVariant();
            } else {
                return getVariantPointer(*t.category.begin());
            }
        }
        case LogColumn::Money: return as<double>(t.amount);
        case LogColumn::From: return getVariantPointer(t.from);
        case LogColumn::To: return getVariantPointer(t.to);
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
    UNUSED(parent);
    return log.size();
}

int LogModel::columnCount(const QModelIndex &parent) const
{
    UNUSED(parent);
    return LogColumn::Count;
}

Qt::ItemFlags LogModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return 0;

    return Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | QAbstractItemModel::flags(index);
}

bool LogModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole)
        return false;

    Transaction &t = log[index.row()];

    switch(index.column())
    {
        case LogColumn::Date: t.date = value.toDate();
        case LogColumn::Type: t.type = as<Transaction::Type::t>(value.toInt());
        case LogColumn::Category: {
            if(value.isNull()) {
                t.category.clear();
            } else {
                t.category.insert(value.value<CategoryNodeP>());
            }
        }
        case LogColumn::Money: t.amount = value.toDouble();
        case LogColumn::From: t.from = value.value<WalletNodeP>();
        case LogColumn::To: t.to = value.value<WalletNodeP>();
        case LogColumn::Note: t.note = value.toString();
    }

    emit dataChanged(index, index);

    return true;
}

bool LogModel::insertRows(int position, int rows, const QModelIndex &parent)
{
    UNUSED(parent);
    log.insert(position, rows, Transaction());
    return true;
}

bool LogModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    UNUSED(parent);
    log.remove(position, rows);
    return true;
}

template <>
QString extractPathString<Category>(const Node<Category> *node) {
    return node->data;
}

template <>
QString extractPathString<Wallet>(const Node<Wallet> *node) {
    return node->data.name;
}

} // namespace cashbook
