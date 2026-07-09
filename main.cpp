#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName("softBy_DOCtorKamski");
    a.setApplicationName("gifmanager");
    MainWindow w;
    w.show();
    return a.exec();
}
