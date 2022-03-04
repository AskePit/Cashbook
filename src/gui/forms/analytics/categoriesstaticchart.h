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

    std::vector<RectData> getCurrentValues();
    Q_INVOKABLE std::vector<Rect> getCurrenRects(float windowWidth, float windowHeight);

    Q_INVOKABLE void gotoNode(const QString& nodeName);
    Q_INVOKABLE void goUp();

private:
    void _getCurrenRects(std::span<Rect> res, QRectF space, qreal wholeSquare);

    NodeX m_tree;
    NodeX* m_currNode {nullptr};
};

namespace Ui {
class CategoriesStaticChart;
}

namespace cashbook
{

#if 0
class CategoriesStaticChart : public QWidget
{
    Q_OBJECT

public:
    struct PieChart
    {
        PieChart(QWidget* parent = nullptr);

        void initUi();
        void setCategoryData(const Node<Category>* category, const CategoryMoneyMap& categoriesMap);
        void increaseFrame();
        void decreaseFrame();
        void fillChart();

        const Node<Category>* parentCategory {nullptr};
        std::vector<std::pair<Category, Money>> data;
        std::vector<Money> sums;

        size_t framesCount {0};
        size_t frame {0};

        QChart* chart {nullptr};
        QPieSeries* series {nullptr};
        QChartView* view {nullptr};
    };

    explicit CategoriesStaticChart(QWidget *parent = nullptr);
    ~CategoriesStaticChart();

    void init(const Data& data);

private slots:
    void on_backButton_clicked();

private:
    void updatePeriod();

    void onLeftPieClicked(QPieSlice* slice);
    void onRightPieClicked(QPieSlice* slice);

    void swapCharts();

    Ui::CategoriesStaticChart *ui;

    const Data* m_data {nullptr};

    CategoryMoneyMap m_inCategoriesMap;
    CategoryMoneyMap m_outCategoriesMap;

    PieChart m_leftChart;
    PieChart m_rightChart;
};
#endif // 0

} // namespace cashbook

#endif // CATEGORIESSTATICCHART_H
