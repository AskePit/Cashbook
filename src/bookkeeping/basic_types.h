#ifndef BOOKKEEPING_BASIC_TYPES_H
#define BOOKKEEPING_BASIC_TYPES_H

#include "common.h"
#include <askelib_qt/askelib/std/tree.h>
#include <askelib_qt/askelib/std/money.h>
#include <QString>
#include <QVector>
#include <QUuid>
#include <QDate>
#include <QVariant>
#include <functional>

namespace cashbook
{

struct Month
{
    Month() = default;
    Month(const QDate &date);

    int year {0};
    int month {0};

    bool operator==(const Month &other) const;
    bool operator!=(const Month &other) const {
        return !(*this == other);
    }
    QString toString() const;
    QDate toDate() const;

private:
    static QString monthToString(int m);
};

bool operator<(const Month &m1, const Month &m2);
bool operator>(const Month &m1, const Month &m2);

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

struct Category : public IdableString
{
    bool regular {false};

    Category() : IdableString() {}
    Category(const char *str) : IdableString(str) {}
    Category(const QString &str) : IdableString(str) {}
};

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

struct Plan
{
    QString name;
    Transaction::Type::t type {Transaction::Type::Out};
    ArchNode<Category> category;
    Money amount;
};

struct Task
{
    Transaction::Type::t type {Transaction::Type::Out};
    ArchNode<Category> category;
    QDate from;
    QDate to;
    Money amount;
    Money spent;
    Money rest;
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

template <class T>
QString archNodeToString(ArchNode<T> arch)
{
    if(arch.isValidPointer()) {
        return pathToString(arch.toPointer());
    } else {
        return arch.toString();
    }
}

template <class T>
QString archNodeToShortString(ArchNode<T> arch)
{
    if(arch.isValidPointer()) {
        return pathToShortString(arch.toPointer());
    } else {
        return arch.toString();
    }
}

QString formatMoney(const Money &money, bool symbol = true);

} // namespace cashbook

namespace std
{
    template <class T>
    struct hash<cashbook::ArchNode<T>>
    {
        std::size_t operator()(const cashbook::ArchNode<T>& node) const
        {
            if(node.isValidPointer()) {
                return std::hash<const Node<T>*>()(node.toPointer());
            } else {
                return std::hash<std::string>()(node.toString().toStdString());
            }
        }
    };
}

Q_DECLARE_METATYPE(cashbook::IdableString *)
Q_DECLARE_METATYPE(const cashbook::IdableString *)
Q_DECLARE_METATYPE(Node<cashbook::IdableString> *)
Q_DECLARE_METATYPE(const Node<cashbook::IdableString> *)
Q_DECLARE_METATYPE(Node<cashbook::Wallet> *)
Q_DECLARE_METATYPE(const Node<cashbook::Wallet> *)
Q_DECLARE_METATYPE(cashbook::Category *)
Q_DECLARE_METATYPE(const cashbook::Category *)
Q_DECLARE_METATYPE(Node<cashbook::Category> *)
Q_DECLARE_METATYPE(const Node<cashbook::Category> *)
Q_DECLARE_METATYPE(cashbook::Transaction::Type::t)

#endif // BOOKKEEPING_BASIC_TYPES_H
