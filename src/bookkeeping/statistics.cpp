#include "statistics.h"

#include <QObject>
#include <QDate>

namespace cashbook
{

Month::Month(const QDate &date)
{
    year = date.year();
    month = date.month();
}

bool Month::operator<(const Month &other) {
    if(year < other.year) {
        return true;
    } else if(year > other.year) {
        return false;
    }

    return month < other.month;
}

QString Month::toString() const {
    return QString("%1 %2").arg(year).arg(monthToString(month));
}

QString Month::monthToString(int m) {
    switch(m) {
        default:
        case 1: return QObject::tr("янв");
        case 2: return QObject::tr("фев");
        case 3: return QObject::tr("мар");
        case 4: return QObject::tr("апр");
        case 5: return QObject::tr("май");
        case 6: return QObject::tr("июн");
        case 7: return QObject::tr("июл");
        case 8: return QObject::tr("авг");
        case 9: return QObject::tr("сен");
        case 10: return QObject::tr("окт");
        case 11: return QObject::tr("ноя");
        case 12: return QObject::tr("дек");
    }
}

bool operator<(const Month &m1, const Month &m2) {
    if(m1.year < m2.year) {
        return true;
    } else if(m1.year > m2.year) {
        return false;
    }

    return m1.month < m2.month;
}

bool operator>(const Month &m1, const Month &m2) {
    if(m1.year > m2.year) {
        return true;
    } else if(m1.year < m2.year) {
        return false;
    }

    return m1.month > m2.month;
}

} // namespace cashbook
