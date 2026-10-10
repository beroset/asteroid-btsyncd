/*
 * Copyright (C) 2026 - The asteroid-btsyncd contributors
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "keyboardreports.h"

int main()
{
    if (!KeyboardReports::isValidKeyState(QByteArray(8, 0))
        || KeyboardReports::isValidKeyState(QByteArray(7, 0))
        || KeyboardReports::isValidKeyState(QByteArray(9, 0)))
        return 1;

    QByteArray state(8, 0);
    state[2] = 0x04;
    if (!KeyboardReports::isValidKeyState(state))
        return 2;
    state[1] = 1;
    if (KeyboardReports::isValidKeyState(state))
        return 3;
    state[1] = 0;
    state[2] = static_cast<char>(0x66);
    if (KeyboardReports::isValidKeyState(state))
        return 4;

    if (!KeyboardReports::isValidReportOutput(QByteArray::fromHex("021f"))
        || KeyboardReports::isValidReportOutput(QByteArray::fromHex("021f00"))
        || KeyboardReports::isValidReportOutput(QByteArray::fromHex("011f"))
        || KeyboardReports::isValidReportOutput(QByteArray::fromHex("02e0")))
        return 5;

    if (!KeyboardReports::isValidBootOutput(QByteArray::fromHex("1f"))
        || KeyboardReports::isValidBootOutput(QByteArray::fromHex("1f00"))
        || KeyboardReports::isValidBootOutput(QByteArray::fromHex("e0")))
        return 6;

    return 0;
}
