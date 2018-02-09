#include "mainwindow.h"
#include <QApplication>
#include <QDebug>

#include "bookkeeping/bookkeeping.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    cashbook::Data data;
    cashbook::MainWindow w(data);
    w.show();

    return a.exec();
}
