#ifndef BOOKKEEPING_BASIC_TYPES_H
#define BOOKKEEPING_BASIC_TYPES_H

#include <askelib/std/tree.h>
#include <askelib/std/money.h>
#include <QString>
#include <QVector>
#include <QUuid>
#include <QDate>
#include <QVariant>
#include <functional>
#include <vector>

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

    void setId(const QUuid &id);
    void setString(const QString &str);
};

using Owner = IdableString;
using Bank = IdableString;

class ArchiveString
{
public:
    explicit ArchiveString(const QString& str)
        : m_str(str)
    {}

    explicit ArchiveString(QString&& str)
        : m_str(std::move(str))
    {}

    const QString& get() const {
        return m_str;
    }

private:
    QString m_str;
};

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

    ArchPointer(const ArchiveString &str)
        : QVariant(QVariant::fromValue<QString>(str.get()))
    {}

    ArchPointer &operator =(const T *node)
    {
        this->QVariant::operator =(QVariant::fromValue<const T*>(node));
        return *this;
    }

    ArchPointer &operator =(const ArchiveString &str)
    {
        this->QVariant::operator =(str.get());
        return *this;
    }

    ArchPointer &operator =(ArchiveString &&str)
    {
        this->QVariant::operator =(std::move(str.get()));
        return *this;
    }

    const T *toPointer() const {
        return value<const T*>();
    }

    bool isValidPointer() const {
        return canConvert<const T*>();
    }

    bool isNullPointer() const {
        return isValidPointer() && toPointer() == nullptr;
    }

    bool isArchived() const {
        return !isValidPointer();
    }

    void setNull() {
        this->QVariant::operator =(QVariant::fromValue<const T*>(nullptr));
    }
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

const QString pathConcat {"/"};

template <class T>
QString extractPathString(const Node<T> *node);

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
QString formatPercent(double percent);

/**
 * @brief The AbstractChangable interface.
 * @details Interface for an object that has a "changed" state, which can be reseted.
 */
class AbstractChangable
{
public:
    virtual bool isChanged() const = 0;
    virtual void resetChanged() = 0;
};

/**
 * @brief The Changable class.
 * @details Represents spesific object that can be changed and can change itself.
 */
class Changable : public AbstractChangable
{
public:
    bool isChanged() const override {
        return m_changed;
    }

    void resetChanged() override {
        m_changed = false;
    }

    virtual void setChanged() {
        m_changed = true;
    }

    /**
     * Proxy function that sets object changed if `changed` is `true`
     * and returns `changed`.
     *
     * This function is usefull for oneliners which return result from function.
     */
    bool changeFilter(bool changed) {
        m_changed |= changed;
        return changed;
    }

private:
    bool m_changed;
};

/**
 * @brief The ChangeObservable class.
 * @details Represents object that contains or owns AbstractChangeable objects.
 *          If any of these objects are changed, this object is considered
 *          being changed too.
 */
class ChangeObservable : public AbstractChangable
{
public:
    void setChangableItems(std::vector<AbstractChangable*> items) {
        m_items = items;
    }

    void addChangableItem(AbstractChangable* item) {
        m_items.push_back(item);
    }

    std::vector<AbstractChangable*> &changableItems() {
        return m_items;
    }

    void removeChangableItem(AbstractChangable* toRemove) {
        m_items.erase( std::remove(m_items.begin(), m_items.end(), toRemove), m_items.end() );
    }

    bool isChanged() const final {
        for(AbstractChangable *item : m_items) {
            if(item->isChanged()) {
                return true;
            }
        }
        return false;
    }

    void resetChanged() final {
        for(AbstractChangable *item : m_items) {
            item->resetChanged();
        }
    }

protected:
    std::vector<AbstractChangable*> m_items;
};

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

static const QDate Today {QDate::currentDate()};
static const QDate MonthBegin(Today.year(), Today.month(), 1);

Q_DECLARE_METATYPE(cashbook::IdableString *)
Q_DECLARE_METATYPE(const cashbook::IdableString *)
Q_DECLARE_METATYPE(Node<cashbook::IdableString> *)
Q_DECLARE_METATYPE(const Node<cashbook::IdableString> *)

#endif // BOOKKEEPING_BASIC_TYPES_H
