#include "bookkeeping.h"

namespace cashbook
{

Data::Data()
    : inCategoriesModel(statistics.inCategories)
    , outCategoriesModel(statistics.outCategories)
    , logModel(statistics)
    , tasks(logModel)
    , briefModel(statistics.brief)
{
    connect(&ownersModel, &OwnersModel::nodesGonnaBeRemoved, this, &Data::onOwnersRemove);
    connect(&inCategoriesModel, &CategoriesModel::nodesGonnaBeRemoved, this, &Data::onInCategoriesRemove);
    connect(&outCategoriesModel, &CategoriesModel::nodesGonnaBeRemoved, this, &Data::onOutCategoriesRemove);
    connect(&walletsModel, &WalletsModel::nodesGonnaBeRemoved, this, &Data::onWalletsRemove);

    connect(this, &Data::categoriesStatisticsUpdated, [this](){
        updateTasks();
    });
}

void Data::onOwnersRemove(QStringList paths)
{
    auto nodes = walletsModel.rootItem->toList();
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
    for(Transaction &t : logModel.log) {
        if(t.type != Transaction::Type::In) {
            continue;
        }

        invalidateArchNode(t.category, paths);
    }
}

void Data::onOutCategoriesRemove(QStringList paths)
{
    for(Transaction &t : logModel.log) {
        if(t.type != Transaction::Type::Out) {
            continue;
        }

        invalidateArchNode(t.category, paths);
    }
}

void Data::onWalletsRemove(QStringList paths)
{
    for(Transaction &t : logModel.log) {
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

    if(logModel.log.empty()) {
        return;
    }

    const QDate &logBegin = logModel.log.at(logModel.log.size()-1).date;
    const QDate &logEnd = logModel.log.at(0).date;

    if(to < logBegin || from > logEnd) {
        return;
    }

    size_t i = 0;
    while(i < logModel.log.size()) {
        const Transaction &t = logModel.log[i++];
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
        logModel.updateTask(task);
    }
}

bool Data::anchoreTransactions()
{
    bool did = logModel.anchoreTransactions();
    if(did) {
        emit categoriesStatisticsUpdated();
        walletsModel.update();
    }

    return did;
}

Node<Wallet> *Data::walletFromPath(const QString &path) {
    return nodeFromPath<Wallet, WalletsModel>(walletsModel, path);
}

Node<Category> *Data::inCategoryFromPath(const QString &path) {
    return nodeFromPath<Category, CategoriesModel>(inCategoriesModel, path);
}

Node<Category> *Data::outCategoryFromPath(const QString &path) {
    return nodeFromPath<Category, CategoriesModel>(outCategoriesModel, path);
}

void Data::clear() {
    ownersModel.clear();
    walletsModel.clear();
    inCategoriesModel.clear();
    outCategoriesModel.clear();
    logModel.clear();
    plans.clear();
    tasks.clear();
}

} // namespace cashbook
