/* Driver only: expected geometry, weights and integrals are computed independently in Python. */
#include "Common/include/adaptation/CNativeField3D.hpp"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <stdexcept>
using namespace SU2Native3D;
int main() {
  std::cout << std::setprecision(21);
  size_t count;
  while (std::cin >> count) {
    std::vector<DonorCell> donors(count);
    for (auto& d : donors) {
      std::cin >> d.cell.id;
      for (size_t i = 0; i < 4; ++i) {
        auto& n = d.cell.v[i];
        auto& m = d.sensor[i];
        std::cin >> n.id >> n.p.x >> n.p.y >> n.p.z >> m.xx >> m.xy >> m.xz >> m.yy >> m.yz >> m.zz;
      }
    }
    std::sort(donors.begin(), donors.end(), [](const DonorCell& a, const DonorCell& b) { return a.Key() < b.Key(); });
    FrozenField field(donors, {}, true);
    std::vector<const Cell*> cells;
    for (const auto& d : donors) cells.push_back(&d.cell);
    size_t queries;
    std::cin >> queries;
    for (size_t i = 0; i < queries; ++i) {
      Point a, b;
      std::cin >> a.x >> a.y >> a.z >> b.x >> b.y >> b.z;
      if (!std::cin) return 2;
      try {
        const auto length = field.SensorEdgeLength(a, b);
        const auto pieces = TraceSegment(cells, a, b);
        std::cout << 1 << ' ' << length << ' ' << pieces.size();
        for (const auto& piece : pieces) {
          for (auto id : donors[piece.donor].Key()) std::cout << ' ' << id;
          std::cout << ' ' << piece.begin << ' ' << piece.end << ' ' << piece.width;
          for (auto w : piece.begin_weights) std::cout << ' ' << w;
          for (auto w : piece.end_weights) std::cout << ' ' << w;
        }
        std::cout << '\n';
      } catch (const std::runtime_error& e) {
        std::cout << 0 << '\n';
        std::cerr << "REJECT " << e.what() << '\n';
      }
    }
    const auto& s = field.Statistics();
    std::cerr << std::setprecision(17) << "STATS {\"donors\":" << donors.size() << ",\"requests\":" << s.edge_requests
              << ",\"failures\":" << s.edge_failures << ",\"candidates\":" << s.edge_box_candidates
              << ",\"pieces\":" << s.edge_pieces << ",\"maximum_candidates\":" << s.maximum_edge_candidates
              << ",\"maximum_pieces\":" << s.maximum_edge_pieces << ",\"index_bytes\":" << s.index_bytes
              << ",\"edge_seconds\":" << s.edge_seconds << ",\"search_seconds\":" << s.edge_search_seconds
              << ",\"trace_seconds\":" << s.edge_trace_seconds << ",\"integral_seconds\":" << s.edge_integral_seconds
              << ",\"exact_norms\":" << s.kernel.exact_norms << ",\"filtered_norms\":" << s.kernel.filtered_norms
              << "}\n";
  }
}
