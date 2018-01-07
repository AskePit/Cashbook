#include "mainwindow.h"
#include "ui_mainwindow.h"

namespace cashbook
{

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    {
        auto food = t.addChild("Food");
        t.addChild("Rent");
        auto clothes = t.addChild("Clothes");
        auto study = t.addChild("Study");

        clothes->addChild("Pants");
        study->addChild("Internet-cources");
        study->addChild("Books");

        food->addChild("Bread");
        auto sweets = food->addChild("Sweets");
        auto meat = food->addChild("Meat");
        auto drinks = food->addChild("Drinks");

        sweets->addChild("Ice-Cream");
        sweets->addChild("Chocolate");
        sweets->addChild("Cookies");

        meat->addChild("Pork");
        auto fish = meat->addChild("Fish");
        meat->addChild("Chicken");

        fish->addChild("Salmon");
        fish->addChild("Cod");
        fish->addChild("Mackerel");
        fish->addChild("Cat-fish");
        fish->addChild("Perch");

        auto alcohol = drinks->addChild("Alcohol");
        auto alcoholFree = drinks->addChild("Alcohol-Free");

        alcohol->addChild("Vodka");
        auto coke = alcoholFree->addChild("Coke");

        coke->addChild("Coka-Cola");
        coke->addChild("Pepsi");
    }

    {
        auto he = w.addChild(Wallet("Он"));
        auto she = w.addChild(Wallet("Она"));

        auto his_cash = he->addChild(Wallet("Наличка"));
        his_cash->addChild(Wallet("Кошелек", 515.50));
        his_cash->addChild(Wallet("Карман", 21));
        his_cash->addChild(Wallet("Мелочь", 9.25));

        auto his_cards = he->addChild(Wallet("Карты"));
        his_cards->addChild(Wallet("Сбер", 15100.87));
        his_cards->addChild(Wallet("Бин", 905.14));
        auto his_deps = he->addChild(Wallet("Вклады"));
        his_deps->addChild(Wallet("Тиньк", 100000));


        auto her_cash = she->addChild(Wallet("Наличка"));
        her_cash->addChild(Wallet("Кошелек", 100));
        auto wallet2 = her_cash->addChild(Wallet("Кошелек2"));
        wallet2->addChild(Wallet("Отделение 1", 5000));
        wallet2->addChild(Wallet("Отделение 2", 650));
        wallet2->addChild(Wallet("Отделение 3", 50));
        wallet2->addChild(Wallet("Отсек для мелочи", 48.20));
        her_cash->addChild(Wallet("Карман", 5));

        auto her_cards = she->addChild(Wallet("Карты"));
        her_cards->addChild(Wallet("Сбер", 28700.0));
        auto her_deps = she->addChild(Wallet("Вклады"));
        her_deps->addChild(Wallet("Бин", 200000));
    }

    ui->categoriesTree->setModel(&t);
    ui->walletsTree->setModel(&w);

    ui->walletsTree->expandAll();
    ui->walletsTree->resizeColumnToContents(0);
}

MainWindow::~MainWindow()
{
    delete ui;
}

} // namespace cashbook


void cashbook::MainWindow::on_categoriesTree_clicked(const QModelIndex &index)
{
    ui->statusbar->showMessage(pathToString(t.getItem(index)));
}

void cashbook::MainWindow::on_walletsTree_clicked(const QModelIndex &index)
{
    ui->statusbar->showMessage(pathToString(w.getItem(index)));
}
