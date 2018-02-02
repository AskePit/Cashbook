#include "mainwindow.h"
#include <QApplication>
#include <QDebug>

#include "bookkeeping.h"
#include "types.h"

void fillDataExample(cashbook::Data &data)
{
    using namespace cashbook;

    auto &o = data.outCategories;
    auto &i = data.inCategories;
    auto &w = data.wallets;
    auto &u = data.owners;
    auto &l = data.log;

    auto food = o.addChild("Food");
    auto rent = o.addChild("Rent");
    auto clothes = o.addChild("Clothes");
    auto study = o.addChild("Study");

    auto pants = clothes->addChild("Pants");
    auto cources = study->addChild("Internet-cources");
    auto books = study->addChild("Books");

    auto bread = food->addChild("Bread");
    auto sweets = food->addChild("Sweets");
    auto meat = food->addChild("Meat");
    auto drinks = food->addChild("Drinks");

    auto iceCream = sweets->addChild("Ice-Cream");
    auto choco = sweets->addChild("Chocolate");
    auto cookies = sweets->addChild("Cookies");

    auto pork = meat->addChild("Pork");
    auto fish = meat->addChild("Fish");
    auto chicken = meat->addChild("Chicken");

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

    auto salary = i.addChild("Зарплата");
    auto sell = i.addChild("Продажа");
    auto deposit = i.addChild("Вклад");

    auto he = w.addChild(Wallet("Он"));
    auto she = w.addChild(Wallet("Она"));

    auto his_cash = he->addChild(Wallet("Наличка"));
    his_cash->addChild(Wallet("Кошелек", 515.50));
    his_cash->addChild(Wallet("Карман", 21));
    his_cash->addChild(Wallet("Мелочь", 9.25));

    auto his_cards = he->addChild(Wallet("Карты"));
    auto sber = his_cards->addChild(Wallet("Сбер", 15100.87));
    his_cards->addChild(Wallet("Бин", 905.14));
    auto his_deps = he->addChild(Wallet("Вклады"));
    his_deps->addChild(Wallet("Тиньк", 100000));


    auto her_cash = she->addChild(Wallet("Наличка"));
    auto wallet = her_cash->addChild(Wallet("Кошелек", 100));
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

    u.owners.push_back("He");
    u.owners.push_back("She");

    auto &log = l.log;
    {
        Transaction t;
        t.date = QDate(2018, 1, 4);
        t.note = "some food";
        t.type = Transaction::Type::Out;
        t.category = {chicken};
        t.amount = 518;
        t.from = wallet;
        log.push_back(t);
    }
    {
        Transaction t;
        t.date = QDate(2018, 1, 13);
        t.note = "rent";
        t.type = Transaction::Type::Out;
        t.category = {rent};
        t.amount = 13000;
        t.from = sber;
        log.push_back(t);
    }
    {
        Transaction t;
        t.date = QDate(2018, 1, 15);
        t.note = "salary";
        t.type = Transaction::Type::In;
        t.category = {salary};
        t.amount = 25705;
        t.to = sber;
        log.push_back(t);
    }
    {
        Transaction t;
        t.date = QDate(2018, 1, 30);
        t.note = "transfer";
        t.type = Transaction::Type::Transfer;
        t.amount = 25705;
        t.from = wallet;
        t.to = sber;
        log.push_back(t);
    }

    UNUSED(pants, cources, books, bread, iceCream, choco, cookies, pork, sell, deposit);
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    cashbook::Data data;
    //fillDataExample(data);

    cashbook::MainWindow w(data);
    w.show();

    return a.exec();
}
