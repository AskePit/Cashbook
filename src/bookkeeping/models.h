#ifndef BOOKKEEPING_MODELS_H
#define BOOKKEEPING_MODELS_H

#include "bookkeeping/bookkeeping.h"

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QStyledItemDelegate>
#include <QItemDelegate>

class QTreeView;

namespace cashbook
{

class CategoryMoneyMap;
struct Statistics;
class BriefStatistics;

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

class TableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    TableModel(QObject *parent = 0) : QAbstractTableModel(parent) {}

    void update() {
        beginResetModel();
        endResetModel();
    }
};

class CategoriesColumn
{
public:
    enum t {
        Name = 0,

        Count
    };
};

class CategoriesModel : public TreeModel
{
    Q_OBJECT

public:
    CategoriesData& m_data;

    CategoriesModel(CategoriesData &data, QObject *parent = 0);

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
    WalletsData& m_data;

    WalletsModel(WalletsData& data, QObject *parent = 0);

    Node<Wallet> *getItem(const QModelIndex &index) const;
    QModelIndex itemIndex(const Node<Wallet> *item) const;

    Node<Wallet> *addChild(const Wallet &data) {
        auto node = new Node<Wallet>(data, m_data.rootItem);
        m_data.rootItem->children.push_back(node);
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
};

class OwnersModel : public TableModel
{
    Q_OBJECT

public:
    OwnersData& m_data;

    OwnersModel(OwnersData& ownersData, QObject *parent = 0);
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
    LogData& m_data;

    LogModel(LogData &data, QObject *parent = 0);
    ~LogModel();

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    bool insertRows(int position, int rows, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int position, int rows, const QModelIndex &parent = QModelIndex()) override;

    bool copyTop();
    bool anchoreTransactions();
    void appendTransactions(const std::vector<Transaction> &transactions);

    void updateNote(size_t row, const QString &note);
    void updateTask(Task &task) const;
    bool normalizeData();
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
    PlansTermData& m_data;

    PlansModel(PlansTermData& data, QObject *parent = 0);
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
    TasksListsData& m_data;

    TasksModel(TasksListsData& data, const LogData &log, QObject *parent = 0);
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

private:
    const LogData &m_log;
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

class DataModels;

class ModelsDelegate : public QItemDelegate
{
    Q_OBJECT

public:
    ModelsDelegate(DataModels &dataModels, QObject* parent = nullptr);
    ~ModelsDelegate();

    virtual void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
    virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;
    virtual void setEditorData(QWidget* editor, const QModelIndex& index) const override;
    virtual void setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const override;
    virtual bool eventFilter(QObject *object, QEvent *event) override;

private:
    DataModels &m_data;
};

class DataModels
{
public:
    DataModels(Data& data);

    bool anchoreTransactions();

    OwnersModel ownersModel;
    WalletsModel walletsModel;
    CategoriesModel inCategoriesModel;
    CategoriesModel outCategoriesModel;
    LogModel logModel;
    std::array<PlansModel, PlanTerm::Count> plansModels;
    std::array<TasksModel, TaskStatus::Count> tasksModels;
    BriefModel briefStatisticsModel;

    Data& m_data;
};

} // namespace cashbook

#endif // BOOKKEEPING_MODELS_H
