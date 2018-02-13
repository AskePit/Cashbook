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

struct Month
{
    Month(const QDate &date);

    int year {0};
    int month {0};

    bool operator<(const Month &other);
    QString toString() const;

private:
    static QString monthToString(int m);
};

bool operator<(const Month &m1, const Month &m2);
bool operator>(const Month &m1, const Month &m2);

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
    void propagateMoney(const Node<Category> *node, const Money &amount) {
        while(node) {
            (*this)[node] += amount;
            node = node->parent;
        }
    }
};

struct Statistics {
    BriefStatistics brief;
    CategoryMoneyMap inCategories;
    CategoryMoneyMap outCategories;
};

} // namespace cashbook

#endif // BOOKKEEPING_STATISTICS_H
