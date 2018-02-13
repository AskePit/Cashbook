#ifndef BOOKKEEPING_MODELS_H
#define BOOKKEEPING_MODELS_H

#include "basic_types.h"
#include "statistics.h"

#include <std/rvector.h>
#include <QAbstractItemModel>
#include <QStyledItemDelegate>
#include <QItemDelegate>

namespace cashbook
{

/**
 * @brief The TreeModel class
 * @details This class is mainly intended to make public some of protected
 *          methods to make it possible to use them in template nonmember
 *          functions.
 */
class TreeModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    TreeModel(QObject *parent = 0) : QAbstractItemModel(parent) {}

    QModelIndex createIndex(int row, int column, void *data) const {
        return QAbstractItemModel::createIndex(row, column, data);
    }

    void beginInsertRows(const QModelIndex &parent, int first, int last) {
        return QAbstractItemModel::beginInsertRows(parent, first, last);
    }

    void endInsertRows() {
        return QAbstractItemModel::endInsertRows();
    }

    void beginRemoveRows(const QModelIndex &parent, int first, int last) {
        return QAbstractItemModel::beginRemoveRows(parent, first, last);
    }

    void endRemoveRows() {
        return QAbstractItemModel::endRemoveRows();
    }

    bool beginMoveRows(const QModelIndex &sourceParent, int sourceFirst, int sourceLast, const QModelIndex &destinationParent, int destinationRow) {
        return QAbstractItemModel::beginMoveRows(sourceParent, sourceFirst, sourceLast, destinationParent, destinationRow);
    }

    void endMoveRows() {
        return QAbstractItemModel::endMoveRows();
    }

    void update() {
        beginResetModel();
        endResetModel();
        emit recalculated();
    }

signals:
    void nodesGonnaBeRemoved(QStringList nodeIds);
    void recalculated();
};

class CategoriesColumn
{
public:
    enum t {
        Name = 0,
        Regular,
        Statistics,

        Count
    };
};

class CategoriesModel : public TreeModel
{
    Q_OBJECT

public:
    Tree<Category> *rootItem;
    CategoryMoneyMap &statistics;

    CategoriesModel(CategoryMoneyMap &statistics, QObject *parent = 0);
    ~CategoriesModel();

    Node<Category> *getItem(const QModelIndex &index) const;

    Node<Category> *addChild(const Category &data) {
        auto node = new Node<Category>(data, rootItem);
        rootItem->children.push_back(node);
        return node;
    }

    Node<Category> *child(const Category &data) {
        return rootItem->child(data);
    }

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;

    bool insertRows(int position, int rows,
                    const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int position, int rows,
                    const QModelIndex &parent = QModelIndex()) override;

    bool moveRow(const QModelIndex &sourceParent, int sourceRow, const QModelIndex &destinationParent, int destinationChild);

    void clear() {
        emit beginResetModel();
        delete rootItem;
        rootItem = new Tree<Category>;
        emit endResetModel();
    }
};

class WalletColumn
{
public:
    enum t {
        Name = 0,
        Amount = 1,

        Count
    };
};

class WalletsModel : public TreeModel
{
    Q_OBJECT

public:
    Tree<Wallet> *rootItem;

    WalletsModel(QObject *parent = 0);
    ~WalletsModel();

    Node<Wallet> *getItem(const QModelIndex &index) const;

    Node<Wallet> *addChild(const Wallet &data) {
        auto node = new Node<Wallet>(data, rootItem);
        rootItem->children.push_back(node);
        return node;
    }

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value,
                 int role = Qt::EditRole) override;

    bool insertRows(int position, int rows,
                    const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int position, int rows,
                    const QModelIndex &parent = QModelIndex()) override;

    bool moveRow(const QModelIndex &sourceParent, int sourceRow, const QModelIndex &destinationParent, int destinationChild);

    void clear() {
        emit beginResetModel();
        delete rootItem;
        rootItem = new Tree<Wallet>;
        emit endResetModel();
    }
};

class OwnersModel : public QAbstractListModel
{
    Q_OBJECT

public:
    QVector<Owner> owners;

    OwnersModel(QObject *parent = 0);
    ~OwnersModel();

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    bool insertRows(int position, int rows, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int position, int rows, const QModelIndex &parent = QModelIndex()) override;
    bool moveRow(const QModelIndex &sourceParent, int sourceRow, const QModelIndex &destinationParent, int destinationChild);

    void clear() {
        emit beginResetModel();
        owners.clear();
        emit endResetModel();
    }

signals:
    void nodesGonnaBeRemoved(QStringList nodeIds);
};

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

class LogModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    aske::rvector<Transaction> log;
    Statistics &statistics;
    int unanchored {0};

    LogModel(Statistics &statistics, QObject *parent = 0);
    ~LogModel();

    bool anchoreTransactions();

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    bool insertRows(int position, int rows, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int position, int rows, const QModelIndex &parent = QModelIndex()) override;

    void clear() {
        emit beginResetModel();
        log.clear();
        emit endResetModel();
        unanchored = 0;
    }
};

class BoolDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    BoolDelegate(QObject* parent = nullptr);
    ~BoolDelegate();

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    virtual void setEditorData(QWidget* editor, const QModelIndex& index) const;
    virtual void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const;
};

class Data;

class LogItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    LogItemDelegate(Data &data, QObject* parent = nullptr);
    ~LogItemDelegate();

    virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const;
    virtual void setEditorData(QWidget* editor, const QModelIndex& index) const;
    virtual void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const;

private:
    const Data &m_data;
};

} // namespace cashbook

#endif // BOOKKEEPING_MODELS_H
