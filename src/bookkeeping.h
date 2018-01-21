#ifndef BOOKKEEPING_H
#define BOOKKEEPING_H

#include <std/tree.h>
#include <std/money.h>
#include <QString>
#include <QVector>
#include <QUuid>
#include <QDate>
#include <QObject>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <QStyledItemDelegate>

namespace cashbook
{

struct Idable
{
    QUuid id;
};

class IdableString : public Idable, public QString
{
public:
    IdableString()
        : QString()
    {}

    IdableString(const char *str)
        : QString(str)
    {}

    IdableString(const QString &str)
        : QString(str)
    {}
};

Q_DECLARE_METATYPE(Node<IdableString> *)
Q_DECLARE_METATYPE(const Node<IdableString> *)

using Owner = IdableString;

/**
 * @brief Archiveable pointer.
 * @details Wrapper around existing pointer OR archived nonexisting one.
 *
 * `ArchPointer<T>` wrapper allows to contain:
 * - valid `const T *` which belongs to any `Owner`;
 * - `QString` representing former `const T*` which does not exist anymore.
 */
/*template <class T>
class ArchPointer : public QVariant
{
public:
    ArchPointer(const QVariant &v)
        : QVariant(v)
    {}

    ArchPointer(T *p)
        : QVariant(QVariant::fromValue<const T*>(p))
    {}

    ArchPointer(const QString &str)
        : QVariant(QVariant::fromValue<QString>(str))
    {}

private:

};*/

struct Wallet : public Idable
{
    class Type
    {
    public:
        enum t {
            Common = 0,
            Cash,
            Card,
            Account,
            Deposit,
            EMoney,
        };

        static QString toString(Type::t type) {
            switch(type) {
                case Common: return QObject::tr("Общий");
                case Cash: return QObject::tr("Наличные");
                case Card: return QObject::tr("Карта");
                case Account: return QObject::tr("Счет");
                case Deposit: return QObject::tr("Вклад");
                case EMoney: return QObject::tr("Эл. деньги");
            }
        }

        static QVector<Type::t> enumerate() {
            return {Common, Cash, Card, Account, Deposit, EMoney};
        }
    };

    Wallet() {}
    Wallet(const QString &n)
        : name(n)
    {}
    Wallet(const QString &n, Money a)
        : name(n)
        , amount(a)
    {}

    bool operator==(const Wallet &other) const
    {
        return name == other.name
            && id == other.id
            && amount == other.amount
            && type == other.type
            && canBeNegative == other.canBeNegative
            && owners == other.owners;
    }

    Type::t type {Type::Common};
    QString name;
    QVector<const Owner *> owners;
    bool canBeNegative {false};
    Money amount;
};

Q_DECLARE_METATYPE(Node<Wallet> *)
Q_DECLARE_METATYPE(const Node<Wallet> *)

using Category = IdableString;

/**
 * @brief Archiveable node.
 * @details Wrapper around existing node OR archived nonexisting node.
 *
 * `Transaction` can refer to existent `Category`s or `Wallet`s. But sometimes
 * some old `Transaction`s may refer to objects that do not exist anymore,
 * because were removed later. Still, `Transaction` should somehow preserve
 * information about such nonexistent nodes for history and statistics purposes.
 *
 * `ArchNode<T>` wrapper allows to contain:
 * - valid `Node<T> *` which belongs to any `Category`s or `Wallet`s tree;
 * - `QString` representing former `Node<T>*` which does not exist anymore.
 */
template <class T>
class ArchNode : public QVariant
{
public:
    ArchNode()
        : QVariant()
    {}

    ArchNode(const QVariant &v)
        : QVariant(v)
    {}

    ArchNode(const Node<T> *node)
        : QVariant(QVariant::fromValue<const Node<T>*>(node))
    {}

    ArchNode(const QString &str)
        : QVariant(QVariant::fromValue<QString>(str))
    {}

    ArchNode &operator =(const Node<T> *node)
    {
        this->QVariant::operator =(QVariant::fromValue<const Node<T>*>(node));
        return *this;
    }

    ArchNode &operator =(const QString &str)
    {
        this->QVariant::operator =(str);
        return *this;
    }

    const Node<T> *toNode() const {
        return value<const Node<T>*>();
    }

    bool isValidPointer() const {
        return canConvert<const Node<T>*>();
    }

    bool isArchived() const {
        return !isValidPointer();
    }
};

struct Transaction
{
    class Type
    {
    public:
        enum t {
            In = 0,
            Out,
            Transfer,
        };

        static QString toString(Type::t type) {
            switch(type) {
                case In: return QObject::tr("Доход");
                default:
                case Out: return QObject::tr("Трата");
                case Transfer: return QObject::tr("Перемещение");
            }
        }

        static QVector<Type::t> enumerate() {
            return {In, Out, Transfer};
        }
    };

    QDate date;
    QString note;
    Type::t type {Type::Out};
    QVector< ArchNode<Category> > category;
    Money amount;
    ArchNode<Wallet> from { QVariant::fromValue<const Node<Wallet>*>(nullptr) };
    ArchNode<Wallet> to { QVariant::fromValue<const Node<Wallet>*>(nullptr) };
};

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

signals:
    void nodesGonnaBeRemoved(QStringList nodeIds);
};

class CategoriesModel : public TreeModel
{
    Q_OBJECT

public:
    Tree<Category> *rootItem;

    CategoriesModel(QObject *parent = 0);
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

signals:
    void nodesGonnaBeRemoved(QStringList nodeIds);
};

class LogModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    QVector<Transaction> log;

    LogModel(QObject *parent = 0);
    ~LogModel();

    int getTransactionIndex(const QModelIndex &index) const;
    int getTransactionIndex(int modelIndex) const;
    Transaction &getTransaction(const QModelIndex &index);
    const Transaction &getTransaction(const QModelIndex &index) const;

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

    bool insertRows(int position, int rows, const QModelIndex &parent = QModelIndex()) override;
    bool removeRows(int position, int rows, const QModelIndex &parent = QModelIndex()) override;
};

const QString pathConcat {"/"};

template <class T>
QString extractPathString(const Node<T> *node);

template <>
QString extractPathString<Category>(const Node<Category> *node);

template <>
QString extractPathString<Wallet>(const Node<Wallet> *node);

template <class T>
QString pathToString(const Node<T> *node)
{
    if(!node) {
        return "";
    }

    QStringList l;
    while(node->parent) {
        l.push_front(extractPathString(node));
        node = node->parent;
    }

    return l.join(pathConcat);
}

class Data : public QObject
{
    Q_OBJECT

public:
    Data();

    OwnersModel owners;
    WalletsModel wallets;
    CategoriesModel inCategories;
    CategoriesModel outCategories;
    LogModel log;

    Node<Wallet> *walletFromPath(const QString &path) {
        return nodeFromPath<Wallet, WalletsModel>(wallets, path);
    }

    Node<Category> *inCategoryFromPath(const QString &path) {
        return nodeFromPath<Category, CategoriesModel>(inCategories, path);
    }

    Node<Category> *outCategoryFromPath(const QString &path) {
        return nodeFromPath<Category, CategoriesModel>(outCategories, path);
    }

private:
    template<class T, class Model>
    Node<T> *nodeFromPath(const Model &model, const QString &path) {
        QStringList l = path.split(pathConcat, QString::KeepEmptyParts);
        Node<T> *node = model.rootItem;
        for(const auto &s : l) {
            bool found = false;
            for(const auto &child : node->children) {
                if(extractPathString(child) == s) {
                    node = child;
                    found = true;
                    break;
                }
            }
            if(!found) {
                return nullptr;
            }
        }
        return node;
    }

    void onOwnersRemove(QStringList paths);
    void onInCategoriesRemove(QStringList paths);
    void onOutCategoriesRemove(QStringList paths);
    void onWalletsRemove(QStringList paths);
};

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

#endif // BOOKKEEPING_H
