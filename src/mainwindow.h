#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QEvent>
#include "bookkeeping/bookkeeping.h"

namespace Ui {
class MainWindow;
}

namespace cashbook
{

class ClickFilter : public QObject {
    Q_OBJECT
public:
    virtual bool eventFilter(QObject *watched, QEvent *event) {
        if(event->type() == QEvent::MouseButtonPress) {
            emit mouseClicked(qobject_cast<QWidget *>(watched));
        }

        return QObject::eventFilter(watched, event);
    }

signals:
    void mouseClicked(QWidget *watched);
};

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

    void on_addShortPlanButton_clicked();
    void on_removeShortPlanButton_clicked();
    void on_upShortPlanButton_clicked();
    void on_downShortPlanButton_clicked();

    void on_addMiddlePlanButton_clicked();
    void on_removeMiddlePlanButton_clicked();
    void on_upMiddlePlanButton_clicked();
    void on_downMiddlePlanButton_clicked();

    void on_addLongPlanButton_clicked();
    void on_removeLongPlanButton_clicked();
    void on_upLongPlanButton_clicked();
    void on_downLongPlanButton_clicked();

    void on_addActiveTaskButton_clicked();
    void on_removeActiveTaskButton_clicked();
    void on_upActiveTaskButton_clicked();
    void on_downActiveTaskButton_clicked();

    void on_actionSave_triggered();

    void on_mainButton_clicked();
    void on_categoriesButton_clicked();
    void on_plansButton_clicked();

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

    void showUnanchoredSum();
    void hideUnanchoredSum();

    void showShortPlans(bool show);
    void showMiddlePlans(bool show);
    void showLongPlans(bool show);

    void showActiveTasks(bool show);
    void showCompletedTasks(bool show);

protected:
    void closeEvent(QCloseEvent *event);

private:
    Ui::MainWindow *ui;
    Data &m_data;
    LogItemDelegate m_logDelegate;
    PlanDelegate m_shortPlansDelegate;
    PlanDelegate m_middlePlansDelegate;
    PlanDelegate m_longPlansDelegate;
    TaskDelegate m_activeTasksDelegate;
    BoolDelegate m_boolDelegate;
    CategoriesViewEventFilter m_categoriesEventFilter;
    ClickFilter m_clickFilter;

    bool m_changed {false};
    QModelIndex m_noteContextIndex;
};

} // namespace cashbook

#endif // MAINWINDOW_H
