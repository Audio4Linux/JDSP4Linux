#include <eeleditor.h>
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle(QStyleFactory::create("Fusion"));
    EELEditor w;
    w.show();
    return a.exec();
}
