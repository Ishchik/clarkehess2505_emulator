#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.setWindowTitle("Clarke-hess 2505 emulator v1.0 15.05.19");
    w.show();

    return a.exec();
}
