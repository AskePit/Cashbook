#ifndef SERIALIZATION_H
#define SERIALIZATION_H

class QString;

namespace cashbook
{

class Data;

void save(Data &data);
void load(Data &data);

} // namespace cashbook

#endif // SERIALIZATION_H
