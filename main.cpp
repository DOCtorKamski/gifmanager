#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setOrganizationName("gifmanager");
    a.setApplicationName("gifmanager");
    MainWindow w;
    w.show();
    return a.exec();
}
