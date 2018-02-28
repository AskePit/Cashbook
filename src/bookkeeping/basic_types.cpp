#include "basic_types.h"
#include "common.h"

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


QString Wallet::Type::toString(Type::t type) {
    switch(type) {
        default:
        case Common: return QObject::tr("Общий");
        case Cash: return QObject::tr("Наличные");
        case Card: return QObject::tr("Карта");
        case Account: return QObject::tr("Счет");
        case Deposit: return QObject::tr("Вклад");
        case EMoney: return QObject::tr("Эл. деньги");
    }
}

QString Wallet::Type::toConfigString(Type::t type) {
    switch(type) {
        default:
        case Common: return QStringLiteral("Common");
        case Cash: return QStringLiteral("Cash");
        case Card: return QStringLiteral("Card");
        case Account: return QStringLiteral("Account");
        case Deposit: return QStringLiteral("Deposit");
        case EMoney: return QStringLiteral("EMoney");
    }
}

Wallet::Type::t Wallet::Type::fromConfigString(const QString &str) {
    if(str == "Common") {
        return Common;
    } else if(str == "Cash") {
        return Cash;
    } else if(str == "Card") {
        return Card;
    } else if(str == "Account") {
        return Account;
    } else if(str == "Deposit") {
        return Deposit;
    } else if(str == "EMoney") {
        return EMoney;
    } else {
        return Common;
    }
}

QVector<Wallet::Type::t> Wallet::Type::enumerate() {
    return {Common, Cash, Card, Account, Deposit, EMoney};
}

Wallet::Wallet()
{}

Wallet::Wallet(const QString &n)
    : name(n)
{}

Wallet::Wallet(const QString &n, Money a)
    : name(n)
    , amount(a)
{}

bool Wallet::operator==(const Wallet &other) const
{
    return name == other.name
        && id == other.id
        && amount == other.amount
        && type == other.type
        && canBeNegative == other.canBeNegative
        && owner == other.owner;
}

QString Transaction::Type::toString(Type::t type) {
    switch(type) {
        case In: return QObject::tr("Доход");
        default:
        case Out: return QObject::tr("Трата");
        case Transfer: return QObject::tr("Перевод");
    }
}

QString Transaction::Type::toConfigString(Type::t type) {
    switch(type) {
        case In: return QStringLiteral("In");
        default:
        case Out: return QStringLiteral("Out");
        case Transfer: return QStringLiteral("Transfer");
    }
}

Transaction::Type::t Transaction::Type::fromConfigString(const QString &str) {
    if(str == "In") {
        return In;
    } else if(str == "Out") {
        return Out;
    } else if(str == "Transfer") {
        return Transfer;
    } else {
        return Out;
    }
}

QVector<Transaction::Type::t> Transaction::Type::enumerate() {
    return {In, Out, Transfer};
}

template <>
QString extractPathString<Category>(const Node<Category> *node) {
    return node ? node->data : "";
}

template <>
QString extractPathString<Wallet>(const Node<Wallet> *node) {
    return node ? node->data.name : "";
}

QString formatMoney(const Money &money)
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

    QString symbol = Currency::symbol(money.currency());

    if(money.cents()) {
        QString cents = QString::number(money.cents());
        if(cents.startsWith('-')) {
            cents = cents.mid(1);
        }
        if(cents.size() == 1) {
            cents = QStringLiteral("0") + cents;
        }
        return QString("%1,%2 %3").arg(units, cents, symbol);
    } else {
        return QString("%1 %2").arg(units, symbol);
    }
}

} // namespace cashbook
