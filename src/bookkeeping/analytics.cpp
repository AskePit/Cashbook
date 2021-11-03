#include "analytics.h"
#include "ui_mainwindow.h"

#include <QComboBox>

namespace cashbook
{

WalletsAnalytics::WalletsAnalytics(const Data& data)
    : m_chart()
    , m_series(&m_chart)
    , m_view(&m_chart)
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

    for(const Owner& owner : m_data.owners.owners) {
        m_ownerCombo->addItem(owner);
    }
    m_ownerCombo->addItem(AllOption);
    m_ownerCombo->setCurrentText(AllOption);

    for(const Bank& bank : m_data.banks.banks) {
        m_bankCombo->addItem(bank);
    }
    m_bankCombo->addItem(AllOption);
    m_bankCombo->addItem(NoBankOption);
    m_bankCombo->setCurrentText(AllOption);


    for(cashbook::Wallet::Availability::t t : cashbook::Wallet::Availability::enumerate()) {
        QString name = cashbook::Wallet::Availability::toString(t);

        m_availabilityFromCombo->addItem(name, t);
        m_availabilityToCombo->addItem(name, t);
    }
    m_availabilityFromCombo->addItem(AllOption);
    m_availabilityFromCombo->setCurrentText(AllOption);
    m_availabilityToCombo->addItem(AllOption);
    m_availabilityToCombo->setCurrentText(AllOption);

    /*m_m_series->setPieStartAngle(-90);
    m_m_series->setPieEndAngle(90);*/
    m_series.setPieSize(0.7);
    m_series.setHoleSize(0.3);
    m_chart.addSeries(&m_series);
    m_chart.legend()->hide();
    m_chart.setTheme(QChart::ChartThemeBrownSand);
    m_chart.setBackgroundBrush(QBrush(Qt::white));

    m_view.setRenderHint(QPainter::Antialiasing, true);

    ui->walletsAnalysisTabLayout->addWidget(&m_view);
}

void WalletsAnalytics::updateAnalytics()
{
    QString ownerFilter = m_ownerCombo->currentText();
    QString bankFilter = m_bankCombo->currentText();
    auto availFromFilter = static_cast<cashbook::Wallet::Availability::t>(m_availabilityFromCombo->currentIndex());
    auto availToFilter = static_cast<cashbook::Wallet::Availability::t>(m_availabilityToCombo->currentIndex());

    const bool allOwners = (ownerFilter == AllOption);
    const bool allBanks = (bankFilter == AllOption);
    const bool allAvail = (availFromFilter == cashbook::Wallet::Availability::Count && availToFilter == cashbook::Wallet::Availability::Count);
    const bool noBank = (bankFilter == NoBankOption);

    if(!allAvail) {
        if(availFromFilter == cashbook::Wallet::Availability::Count) {
            availFromFilter = cashbook::Wallet::Availability::Free;
        }
        if(availToFilter == cashbook::Wallet::Availability::Count) {
            availToFilter = static_cast<cashbook::Wallet::Availability::t>( static_cast<int>(cashbook::Wallet::Availability::Count) - 1 );
        }

        if(availFromFilter > availToFilter) {
            std::swap(availFromFilter, availToFilter);
        }
    }

    m_series.clear();

    std::map<QString, Money> data;

    std::vector<Node<Wallet>*> l = m_data.wallets.rootItem->toList();
    for(const auto& wallet : l) {
        if(!wallet) continue;
        if(!wallet->isLeaf()) continue;
        if(wallet->data.type == Wallet::Type::Points) continue;

        if(!allAvail) {
            if(wallet->data.info->availability < availFromFilter) continue;
            if(wallet->data.info->availability > availToFilter) continue;
        }

        if(!allOwners) {
            const ArchPointer<Owner>& ownerPointer = wallet->data.info->owner;
            if(!ownerPointer.isValidPointer()) continue;
            if(ownerPointer.isNullPointer()) continue;
            if(*ownerPointer.toPointer() != ownerFilter) continue;
        }

        const Bank* bank = nullptr;

        if(const auto* info = dynamic_cast<Wallet::AccountInfo*>(wallet->data.info.get())) {
            if(info->bank.isValidPointer() && !info->bank.isNullPointer()) {
                //m_data[*info->bank.toPointer()] += wallet->m_data.amount;
                bank = info->bank.toPointer();
            }
        } else if(const auto* investInfo = dynamic_cast<Wallet::InvestmentInfo*>(wallet->data.info.get())) {
            if(investInfo->account) {
                if(investInfo->account->bank.isValidPointer() && !investInfo->account->bank.isNullPointer()) {
                    //m_data[*investInfo->account->bank.toPointer()] += wallet->m_data.amount;
                    bank = investInfo->account->bank.toPointer();
                }
            }
        }

        if(bank) {
            if(noBank) continue;
            if(!allBanks) {
                if(*bank != bankFilter) continue;
            }
            data[*bank] += wallet->data.amount;
        } else {
            if(allBanks || noBank) {
                data[NoBankOption] += wallet->data.amount;
            }
        }
    }

    std::vector<std::pair<QString, Money>> dataSorted;
    dataSorted.resize(data.size());

    std::move(data.begin(), data.end(), dataSorted.begin());
    std::sort(dataSorted.begin(), dataSorted.end(), [](const std::pair<QString, Money>& a, const std::pair<QString, Money>& b)
    {
        return a.second > b.second;
    });

    Money sum;

    for (auto&& [bank, money] : dataSorted) {
        sum += money;
        const double amount = static_cast<double>(money);
        QPieSlice* slice = m_series.append(QString("%1<br>%2").arg(bank, formatMoney(money)), amount);
        Q_UNUSED(slice);
    }

    m_chart.setTitle(QObject::tr("Распределение денег по банкам<br>%1").arg(formatMoney(sum)));

    m_series.setLabelsVisible(true);
    m_series.setLabelsPosition(QPieSlice::LabelOutside);
}

} // namespace cashbook
