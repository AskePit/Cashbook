#ifndef BOOKKEEPING_H
#define BOOKKEEPING_H

#include <std/tree.h>
#include <std/money.h>
#include <QString>
#include <QSet>
#include <QVector>
#include <QUuid>
#include <QDate>
#include <QObject>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>

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

Q_DECLARE_METATYPE(IdableString)

using Owner = IdableString;

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

    Type::t type {Type::Common};
    QString name;
    QSet<Owner *> owners;
    bool canBeNegative {false};
    Money amount;
};

using Category = IdableString;

struct Transaction
{
    QDate date;
    QString note;
    QSet<Category *> category;
    Money amount;
    Wallet *from {nullptr};
    Wallet *to {nullptr};
    QSet<Owner *> whoDid;
    QSet<Owner *> forWhom;
};

class CategoriesModel : public QAbstractItemModel
{
    Q_OBJECT

public:
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

private:
    Tree<Category> *rootItem;
};

class WalletsModel : public QAbstractItemModel
{
    Q_OBJECT

public:
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

private:
    Tree<Wallet> *rootItem;
};

struct Data
{
    QVector<Owner> owners;
    WalletsModel wallets;
    CategoriesModel categories;
    QVector<Transaction> log;
};

template <class T>
QString pathToString(Node<T> *node);

} // namespace cashbook

#endif // BOOKKEEPING_H
