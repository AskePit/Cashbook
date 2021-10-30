#include "widgets/mainwindow.h"
#include <QApplication>
#include <QStyleFactory>

#include "bookkeeping/bookkeeping.h"

int main(int argc, char *argv[])
{
    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    QApplication a(argc, argv);

    cashbook::Data data;
    cashbook::MainWindow w(data);
    w.show();

    return a.exec();
}
