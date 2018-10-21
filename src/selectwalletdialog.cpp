#include "selectwalletdialog.h"
#include "ui_selectwalletdialog.h"

namespace cashbook {

SelectWalletDialog::SelectWalletDialog(WalletsModel &model, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SelectWalletDialog),
    m_model(model)
{
    ui->setupUi(this);
    ui->pushButton->setModel(model);
}

SelectWalletDialog::~SelectWalletDialog()
{
    delete ui;
}

const Node<Wallet>* SelectWalletDialog::getWalletNode()
{
    return ui->pushButton->node();
}

} // namespace cashbook
