#include "bookkeeping.h"

namespace cashbook
{

Data::Data()
    : inCategories(statistics.received)
    , outCategories(statistics.spent)
    , log(statistics.brief)
{
    connect(&owners, &OwnersModel::nodesGonnaBeRemoved, this, &Data::onOwnersRemove);
    connect(&inCategories, &CategoriesModel::nodesGonnaBeRemoved, this, &Data::onInCategoriesRemove);
    connect(&outCategories, &CategoriesModel::nodesGonnaBeRemoved, this, &Data::onOutCategoriesRemove);
    connect(&wallets, &WalletsModel::nodesGonnaBeRemoved, this, &Data::onWalletsRemove);
}

void Data::onOwnersRemove(QStringList paths)
{
    auto nodes = wallets.rootItem->toList();
    for(auto *node : nodes) {
        for(ArchPointer<Owner> &owner : node->data.owners) {
            if(owner.isValidPointer()) {
                QString name = *owner.toPointer();
                if(paths.contains(name)) {
                    owner = name; // invalidate ArchPointer by assigning QString to it.
                }
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

        if(t.category.empty()) {
            continue;
        }

        invalidateArchNode(t.category.first(), paths);
    }
}

void Data::onOutCategoriesRemove(QStringList paths)
{
    for(Transaction &t : log.log) {
        if(t.type != Transaction::Type::Out) {
            continue;
        }

        if(t.category.empty()) {
            continue;
        }

        invalidateArchNode(t.category.first(), paths);
    }
}

void Data::onWalletsRemove(QStringList paths)
{
    for(Transaction &t : log.log) {
        invalidateArchNode(t.from, paths);
        invalidateArchNode(t.to, paths);
    }
}

/*static void fillStatisticsVector(const std::map<const Node<Category>*, Money> &from, QVector<MoneyTag> &to)
{
    for(const auto &p : from) {
        MoneyTag row;
        row.node = p.first;
        row.amount = p.second;
        to.push_back(row);
    }

    auto cmp = [](const MoneyTag &a, const MoneyTag &b)
    {
         return a.amount > b.amount;
    };

    std::sort(to.begin(), to.end(), cmp);
}*/

void Data::loadStatistics()
{
    statistics.spent.clear();
    statistics.received.clear();

    /*std::map<const Node<Category>*, Money> topSpent;
    std::map<const Node<Category>*, Money> topRegularSpent;
    std::map<const Node<Category>*, Money> topIrregularSpent;

    std::map<const Node<Category>*, Money> topReceived;
    std::map<const Node<Category>*, Money> topRegularReceived;
    std::map<const Node<Category>*, Money> topIrregularReceived;*/

    if(log.log.empty()) {
        return;
    }

    int month = log.log[0].date.month();

    int i = 0;
    while(i < log.log.size()) {
        const Transaction &t = log.log[i++];
        if(t.date.month() != month) {
            break;
        }

        if(t.type == Transaction::Type::Out) {
            if(!t.category.empty()) {
                const ArchNode<Category> &archNode = t.category.first();
                if(archNode.isValidPointer()) {
                    const Node<Category> *node = archNode.toPointer();
                    while(node) {
                        statistics.spent.top[node] += t.amount;

                        if(node->data.regular) {
                            statistics.spent.topRegular[node] += t.amount;
                        } else {
                            statistics.spent.topIrregular[node] += t.amount;
                        }
                        node = node->parent;
                    }
                }
            }
        }

        if(t.type == Transaction::Type::In) {
            if(!t.category.empty()) {
                const ArchNode<Category> &archNode = t.category.first();
                if(archNode.isValidPointer()) {
                    const Node<Category> *node = archNode.toPointer();
                    while(node) {
                        statistics.received.top[node] += t.amount;

                        if(node->data.regular) {
                            statistics.received.topRegular[node] += t.amount;
                        } else {
                            statistics.received.topIrregular[node] += t.amount;
                        }
                        node = node->parent;
                    }
                }
            }
        }
    }

    /*fillStatisticsVector(topSpent, statistics.spent.top);
    fillStatisticsVector(topRegularSpent, statistics.spent.topRegular);
    fillStatisticsVector(topIrregularSpent, statistics.spent.topIrregular);
    fillStatisticsVector(topReceived, statistics.received.top);
    fillStatisticsVector(topRegularReceived, statistics.received.topRegular);
    fillStatisticsVector(topIrregularReceived, statistics.received.topIrregular);*/
}

bool Data::anchoreTransactions()
{
    bool did = log.anchoreTransactions();
    if(did) {
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
}

} // namespace cashbook
