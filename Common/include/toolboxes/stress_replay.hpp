/* Same-state, same-grid diagnostic only. SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once
#include "stress_field.hpp"
#include <map>
#include <set>

namespace StressField {
class Replay {
  std::map<std::string, Tensor> records;
  std::set<std::string> used;
  std::string identity;
  bool reading;

 public:
  Replay(std::string fingerprint, bool read, const std::string& path)
      : identity(std::move(fingerprint)), reading(read) {
    if (!read) {
      Require(!std::ifstream(path).good(), "replay output already exists; use a new path");
      return;
    }
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    Require(file && file.tellg() > 0 && file.tellg() <= 64 * 1024 * 1024, "missing/oversized replay file");
    file.seekg(0);
    std::string magic, stored, checksum, payload;
    std::getline(file, magic);
    std::getline(file, stored);
    std::getline(file, checksum);
    payload.assign(std::istreambuf_iterator<char>(file), {});
    Require(magic == "SU2_STRESS_REPLAY_1 COUPLING_CONTROL_EXACT INTERNAL_UNITS",
            "invalid replay schema/purpose/units");
    Require(stored == identity, "replay mesh/state/model fingerprint mismatch");
    Require(checksum == SHA256(payload), "replay payload checksum mismatch");
    std::istringstream rows(payload);
    std::string line;
    while (std::getline(rows, line)) {
      std::istringstream record(line);
      std::string key, extra;
      Require(bool(record >> std::quoted(key)), "malformed replay key");
      Tensor value{};
      for (auto& row : value)
        for (double& x : row) Require(bool(record >> x), "truncated replay record");
      Validate(value);
      Require(!(record >> extra), "trailing replay record data");
      Require(
          key.size() == 64 && key.find_first_not_of("0123456789abcdef") == std::string::npos && records.size() < 300000,
          "invalid replay key or too many records");
      Require(records.emplace(key, value).second, "duplicate replay record");
    }
    Require(rows.eof() && !records.empty(), "empty/malformed replay payload");
  }
  static void Validate(const Tensor& value) {
    double scale = 1;
    for (const auto& row : value)
      for (double x : row) {
        Require(std::isfinite(x), "nonfinite replay tensor");
        scale = std::max(scale, std::abs(x));
      }
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j)
        Require(std::abs(value[i][j] - value[j][i]) <= 1e-12 * scale, "nonsymmetric replay tensor");
    Require(std::abs(value[0][0] + value[1][1] + value[2][2]) <= 1e-12 * scale, "nonzero replay trace");
  }
  Tensor Exchange(const std::string& query, const Tensor& value) {
    const auto key = SHA256(query);
    if (reading) {
      auto it = records.find(key);
      Require(it != records.end(), "missing replay query: changed location, state or reconstruction");
      used.insert(key);
      return it->second;
    }
    Validate(value);
    Require(records.size() < 300000, "replay record limit exceeded");
    auto inserted = records.emplace(key, value);
    Require(inserted.second || inserted.first->second == value, "inconsistent repeated replay query");
    return value;
  }
  void Finish(const std::string& path) const {
    Require(!records.empty(), "no stress queries captured/replayed");
    if (reading) {
      Require(used.size() == records.size(), "unused replay records: assembly paths differ");
      return;
    }
    std::ostringstream data;
    data << std::setprecision(17);
    for (const auto& entry : records) {
      data << std::quoted(entry.first);
      for (const auto& row : entry.second)
        for (double x : row) data << ' ' << x;
      data << '\n';
    }
    const auto payload = data.str();
    Require(payload.size() <= 64 * 1024 * 1024, "replay payload too large");
    Require(!std::ifstream(path).good(), "refusing to overwrite replay output");
    std::ofstream file(path, std::ios::binary);
    file << "SU2_STRESS_REPLAY_1 COUPLING_CONTROL_EXACT INTERNAL_UNITS\n"
         << identity << '\n'
         << SHA256(payload) << '\n'
         << payload;
    file.close();
    Require(bool(file), "failed writing replay file");
  }
};
}  // namespace StressField
