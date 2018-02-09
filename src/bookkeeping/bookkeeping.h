#ifndef BOOKKEEPING_H
#define BOOKKEEPING_H

#include "models.h"
#include "statistics.h"

namespace cashbook
{

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

    BriefStatistics briefStatistics;

    Node<Wallet> *walletFromPath(const QString &path);
    Node<Category> *inCategoryFromPath(const QString &path);
    Node<Category> *outCategoryFromPath(const QString &path);

    bool anchoreTransactions();
    void clear();

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
