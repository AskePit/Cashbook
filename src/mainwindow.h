#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "bookkeeping.h"

namespace Ui {
class MainWindow;
}

namespace cashbook
{

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(Data &data, QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_addUserButton_clicked();

    void on_removeUserButton_clicked();

    void on_upUserButton_clicked();

    void on_downUserButton_clicked();

    void on_addTransactionButton_clicked();

    void on_removeTransactionButton_clicked();

    void on_addWalletSiblingButton_clicked();

    void on_addWalletChildButton_clicked();

    void on_removeWalletButton_clicked();

private:
    Ui::MainWindow *ui;
    Data &m_data;
    LogItemDelegate m_logDelegate;
};

} // namespace cashbook

#endif // MAINWINDOW_H
