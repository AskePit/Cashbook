#pragma once

#include <stdint.h>
#include <cassert>
#include <cmath>
#include <utility>
#include <array>

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

        Count,

        UserDefined = 1024
    };

    static double weight(Currency::t currency);
    static void setWeight(Currency::t currency, double weight);

private:
    static std::array<double, Count> weights;
};

template<Currency::t C = Currency::Default>
class TMoney {
public:
    using money_pair = std::pair<intmax_t, intmax_t>;
    static constexpr Currency::t CurrencyT = C;

    // creation
    TMoney()
    {}

    explicit TMoney(intmax_t units, intmax_t cents)
        : m_amount(units*CENTS_PER_UNIT + cents)
    {}

    explicit TMoney(intmax_t cents)
        : m_amount(cents)
    {}

    TMoney(double value)
    {
        double cents = value*CENTS_PER_UNIT;
        cents = std::round(cents*CENTS_PER_UNIT)/CENTS_PER_UNIT; // round hack
        m_amount = intmax_t(cents);
    }

    friend void swap(TMoney& first, TMoney& second) // nothrow
    {
        using std::swap;

        swap(first.m_amount, second.m_amount);
    }

    TMoney(const TMoney &other)
        : m_amount(other.m_amount)
    {}

    TMoney(TMoney &&other)
        : TMoney()
    {
        swap(*this, other);
    }

    TMoney &operator=(TMoney other)
    {
        swap(*this, other);
        return *this;
    }

    // casts
    explicit operator double() const
    {
        return m_amount/(double)CENTS_PER_UNIT;
    }

    intmax_t units() const
    {
        return m_amount/CENTS_PER_UNIT;
    }

    intmax_t cents() const
    {
        return m_amount%CENTS_PER_UNIT;
    }

    intmax_t as_cents() const
    {
        return m_amount;
    }

    money_pair units_cents() const
    {
        return {units(), cents()};
    }

    bool isZero() const
    {
        return m_amount == 0;
    }

    template<Currency::t OtherCurrency>
    void convertTo() {
        double ratio = Currency::weight(OtherCurrency) / Currency::weight(C);
        intmax_t cents = static_cast<intmax_t>(static_cast<double>(m_amount) * ratio);
        return TMoney<OtherCurrency>(cents);
    }

    // arithmetic
    TMoney &operator+=(const TMoney &other);
    TMoney &operator-=(const TMoney &other);
    TMoney &operator*=(double mult) { m_amount = intmax_t(m_amount*mult); return *this; }
    TMoney &operator/=(double div) { m_amount = intmax_t(m_amount/div); return *this; }
    TMoney &operator%=(intmax_t div) { m_amount %= div; return *this; }

    auto operator<=>(const TMoney&) const = default;

private:
    static constexpr int CENTS_PER_UNIT {100};

    intmax_t m_amount {0};
};

template<Currency::t C>
TMoney<C> operator+(const TMoney<C> &m1, const TMoney<C> &m2);
template<Currency::t C>
TMoney<C> operator-(const TMoney<C> &m1, const TMoney<C> &m2);
template<Currency::t C>
TMoney<C> operator+(double m1, const TMoney<C> &m2);
template<Currency::t C>
TMoney<C> operator-(double m1, const TMoney<C> &m2);
template<Currency::t C>
TMoney<C> operator+(const TMoney<C> &m1, double m2);
template<Currency::t C>
TMoney<C> operator-(const TMoney<C> &m1, double m2);

template<Currency::t C>
TMoney<C> operator*(const TMoney<C> &m1, double mult);
template<Currency::t C>
TMoney<C> operator/(const TMoney<C> &m1, double div);
template<Currency::t C>
TMoney<C> operator%(const TMoney<C> &m1, intmax_t div);

using Money = TMoney<>;

#include "money.hpp"