#ifndef COMMON_H
#define COMMON_H

#include <QDate>
#include <vector>

#define UNUSED(x) (void)(x)

static const QDate today {QDate::currentDate()};

/**
 * @brief The AbstractChangable interface.
 * @details Interface for an object that has a "changed" state, which can be reseted.
 */
class AbstractChangable
{
public:
    virtual bool isChanged() const = 0;
    virtual void resetChanged() = 0;
};

/**
 * @brief The Changable class.
 * @details Represents spesific object that can be changed and can change itself.
 */
class Changable : public AbstractChangable
{
protected:
    bool m_changed;

public:
    virtual bool isChanged() const override {
        return m_changed;
    }

    virtual void resetChanged() override {
        m_changed = false;
    }

protected:
    void setChanged() {
        m_changed = true;
    }

    /**
     * Proxy function that sets object changed if `changed` is `true`
     * and returns `changed`.
     *
     * This function is usefull for oneliners which return result from function.
     */
    bool changeFilter(bool changed) {
        m_changed |= changed;
        return changed;
    }
};

/**
 * @brief The ChangeObservable class.
 * @details Represents object that contains or owns AbstractChangeable objects.
 *          If any of these objects are changed, this object is considered
 *          being changed too.
 */
class ChangeObservable : public AbstractChangable
{
private:
    std::vector<AbstractChangable*> m_items;

public:
    virtual void setChangableItems(std::vector<AbstractChangable*> items) {
        m_items = items;
    }

    virtual void addChangableItem(AbstractChangable* item) {
        m_items.push_back(item);
    }

    virtual std::vector<AbstractChangable*> &changableItems() {
        return m_items;
    }

    virtual void removeChangableItem(AbstractChangable* toRemove) {
        m_items.erase( std::remove(m_items.begin(), m_items.end(), toRemove), m_items.end() );
    }

    virtual bool isChanged() const override {
        for(AbstractChangable *item : m_items) {
            if(item->isChanged()) {
                return true;
            }
        }
        return false;
    }

    virtual void resetChanged() override {
        for(AbstractChangable *item : m_items) {
            item->resetChanged();
        }
    }
};

#endif // COMMON_H
