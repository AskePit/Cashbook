#include "bookkeeping.h"

namespace cashbook
{

Data::Data()
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

static void fillStatisticsVector(const std::map<QString, Money> &from, QVector<CategoryMoneyRow> &to)
{
    for(const auto &p : from) {
        CategoryMoneyRow row;
        row.name = p.first;
        row.amount = p.second;
        to.push_back(row);
    }

    auto cmp = [](const CategoryMoneyRow &a, const CategoryMoneyRow &b)
    {
         return a.amount > b.amount;
    };

    std::sort(to.begin(), to.end(), cmp);
}

void Data::loadStatistics()
{
    statistics.topSpent.clear();
    statistics.topRegularSpent.clear();
    statistics.topIrregularSpent.clear();

    std::map<QString, Money> topSpent;
    std::map<QString, Money> topRegularSpent;
    std::map<QString, Money> topIrregularSpent;

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
            if(!t.category.empty() && t.category.first().isValidPointer()) {
                const ArchNode<Category> &archNode = t.category.first();
                if(archNode.isValidPointer()) {
                    const Node<Category> *node = archNode.toPointer();
                    while(node) {
                        QString path = pathToShortString(node);
                        topSpent[path] += t.amount;

                        if(node->data.regular) {
                            topRegularSpent[path] += t.amount;
                        } else {
                            topIrregularSpent[path] += t.amount;
                        }
                        node = node->parent;
                    }

                }
            }
        }
    }

    fillStatisticsVector(topSpent, statistics.topSpent);
    fillStatisticsVector(topRegularSpent, statistics.topRegularSpent);
    fillStatisticsVector(topIrregularSpent, statistics.topIrregularSpent);
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
