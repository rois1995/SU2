#pragma once

#include <cctype>
#include <fstream>
#include <iterator>
#include <sstream>
#include <string>

#include "../../../Common/include/option_structure.hpp"
#include "../../../Common/include/parallelization/mpi_structure.hpp"

namespace legacy_metric_test {

inline std::string Pin(std::string text) {
  bool norm = false, hgrad = false;
  std::istringstream lines(text);
  for (std::string line; std::getline(lines, line);) {
    const auto equal = line.find('=');
    if (equal == std::string::npos || equal > line.find('%')) continue;
    std::string key;
    for (unsigned char c : line.substr(0, equal))
      if (!std::isspace(c)) key += static_cast<char>(std::toupper(c));
    norm |= key == "ADAP_NORM";
    hgrad |= key == "ADAP_HGRAD";
  }

  if ((!norm || !hgrad) && !text.empty() && text.back() != '\n') text += '\n';
  if (!norm) text += "ADAP_NORM= 1.0\n";
  if (!hgrad) text += "ADAP_HGRAD= 1.3\n";
  return text;
}

inline std::istringstream Stream(std::istream& input) {
  return std::istringstream(Pin(std::string(std::istreambuf_iterator<char>(input), {})));
}

inline std::string DriverFile(const std::string& path) {
  std::ifstream input(path);
  if (!input) return path;
  const std::string source(std::istreambuf_iterator<char>(input), {});
  const auto pinned = Pin(source);
  if (source == pinned) return path;
  const std::string generated = "def1_legacy_" + path.substr(path.find_last_of("/\\") + 1);
  if (SU2_MPI::GetRank() == MASTER_NODE) {
    std::ofstream output(generated);
    output << pinned;
    output.close();
    if (!output) SU2_MPI::Error("Cannot write the fixture's legacy metric configuration.", CURRENT_FUNCTION);
  }
  SU2_MPI::Barrier(SU2_MPI::GetComm());
  return generated;
}

}  // namespace legacy_metric_test
