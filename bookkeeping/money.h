/*! @file
 *
 * Simple money type.
 *
 */

#ifndef ASKELIB_STD_DECIMAL_H
#define ASKELIB_STD_DECIMAL_H

#include <stdint.h>
#include <cassert>
#include <cmath>
#include <utility>
#include <map>

class Currency
{
public:

    enum t {
        Default = 0,

        Rub = Default,
        Usd,
        Eur,
        Gbp,
        Jpy,
        Btc,

        UserDefined = 1024
    };

    static double weight(Currency::t currency);
    static void setWeight(Currency::t currency, double weight);

private:
    static std::map<Currency::t, double> weights;
};

class Money {
public:
    using money_pair = std::pair<intmax_t, intmax_t>;

    // creation
    Money()
    {}

    explicit Money(intmax_t units, intmax_t cents)
        : m_amount(units*m_centsPerUnit + cents)
    {}

    explicit Money(intmax_t cents)
        : m_amount(cents)
    {}

    Money(double value, Currency::t curr = Currency::Default)
        : m_currency(curr)
    {
        double cents = value*m_centsPerUnit;
        cents = std::round(cents*m_centsPerUnit)/m_centsPerUnit; // round hack
        m_amount = intmax_t(cents);
    }

    friend void swap(Money& first, Money& second) // nothrow
    {
        using std::swap;

        swap(first.m_amount, second.m_amount);
        swap(first.m_currency, second.m_currency);
        swap(first.m_centsPerUnit, second.m_centsPerUnit);
    }

    Money(const Money &other)
        : m_amount(other.m_amount)
        , m_currency(other.m_currency)
        , m_centsPerUnit(other.m_centsPerUnit)
    {}

    Money(Money &&other)
        : Money()
    {
        swap(*this, other);
    }

    Money &operator=(Money other)
    {
        swap(*this, other);
        return *this;
    }

    // casts
    explicit operator double() const
    {
        return m_amount/(double)m_centsPerUnit;
    }

    intmax_t units() const
    {
        return m_amount/m_centsPerUnit;
    }

    intmax_t cents() const
    {
        return m_amount%m_centsPerUnit;
    }

    intmax_t as_cents() const
    {
        return m_amount;
    }

    int centsPerUnit() const
    {
        return m_centsPerUnit;
    }

    void setCentsPerUnit(int centsPerUnit)
    {
        m_centsPerUnit = centsPerUnit;
    }

    money_pair units_cents() const
    {
        return {units(), cents()};
    }

    Currency::t currency() const
    {
        return m_currency;
    }

    bool isZero() const
    {
        return m_amount == 0;
    }

    void setCurrency(Currency::t currency)
    {
        m_currency = currency;
    }

    void convertTo(Currency::t currency) {
        double ratio = Currency::weight(currency) / Currency::weight(m_currency);
        m_amount = static_cast<intmax_t>(static_cast<double>(m_amount) * ratio);
        m_currency = currency;
    }

    Money convertToCopy(Currency::t currency) const {
        Money money(*this);
        money.convertTo(currency);
        return money;
    }

    // arithmetic
    Money &operator+=(const Money &other);
    Money &operator-=(const Money &other);
    Money &operator*=(double mult) { m_amount = intmax_t(m_amount*mult); return *this; }
    Money &operator/=(double div) { m_amount = intmax_t(m_amount/div); return *this; }
    Money &operator%=(intmax_t div) { m_amount %= div; return *this; }

    bool operator==(const Money &other) const
    {
        return m_amount == other.m_amount
            && m_currency == other.m_currency
            && m_centsPerUnit == other.m_centsPerUnit;
    }
    bool operator!=(const Money &other) const { return !((*this) == other); }
    bool operator< (const Money &other) const { return m_amount <  other.m_amount; }
    bool operator<=(const Money &other) const { return m_amount <= other.m_amount; }
    bool operator> (const Money &other) const { return m_amount >  other.m_amount; }
    bool operator>=(const Money &other) const { return m_amount >= other.m_amount; }

    bool operator==(double other) const { return (double)m_amount == other; }
    bool operator!=(double other) const { return (double)m_amount != other; }
    bool operator< (double other) const { return (double)m_amount <  other; }
    bool operator<=(double other) const { return (double)m_amount <= other; }
    bool operator> (double other) const { return (double)m_amount >  other; }
    bool operator>=(double other) const { return (double)m_amount >= other; }

private:
    intmax_t m_amount {0};
    Currency::t m_currency {Currency::Rub};
    int m_centsPerUnit {100};
};

Money operator+(const Money &m1, const Money &m2);
Money operator-(const Money &m1, const Money &m2);
Money operator+(double m1, const Money &m2);
Money operator-(double m1, const Money &m2);
Money operator+(const Money &m1, double m2);
Money operator-(const Money &m1, double m2);

Money operator*(const Money &m1, double mult);
Money operator/(const Money &m1, double div);
Money operator%(const Money &m1, intmax_t div);

#endif //ASKELIB_STD_DECIMAL_H
