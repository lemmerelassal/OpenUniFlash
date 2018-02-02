/*
OpenUniFlash - A universal NAND and NOR Flash programmer
Copyright (C) 2010-2018  Lemmer EL ASSAL, Axel GEMBE, Maël Blocteur

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
Also add information on how to contact you by electronic and paper mail.
*/

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
