#ifndef BOOKKEEPING_STATISTICS_H
#define BOOKKEEPING_STATISTICS_H

#include "basic_types.h"
#include <QVector>
#include <askelib_qt/askelib/std/money.h>
#include <map>
#include <functional>

class QDate;

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

using BriefStatistics = std::map<Month, BriefStatisticsRecord, std::greater<Month>>;

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

} // namespace cashbook

#endif // BOOKKEEPING_STATISTICS_H
