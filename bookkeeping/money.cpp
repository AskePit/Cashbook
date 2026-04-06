#include "money.h"
#include <mutex>

std::map<Currency::t, double> Currency::weights = {
    {Currency::Rub, 1.0},
    {Currency::Usd, 1.0},
    {Currency::Eur, 1.0},
    {Currency::Gbp, 1.0},
    {Currency::Jpy, 1.0},
    {Currency::Btc, 1.0},

    {Currency::UserDefined, 1.0},
};

static std::mutex currWeightsMutex;

double Currency::weight(Currency::t currency) {
    return weights[currency];
}

void Currency::setWeight(Currency::t currency, double weight) {
    std::lock_guard<std::mutex> lock(currWeightsMutex);
    (void)lock;

    weights[currency] = weight;
}

Money &Money::operator+=(const Money &other) {
    if(m_currency == other.m_currency) {
        m_amount += other.m_amount;
    } else {
        return (*this) += other.convertToCopy(m_currency);
    }

    return *this;
}

Money &Money::operator-=(const Money &other) {
    if(m_currency == other.m_currency) {
        m_amount -= other.m_amount;
    } else {
        return (*this) -= other.convertToCopy(m_currency);
    }

    return *this;
}

Money operator+(const Money &m1, const Money &m2) {
    if(m1.currency() == m2.currency()) {
        return Money(m1.as_cents() + m2.as_cents());
    }

    return m1 + m2.convertToCopy(m1.currency());
}

Money operator-(const Money &m1, const Money &m2) {
    if(m1.currency() == m2.currency()) {
        return Money(m1.as_cents() - m2.as_cents());
    }

    return m1 - m2.convertToCopy(m1.currency());
}

Money operator+(double m1, const Money &m2) { return Money(m1) + m2; }
Money operator-(double m1, const Money &m2) { return Money(m1) - m2; }
Money operator+(const Money &m1, double m2) { return m1 + Money(m2); }
Money operator-(const Money &m1, double m2) { return m1 - Money(m2); }

Money operator*(const Money &m1, double mult)  { return Money( intmax_t(m1.as_cents()*mult) ); }
Money operator/(const Money &m1, double div)   { return Money( intmax_t(m1.as_cents()/div) ); }
Money operator%(const Money &m1, intmax_t div) { return Money( intmax_t(m1.as_cents()%div) ); }
