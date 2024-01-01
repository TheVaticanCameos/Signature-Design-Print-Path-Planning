#include "SignatureDesigner.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    SignatureDesigner w;
    w.show();
    return a.exec();
}
