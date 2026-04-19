#include "categorypropertieswindow.h"
#include "ui_categorypropertieswindow.h"

namespace cashbook {

CategoryPropertiesWindow::CategoryPropertiesWindow(Category& category, QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::CategoryPropertiesWindow)
    , m_category(category)
{
    ui->setupUi(this);
    fillGuiFromCategory();
}

CategoryPropertiesWindow::~CategoryPropertiesWindow()
{
    delete ui;
}

void CategoryPropertiesWindow::fillGuiFromCategory()
{
    ui->regularCheck->setChecked(m_category.regular);
}

void CategoryPropertiesWindow::fillCategoryFromGui()
{
    m_category.regular = ui->regularCheck->isChecked();
}

void CategoryPropertiesWindow::on_buttonBox_accepted()
{
    fillCategoryFromGui();
}

} // namespace cashbook
