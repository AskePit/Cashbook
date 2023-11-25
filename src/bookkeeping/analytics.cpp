#include "analytics.h"
#include "ui_mainwindow.h"

#include <QComboBox>
#include <QRectF>
#include <QDateTimeAxis>
#include <QValueAxis>

#define SPLINE 0

#if SPLINE
#include <QSplineSeries>
#else
#include <QLineSeries>
#endif

namespace cashbook
{

WalletsAnalytics::WalletsAnalytics(const Data& data, QWidget* parent)
    : m_chart(new QChart())
    , m_series(new QPieSeries(parent))
    , m_view(new QChartView(m_chart, parent))
    , m_data(data)
{
}

static const QString AllOption = QObject::tr("Все");
static const QString NoBankOption = QObject::tr("Вне банка");

void WalletsAnalytics::initUi(Ui::MainWindow* ui)
{
    m_criteriaCombo = ui->walletsAnalysisCriteriaCombo;
    m_ownerCombo = ui->walletsAnalysisOwnerCombo;
    m_bankCombo = ui->walletsAnalysisBankCombo;
    m_availabilityFromCombo = ui->walletsAnalysisAvailabilityFromCombo;
    m_availabilityToCombo = ui->walletsAnalysisAvailabilityToCombo;
    m_moneyTypeCombo = ui->walletsAnalysisMoneyTypeCombo;

    for(const Owner& owner : m_data.owners.owners) {
        m_ownerCombo->addItem(owner);
    }
    m_ownerCombo->addItem(AllOption);
    m_ownerCombo->setCurrentText(AllOption);

    for(const Bank& bank : m_data.banks.banks) {
        m_bankCombo->addItem(bank);
    }
    m_bankCombo->addItem(NoBankOption);
    m_bankCombo->addItem(AllOption);
    m_bankCombo->setCurrentText(AllOption);

    for(Wallet::Availability::t t : Wallet::Availability::enumerate()) {
        QString name = Wallet::Availability::toString(t);

        m_availabilityFromCombo->addItem(name, t);
        m_availabilityToCombo->addItem(name, t);
    }
    m_availabilityFromCombo->addItem(AllOption);
    m_availabilityFromCombo->setCurrentText(AllOption);
    m_availabilityToCombo->addItem(AllOption);
    m_availabilityToCombo->setCurrentText(AllOption);

    for(Wallet::Type::t t : Wallet::Type::enumerate()) {
        QString name = Wallet::Type::toString(t);
        m_moneyTypeCombo->addItem(name, t);
    }
    m_moneyTypeCombo->addItem(AllOption);
    m_moneyTypeCombo->setCurrentText(AllOption);

    /*m_m_series->setPieStartAngle(-90);
    m_m_series->setPieEndAngle(90);*/
    m_series->setPieSize(0.7);
    m_series->setHoleSize(0.3);
    m_chart->addSeries(m_series);
    m_chart->legend()->hide();
    m_chart->setTheme(QChart::ChartThemeBrownSand);
    m_chart->setBackgroundBrush(QBrush(Qt::white));

    m_view->setRenderHint(QPainter::Antialiasing, true);

    ui->walletsAnalysisTabLayout->addWidget(m_view);
}

void WalletsAnalytics::updateAnalytics()
{
    QString ownerFilter = m_ownerCombo->currentText();
    QString bankFilter = m_bankCombo->currentText();
    auto availFromFilter = static_cast<Wallet::Availability::t>(m_availabilityFromCombo->currentIndex());
    auto availToFilter = static_cast<Wallet::Availability::t>(m_availabilityToCombo->currentIndex());
    auto moneyTypeFilter = static_cast<Wallet::Type::t>(m_moneyTypeCombo->currentIndex());

    const bool allOwners = (ownerFilter == AllOption);
    const bool allBanks = (bankFilter == AllOption);
    const bool allAvail = (availFromFilter == Wallet::Availability::Count && availToFilter == Wallet::Availability::Count);
    const bool allMoneyTypes = (moneyTypeFilter == Wallet::Type::Count);

    if(!allAvail) {
        if(availFromFilter == Wallet::Availability::Count) {
            availFromFilter = Wallet::Availability::Free;
        }
        if(availToFilter == Wallet::Availability::Count) {
            availToFilter = static_cast<Wallet::Availability::t>( static_cast<int>(Wallet::Availability::Count) - 1 );
        }

        if(availFromFilter > availToFilter) {
            std::swap(availFromFilter, availToFilter);
        }
    }

    m_series->clear();

    std::map<QString, Money> data;

    Type::t aim = static_cast<Type::t>(m_criteriaCombo->currentIndex());

    std::vector<Node<Wallet>*> l = m_data.wallets.rootItem->getLeafs();
    for(const auto& wallet : l) {

        // wallet filter
        if(!wallet) continue;
        if(wallet->data.type == Wallet::Type::Points) continue;

        Money money = wallet->data.amount;
        if(money == Money(0.0f)) continue;

        Wallet::Type::t walletType = wallet->data.type;
        if(!allMoneyTypes) {
            if(walletType != moneyTypeFilter) continue;
        }

        Wallet::Availability::t availability = wallet->data.info->availability;
        if(!allAvail) {
            if(availability < availFromFilter) continue;
            if(availability > availToFilter) continue;
        }

        Owner owner;
        {
            const ArchPointer<Owner>& ownerPointer = wallet->data.info->owner;
            if(ownerPointer.isValidPointer() && !ownerPointer.isNullPointer()) {
                owner = *ownerPointer.toPointer();
            } else {
                owner = AllOption;
            }
            if(!allOwners) {
                if(owner != ownerFilter) continue;
            }
        }

        Bank bank;
        {
            bool foundBank = false;
            if(const auto* info = dynamic_cast<Wallet::AccountInfo*>(wallet->data.info.get())) {
                if(info->bank.isValidPointer() && !info->bank.isNullPointer()) {
                    bank = *info->bank.toPointer();
                    foundBank = true;
                }
            } else if(const auto* investInfo = dynamic_cast<Wallet::InvestmentInfo*>(wallet->data.info.get())) {
                if(investInfo->account) {
                    if(investInfo->account->bank.isValidPointer() && !investInfo->account->bank.isNullPointer()) {
                        bank = *investInfo->account->bank.toPointer();
                        foundBank = true;
                    }
                }
            }

            if(!foundBank) {
                bank = NoBankOption;
            }

            if(!allBanks) {
                if(bank != bankFilter) continue;
            }
        }

        // wallet data collect
        QString dataKey;
        switch (aim)
        {
            case Type::Banks:
                dataKey = bank;
                break;
            case Type::Availability:
                dataKey = Wallet::Availability::toString(availability);
                break;
            case Type::Owners:
                dataKey = owner;
                break;
            case Type::MoneyType:
                dataKey = Wallet::Type::toString(walletType);
                break;
        }
        data[dataKey] += money;
    }

    std::vector<std::pair<QString, Money>> dataSorted;
    dataSorted.resize(data.size());

    std::move(data.begin(), data.end(), dataSorted.begin());
    std::sort(dataSorted.begin(), dataSorted.end(), [](const std::pair<QString, Money>& a, const std::pair<QString, Money>& b)
    {
        return a.second > b.second;
    });

    Money sum;

    for (auto&& [_, money] : dataSorted) {
        sum += money;
    }

    for (auto&& [bank, money] : dataSorted) {
        const double amount = static_cast<double>(money);
        const double percent = amount/static_cast<double>(sum)*100.0f;

        QPieSlice* slice = m_series->append(
            QString("%1 %2%<br>%3").arg(bank, formatPercent(percent), formatMoney(money)),
            amount
        );
        Q_UNUSED(slice);
    }

    m_chart->setTitleFont(QFont("Segoe UI", 12));
    m_chart->setTitle(QObject::tr("Общая сумма:<br>%1").arg(formatMoney(sum)));

    m_series->setLabelsVisible(true);
    m_series->setLabelsPosition(QPieSlice::LabelOutside);
}

CategoriesAnalytics::CategoriesAnalytics(DataModels& dataModels, QWidget* parent)
    : m_chart(new QChart())
#if SPLINE
    , m_series(new QSplineSeries(parent))
#else
    , m_series(new QLineSeries(parent))
#endif
    , m_view(new QChartView(m_chart, parent))
    , m_dataModels(dataModels)
{
}

void CategoriesAnalytics::initUi(Ui::MainWindow* ui)
{
    m_categoryTypeCombo = ui->categoriesTypeBox;
    m_categoryCombo = ui->categoriesCategoryButton;
    m_dateFromEdit = ui->categoriesDateFrom;
    m_dateToEdit = ui->categoriesDateTo;
    m_thisMonthButton = ui->categoriesThisMonthButton;
    m_thisYearButton = ui->categoriesThisYearButton;
    m_monthButton = ui->categoriesMonthButton;
    m_yearButton = ui->categoriesYearButton;
    m_densityCombo = ui->categoriesDensityBox;

    m_categoryCombo->setModel(m_dataModels.outCategoriesModel);

    QObject::connect(m_categoryTypeCombo, &QComboBox::currentIndexChanged, m_categoryTypeCombo, [this](int index) {
        auto& model = index == 0 ? m_dataModels.outCategoriesModel : m_dataModels.inCategoriesModel;
        m_categoryCombo->setModel(model);
    });

    m_categoryCombo->setUpdateCallback([this](const Node<Category>*){
        updateAnalytics();
    });

    QObject::connect(m_densityCombo, &QComboBox::currentIndexChanged, m_densityCombo, [this](int) {
        updateAnalytics();
    });

    m_dateFromEdit->setDate(QDate(Today.year(), 1, 1));
    m_dateToEdit->setDate(Today);
    m_dateToEdit->setMaximumDate(Today);

    QObject::connect(m_dateFromEdit, &QDateEdit::dateChanged, m_dateFromEdit, [this](const QDate &) {
        updateAnalytics();
    });

    QObject::connect(m_dateToEdit, &QDateEdit::dateChanged, m_dateToEdit, [this](const QDate &) {
        updateAnalytics();
    });

    QObject::connect(m_monthButton, &QPushButton::pressed, m_monthButton, [this]() {
        m_canUpdate = false;
        m_dateFromEdit->setDate(Today.addDays(-30));
        m_dateToEdit->setDate(Today);
        setDensity(Density::Day);
        m_canUpdate = true;

        updateAnalytics();
    });

    QObject::connect(m_yearButton, &QPushButton::pressed, m_yearButton, [this]() {
        m_canUpdate = false;
        m_dateFromEdit->setDate(Today.addYears(-1));
        m_dateToEdit->setDate(Today);
        setDensity(Density::Month);
        m_canUpdate = true;

        updateAnalytics();
    });

    QObject::connect(m_thisMonthButton, &QPushButton::pressed, m_thisMonthButton, [this]() {
        m_canUpdate = false;
        m_dateFromEdit->setDate(QDate(Today.year(), Today.month(), 1));
        m_dateToEdit->setDate(Today);
        setDensity(Density::Day);
        m_canUpdate = true;

        updateAnalytics();
    });

    QObject::connect(m_thisYearButton, &QPushButton::pressed, m_thisYearButton, [this]() {
        m_canUpdate = false;
        m_dateFromEdit->setDate(QDate(Today.year(), 1, 1));
        m_dateToEdit->setDate(Today);
        setDensity(Density::Month);
        m_canUpdate = true;

        updateAnalytics();
    });

    QDateTimeAxis *axisX = new QDateTimeAxis;
    axisX->setRange(m_dateFromEdit->dateTime(), m_dateToEdit->dateTime());
    axisX->setFormat("dd-MM-yyyy");
    m_chart->addAxis(axisX, Qt::AlignBottom);


    QValueAxis *axisY = new QValueAxis;
    axisY->setRange(0, 1000);
    m_chart->addAxis(axisY, Qt::AlignLeft);

    m_chart->legend()->hide();
    //m_chart->setTheme(QChart::ChartThemeBrownSand);
    //m_chart->setBackgroundBrush(QBrush(Qt::white));

    m_chart->addSeries(m_series);
    m_series->setPointsVisible(true);
    m_series->attachAxis(axisX);
    m_series->attachAxis(axisY);
    m_series->setPointLabelsVisible(false);
    m_series->setPointLabelsFormat("@xPoint: @yPoint");

    QObject::connect(m_series, &QXYSeries::hovered, m_series, [this](const QPointF &point, bool state) {
        int index = -1;

        const auto& points = m_series->points();

        for(int i = 0; i < points.size(); ++i) {
            m_series->setPointConfiguration(i, QXYSeries::PointConfiguration::LabelVisibility, false);
        }

        qreal closestDist = std::numeric_limits<qreal>::max();

        for(int i = 0; i < points.size(); ++i) {
            auto& currentPoint = points[i];
            const qreal xDist = currentPoint.x() - point.x();
            const qreal yDist = currentPoint.y() - point.y();

            const qreal dist = qSqrt(xDist*xDist + yDist*yDist);

            if (dist < closestDist) {
                index = i;
                closestDist = dist;
            }
        }

        const QPointF& truePoint = points[index];
        QDateTime d;
        d.setMSecsSinceEpoch(truePoint.x());

        m_series->setPointConfiguration(index, QXYSeries::PointConfiguration::LabelVisibility, state);

        QString label;

        if (getDensity() == Density::Day) {
            label = QString("%1: @yPoint").arg(d.date().toString("dd MMM yyyy"));
        } else {
            label = QString("%1: @yPoint").arg(d.date().toString("MMM yyyy"));
        }

        m_series->setPointConfiguration(
            index,
            QXYSeries::PointConfiguration::LabelFormat,
            label
        );
    });

    m_view->setRenderHint(QPainter::Antialiasing, true);
    ui->categorieAnalysisTabLayout->addWidget(m_view);

    m_canUpdate = true;
}

static bool isNodeBelongsTo(const Node<Category> *node, const Node<Category> *parent) {
    while(node) {
        if(node == parent) {
                return true;
        }
        node = node->parent;
    }

    return false;
}

void CategoriesAnalytics::updateAnalytics()
{
    if (!m_canUpdate) {
        return;
    }

    QDateTimeAxis* axisX = nullptr;
    QValueAxis* axisY = nullptr;
    {
        auto horAxes = m_chart->axes(Qt::Horizontal);
        auto verAxes = m_chart->axes(Qt::Vertical);

        axisX = qobject_cast<QDateTimeAxis*>(horAxes[0]);
        axisY = qobject_cast<QValueAxis*>(verAxes[0]);
    }

    axisX->setRange(m_dateFromEdit->dateTime(), m_dateToEdit->dateTime());

    const Node<Category>* analyzedCategory = m_categoryCombo->node();

    if (!analyzedCategory) {
        return;
    }

    m_series->clear();

    std::map<QDate, Money> data;

    const auto log = m_dataModels.m_data.log.log;

    for(const Transaction& t : log) {
        if(t.date > m_dateToEdit->dateTime().date()) {
            continue;
        }

        if(t.date < m_dateFromEdit->dateTime().date()) {
            break;
        }

        const Node<Category>* categoryNode = t.category.toPointer();
        if (categoryNode && isNodeBelongsTo(categoryNode, analyzedCategory)) {
            if (getDensity() == Density::Day) {
                data[t.date] += t.amount;
            } else {
                // month density
                QDate date(t.date.year(), t.date.month(), 1);
                data[date] += t.amount;
            }
        }
    }

    qreal yMin = std::numeric_limits<qreal>::max();
    qreal yMax = 0.0;

    for(const std::pair<const QDate, Money>& t : data) {
        m_series->append(QDateTime(t.first, QTime()).toMSecsSinceEpoch(), static_cast<double>(t.second));

        yMin = std::min(yMin, static_cast<double>(t.second));
        yMax = std::max(yMax, static_cast<double>(t.second));
    }

    axisY->setRange(yMin, yMax);
    axisY->applyNiceNumbers();
    m_chart->setTitleFont(QFont("Segoe UI", 12));
    m_chart->setTitle(analyzedCategory->data);
}

CategoriesAnalytics::Density CategoriesAnalytics::getDensity() const
{
    return Density(m_densityCombo->currentIndex());
}

void CategoriesAnalytics::setDensity(Density density)
{
    m_densityCombo->setCurrentIndex(static_cast<int>(density));
}

} // namespace cashbook
