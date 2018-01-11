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
    void on_inCategoriesTree_clicked(const QModelIndex &index);
    void on_outCategoriesTree_clicked(const QModelIndex &index);
    void on_walletsTree_clicked(const QModelIndex &index);

private:
    Ui::MainWindow *ui;
    Data &m_data;
    LogItemDelegate m_logDelegate;
};

} // namespace cashbook

#endif // MAINWINDOW_H
