#ifndef ANALYTICS_H
#define ANALYTICS_H

#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QPieSlice>

#include "bookkeeping/bookkeeping.h"

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

    WalletsAnalytics(const Data& data);
    void initUi(Ui::MainWindow* ui);

    void updateAnalytics();

private:    
    QChart m_chart;
    QPieSeries m_series;
    QChartView m_view;

    const Data& m_data;

    QComboBox *m_criteriaCombo {nullptr};
    QComboBox *m_ownerCombo {nullptr};
    QComboBox *m_bankCombo {nullptr};
    QComboBox *m_availabilityFromCombo {nullptr};
    QComboBox *m_availabilityToCombo {nullptr};
    QComboBox *m_moneyTypeCombo {nullptr};
};

} // namespace cashbook

#endif // ANALYTICS_H
