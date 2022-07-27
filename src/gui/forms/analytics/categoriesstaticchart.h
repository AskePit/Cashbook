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

class NodeX
{
public:
    NodeX() = default;
    NodeX(QString name, int value = 0)
        : name(name)
        , value(value)
    {}

    bool isLeaf() const {
        return children.empty();
    }

    int getSum() const {
        return std::accumulate(children.begin(), children.end(), value, [](int sum, const NodeX& child) {
            return sum + (child.isLeaf() ? child.value : child.getSum());
        });
    }

    NodeX& addChild(NodeX&& node) {
        NodeX& moved = children.emplace_back(std::move(node));
        moved.parent = this;
        return moved;
    }

    QString name;
    int value {0};
    std::vector<NodeX> children;
    NodeX* parent {nullptr};
};

Q_DECLARE_METATYPE(NodeX)

namespace cashbook
{

NodeX GetSampleTree();

using RectData = std::pair<QString, qreal>;

class Rect {
    Q_GADGET

    Q_PROPERTY(QString name MEMBER name CONSTANT FINAL)
    Q_PROPERTY(qreal percentage MEMBER percentage CONSTANT FINAL)
    Q_PROPERTY(qreal x MEMBER x CONSTANT FINAL)
    Q_PROPERTY(qreal y MEMBER y CONSTANT FINAL)
    Q_PROPERTY(qreal w MEMBER w CONSTANT FINAL)
    Q_PROPERTY(qreal h MEMBER h CONSTANT FINAL)

public:

    Rect() = default;

    Rect(const RectData& data, qreal x, qreal y, qreal w = 0, qreal h = 0)
        : name(data.first)
        , percentage(data.second)
        , x(x)
        , y(y)
        , w(w)
        , h(h)
    {}

    QString name;
    qreal percentage;
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
    Q_INVOKABLE void updatePeriod();

    Q_INVOKABLE void gotoNode(const QString& nodeName);
    Q_INVOKABLE void goUp();

    std::vector<RectData> getCurrentValues();
    Q_INVOKABLE std::vector<Rect> getCurrenRects(float windowWidth, float windowHeight);

signals:
    void onUpdated();

private:
    void _getCurrenRects(std::span<Rect> res, QRectF space, qreal wholeSquare);

    const Data* m_data {nullptr};

    QDate m_from;
    QDate m_to;

    CategoryMoneyMap m_inCategoriesMap;
    CategoryMoneyMap m_outCategoriesMap;

    NodeX m_tree;
    NodeX* m_currNode {nullptr};
};

} // namespace cashbook

#endif // CATEGORIESSTATICCHART_H
