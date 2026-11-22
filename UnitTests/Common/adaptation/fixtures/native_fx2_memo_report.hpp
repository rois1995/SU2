/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once

#include "../../../../Common/include/adaptation/CNativeBoundaryParameters3D.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unistd.h>
#include <vector>

namespace NativeFX2Fixture {
inline std::string Sha256(const std::vector<char>& bytes) {
  char path[] = "/tmp/su2-fx2-digest-XXXXXX";
  const int descriptor = mkstemp(path);
  if (descriptor < 0) throw std::runtime_error("FX2 could not create a private digest input.");
  struct RemoveInput {
    const char* path;
    ~RemoveInput() { std::remove(path); }
  } remove_input{path};
  auto* input = fdopen(descriptor, "wb");
  if (!input) {
    close(descriptor);
    throw std::runtime_error("FX2 could not open the digest input.");
  }
  const auto written = std::fwrite(bytes.data(), 1, bytes.size(), input);
  const int flushed = std::fclose(input);
  if (written != bytes.size() || flushed != 0) throw std::runtime_error("FX2 could not write the digest input.");

  const auto command = std::string("sha256sum -- ") + path;
  auto* output = popen(command.c_str(), "r");
  if (!output) throw std::runtime_error("FX2 could not start sha256sum.");
  std::array<char, 256> line{};
  const bool read = std::fgets(line.data(), line.size(), output) != nullptr;
  const int status = pclose(output);
  const std::string digest(line.data(), 64);
  if (!read || status != 0 || line[64] != ' ' || digest.find_first_not_of("0123456789abcdef") != std::string::npos)
    throw std::runtime_error("FX2 could not read a SHA256 digest.");
  return digest;
}

inline std::string CallRow(const SU2NativeGeometry::AtlasBudget* budget, const char* exception,
                           const std::string& reason, const SU2Native3D::BoundaryParameterState* state) {
  std::ostringstream out;
  if (budget) {
    out << "work=" << budget->work << " work_exhausted=" << budget->work_exhausted
        << " time_exhausted=" << budget->time_exhausted << " next_time_check=" << budget->next_time_check
        << " caller_bytes=" << budget->caller_bytes << " maximum_bytes=" << budget->maximum_bytes
        << " peak_bytes=" << budget->peak_bytes << " peak_over_maximum=" << (budget->peak_bytes > budget->maximum_bytes);
  } else {
    out << "work=unavailable work_exhausted=unavailable time_exhausted=unavailable next_time_check=unavailable"
           " caller_bytes=unavailable maximum_bytes=unavailable peak_bytes=unavailable peak_over_maximum=unavailable";
  }
  out << " exception=" << exception << " message=" << std::quoted(reason)
      << " snapshot_sha256=" << (state ? Sha256(state->Snapshot()) : "none")
      << " payload_sha256=" << (state ? Sha256(state->Payload()) : "none");
  return out.str();
}
}  // namespace NativeFX2Fixture
