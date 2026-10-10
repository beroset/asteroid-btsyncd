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

#include <QtTest/QtTest>

class KeyboardReportsTest : public QObject
{
    Q_OBJECT

private slots:
    void keyStateValidation()
    {
        QVERIFY(KeyboardReports::isValidKeyState(QByteArray(KeyboardReports::KeyStateSize, 0)));
        QVERIFY(!KeyboardReports::isValidKeyState(QByteArray(KeyboardReports::KeyStateSize - 1, 0)));
        QVERIFY(!KeyboardReports::isValidKeyState(QByteArray(KeyboardReports::KeyStateSize + 1, 0)));

        QByteArray state(KeyboardReports::KeyStateSize, 0);
        state[KeyboardReports::KeyUsageOffset] = 0x04;
        QVERIFY(KeyboardReports::isValidKeyState(state));
        state[KeyboardReports::ReservedByteOffset] = 1;
        QVERIFY(!KeyboardReports::isValidKeyState(state));
        state[KeyboardReports::ReservedByteOffset] = 0;
        state[KeyboardReports::KeyUsageOffset] =
            static_cast<char>(KeyboardReports::MaxKeyUsage + 1);
        QVERIFY(!KeyboardReports::isValidKeyState(state));
    }

    void outputReportValidation()
    {
        QVERIFY(KeyboardReports::isValidReportOutput(QByteArray::fromHex("1f")));
        QVERIFY(!KeyboardReports::isValidReportOutput(QByteArray()));
        QVERIFY(!KeyboardReports::isValidReportOutput(QByteArray::fromHex("1f00")));
        QVERIFY(!KeyboardReports::isValidReportOutput(QByteArray::fromHex("e0")));
    }

    void bootOutputValidation()
    {
        QVERIFY(KeyboardReports::isValidBootOutput(QByteArray::fromHex("1f")));
        QVERIFY(!KeyboardReports::isValidBootOutput(QByteArray::fromHex("1f00")));
        QVERIFY(!KeyboardReports::isValidBootOutput(QByteArray::fromHex("e0")));
    }
};

QTEST_APPLESS_MAIN(KeyboardReportsTest)

#include "keyboardreports_test.moc"
