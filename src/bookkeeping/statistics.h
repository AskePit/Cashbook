#ifndef BOOKKEEPING_STATISTICS_H
#define BOOKKEEPING_STATISTICS_H

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

struct CategoryMoneyRow {
    QString name;
    Money amount;
};

struct Statistics {
    QVector<CategoryMoneyRow> topSpent;
    QVector<CategoryMoneyRow> topRegularSpent;
    QVector<CategoryMoneyRow> topIrregularSpent;
};

} // namespace cashbook

#endif // BOOKKEEPING_STATISTICS_H
