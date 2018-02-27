#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "bookkeeping/bookkeeping.h"

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
    void on_anchoreTransactionsButton_clicked();

    void on_addWalletSiblingButton_clicked();
    void on_addWalletChildButton_clicked();
    void on_removeWalletButton_clicked();
    void on_upWalletButton_clicked();
    void on_downWalletButton_clicked();
    void on_outWalletButton_clicked();
    void on_inWalletButton_clicked();

    void on_addInCategorySiblingButton_clicked();
    void on_addInCategoryChildButton_clicked();
    void on_removeInCategoryButton_clicked();
    void on_upInCategoryButton_clicked();
    void on_downInCategoryButton_clicked();
    void on_outInCategoryButton_clicked();
    void on_inInCategoryButton_clicked();

    void on_addOutCategorySiblingButton_clicked();
    void on_addOutCategoryChildButton_clicked();
    void on_removeOutCategoryButton_clicked();
    void on_upOutCategoryButton_clicked();
    void on_downOutCategoryButton_clicked();
    void on_outOutCategoryButton_clicked();
    void on_inOutCategoryButton_clicked();

    void on_actionSave_triggered();
    void on_actionOpen_triggered();

    void on_mainButton_clicked();
    void on_categoriesButton_clicked();
    void on_usersButton_clicked();

    void on_thisMonthButton_clicked();
    void on_thisYearButton_clicked();
    void on_monthButton_clicked();
    void on_yearButton_clicked();

    void showInCategoryMenu(const QPoint& point);
    void showOutCategoryMenu(const QPoint& point);
    void showLogContextMenu(const QPoint& point);

    void on_actionInStatement_triggered();
    void on_actionOutStatement_triggered();
    void on_actionEditNote_triggered();

private:
    void loadFile();
    void saveFile();

    void onActionStatementTriggered(Transaction::Type::t type);

protected:
    void closeEvent(QCloseEvent *event);

private:
    Ui::MainWindow *ui;
    Data &m_data;
    LogItemDelegate m_logDelegate;
    BoolDelegate m_boolDelegate;
    CategoriesViewEventFilter m_categoriesEventFilter;

    bool m_changed {false};
    QModelIndex m_noteContextIndex;
};

} // namespace cashbook

#endif // MAINWINDOW_H
