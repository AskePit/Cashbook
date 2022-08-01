#ifndef CATEGORIESSTATICCHART_H
#define CATEGORIESSTATICCHART_H

#include <QWidget>
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>

#include "bookkeeping/bookkeeping.h"

#include <QPoint>
#include <QRectF>
#include <QObject>

#include <span>

namespace cashbook
{

class Rect {
    Q_GADGET

    Q_PROPERTY(QString name MEMBER name CONSTANT FINAL)
    Q_PROPERTY(QString sum MEMBER sum CONSTANT FINAL)
    Q_PROPERTY(qreal percentage MEMBER percentage CONSTANT FINAL)
    Q_PROPERTY(qreal x MEMBER x CONSTANT FINAL)
    Q_PROPERTY(qreal y MEMBER y CONSTANT FINAL)
    Q_PROPERTY(qreal w MEMBER w CONSTANT FINAL)
    Q_PROPERTY(qreal h MEMBER h CONSTANT FINAL)
    Q_PROPERTY(bool isLeaf MEMBER isLeaf CONSTANT FINAL)

public:
    QString name;
    QString sum;
    qreal percentage {0.0f};
    bool isLeaf {false};
    qreal x {0.0f};
    qreal y {0.0f};
    qreal w {0.0f};
    qreal h {0.0f};
};

class TreemapModel : public QObject
{
    Q_OBJECT
public:
    explicit TreemapModel(QObject *parent = 0);

    void init(const Data& data);

    void setDateFrom(QDate from) {
        m_from = from;
    }

    void setDateTo(QDate to) {
        m_to = to;
    }

    void setCategoriesType(int index);

    Q_INVOKABLE void updatePeriod();

    Q_INVOKABLE bool gotoNode(const QString& nodeName);
    Q_INVOKABLE bool goUp();

    Q_INVOKABLE std::vector<Rect> getCurrenRects(float windowWidth, float windowHeight);
    Q_INVOKABLE QString getTotalSum() const;
    Q_INVOKABLE QString getCategoryPath() const;

signals:
    void onUpdated();

private:
    std::vector<Rect> _getCurrentValues();
    void _getCurrenRects(std::span<Rect> res, QRectF space, qreal wholeSquare);
    CategoryMoneyMap& _getCategories() {
        return m_categoriesType ? m_inCategoriesMap : m_outCategoriesMap;
    }
    const CategoryMoneyMap& _getCategories() const {
        return m_categoriesType ? m_inCategoriesMap : m_outCategoriesMap;
    }

    const Data* m_data {nullptr};

    QDate m_from;
    QDate m_to;

    int m_categoriesType {0};
    CategoryMoneyMap m_inCategoriesMap;
    CategoryMoneyMap m_outCategoriesMap;

    const Node<Category>* m_parentCategory {nullptr};
};

} // namespace cashbook

#endif // CATEGORIESSTATICCHART_H
