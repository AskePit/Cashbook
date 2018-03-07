#ifndef BOOKKEEPING_H
#define BOOKKEEPING_H

#include "models.h"

namespace cashbook
{

struct Plans {
    PlansModel shortPlans;
    PlansModel middlePlans;
    PlansModel longPlans;

    void clear() {
        shortPlans.clear();
        middlePlans.clear();
        longPlans.clear();
    }
};

struct Tasks {
    TasksModel active;
    TasksModel completed;

    void clear() {
        active.clear();
        completed.clear();
    }
};

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
    Plans plans;
    Tasks tasks;
    BriefModel briefModel;

    Statistics statistics;

    Node<Wallet> *walletFromPath(const QString &path);
    Node<Category> *inCategoryFromPath(const QString &path);
    Node<Category> *outCategoryFromPath(const QString &path);

    void loadCategoriesStatistics(const QDate &from, const QDate &to);
    void updateTasks();
    void updateTasks(TasksModel &tasksModel);
    bool anchoreTransactions();
    void clear();

signals:
    void categoriesStatisticsUpdated();

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

} // namespace cashbook

#endif // BOOKKEEPING_H
