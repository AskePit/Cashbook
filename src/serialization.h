#ifndef SERIALIZATION_H
#define SERIALIZATION_H

#include <QByteArray>

class QString;

namespace cashbook
{

class Data;

QByteArray save(const Data &data);
void save(const Data &data, const QString &fileName);

} // namespace cashbook

#endif // SERIALIZATION_H
