inline double Currency::weight(Currency::t currency) {
    return weights[currency];
}

inline void Currency::setWeight(Currency::t currency, double weight) {
    weights[currency] = weight;
}

template<Currency::t C>
inline TMoney<C> &TMoney<C>::operator+=(const TMoney<C> &other) {
    m_amount += other.m_amount;
    return *this;
}

template<Currency::t C>
inline TMoney<C> &TMoney<C>::operator-=(const TMoney<C> &other) {
    m_amount -= other.m_amount;
    return *this;
}

template<Currency::t C>
inline TMoney<C> operator+(const TMoney<C> &m1, const TMoney<C> &m2) {
    return TMoney<C>(m1.as_cents() + m2.as_cents());
}

template<Currency::t C>
inline TMoney<C> operator-(const TMoney<C> &m1, const TMoney<C> &m2) {
    return TMoney<C>(m1.as_cents() - m2.as_cents());
}

template<Currency::t C>
inline TMoney<C> operator+(double m1, const TMoney<C> &m2) { return TMoney<C>(m1) + m2; }

template<Currency::t C>
inline TMoney<C> operator-(double m1, const TMoney<C> &m2) { return TMoney<C>(m1) - m2; }

template<Currency::t C>
inline TMoney<C> operator+(const TMoney<C> &m1, double m2) { return m1 + TMoney<C>(m2); }

template<Currency::t C>
inline TMoney<C> operator-(const TMoney<C> &m1, double m2) { return m1 - TMoney<C>(m2); }

template<Currency::t C>
inline TMoney<C> operator*(const TMoney<C> &m1, double mult)  { return TMoney<C>( intmax_t(m1.as_cents()*mult) ); }

template<Currency::t C>
inline TMoney<C> operator/(const TMoney<C> &m1, double div)   { return TMoney<C>( intmax_t(m1.as_cents()/div) ); }

template<Currency::t C>
inline TMoney<C> operator%(const TMoney<C> &m1, intmax_t div) { return TMoney<C>( intmax_t(m1.as_cents()%div) ); }
