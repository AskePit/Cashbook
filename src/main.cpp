#include "gui/forms/mainwindow.h"
#include <QApplication>
#include <QStyleFactory>
#include <QQuickWindow>

#include "bookkeeping/bookkeeping.h"

int main(int argc, char *argv[])
{
    QApplication::setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGL);

    QApplication a(argc, argv);

    cashbook::Data data;
    cashbook::MainWindow w(data);
    w.show();

    return a.exec();
}
