#include "basic_types.h"

namespace cashbook
{

Month::Month(const QDate &date)
{
    year = date.year();
    month = date.month();
}

/*bool Month::operator<(const Month &other) const {
    if(year < other.year) {
        return true;
    } else if(year > other.year) {
        return false;
    }

    return month < other.month;
}*/

bool Month::operator==(const Month &other) const {
    return year == other.year && month == other.month;
}

QString Month::toString() const {
    return QString("%1 %2").arg(year).arg(monthToString(month));
}

QDate Month::toDate() const {
    return QDate(year, month, 1);
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

IdableString::IdableString()
    : QString()
{}

IdableString::IdableString(const char *str)
    : QString(str)
{}

IdableString::IdableString(const QString &str)
    : QString(str)
{}

void IdableString::setId(const QUuid &uid) {
    id = uid;
}

void IdableString::setString(const QString &str) {
    QString::operator =(str);
}

static QString getCurrencySymbol(Currency::t type)
{
    switch(type) {
        default: return "";
        case Currency::Rub: return "₽";
        case Currency::Usd: return "$";
        case Currency::Eur: return "€";
        case Currency::Gbp: return "£";
        case Currency::Jpy: return "¥";
        case Currency::Btc: return "฿";
    }
}

QString formatMoney(const Money &money, bool symbol /*= true*/)
{
    QString units = QString::number(money.units());

    int start = units.size()%3;
    int n = units.size()/3;

    if(start == 0) {
        start += 3;
        n -= 1;
    }

    for(int i = 0; i<n; ++i) {
        int index = start + 3*i + i;
        units.insert(index, ' ');
    }

    QString symbolSign = symbol ? getCurrencySymbol(money.currency()) : "";

    if(money.cents()) {
        QString cents = QString::number(money.cents());
        if(cents.startsWith('-')) {
            cents = cents.mid(1);
        }
        if(cents.size() == 1) {
            cents = QStringLiteral("0") + cents;
        }
        return symbol ? QString("%1,%2 %3").arg(units, cents, symbolSign) : QString("%1,%2").arg(units, cents);
    } else {
        return symbol ? QString("%1 %2").arg(units, symbolSign) : QString("%1").arg(units);
    }
}

QString formatPercent(double percent)
{
    QString percentStr = QString::number(percent, 'f', 2);
    while(percentStr.size()) {
        const bool noDot = percentStr.indexOf('.') == -1;
        const bool noComma = percentStr.indexOf(',') == -1;
        if(noDot && noComma) {
            break;
        }

        QChar ch = percentStr[percentStr.size() - 1];
        if(ch == '0' || ch == '.' || ch == ',') {
            percentStr.chop(1);
        } else {
            break;
        }
    }

    return percentStr;
}

} // namespace cashbook
