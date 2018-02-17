#include "statistics.h"

#include <QObject>
#include <QDate>

namespace cashbook
{

void CategoryMoneyMap::propagateMoney(const Node<Category> *node, const Money &amount) {
    while(node) {
        (*this)[node] += amount;
        node = node->parent;
    }
}

} // namespace cashbook
