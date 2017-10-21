#include <QtGui/QApplication>
#include <QLocale>
#include <QTranslator>
#include <QMessageBox>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QString localeName = QLocale::system().name();

    QTranslator translator;
    translator.load(QString("ProgSkeet_%1").arg(localeName));
    a.installTranslator(&translator);

//#ifndef DEBUG
//    QMessageBox::StandardButton res = QMessageBox::warning(
//                NULL,
//                QApplication::translate("Global", "%1 warning").arg(TARGETNAME),
//                QApplication::translate("Global", "Dear user,\n"
//                                        "By proceeding, you acknowledge that this software is still in beta phase.\n"
//                                        "It may contain bugs and they should be reported in the forum.\n"),
//                QMessageBox::Yes | QMessageBox::No,
//                QMessageBox::No);

//    if (res == QMessageBox::No)
//        return 1;
//#endif

    MainWindow w;
    w.show();

    return a.exec();
}
