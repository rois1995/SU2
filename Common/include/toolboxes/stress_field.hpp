/* Fixed physical, kinematic stress fields for the experimental SA/SST interface.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <exception>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <limits>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace StressField {
using Point = std::array<double, 3>;
using Tensor = std::array<Point, 3>;

inline void Require(bool valid, const std::string& message) {
  if (!valid) throw std::runtime_error("Stress field: " + message);
}

// SHA-256 also binds geometry and units into the restart identity.
inline std::string SHA256(const std::string& bytes) {
  const uint32_t constants[64] = {
      0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
      0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
      0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
      0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
      0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
      0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
      0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
      0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};
  uint32_t h[8] = {0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19};
  std::string data = bytes;
  data += char(0x80);
  while (data.size() % 64 != 56) data += char(0);
  const uint64_t bits = uint64_t(bytes.size()) * 8;
  for (int i = 7; i >= 0; --i) data += char(bits >> (8 * i));
  auto rotate = [](uint32_t x, int n) { return (x >> n) | (x << (32 - n)); };
  for (size_t offset = 0; offset < data.size(); offset += 64) {
    uint32_t w[64];
    for (int i = 0; i < 16; ++i) {
      w[i] = 0;
      for (int j = 0; j < 4; ++j) w[i] = (w[i] << 8) | uint8_t(data[offset + 4 * i + j]);
    }
    for (int i = 16; i < 64; ++i) {
      const auto a = rotate(w[i - 15], 7) ^ rotate(w[i - 15], 18) ^ (w[i - 15] >> 3);
      const auto b = rotate(w[i - 2], 17) ^ rotate(w[i - 2], 19) ^ (w[i - 2] >> 10);
      w[i] = w[i - 16] + a + w[i - 7] + b;
    }
    uint32_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], z = h[7];
    for (int i = 0; i < 64; ++i) {
      const uint32_t t1 =
          z + (rotate(e, 6) ^ rotate(e, 11) ^ rotate(e, 25)) + ((e & f) ^ (~e & g)) + constants[i] + w[i];
      const uint32_t t2 = (rotate(a, 2) ^ rotate(a, 13) ^ rotate(a, 22)) + ((a & b) ^ (a & c) ^ (b & c));
      z = g;
      g = f;
      f = e;
      e = d + t1;
      d = c;
      c = b;
      b = a;
      a = t1 + t2;
    }
    h[0] += a;
    h[1] += b;
    h[2] += c;
    h[3] += d;
    h[4] += e;
    h[5] += f;
    h[6] += g;
    h[7] += z;
  }
  std::ostringstream out;
  for (auto x : h) out << std::hex << std::setw(8) << std::setfill('0') << x;
  return out.str();
}

inline Tensor FromCoefficients(const std::array<double, 5>& c) {
  const double a = c[0] / std::sqrt(2.0), b = c[1] / std::sqrt(6.0);
  return {{{a + b, c[2] / std::sqrt(2.0), c[3] / std::sqrt(2.0)},
           {c[2] / std::sqrt(2.0), -a + b, c[4] / std::sqrt(2.0)},
           {c[3] / std::sqrt(2.0), c[4] / std::sqrt(2.0), -2 * b}}};
}

class Field {
  mutable std::atomic<uint64_t> face_queries{0}, source_queries{0}, other_queries{0}, failed_queries{0}, query_ns{0};
  struct QueryTimer {
    const Field& field;
    const int exceptions = std::uncaught_exceptions();
    const std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    ~QueryTimer() {
      field.query_ns.fetch_add(std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now()-start).count(), std::memory_order_relaxed);
      if (std::uncaught_exceptions() > exceptions) field.failed_queries.fetch_add(1, std::memory_order_relaxed);
    }
  };
  struct Node {
    Point x;
    std::array<double, 5> c;
  };
  struct Cell {
    std::array<uint32_t, 4> nodes{};
    Tensor inverse{};
    Point lower{}, upper{};
    double volume;
  };
  struct Box {
    Point lower{}, upper{};
    size_t begin, end;
    int left = -1, right = -1;
  };
  struct Wall {
    Point normal;
    double offset, weight;
  };
  int dim = 0;
  double uref = 0, epsilon = 0, ell = 0, wall_length = 0;
  double volume = 0, norm_inf = 0, norm_h1 = 0, diameter = 0;
  std::string identity, scenario, purpose, geometry, support;
  std::vector<Node> nodes;
  std::vector<Cell> cells;
  std::vector<Wall> walls;
  std::vector<size_t> order;
  std::vector<Box> boxes;
  Point support_lower{}, support_upper{}, support_width{}, center{};
  double radius = 0, taper_start = 0, taper_end = 0;

  int BuildBox(size_t begin, size_t end) {
    Box box;
    box.begin = begin;
    box.end = end;
    box.lower.fill(std::numeric_limits<double>::infinity());
    box.upper.fill(-std::numeric_limits<double>::infinity());
    for (size_t p = begin; p < end; ++p)
      for (int j = 0; j < dim; ++j) {
        box.lower[j] = std::min(box.lower[j], cells[order[p]].lower[j]);
        box.upper[j] = std::max(box.upper[j], cells[order[p]].upper[j]);
      }
    const int index = boxes.size();
    boxes.push_back(box);
    if (end - begin > 8) {
      int axis = 0;
      for (int j = 1; j < dim; ++j)
        if (box.upper[j] - box.lower[j] > box.upper[axis] - box.lower[axis]) axis = j;
      const size_t middle = (begin + end) / 2;
      std::nth_element(order.begin() + begin, order.begin() + middle, order.begin() + end, [&](size_t a, size_t b) {
        const double x = cells[a].lower[axis] + cells[a].upper[axis], y = cells[b].lower[axis] + cells[b].upper[axis];
        return x == y ? a < b : x < y;
      });
      const int left = BuildBox(begin, middle), right = BuildBox(middle, end);
      boxes[index].left = left;
      boxes[index].right = right;
    }
    return index;
  }

  bool Weights(size_t id, const Point& x, std::array<double, 4>& w) const {
    const auto& cell = cells[id];
    w = {1, 0, 0, 0};
    for (int i = 0; i < dim; ++i) {
      for (int j = 0; j < dim; ++j) w[i + 1] += cell.inverse[i][j] * (x[j] - nodes[cell.nodes[0]].x[j]);
      w[0] -= w[i + 1];
    }
    for (int i = 0; i <= dim; ++i)
      if (w[i] < -1e-12 || w[i] > 1 + 1e-12) return false;
    return true;
  }

  void Locate(int index, const Point& x, size_t& found) const {
    const auto& box = boxes[index];
    for (int j = 0; j < dim; ++j)
      if (x[j] < box.lower[j] || x[j] > box.upper[j]) return;
    if (box.left >= 0) {
      Locate(box.left, x, found);
      Locate(box.right, x, found);
      return;
    }
    std::array<double, 4> w;
    for (size_t p = box.begin; p < box.end; ++p)
      if (order[p] < found && Weights(order[p], x, w)) found = order[p];
  }

  void Audit() {
    std::vector<std::array<uint32_t, 4>> connectivity;
    connectivity.reserve(cells.size());
    for (const auto& cell : cells) {
      auto ids = cell.nodes;
      std::sort(ids.begin(), ids.end());
      connectivity.push_back(ids);
    }
    std::sort(connectivity.begin(), connectivity.end());
    Require(std::adjacent_find(connectivity.begin(), connectivity.end()) == connectivity.end(), "duplicate simplices");
    double integral = 0;
    for (const auto& node : nodes) {
      double n = 0;
      for (double c : node.c) n += c * c;
      norm_inf = std::max(norm_inf, std::sqrt(n));
      if (dim == 2) Require(node.x[2] == 0 && node.c[3] == 0 && node.c[4] == 0, "planar field needs z=c3=c4=0");
    }
    for (auto& cell : cells) {
      Tensor a{};
      for (int j = 0; j < dim; ++j)
        for (int i = 0; i < dim; ++i) a[i][j] = nodes[cell.nodes[j + 1]].x[i] - nodes[cell.nodes[0]].x[i];
      double det = 0;
      if (dim == 2) {
        det = a[0][0] * a[1][1] - a[0][1] * a[1][0];
        Require(det > 0 && std::isfinite(det), "nonpositive triangle orientation");
        cell.inverse[0] = {a[1][1] / det, -a[0][1] / det, 0};
        cell.inverse[1] = {-a[1][0] / det, a[0][0] / det, 0};
      } else {
        for (int i = 0; i < 3; ++i) {
          const int j = (i + 1) % 3, k = (i + 2) % 3;
          cell.inverse[i] = {a[1][j] * a[2][k] - a[1][k] * a[2][j], a[2][j] * a[0][k] - a[2][k] * a[0][j],
                             a[0][j] * a[1][k] - a[0][k] * a[1][j]};
        }
        for (int i = 0; i < 3; ++i) det += a[0][i] * cell.inverse[i][0];
        Require(det > 0 && std::isfinite(det), "nonpositive tetrahedron orientation");
        for (auto& row : cell.inverse)
          for (auto& v : row) v /= det;
      }
      cell.volume = det / (dim == 2 ? 2 : 6);
      volume += cell.volume;
      cell.lower = cell.upper = nodes[cell.nodes[0]].x;
      double mass = 0, stiffness = 0;
      for (int c = 0; c < 5; ++c) {
        double sum = 0, squares = 0;
        for (int i = 0; i <= dim; ++i) {
          double v = nodes[cell.nodes[i]].c[c];
          sum += v;
          squares += v * v;
        }
        mass += (sum * sum + squares) / ((dim + 1) * (dim + 2));
        for (int j = 0; j < dim; ++j) {
          double gradient = 0;
          for (int i = 0; i < dim; ++i)
            gradient += cell.inverse[i][j] * (nodes[cell.nodes[i + 1]].c[c] - nodes[cell.nodes[0]].c[c]);
          stiffness += gradient * gradient;
        }
      }
      integral += cell.volume * (mass + ell * ell * stiffness);
      for (int i = 0; i <= dim; ++i) {
        for (int j = 0; j < dim; ++j) {
          cell.lower[j] = std::min(cell.lower[j], nodes[cell.nodes[i]].x[j]);
          cell.upper[j] = std::max(cell.upper[j], nodes[cell.nodes[i]].x[j]);
        }
        for (int k = 0; k < i; ++k) {
          double d = 0;
          for (int j = 0; j < dim; ++j) d += std::pow(nodes[cell.nodes[i]].x[j] - nodes[cell.nodes[k]].x[j], 2);
          diameter = std::max(diameter, std::sqrt(d));
        }
      }
    }
    norm_h1 = std::sqrt(integral / volume);
    Require(std::isfinite(norm_h1) && std::isfinite(norm_inf), "nonfinite raw norm");
    if (purpose == "SCENARIO")
      Require(std::max(norm_inf, norm_h1) <= epsilon * (1 + 1e-10), "raw field exceeds declared bounds");
    order.resize(cells.size());
    std::iota(order.begin(), order.end(), 0);
    BuildBox(0, cells.size());
  }

 public:
  explicit Field(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    Require(bool(file), "cannot open " + filename);
    const auto length = file.tellg();
    Require(length > 0 && length <= 64 * 1024 * 1024, "file must be between 1 byte and 64 MiB per rank");
    file.seekg(0);
    std::string bytes(size_t(length), '\0');
    file.read(&bytes[0], length);
    Require(bool(file), "short read");
    identity = SHA256(bytes);
    const auto separator = bytes.find("\nDATA\n");
    Require(separator != std::string::npos, "missing DATA delimiter");
    std::istringstream header(bytes.substr(0, separator));
    auto key = [&](const char* expected) {
      std::string s;
      header >> s;
      Require(s == expected, std::string("expected ") + expected);
    };
    auto number = [&]() {
      double x;
      Require(bool(header >> x) && std::isfinite(x), "invalid numeric metadata");
      return x;
    };
    key("SU2_STRESS_FIELD");
    int version = 0;
    Require(bool(header >> version) && version == 1, "unsupported schema version");
    key("SCENARIO");
    header >> scenario;
    Require(!scenario.empty() && scenario.size() <= 128, "invalid scenario id");
    key("PURPOSE");
    header >> purpose;
    Require(purpose == "SCENARIO" || purpose == "COUPLING_CONTROL", "unknown purpose");
    key("FRAME");
    std::string frame;
    header >> frame;
    Require(frame == "CARTESIAN_METERS", "only physical Cartesian metres supported");
    key("DIM");
    header >> dim;
    Require(dim == 2 || dim == 3, "dimension must be 2 or 3");
    key("COUNTS");
    size_t nn = 0, nc = 0;
    header >> nn >> nc;
    Require(nn >= size_t(dim + 1) && nn <= 100000 && nc > 0 && nc <= 300000,
            "replicated prototype limit: 100000 nodes / 300000 cells");
    key("SCALES");
    uref = number();
    const double lref = number();
    epsilon = number();
    ell = number();
    wall_length = number();
    Require(uref > 0 && lref > 0 && epsilon >= 0 && ell > 0 && wall_length >= 0, "invalid reference scales");
    key("NORMS");
    const double stored_inf = number(), stored_h1 = number(), stored_volume = number(), stored_h = number();
    key("GEOMETRY");
    header >> geometry;
    Require(geometry == "NONE" || geometry == "PLANES" || geometry == "CIRCLE", "unsupported geometry provider");
    if (geometry == "PLANES") {
      size_t count = 0;
      header >> count;
      Require(count > 0 && count <= 32 && wall_length > 0, "invalid wall count or wall length");
      for (size_t i = 0; i < count; ++i) {
        Wall w;
        for (auto& n : w.normal) n = number();
        w.offset = number();
        w.weight = number();
        double n = 0;
        for (double x : w.normal) n += x * x;
        Require(std::abs(n - 1) < 1e-12 && w.weight >= 0, "wall normals must be unit length and weights nonnegative");
        if (dim == 2) Require(w.normal[2] == 0, "planar wall normal has z component");
        walls.push_back(w);
      }
    } else if (geometry == "CIRCLE") {
      for (auto& x : center) x = number();
      radius = number();
      Require(dim == 2 && center[2] == 0 && radius > 0 && wall_length > 0,
              "circle requires a planar field and positive radius/length");
    }
    key("SUPPORT");
    header >> support;
    Require(support == "NONE" || support == "BOX" || support == "RADIAL", "unsupported support");
    if (support == "BOX") {
      for (auto& x : support_lower) x = number();
      for (auto& x : support_upper) x = number();
      for (auto& x : support_width) x = number();
      for (int j = 0; j < dim; ++j)
        Require(support_upper[j] > support_lower[j] && support_width[j] > 0 &&
                    2 * support_width[j] <= support_upper[j] - support_lower[j],
                "invalid box taper");
    } else if (support == "RADIAL") {
      Require(geometry == "CIRCLE", "radial support requires circular geometry");
      taper_start = number();
      taper_end = number();
      Require(radius < taper_start && taper_start < taper_end, "invalid radial taper");
    }
    key("ENCODING");
    std::string encoding;
    header >> encoding;
    Require(encoding == "TEXT" || encoding == "BINARY_LE", "unsupported encoding");
    key("PAYLOAD_SHA256");
    std::string digest;
    header >> digest;
    std::string extra;
    Require(!(header >> extra), "unexpected header content");
    const std::string payload = bytes.substr(separator + 6);
    Require(digest.size() == 64 && SHA256(payload) == digest, "payload SHA-256 mismatch");
    nodes.resize(nn);
    cells.resize(nc);
    std::istringstream text(payload);
    size_t cursor = 0;
    auto read = [&](bool integer) {
      double value = 0;
      if (encoding == "TEXT")
        Require(bool(text >> value), "truncated numeric payload");
      else {
        const size_t width = integer ? 4 : 8;
        Require(cursor + width <= payload.size(), "short binary payload");
        uint64_t bits = 0;
        for (size_t j = 0; j < width; ++j) bits |= uint64_t(uint8_t(payload[cursor++])) << (8 * j);
        if (integer)
          value = double(bits);
        else {
          static_assert(sizeof(double) == 8, "float64 required");
          std::memcpy(&value, &bits, 8);
        }
      }
      Require(std::isfinite(value), "NaN/Inf payload");
      return value;
    };
    for (auto& node : nodes) {
      for (auto& x : node.x) x = read(false);
      for (auto& c : node.c) c = read(false);
    }
    for (auto& cell : cells)
      for (int j = 0; j <= dim; ++j) {
        double id = read(true);
        Require(id >= 0 && id < nn && id == std::floor(id), "invalid zero-based node index");
        cell.nodes[j] = uint32_t(id);
      }
    Require(encoding == "TEXT" ? !(text >> extra) : cursor == payload.size(), "trailing payload content");
    Audit();
    auto agrees = [](double x, double y) {
      return std::abs(x - y) <= 1e-10 * std::max(std::abs(x), std::abs(y)) + 1e-14;
    };
    Require(agrees(norm_inf, stored_inf) && agrees(norm_h1, stored_h1) && agrees(volume, stored_volume) &&
                agrees(diameter, stored_h),
            "norm/measure/diameter certificate mismatch");
  }

  Tensor Evaluate(const Point& x, char kind = 'N') const {
    (kind == 'F' ? face_queries : kind == 'S' ? source_queries : other_queries).fetch_add(1, std::memory_order_relaxed);
    QueryTimer timer{*this};
    for (double v : x) Require(std::isfinite(v), "nonfinite query coordinate");
    if (dim == 2) Require(x[2] == 0, "out-of-plane query");
    auto smooth = [](double t) {
      t = std::max(0.0, std::min(1.0, t));
      return t * t * t * (10 + t * (-15 + 6 * t));
    };
    double chi = 1, r = 0;
    if (support == "BOX")
      for (int j = 0; j < dim; ++j)
        chi *=
            smooth((x[j] - support_lower[j]) / support_width[j]) * smooth((support_upper[j] - x[j]) / support_width[j]);
    if (geometry == "CIRCLE") {
      r = std::hypot(x[0] - center[0], x[1] - center[1]);
      Require(r >= radius - 1e-12, "query inside circular solid");
      if (std::abs(r - radius) <= 1e-12) return {};
      if (support == "RADIAL") chi = 1 - smooth((r - taper_start) / (taper_end - taper_start));
    }
    Tensor l{{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
    double sum = 0;
    auto wall = [&](const Point& n, double d, double weight) {
      Require(d >= -1e-12, "query on solid side of plane");
      if (weight == 0) return false;
      if (d <= 1e-12) return true;
      const double b = weight * wall_length / d;
      Require(std::isfinite(b), "wall scale overflow");
      sum += b;
      Point v{};
      for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) v[i] += l[i][j] * n[j];
      double denom = 1 / b;
      for (int i = 0; i < 3; ++i) denom += n[i] * v[i];
      for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j) l[i][j] -= v[i] * v[j] / denom;
      return false;
    };
    for (const auto& w : walls) {
      double d = -w.offset;
      for (int j = 0; j < 3; ++j) d += w.normal[j] * x[j];
      if (wall(w.normal, d, w.weight)) return {};
    }
    if (geometry == "CIRCLE") wall({(x[0] - center[0]) / r, (x[1] - center[1]) / r, 0}, r - radius, 1);
    if (chi == 0) return {};
    size_t found = cells.size();
    Locate(0, x, found);
    Require(found < cells.size(), "nonzero query outside field mesh");
    std::array<double, 4> w;
    Weights(found, x, w);
    std::array<double, 5> c{};
    for (int i = 0; i <= dim; ++i)
      for (int j = 0; j < 5; ++j) c[j] += w[i] * nodes[cells[found].nodes[i]].c[j];
    const auto b = FromCoefficients(c);
    Tensor result{};
    for (int i = 0; i < 3; ++i)
      for (int j = 0; j < 3; ++j)
        for (int k = 0; k < 3; ++k)
          for (int m = 0; m < 3; ++m) result[i][j] += l[i][k] * b[k][m] * l[m][j];
    const double trace = (result[0][0] + result[1][1] + result[2][2]) / 3;
    for (int i = 0; i < 3; ++i) result[i][i] -= trace;
    const double factor = uref * uref * chi / ((1 + sum) * (1 + sum));
    for (auto& row : result)
      for (auto& v : row) {
        v *= factor;
        Require(std::isfinite(v), "nonfinite mapped stress");
      }
    return result;
  }

  const std::string& Identity() const { return identity; }
  const std::string& Purpose() const { return purpose; }
  const std::string& Geometry() const { return geometry; }
  int Dimension() const { return dim; }
  size_t MemoryBytes() const {
    return nodes.capacity() * sizeof(Node) + cells.capacity() * sizeof(Cell) + boxes.capacity() * sizeof(Box) +
           order.capacity() * sizeof(size_t);
  }
  std::string Summary() const {
    std::ostringstream out;
    out << std::setprecision(17) << "scenario=" << scenario << " sha256=" << identity << " nodes=" << nodes.size()
        << " cells=" << cells.size() << " norm_inf=" << norm_inf << " norm_H1=" << norm_h1 << " measure=" << volume
        << " H/ell=" << diameter / ell << " bytes_per_rank=" << MemoryBytes();
    return out.str();
  }
  std::string QuerySummary() const {
    std::ostringstream out;
    out << "face_queries=" << face_queries.load() << " source_queries=" << source_queries.load()
        << " other_queries=" << other_queries.load() << " failed_queries=" << failed_queries.load()
        << " query_seconds=" << query_ns.load()*1e-9 << " bytes_per_rank=" << MemoryBytes();
    return out.str();
  }
};
}  // namespace StressField
