#ifndef CATEGORIESSTATICCHART_H
#define CATEGORIESSTATICCHART_H

#include <QWidget>
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>

#include "bookkeeping/bookkeeping.h"

namespace Ui {
class CategoriesStaticChart;
}

namespace cashbook
{

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

} // namespace cashbook

#endif // CATEGORIESSTATICCHART_H
