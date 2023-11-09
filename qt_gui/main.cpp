#include "vitalsign.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    VitalSign w;
    w.show();
    return a.exec();
}
