#ifndef WALLETPROPERTIESWINDOW_H
#define WALLETPROPERTIESWINDOW_H

#include <QDialog>
#include "bookkeeping/bookkeeping.h"

namespace Ui {
class WalletPropertiesWindow;
}

namespace cashbook {

class WalletPropertiesWindow : public QDialog
{
    Q_OBJECT

public:
    explicit WalletPropertiesWindow(Wallet& wallet, const OwnersData& owners, const BanksData& banks, QWidget *parent = nullptr);
    ~WalletPropertiesWindow();

private slots:
    void on_buttonBox_accepted();

private:
    void fillGuiFromWallet();
    void fillWalletFromGui();

    void changeGuiByWalletType(int walletType);


    Ui::WalletPropertiesWindow *ui;

    Wallet& m_wallet;
    const OwnersData& m_owners;
    const BanksData& m_banks;
};

} // namespace cashbook

#endif // WALLETPROPERTIESWINDOW_H
