#ifndef COMMON_H
#define COMMON_H

#include <QDate>

#define UNUSED(...) (void)__VA_ARGS__
#define as static_cast

static const QDate today {QDate::currentDate()};

#endif // COMMON_H
