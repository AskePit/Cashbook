#ifndef SERIALIZATION_H
#define SERIALIZATION_H

class QString;

namespace cashbook
{

class Data;

void save(const Data &data, const QString &fileName);
void load(Data &data, const QString &fileName);

} // namespace cashbook

#endif // SERIALIZATION_H
