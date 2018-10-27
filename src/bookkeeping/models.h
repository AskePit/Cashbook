#ifndef BOOKKEEPING_MODELS_H
#define BOOKKEEPING_MODELS_H

#include "basic_types.h"
#include "statistics.h"

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QItemDelegate>
#include <set>
#include <deque>

class QTreeView;

namespace cashbook
{

/**
 * @brief The TreeModel class
 * @details This class is mainly intended to make public some of protected
 *          methods to make it possible to use them in template nonmember
 *          functions.
 */
class TreeModel : public QAbstractItemModel, public Changable
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

class TableModel : public QAbstractTableModel, public Changable
{
    Q_OBJECT

public:
    TableModel(QObject *parent = 0) : QAbstractTableModel(parent) {}
};

class CategoriesColumn
{
public:
    enum t {
        Name = 0,
        Statistics,
        Regular,

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
    QModelIndex itemIndex(const Node<Category> *item) const;

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
    QModelIndex itemIndex(const Node<Wallet> *item) const;

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

class OwnersModel : public TableModel
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
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

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
        Start = 0,

        Date = Start,
        Type,
        Category,
        Money,
        From,
        To,
        Note,

        Count
    };
};

class LogModel : public TableModel
{
    Q_OBJECT

public:
    std::deque<Transaction> log;
    Statistics &statistics;
    int unanchored {0};
    std::set<Month> changedMonths;

    LogModel(Statistics &statistics, QObject *parent = 0);
    ~LogModel();

    bool anchoreTransactions();
    bool copyTop();
    bool canAnchore() const;
    void appendTransactions(const std::vector<Transaction> &transactions);

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

    void updateNote(int row, const QString &note);
    void updateTask(Task &task) const;
    void normalizeData();
};

class FilteredLogModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    FilteredLogModel(const QDate &from, const QDate &to, Transaction::Type::t type, const Node<Category> *category, QObject *parent = 0);
    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QDate m_from;
    QDate m_to;
    Transaction::Type::t m_type;
    const Node<Category> *m_category {nullptr};
};

class PlansColumn
{
public:
    enum t {
        Name = 0,
        Type,
        Category,
        Money,

        Count
    };
};

class PlansModel : public TableModel
{
    Q_OBJECT

public:
    QVector<Plan> plans;

    PlansModel(QObject *parent = 0);
    ~PlansModel();

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    bool insertRows(int position, int rows, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int position, int rows, const QModelIndex &parent = QModelIndex()) override;
    bool moveRow(const QModelIndex &sourceParent, int sourceRow, const QModelIndex &destinationParent, int destinationChild);

    void clear() {
        emit beginResetModel();
        plans.clear();
        emit endResetModel();
    }

    void insertPlan(const Plan &plan);
};

class TasksColumn
{
public:
    enum t {
        Type,
        Category,
        From,
        To,
        Money,
        Spent,
        Rest,

        Count
    };
};

class TasksModel : public TableModel
{
    Q_OBJECT

public:
    QVector<Task> tasks;

    TasksModel(const LogModel &log, QObject *parent = 0);
    ~TasksModel();

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    bool insertRows(int position, int rows, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int position, int rows, const QModelIndex &parent = QModelIndex()) override;
    bool moveRow(const QModelIndex &sourceParent, int sourceRow, const QModelIndex &destinationParent, int destinationChild);

    void clear() {
        emit beginResetModel();
        tasks.clear();
        emit endResetModel();
    }

private:
    const LogModel &m_log;
};

class BriefColumn
{
public:
    enum t {
        Date = 0,
        Common,
        Regular,
        Nonregular,
        Balance,

        Count
    };
};

class BriefRow
{
public:
    enum t {
        Space1 = 0,
        Header = Space1,
        Space2,
        Received,
        Spent,

        Count
    };
};

class BriefModel : public TableModel
{
    Q_OBJECT

public:
    const BriefStatistics &brief;

    BriefModel(BriefStatistics &brief, QObject *parent = 0);
    ~BriefModel();

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
};

class Data;

class ModelsDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    ModelsDelegate(Data &data, QObject* parent = nullptr);
    ~ModelsDelegate();

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    virtual void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    virtual void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    virtual bool eventFilter(QObject *object, QEvent *event) override;

private:
    Data &m_data;
};

class CategoriesViewEventFilter : public QObject {
    Q_OBJECT

public:
    void setViews(QTreeView *in, QTreeView *out);
    virtual bool eventFilter(QObject *watched, QEvent *event);

private:
    QTreeView *m_in, *m_out;
};

} // namespace cashbook

#endif // BOOKKEEPING_MODELS_H
