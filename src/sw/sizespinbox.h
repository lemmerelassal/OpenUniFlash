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

#include <stdint.h>
#include <QSpinBox>

class SizeSpinBox : public QSpinBox
{
    Q_OBJECT
public:
    explicit SizeSpinBox(QWidget* parent = 0);

    static QString formatSize(uint64_t value);
    static uint64_t scanSize(const QString& text);

protected:
    virtual QString textFromValue(int value) const;
    virtual int valueFromText(const QString& text) const;
    virtual QValidator::State validate(QString& text, int& pos) const;
};
