#ifndef ANALYTICS_H
#define ANALYTICS_H

#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QXYSeries>
#include <QtCharts/QPieSlice>
#include <QDateTimeEdit>

#include "bookkeeping/bookkeeping.h"
#include "gui/widgets/widgets.h"

class QComboBox;

namespace Ui {
class MainWindow;
}

namespace cashbook
{

class WalletsAnalytics
{
public:
    class Type
    {
    public:
        enum t {
            Banks = 0,
            Availability,
            Owners,
            MoneyType
        };
    };

    WalletsAnalytics(const Data& data, QWidget* parent = nullptr);
    void initUi(Ui::MainWindow* ui);

    void updateAnalytics();

private:
    QChart* m_chart {nullptr};
    QPieSeries* m_series {nullptr};
    QChartView* m_view {nullptr};

    const Data& m_data;

    QComboBox *m_criteriaCombo {nullptr};
    QComboBox *m_ownerCombo {nullptr};
    QComboBox *m_bankCombo {nullptr};
    QComboBox *m_availabilityFromCombo {nullptr};
    QComboBox *m_availabilityToCombo {nullptr};
    QComboBox *m_moneyTypeCombo {nullptr};
};

class CategoriesAnalytics
{
public:
    CategoriesAnalytics(DataModels& dataModels, QWidget* parent = nullptr);
    void initUi(Ui::MainWindow* ui);

    void updateAnalytics();

private:
    QChart* m_chart {nullptr};
    QXYSeries* m_series {nullptr};
    QChartView* m_view {nullptr};

    DataModels& m_dataModels;

    QComboBox *m_categoryTypeCombo {nullptr};
    CategoryNodeButton *m_categoryCombo {nullptr};
    QDateTimeEdit *m_dateFromEdit {nullptr};
    QDateTimeEdit *m_dateToEdit {nullptr};

    QPushButton *m_thisMonthButton {nullptr};
    QPushButton *m_thisYearButton {nullptr};
    QPushButton *m_monthButton {nullptr};
    QPushButton *m_yearButton {nullptr};
};

} // namespace cashbook

#endif // ANALYTICS_H
