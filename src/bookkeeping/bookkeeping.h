#ifndef BOOKKEEPING_H
#define BOOKKEEPING_H

#include "basic_types.h"
#include <set>
#include <deque>

namespace cashbook
{

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
            Investment,
            CryptoCurrency, // as usual wallet, not investment
            Points,

            Count
        };

        static QString toString(Type::t type);
        static QString toConfigString(Type::t type);
        static Type::t fromConfigString(const QString &str);
        static constexpr std::array<Type::t, Type::Count> enumerate() {
            return {Common, Cash, Card, Account, Deposit, Investment, CryptoCurrency, Points};
        }
    };

    class Availability
    {
    public:
        enum t {
            Free = 0,
            InAMonth,
            InAQarter,
            InAYear,
            In3Years,
            In5Years,
            In10Years,
            In20Years,
            AfterRetirement,
            ForFutureGenerations,

            Count
        };

        static QString toString(Availability::t type);
        static QString toConfigString(Availability::t type);
        static Availability::t fromConfigString(const QString &str);
        static constexpr std::array<Availability::t, Availability::Count> enumerate() {
            return { Free,
                     InAMonth,
                     InAQarter,
                     InAYear,
                     In3Years,
                     In5Years,
                     In10Years,
                     In20Years,
                     AfterRetirement,
                     ForFutureGenerations
            };
        }
    };

    struct Info
    {
        virtual ~Info() {}

        ArchPointer<Owner> owner;
        bool canBeNegative {false};
        Availability::t availability {Availability::Free};
    };

    struct CashInfo : public Info
    {
    };

    struct AccountInfo : public Info
    {
        ArchPointer<Bank> bank;
    };

    struct CardInfo : public AccountInfo
    {
    };

    struct DepositInfo : public AccountInfo
    {
        float incomePercent {0.0f};
    };

    struct InvestmentInfo : public Info
    {
        class Type
        {
        public:
            enum t {
                Common = 0,
                Currency,
                Stocks,
                Metals,
                CryptoCurrency, // as investment, not usual wallet
                RealEstate,

                Count
            };

            static QString toString(Type::t type);
            static QString toConfigString(Type::t type);
            static Type::t fromConfigString(const QString &str);
            static constexpr std::array<Type::t, Type::Count> enumerate() {
                return {Common, Currency, Stocks, Metals, CryptoCurrency, RealEstate};
            }
        };

        Type::t type;
        std::optional<AccountInfo> account;
    };

    struct CryptoCurrencyInfo : public Info
    {
    };

    struct PointsInfo : public Info
    {
    };

    Wallet();
    Wallet(const QString &n);
    Wallet(const QString &n, Money a);

    bool operator==(const Wallet &other) const;

    Type::t type {Type::Common};
    QString name;
    Money amount;

    std::shared_ptr<Info> info {std::make_shared<Info>()};
};

struct Category : public IdableString
{
    bool regular {false};

    Category() : IdableString() {}
    Category(const char *str) : IdableString(str) {}
    Category(const QString &str) : IdableString(str) {}

    void setName(const QString &name) {
        IdableString::setString(name);
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

            Count
        };

        static QString toString(Type::t type);
        static QString toConfigString(Type::t type);
        static Type::t fromConfigString(const QString &str);
        static constexpr std::array<Type::t, Type::Count> enumerate() {
            return {In, Out, Transfer};
        }
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

template <>
QString extractPathString<Category>(const Node<Category> *node);

template <>
QString extractPathString<Wallet>(const Node<Wallet> *node);

struct SpentReceived
{
    Money spent;
    Money received;
};

struct BriefStatisticsRecord
{
    SpentReceived common;
    SpentReceived regular;
};

class BriefStatistics : public std::map<Month, BriefStatisticsRecord, std::greater<Month>>
{};

class CategoryMoneyMap : public std::map<const Node<Category> *, Money>
{
public:
    void propagateMoney(const Node<Category> *node, const Money &amount);
};

struct Statistics {
    BriefStatistics brief;
};

struct PlanTerm {
    enum t {
        Short = 0,
        Middle,
        Long,

        Count
    };

    static constexpr std::array<PlanTerm::t, PlanTerm::Count> enumerate() {
        return {Short, Middle, Long};
    }
};

struct TaskStatus {
    enum t {
        Active = 0,
        Completed,

        Count
    };
};

class OwnersData : public Changable
{
public:

    QVector<Owner> owners;
};

class BanksData : public Changable
{
public:

    QVector<Bank> banks;
};

class WalletsData : public Changable
{
public:

    Tree<Wallet> *rootItem {nullptr};
};

class CategoriesData : public Changable
{
public:

    Tree<Category> *rootItem {nullptr};
};

class LogData : public Changable
{
public:

    LogData(Statistics &statistics)
        : statistics(statistics)
    {}

    void insertRow(int position);
    void insertRows(int position, size_t rows);
    bool copyTop();

    bool canAnchore() const;
    bool anchoreTransactions();
    void appendTransactions(const std::vector<Transaction> &transactions);

    void updateNote(size_t row, const QString &note);
    void updateTask(Task &task) const;
    bool normalizeData();

    std::deque<Transaction> log;
    Statistics &statistics;
    int unanchored {0};
    std::set<Month> changedMonths;
};

class PlansTermData : public Changable
{
public:

    QVector<Plan> plans;
};

class PlansData : public ChangeObservable
{
public:

    PlansData() {
        setChangableItems({
            &shortTerm,
            &middleTerm,
            &longTerm
        });
    }

    PlansTermData shortTerm;
    PlansTermData middleTerm;
    PlansTermData longTerm;
};

class TasksListsData : public Changable
{
public:

    QVector<Task> tasks;
};

class TasksData : public ChangeObservable
{
public:

    TasksData() {
        setChangableItems({
            &active,
            &completed
        });
    }

    TasksListsData active;
    TasksListsData completed;
};

class Data : public QObject, public ChangeObservable
{
    Q_OBJECT
public:
    Data();

    OwnersData owners;
    BanksData banks;
    WalletsData wallets;
    CategoriesData inCategories;
    CategoriesData outCategories;
    LogData log;
    PlansData plans;
    TasksData tasks;

    Statistics statistics;

    Node<Wallet> *walletFromPath(const QString &path);
    Node<Category> *inCategoryFromPath(const QString &path);
    Node<Category> *outCategoryFromPath(const QString &path);

    void updateTasks();
    void updateTasks(TasksListsData &tasks);
    bool anchoreTransactions();
    void clear();
    void importRusReceiptFile(const QString &json, const Node<Wallet> *wallet);
    void importSerbReceiptFile(QStringView receipt, const Node<Wallet> *wallet);

    void onOwnersRemove(QStringList paths);
    void onInCategoriesRemove(QStringList paths);
    void onOutCategoriesRemove(QStringList paths);
    void onWalletsRemove(QStringList paths);

signals:
    void categoriesStatisticsUpdated();

private:
    template<class T, class Model>
    Node<T> *nodeFromPath(const Model &model, const QString &path);
};

template<class T, class Model>
Node<T> *Data::nodeFromPath(const Model &model, const QString &path) {
    QStringList l = path.split(pathConcat, Qt::KeepEmptyParts);
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

} // namespace cashbook

Q_DECLARE_METATYPE(Node<cashbook::Wallet> *)
Q_DECLARE_METATYPE(const Node<cashbook::Wallet> *)
Q_DECLARE_METATYPE(cashbook::Category *)
Q_DECLARE_METATYPE(const cashbook::Category *)
Q_DECLARE_METATYPE(Node<cashbook::Category> *)
Q_DECLARE_METATYPE(const Node<cashbook::Category> *)
Q_DECLARE_METATYPE(cashbook::Transaction::Type::t)

#endif // BOOKKEEPING_H
