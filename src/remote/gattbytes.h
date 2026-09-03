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

#ifndef GATTBYTES_H
#define GATTBYTES_H

#include <QByteArray>

#include <optional>
#include <type_traits>

// Small, header-only helpers for safely decoding the little-endian byte
// layouts used by several GATT characteristics (ANCS, CTS, ...). The bytes
// being decoded here always originate from the paired central device, so
// every access is bounds-checked against the actual buffer size instead of
// trusting the caller's offset/length arithmetic to be correct. Both ANCS
// and CTS used to hand-roll their own (unchecked, in ANCS's case) byte
// indexing; sharing one small, checked primitive removes that duplication
// and the associated out-of-bounds-read risk in one place.
//
// This deliberately stays on plain templates/QByteArray rather than
// std::span, since the wider project currently targets C++17 (bumping to
// C++20 for std::span/concepts was tried and found to make an unrelated,
// pre-existing QStringView comparison in notificationservice.cpp ambiguous
// under this Qt version -- not worth the churn for this one helper).
namespace GattBytes {

// Decodes an unsigned little-endian integer of `length` bytes (1..sizeof(T))
// starting at `offset`. Returns std::nullopt instead of reading out of
// bounds if [offset, offset + length) does not fit within `bytes`.
template <typename T = unsigned int>
std::optional<T> readLittleEndian(const QByteArray &bytes, int offset, int length)
{
    static_assert(std::is_unsigned<T>::value, "readLittleEndian<T> requires an unsigned integer type");

    if (length <= 0 || static_cast<size_t>(length) > sizeof(T) || offset < 0
            || offset + length > bytes.size())
        return std::nullopt;

    T result{};
    for (int i = length - 1; i >= 0; i--)
        result = static_cast<T>((result << 8) | static_cast<unsigned char>(bytes[offset + i]));
    return result;
}

} // namespace GattBytes

#endif // GATTBYTES_H
