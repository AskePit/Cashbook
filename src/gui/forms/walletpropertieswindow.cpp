#include "walletpropertieswindow.h"
#include "ui_walletpropertieswindow.h"

namespace cashbook {

WalletPropertiesWindow::WalletPropertiesWindow(Wallet& wallet, const OwnersData& owners, const BanksData& banks, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::WalletPropertiesWindow)
    , m_wallet(wallet)
    , m_owners(owners)
    , m_banks(banks)
{
    ui->setupUi(this);
    fillGuiFromWallet();
}

WalletPropertiesWindow::~WalletPropertiesWindow()
{
    delete ui;
}

static const QString NoneOption = QObject::tr("Нет");

void WalletPropertiesWindow::fillGuiFromWallet()
{
    disconnect(ui->typeCombo, &QComboBox::currentIndexChanged, this, &WalletPropertiesWindow::changeGuiByWalletType);

    for(QWidget* w : std::array<QWidget*, 8>{
        ui->bankLabel, ui->bankCombo,
        ui->canBeNegativeLabel, ui->canBeNegativeCheck,
        ui->incomePercentLabel, ui->incomePercentSpinBox,
        ui->investmentsLabel, ui->investmentsCombo
    }) {
        w->setVisible(false);
    }
    ui->typeCombo->clear();
    ui->ownerCombo->clear();
    ui->bankCombo->clear();
    ui->investmentsCombo->clear();

    ui->nameEdit->setText(m_wallet.name);

    // populate combos
    for(Wallet::Type::t type : Wallet::Type::enumerate()) {
        ui->typeCombo->addItem(Wallet::Type::toString(type), type);
    }

    for(const Owner& owner : m_owners.owners) {
        ui->ownerCombo->addItem(owner);
    }
    ui->ownerCombo->addItem(NoneOption);

    for(const Bank& bank : m_banks.banks) {
        ui->bankCombo->addItem(bank);
    }
    ui->bankCombo->addItem(NoneOption);

    for(Wallet::InvestmentInfo::Type::t type : Wallet::InvestmentInfo::Type::enumerate()) {
        ui->investmentsCombo->addItem(Wallet::InvestmentInfo::Type::toString(type), type);
    }

    for(Wallet::Availability::t type : Wallet::Availability::enumerate()) {
        ui->availabilityCombo->addItem(Wallet::Availability::toString(type), type);
    }

    ui->typeCombo->setCurrentIndex(m_wallet.type);

    // owner combo
    if(m_wallet.info->owner.isValidPointer() && !m_wallet.info->owner.isNullPointer()) {
        ui->ownerCombo->setCurrentText(*m_wallet.info->owner.toPointer());
    } else {
        ui->ownerCombo->setCurrentText(NoneOption);
    }

    // can be negative check
    ui->canBeNegativeCheck->setChecked(m_wallet.info->canBeNegative);
    ui->availabilityCombo->setCurrentIndex(m_wallet.info->availability);

    bool doBank = false;
    bool doDeposit = false;
    bool doInvestment = false;

    switch(m_wallet.type)
    {
        case Wallet::Type::Common:
        case Wallet::Type::Count:
        case Wallet::Type::Cash:
        case Wallet::Type::CryptoCurrency:
        case Wallet::Type::Points:
            break;
        case Wallet::Type::Deposit:
            doDeposit = true;
            [[fallthrough]];
        case Wallet::Type::Card:
        case Wallet::Type::Account:
            doBank = true;
            break;
        case Wallet::Type::Investment:
            doInvestment = true;
            doBank = true;
            break;
    }

    if(doBank) {
        const Wallet::AccountInfo* info = doInvestment
            ? &static_cast<const Wallet::InvestmentInfo*>(m_wallet.info.get())->account.value()
            : static_cast<const Wallet::AccountInfo*>(m_wallet.info.get());

        // bank combo
        if(info->bank.isValidPointer() && !info->bank.isNullPointer()) {
            ui->bankCombo->setCurrentText(*info->bank.toPointer());
        } else {
            ui->bankCombo->setCurrentText(NoneOption);
        }

        ui->bankLabel->setVisible(true);
        ui->bankCombo->setVisible(true);
    }

    if(doDeposit) {
        const auto* info = static_cast<const Wallet::DepositInfo*>(m_wallet.info.get());

        ui->incomePercentSpinBox->setValue(info->incomePercent);

        ui->incomePercentLabel->setVisible(true);
        ui->incomePercentSpinBox->setVisible(true);
    }

    if(doInvestment) {
        const auto* info = static_cast<const Wallet::InvestmentInfo*>(m_wallet.info.get());

        // investment combo
        ui->investmentsCombo->setCurrentIndex(info->type);

        ui->investmentsLabel->setVisible(true);
        ui->investmentsCombo->setVisible(true);
    }

    connect(ui->typeCombo, &QComboBox::currentIndexChanged, this, &WalletPropertiesWindow::changeGuiByWalletType);
}

void WalletPropertiesWindow::changeGuiByWalletType(int walletType)
{
    for(QWidget* w : std::array<QWidget*, 8>{
        ui->bankLabel, ui->bankCombo,
        ui->canBeNegativeLabel, ui->canBeNegativeCheck,
        ui->incomePercentLabel, ui->incomePercentSpinBox,
        ui->investmentsLabel, ui->investmentsCombo
    }) {
        w->setVisible(false);
    }

    switch(walletType)
    {
        case Wallet::Type::Common:
        case Wallet::Type::Count:
        case Wallet::Type::Cash:
        case Wallet::Type::CryptoCurrency:
        case Wallet::Type::Points:
            break;
        case Wallet::Type::Deposit:
            ui->incomePercentLabel->setVisible(true);
            ui->incomePercentSpinBox->setVisible(true);
            [[fallthrough]];
        case Wallet::Type::Card:
        case Wallet::Type::Account:
            ui->bankLabel->setVisible(true);
            ui->bankCombo->setVisible(true);
            break;
        case Wallet::Type::Investment:
            ui->investmentsLabel->setVisible(true);
            ui->investmentsCombo->setVisible(true);
            ui->bankLabel->setVisible(true);
            ui->bankCombo->setVisible(true);
            break;
    }
}

void WalletPropertiesWindow::fillWalletFromGui()
{
    m_wallet.name = ui->nameEdit->text();
    m_wallet.type = static_cast<Wallet::Type::t>(ui->typeCombo->currentIndex());

    const auto loadBank = [this](Wallet::AccountInfo* info)
    {
        auto it = std::find_if(m_banks.banks.begin(), m_banks.banks.end(), [this](const Bank& str){
            return str == ui->bankCombo->currentText();
        });
        if(it != m_banks.banks.end()) {
            info->bank = &*it;
        }
    };

    switch(m_wallet.type)
    {
        default:
        case Wallet::Type::Common:
            m_wallet.info = std::make_shared<Wallet::Info>();
            break;
        case Wallet::Type::Cash:
            m_wallet.info = std::make_shared<Wallet::CashInfo>();
            break;
        case Wallet::Type::Deposit:
        {
            auto info = std::make_shared<Wallet::DepositInfo>();

            loadBank(info.get());
            info->incomePercent = ui->incomePercentSpinBox->value();

            m_wallet.info = std::move(info);
        }
        break;
        case Wallet::Type::Account:
        {
            auto info = std::make_shared<Wallet::AccountInfo>();
            loadBank(info.get());
            m_wallet.info = std::move(info);
        }
        break;
        case Wallet::Type::Card:
        {
            auto info = std::make_shared<Wallet::CardInfo>();
            loadBank(info.get());
            m_wallet.info = std::move(info);
        }
        break;
        case Wallet::Type::Investment:
        {
            auto info = std::make_shared<Wallet::InvestmentInfo>();
            info->type = static_cast<Wallet::InvestmentInfo::Type::t>(ui->investmentsCombo->currentIndex());

            if(ui->bankCombo->currentText() != tr("None")) {
                info->account = Wallet::AccountInfo();
                loadBank(&info->account.value());
            }
            m_wallet.info = std::move(info);
        }
        break;
        case Wallet::Type::CryptoCurrency:
        {
            m_wallet.info = std::make_shared<Wallet::CryptoCurrencyInfo>();
        }
        break;
        case Wallet::Type::Points:
        {
            m_wallet.info = std::make_shared<Wallet::PointsInfo>();
        }
        break;
    }

    auto it = std::find_if(m_owners.owners.begin(), m_owners.owners.end(), [this](const Owner& str){
        return str == ui->ownerCombo->currentText();
    });
    if(it != m_owners.owners.end()) {
        m_wallet.info->owner = &*it;
    }

    m_wallet.info->canBeNegative = ui->canBeNegativeCheck->isChecked();
    m_wallet.info->availability = static_cast<Wallet::Availability::t>(ui->availabilityCombo->currentIndex());
}


void WalletPropertiesWindow::on_buttonBox_accepted()
{
    fillWalletFromGui();
}

} // namespace cashbook
