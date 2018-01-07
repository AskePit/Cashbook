#include "bookkeeping.h"

namespace cashbook
{

#define UNUSED(...) (void)__VA_ARGS__
#define as static_cast

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
    return 2;
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
    UNUSED(section, orientation, role);
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
    if (parent.isValid() && parent.column() != 0)
        return QModelIndex();

    Node<Wallet> *parentItem = getItem(parent);

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

} // namespace cashbook
