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
ReferenceState DecodeReference(const std::string& bytes, size_t ceiling = SIZE_MAX);
/*! Collective root I/O and elected decoding. Missing file returns false; malformed files never rebase geometry. */
bool LoadReference(const std::string& filename, ReferenceState& state);
/*! Recoverable collective memory admission before byte transport and reference construction.
 * Success with loaded=false means missing file; refusal leaves state unchanged and reports elected reason.
 * maximum_bytes includes caller_bytes; malformed files remain fatal rather than rebasing geometry. */
bool LoadReference(const std::string& filename, ReferenceState& state, bool& loaded, std::string& reason,
                   size_t ceiling, size_t caller_bytes, size_t& maximum_bytes);
void WriteReference(const std::string& filename, const ReferenceState& state);
}  // namespace SU2Native3D
