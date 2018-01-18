#include "bookkeeping.h"
#include "types.h"

#include <QComboBox>
#include <QDebug>

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
        return 0;
    }

    return Qt::ItemIsEditable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled | model->QAbstractItemModel::flags(index);
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
    return list.size();
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

    model->beginRemoveRows(parent, position, position + rows - 1);
    for(int i = 0; i<rows; ++i) {
        parentItem->removeChildAt(position);
    }
    model->endRemoveRows();

    return success;
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

CategoriesModel::CategoriesModel(QObject *parent)
    : TreeModel(parent)
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
    return 1;
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

    return item->data;
}

Qt::ItemFlags CategoriesModel::flags(const QModelIndex &index) const
{
    return common::flags(this, index);
}

Node<Category> *CategoriesModel::getItem(const QModelIndex &index) const
{
    return common::tree::getItem<CategoriesModel, Category>(this, index);
}

QVariant CategoriesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    return common::headerData(section, orientation, role);
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
    item->data = value.toString();
    emit dataChanged(index, index);

    return true;
}

//
// Wallets
//

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
                return as<double>(item->data.amount);
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

int LogModel::getTransactionIndex(const QModelIndex &index) const
{
    return log.size() - index.row() - 1;
}

int LogModel::getTransactionIndex(int modelIndex) const
{
    return log.size() - modelIndex - 1;
}

Transaction &LogModel::getTransaction(const QModelIndex &index)
{
    return log[getTransactionIndex(index)];
}

const Transaction &LogModel::getTransaction(const QModelIndex &index) const
{
    return log[getTransactionIndex(index)];
}

QVariant LogModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    if (role != Qt::DisplayRole && role != Qt::EditRole) {
        return QVariant();
    }

    const Transaction &t = getTransaction(index);

    switch(index.column())
    {
        case LogColumn::Date: return t.date;
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
                const Node<Category> *node = *t.category.begin();
                if(role == Qt::DisplayRole) {
                    return pathToString(node);
                } else {
                    return getVariantPointer(node);
                }
            }
        } break;
        case LogColumn::Money: return as<double>(t.amount);
        case LogColumn::From: {
            if(role == Qt::DisplayRole) {
                return pathToString(t.from);
            } else {
                return getVariantPointer(t.from);
            }
        } break;
        case LogColumn::To: {
            if(role == Qt::DisplayRole) {
                return pathToString(t.to);
            } else {
                return getVariantPointer(t.to);
            }
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

Qt::ItemFlags LogModel::flags(const QModelIndex &index) const
{
    return common::flags(this, index);
}

bool LogModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role != Qt::EditRole) {
        return false;
    }

    Transaction &t = getTransaction(index);

    switch(index.column())
    {
        case LogColumn::Date: t.date = value.toDate(); break;
        case LogColumn::Type: {
            auto oldType = t.type;
            t.type = as<Transaction::Type::t>(value.toInt());
            if(t.type != oldType) {
                t.category.clear();
                t.from = nullptr;
                t.to = nullptr;
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
                t.category.insert(value.value<const Node<Category>*>());
            }
        } break;
        case LogColumn::Money: t.amount = value.toDouble(); break;
        case LogColumn::From: t.from = value.value<const Node<Wallet>*>(); break;
        case LogColumn::To: t.to = value.value<const Node<Wallet>*>(); break;
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
    log.insert(getTransactionIndex(position)+1, rows, t);
    endInsertRows();
    return true;
}

bool LogModel::removeRows(int position, int rows, const QModelIndex &parent)
{
    UNUSED(parent);
    beginRemoveRows(parent, position, position + rows - 1);
    log.remove(getTransactionIndex(position), rows);
    endRemoveRows();
    return true;
}

Data::Data()
{
    connect(&owners, &OwnersModel::rowsRemoved, this, &Data::onOwnersRemove);
    connect(&inCategories, &CategoriesModel::rowsRemoved, this, &Data::onInCategoriesRemove);
    connect(&outCategories, &CategoriesModel::rowsRemoved, this, &Data::onOutCategoriesRemove);
    connect(&wallets, &WalletsModel::rowsRemoved, this, &Data::onWalletsRemove);
}

void Data::onOwnersRemove(const QModelIndex &parent, int first, int last)
{

}

void Data::onInCategoriesRemove(const QModelIndex &parent, int first, int last)
{

}

void Data::onOutCategoriesRemove(const QModelIndex &parent, int first, int last)
{

}

void Data::onWalletsRemove(const QModelIndex &parent, int first, int last)
{

}


template <>
QString extractPathString<Category>(const Node<Category> *node) {
    return node->data;
}

template <>
QString extractPathString<Wallet>(const Node<Wallet> *node) {
    return node->data.name;
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

    const Transaction &record = m_data.log.getTransaction(index);

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
                box->addItem(pathToString(n), getVariantPointer(n));
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
            const auto nodes = m_data.wallets.rootItem->toList();
            for(const Node<Wallet> *n : nodes) {
                box->addItem(pathToString(n), getVariantPointer(n));
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
        // save the current text of the combo box as the current value of the item
        model->setData(index, box->currentData(), Qt::EditRole);
    } else {
        QStyledItemDelegate::setModelData(editor, model, index);
    }
}

} // namespace cashbook
