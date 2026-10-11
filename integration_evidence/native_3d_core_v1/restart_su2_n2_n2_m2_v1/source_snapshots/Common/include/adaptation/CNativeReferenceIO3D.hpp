/*!
 * \file CNativeReferenceIO3D.hpp
 * \brief Versioned bounded immutable faceted native3D reference for adapted mesh reloads.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

#pragma once
#include "CNativeImport3D.hpp"
namespace SU2Native3D {
constexpr size_t ReferenceFileLimit3D = 1024 * 1024;
std::string EncodeReference(const ReferenceState& state);
ReferenceState DecodeReference(const std::string& bytes);
/*! Collective root I/O and elected decoding. Missing file returns false; malformed files never rebase geometry. */
bool LoadReference(const std::string& filename, ReferenceState& state);
void WriteReference(const std::string& filename, const ReferenceState& state);
}  // namespace SU2Native3D
