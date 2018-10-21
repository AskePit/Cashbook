#ifndef SELECTWALLETDIALOG_H
#define SELECTWALLETDIALOG_H

#include <QDialog>
#include "bookkeeping/models.h"

namespace Ui {
class SelectWalletDialog;
}

namespace cashbook {

class SelectWalletDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SelectWalletDialog(WalletsModel &model, QWidget *parent = 0);
    ~SelectWalletDialog();

    const Node<Wallet>* getWalletNode();

private:
    Ui::SelectWalletDialog *ui;
    WalletsModel &m_model;
};

} // namespace cashbook

#endif // SELECTWALLETDIALOG_H
