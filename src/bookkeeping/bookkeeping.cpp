#include "bookkeeping.h"

namespace cashbook
{

Data::Data()
    : inCategories(statistics.inCategories)
    , outCategories(statistics.outCategories)
    , log(statistics)
    , tasks(log)
    , briefModel(statistics.brief)
{
    connect(&owners, &OwnersModel::nodesGonnaBeRemoved, this, &Data::onOwnersRemove);
    connect(&inCategories, &CategoriesModel::nodesGonnaBeRemoved, this, &Data::onInCategoriesRemove);
    connect(&outCategories, &CategoriesModel::nodesGonnaBeRemoved, this, &Data::onOutCategoriesRemove);
    connect(&wallets, &WalletsModel::nodesGonnaBeRemoved, this, &Data::onWalletsRemove);

    connect(this, &Data::categoriesStatisticsUpdated, [this](){
        updateTasks();
    });
}

void Data::onOwnersRemove(QStringList paths)
{
    auto nodes = wallets.rootItem->toList();
    for(auto *node : nodes) {
        ArchPointer<Owner> &owner = node->data.owner;
        if(owner.isValidPointer()) {
            QString name = *owner.toPointer();
            if(paths.contains(name)) {
                owner = name; // invalidate ArchPointer by assigning QString to it.
            }
        }
    }
}

template <class DataType>
static void invalidateArchNode(ArchNode<DataType> &archNode, const QStringList &paths)
{
    if(archNode.isValidPointer()) {
        QString path = pathToString(archNode.toPointer());
        if(paths.contains(path)) {
            archNode = path; // invalidate ArchPointer by assigning QString to it.
        }
    }
}

void Data::onInCategoriesRemove(QStringList paths)
{
    for(Transaction &t : log.log) {
        if(t.type != Transaction::Type::In) {
            continue;
        }

        invalidateArchNode(t.category, paths);
    }
}

void Data::onOutCategoriesRemove(QStringList paths)
{
    for(Transaction &t : log.log) {
        if(t.type != Transaction::Type::Out) {
            continue;
        }

        invalidateArchNode(t.category, paths);
    }
}

void Data::onWalletsRemove(QStringList paths)
{
    for(Transaction &t : log.log) {
        invalidateArchNode(t.from, paths);
        invalidateArchNode(t.to, paths);
    }
}

void Data::loadCategoriesStatistics(const QDate &from, const QDate &to)
{
    statistics.outCategories.clear();
    statistics.inCategories.clear();

    statistics.categoriesFrom = from;
    statistics.categoriesTo = to;

    if(log.log.empty()) {
        return;
    }

    const QDate &logBegin = log.log.at(log.log.size()-1).date;
    const QDate &logEnd = log.log.at(0).date;

    if(to < logBegin || from > logEnd) {
        return;
    }

    size_t i = 0;
    while(i < log.log.size()) {
        const Transaction &t = log.log[i++];
        if(t.date > statistics.categoriesTo) {
            continue;
        }

        if(t.date < statistics.categoriesFrom) {
            break;
        }

        if(t.type == Transaction::Type::Out) {
            const ArchNode<Category> &archNode = t.category;
            if(archNode.isValidPointer()) {
                const Node<Category> *node = archNode.toPointer();
                statistics.outCategories.propagateMoney(node, t.amount);
            }
        }

        if(t.type == Transaction::Type::In) {
            const ArchNode<Category> &archNode = t.category;
            if(archNode.isValidPointer()) {
                const Node<Category> *node = archNode.toPointer();
                statistics.inCategories.propagateMoney(node, t.amount);
            }
        }
    }

    emit categoriesStatisticsUpdated();
}

void Data::updateTasks()
{
    updateTasks(tasks[TaskStatus::Active]);
    updateTasks(tasks[TaskStatus::Completed]);
}

void Data::updateTasks(TasksModel &tasksModel)
{
    for(Task &task : tasksModel.tasks) {
        log.updateTask(task);
    }
}

bool Data::anchoreTransactions()
{
    bool did = log.anchoreTransactions();
    if(did) {
        emit categoriesStatisticsUpdated();
        wallets.update();
    }

    return did;
}

Node<Wallet> *Data::walletFromPath(const QString &path) {
    return nodeFromPath<Wallet, WalletsModel>(wallets, path);
}

Node<Category> *Data::inCategoryFromPath(const QString &path) {
    return nodeFromPath<Category, CategoriesModel>(inCategories, path);
}

Node<Category> *Data::outCategoryFromPath(const QString &path) {
    return nodeFromPath<Category, CategoriesModel>(outCategories, path);
}

void Data::clear() {
    owners.clear();
    wallets.clear();
    inCategories.clear();
    outCategories.clear();
    log.clear();
    plans.clear();
    tasks.clear();
}

} // namespace cashbook
