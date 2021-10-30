#ifndef BOOKKEEPING_H
#define BOOKKEEPING_H

#include "models.h"

namespace cashbook
{

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
    CategoryMoneyMap inCategories;
    CategoryMoneyMap outCategories;
    QDate categoriesFrom;
    QDate categoriesTo;
};

struct PlanTerm {
    enum t {
        Short = 0,
        Middle,
        Long,

        Count
    };

    static QVector<PlanTerm::t> enumerate() {
        return {Short, Middle, Long};
    }
};

struct Plans : public ChangeObservable {
    Plans() {
        setChangableItems({&m_shortPlans, &m_middlePlans, &m_longPlans});
    }

    const PlansModel &operator[](PlanTerm::t term) const {
        switch(term) {
            default:
            case PlanTerm::Short: return m_shortPlans;
            case PlanTerm::Middle: return m_middlePlans;
            case PlanTerm::Long: return m_longPlans;
        }
    }

    PlansModel &operator[](PlanTerm::t term) {
        const Plans *const_this = const_cast<const Plans *>(this);
        return const_cast<PlansModel &>( const_this->operator [](term) );
    }

    void clear() {
        m_shortPlans.clear();
        m_middlePlans.clear();
        m_longPlans.clear();
    }

private:
    PlansModel m_shortPlans;
    PlansModel m_middlePlans;
    PlansModel m_longPlans;
};

struct TaskStatus {
    enum t {
        Active = 0,
        Completed,

        Count
    };
};

struct Tasks : public ChangeObservable {
    Tasks(const LogModel &log)
        : m_active(log), m_completed(log)
    {
        setChangableItems({&m_active, &m_completed});
    }

    const TasksModel &operator[](TaskStatus::t status) const {
        switch(status) {
            default:
            case TaskStatus::Active: return m_active;
            case TaskStatus::Completed: return m_completed;
        }
    }

    TasksModel &operator[](TaskStatus::t status) {
        const Tasks *const_this = const_cast<const Tasks *>(this);
        return const_cast<TasksModel &>( const_this->operator [](status) );
    }

    void clear() {
        m_active.clear();
        m_completed.clear();
    }

private:
    TasksModel m_active;
    TasksModel m_completed;
};

class Data : public QObject, public ChangeObservable
{
    Q_OBJECT

public:
    Data();

    OwnersModel ownersModel;
    WalletsModel walletsModel;
    CategoriesModel inCategoriesModel;
    CategoriesModel outCategoriesModel;
    LogModel logModel;
    Plans plans;
    Tasks tasks;
    BriefModel briefModel;

    Statistics statistics;

    Node<Wallet> *walletFromPath(const QString &path);
    Node<Category> *inCategoryFromPath(const QString &path);
    Node<Category> *outCategoryFromPath(const QString &path);

    void loadCategoriesStatistics(const QDate &from, const QDate &to);
    void updateTasks();
    void updateTasks(TasksModel &tasks);
    bool anchoreTransactions();
    void clear();
    void importReceiptFile(const QString &json, const Node<Wallet> *wallet);

signals:
    void categoriesStatisticsUpdated();

private:
    template<class T, class Model>
    Node<T> *nodeFromPath(const Model &model, const QString &path) {
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

    void onOwnersRemove(QStringList paths);
    void onInCategoriesRemove(QStringList paths);
    void onOutCategoriesRemove(QStringList paths);
    void onWalletsRemove(QStringList paths);
};

} // namespace cashbook

#endif // BOOKKEEPING_H
