#include "analytics.h"
#include "ui_mainwindow.h"

#include <QComboBox>

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

    std::vector<Node<Wallet>*> l = m_data.wallets.rootItem->toList();
    for(const auto& wallet : l) {

        // wallet filter
        if(!wallet) continue;
        if(!wallet->isLeaf()) continue;
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

} // namespace cashbook
