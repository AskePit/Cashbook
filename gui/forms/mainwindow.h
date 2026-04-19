#pragma once

#include "bookkeeping/models.h"
#include "gui/modelsdelegate.h"
#include "gui/forms/analytics/analytics.h"

#include <QMainWindow>
#include <QEvent>

namespace Ui {
class MainWindow;
}

namespace cashbook
{

struct ViewModel {
    QAbstractItemView *view;
    QAbstractItemModel *model;
    QItemDelegate *delegate;
    std::function<void(const QPoint &)> contextMenu;
};

class ViewModelMap : public std::vector<ViewModel> {
public:
    void operator =(std::initializer_list<ViewModel> list);
    void connectModels();
};

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

    void on_removeCompletedTaskButton_clicked();

    void on_actionSave_triggered();

    void on_mainButton_clicked();
    void on_categoriesButton_clicked();
    void on_plansButton_clicked();
    void on_statisticsButton_clicked();

    void on_actionEditNote_triggered();
    void on_actionImportReceipt_triggered();
    void on_actionWalletProperties_triggered();
    void on_actionCategoryProperties_triggered();

    void on_walletsAnalysisCriteriaCombo_currentIndexChanged(int index);
    void on_walletsAnalysisOwnerCombo_currentIndexChanged(int index);
    void on_walletsAnalysisAvailabilityFromCombo_currentIndexChanged(int index);
    void on_walletsAnalysisAvailabilityToCombo_currentIndexChanged(int index);
    void on_walletsAnalysisBankCombo_currentIndexChanged(int index);
    void on_walletsAnalysisMoneyTypeCombo_currentIndexChanged(int index);

private:
    void preLoadSetup();
    void postLoadSetup();

    void loadData();
    void saveData();

    void updateUnanchoredSum();
    void showUnanchoredSum();
    void hideUnanchoredSum();

    void showShortPlans(bool show);
    void showMiddlePlans(bool show);
    void showLongPlans(bool show);

    void showActiveTasks(bool show);
    void showCompletedTasks(bool show);

    void showLogMenu(const QPoint& point);
    void showShortPlansMenu(const QPoint& point);
    void showMiddlePlansMenu(const QPoint& point);
    void showLongPlansMenu(const QPoint& point);
    void showPlansContextMenu(const QPoint& point);
    void showWalletContextMenu(const QPoint& point);
    void showCategoryContextMenu(QTreeView* tree, CategoriesModel& model, const QPoint& point);

    void updateAnalytics();
    void fetchAndImportSerbReceipt(const QString &url, const Node<Wallet> *wallet);

protected:
    void closeEvent(QCloseEvent *event);

private:
    Ui::MainWindow *ui;

    Data &m_data;
    DataModels m_models;
    ModelsDelegate m_modelsDelegate;
    BriefDelegate m_briefDelegate;
    ClickFilter m_clickFilter;

    QModelIndex m_noteContextIndex;

    QTreeView* m_categoryContextTree {nullptr};
    CategoriesModel* m_categoryContextModel {nullptr};

    // analytics
    WalletsAnalytics m_walletAnalytics;
    CategoriesAnalytics m_categoriesAnalytics;
    bool m_allowAnalyticsUpdate {false};
};

} // namespace cashbook
