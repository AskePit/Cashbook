#ifndef BOOKKEEPING_STATISTICS_H
#define BOOKKEEPING_STATISTICS_H

#include "basic_types.h"
#include <QVector>
#include <std/money.h>
#include <map>
#include <functional>

class QDate;

namespace cashbook
{

class CategoryMoneyMap : public std::map<const Node<Category> *, Money>
{
public:
    void propagateMoney(const Node<Category> *node, const Money &amount);
};

struct Statistics {
    CategoryMoneyMap inCategories;
    CategoryMoneyMap outCategories;
    QDate categoriesFrom;
    QDate categoriesTo;
};

} // namespace cashbook

#endif // BOOKKEEPING_STATISTICS_H
