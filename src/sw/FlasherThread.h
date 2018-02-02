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

#pragma once

#include <QThread>

#include "Arguments.h"

/* Base class for all the flasher threads */
class FlasherThread : public QThread
{
    Q_OBJECT

public:
    FlasherThread(CommonArgs commonArgs);

signals:
    void progressChanged(const int cur);
    void progressRangeChanged(const int min, const int max);
    void progressSetText(const QString& text);
    void statusTextAdded(const QString& text);
    void RxSpeedUpdate(const int value);
    void TxSpeedUpdate(const int value);


public slots:
    virtual void onCanceled();

protected:
    void run();

    bool canceled;

    CommonArgs commonArgs;

private:
    virtual void Operation() = 0;
};
