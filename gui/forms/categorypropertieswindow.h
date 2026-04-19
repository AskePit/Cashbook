#pragma once

#include <QDialog>
#include "bookkeeping/bookkeeping.h"

namespace Ui {
class CategoryPropertiesWindow;
}

namespace cashbook {

class CategoryPropertiesWindow : public QDialog
{
    Q_OBJECT

public:
    explicit CategoryPropertiesWindow(Category& category, QWidget *parent = nullptr);
    ~CategoryPropertiesWindow();

private slots:
    void on_buttonBox_accepted();

private:
    void fillGuiFromCategory();
    void fillCategoryFromGui();

    Ui::CategoryPropertiesWindow *ui;

    Category& m_category;
};

} // namespace cashbook
