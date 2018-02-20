#ifndef BOOKKEEPING_BASIC_TYPES_H
#define BOOKKEEPING_BASIC_TYPES_H

#include "common.h"
#include <std/tree.h>
#include <std/money.h>
#include <QString>
#include <QVector>
#include <QUuid>
#include <QDate>
#include <QVariant>

namespace cashbook
{

struct Idable
{
    QUuid id {QUuid::createUuid()};
};

class IdableString : public Idable, public QString
{
public:
    IdableString();
    IdableString(const char *str);
    IdableString(const QString &str);
};

Q_DECLARE_METATYPE(IdableString *)
Q_DECLARE_METATYPE(const IdableString *)
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
template <class T>
class ArchPointer : public QVariant
{
public:
    ArchPointer() // valid null pointer by default
        : QVariant(QVariant::fromValue<const T*>(nullptr))
    {}

    ArchPointer(const QVariant &v)
        : QVariant(v)
    {}

    ArchPointer(const T *node)
        : QVariant(QVariant::fromValue<const T*>(node))
    {}

    ArchPointer(const QString &str)
        : QVariant(QVariant::fromValue<QString>(str))
    {}

    ArchPointer &operator =(const T *node)
    {
        this->QVariant::operator =(QVariant::fromValue<const T*>(node));
        return *this;
    }

    ArchPointer &operator =(const QString &str)
    {
        this->QVariant::operator =(str);
        return *this;
    }

    const T *toPointer() const {
        return value<const T*>();
    }

    bool isValidPointer() const {
        return canConvert<const T*>();
    }

    bool isArchived() const {
        return !isValidPointer();
    }

    void setNull() {
        this->QVariant::operator =(QVariant::fromValue<const T*>(nullptr));
    }
};

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

        static QString toString(Type::t type);
        static QString toConfigString(Type::t type);
        static Type::t fromConfigString(const QString &str);
        static QVector<Type::t> enumerate();
    };

    Wallet();
    Wallet(const QString &n);
    Wallet(const QString &n, Money a);

    bool operator==(const Wallet &other) const;

    Type::t type {Type::Common};
    QString name;
    ArchPointer<Owner> owner;
    bool canBeNegative {false};
    Money amount;
};

Q_DECLARE_METATYPE(Node<Wallet> *)
Q_DECLARE_METATYPE(const Node<Wallet> *)

struct Category : public IdableString
{
    bool regular {false};

    Category() : IdableString() {}
    Category(const char *str) : IdableString(str) {}
    Category(const QString &str) : IdableString(str) {}
};

Q_DECLARE_METATYPE(Category *)
Q_DECLARE_METATYPE(const Category *)
Q_DECLARE_METATYPE(Node<Category> *)
Q_DECLARE_METATYPE(const Node<Category> *)

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
using ArchNode = ArchPointer<Node<T>>;

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

        static QString toString(Type::t type);
        static QString toConfigString(Type::t type);
        static Type::t fromConfigString(const QString &str);
        static QVector<Type::t> enumerate();
    };

    QDate date;
    QString note;
    Type::t type {Type::Out};
    ArchNode<Category> category;
    Money amount;
    ArchNode<Wallet> from;
    ArchNode<Wallet> to;
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

template <class T>
QString pathToShortString(const Node<T> *node)
{
    if(!node) {
        return "";
    }

    QString path = extractPathString(node);
    if(node && node->parent) {
        QString parentPath = extractPathString(node->parent);
        if(!parentPath.isEmpty()) {
            path += " (" + parentPath + ")";
        }
    }

    return path;
}

QString formatMoney(const Money &money);

} // namespace cashbook

#endif // BOOKKEEPING_BASIC_TYPES_H
