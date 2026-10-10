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

#ifndef KEYBOARDREPORTS_H
#define KEYBOARDREPORTS_H

#include <QByteArray>

namespace KeyboardReports {

inline constexpr int ReservedByteOffset = 1;
inline constexpr int KeyUsageOffset = 2;
inline constexpr int KeyCount = 6;
inline constexpr int KeyStateSize = 1 + 1 + KeyCount;
inline constexpr int ModifierCount = 8;
inline constexpr unsigned char MaxKeyUsage = 0x65;
inline constexpr unsigned char InputReportId = 1;
inline constexpr unsigned char OutputReportId = 2;
inline constexpr unsigned int LedCount = 5;
inline constexpr unsigned int LedPaddingBits = 3;
inline constexpr unsigned char LedReservedMask =
    ((1U << LedPaddingBits) - 1U) << LedCount;
inline constexpr int OutputReportSize = 1;
inline constexpr unsigned char InputReportType = 1;
inline constexpr unsigned char OutputReportType = 2;
inline constexpr unsigned char SuspendControlPointValue = 0;
inline constexpr unsigned char ExitSuspendControlPointValue = 1;
inline constexpr unsigned char BootProtocolMode = 0;
inline constexpr unsigned char ReportProtocolMode = 1;

inline bool isValidControlPoint(const QByteArray &value)
{
    return value.size() == 1
        && static_cast<unsigned char>(value.at(0)) <= ExitSuspendControlPointValue;
}

inline bool isValidProtocolMode(const QByteArray &value)
{
    return value.size() == 1
        && static_cast<unsigned char>(value.at(0)) <= ReportProtocolMode;
}

inline bool isValidKeyState(const QByteArray &state)
{
    if (state.size() != KeyStateSize || state.at(ReservedByteOffset) != 0)
        return false;

    for (int i = KeyUsageOffset; i < state.size(); ++i) {
        if (static_cast<unsigned char>(state.at(i)) > MaxKeyUsage)
            return false;
    }
    return true;
}

inline bool isValidReportOutput(const QByteArray &value)
{
    return value.size() == OutputReportSize
        && (static_cast<unsigned char>(value.at(0)) & LedReservedMask) == 0;
}

inline bool isValidBootOutput(const QByteArray &value)
{
    return isValidReportOutput(value);
}

} // namespace KeyboardReports

#endif // KEYBOARDREPORTS_H
