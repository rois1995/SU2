/*!
 * \file CReferenceWall.cpp
 * \brief Reference geometry of 2D boundary-layer walls and wall size rule (ADAP_BL_METHOD= TWO_PASS).
 * \version 8.5.0 "Harrier"
 *
 * SU2 Project Website: https://su2code.github.io
 *
 * The SU2 Project is maintained by the SU2 Foundation
 * (http://su2foundation.org)
 *
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 *
 * SU2 is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * SU2 is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with SU2. If not, see <http://www.gnu.org/licenses/>.
 */

#include "../../include/adaptation/CReferenceWall.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <tuple>

#include "../../../Common/include/parallelization/mpi_structure.hpp"

namespace {

using Vec2 = std::array<passivedouble, 2>;

passivedouble Dist(const Vec2& a, const Vec2& b) { return std::hypot(b[0] - a[0], b[1] - a[1]); }

/*--- Turn angle (radians) between the directions a->b and b->c. ---*/
passivedouble Turn(const Vec2& a, const Vec2& b, const Vec2& c) {
  const passivedouble u[2] = {b[0] - a[0], b[1] - a[1]}, v[2] = {c[0] - b[0], c[1] - b[1]};
  const passivedouble nu = std::hypot(u[0], u[1]), nv = std::hypot(v[0], v[1]);
  if (nu <= 0.0 || nv <= 0.0) return 0.0;
  const passivedouble cosine = std::max(-1.0, std::min(1.0, (u[0] * v[0] + u[1] * v[1]) / (nu * nv)));
  return std::acos(cosine);
}

/*--- Tridiagonal solve (Thomas): a sub, b diagonal, c super, d right-hand side (overwritten by the solution). ---*/
void Thomas(std::vector<passivedouble> a, std::vector<passivedouble> b, std::vector<passivedouble> c,
            std::vector<passivedouble>& d) {
  const auto n = b.size();
  for (unsigned long i = 1; i < n; ++i) {
    const auto w = a[i] / b[i - 1];
    b[i] -= w * c[i - 1];
    d[i] -= w * d[i - 1];
  }
  d[n - 1] /= b[n - 1];
  for (unsigned long i = n - 1; i > 0; --i) d[i - 1] = (d[i - 1] - c[i - 1] * d[i]) / b[i - 1];
}

/*--- Cyclic tridiagonal solve (Sherman-Morrison): a[0] couples to the last unknown, c[n-1] to the first. ---*/
void CyclicThomas(const std::vector<passivedouble>& a, const std::vector<passivedouble>& b,
                  const std::vector<passivedouble>& c, std::vector<passivedouble>& d) {
  const auto n = b.size();
  if (n < 3) {
    /*--- Too small for the cyclic structure: diagonal approximation (not used by the fit, which needs n >= 3). ---*/
    for (unsigned long i = 0; i < n; ++i) d[i] /= b[i];
    return;
  }
  const passivedouble alpha = c[n - 1], beta = a[0], gamma = -b[0];
  std::vector<passivedouble> bb(b), u(n, 0.0);
  bb[0] = b[0] - gamma;
  bb[n - 1] = b[n - 1] - alpha * beta / gamma;
  u[0] = gamma;
  u[n - 1] = alpha;
  std::vector<passivedouble> aa(a), cc(c);
  aa[0] = 0.0;
  cc[n - 1] = 0.0;
  Thomas(aa, bb, cc, d);
  Thomas(aa, bb, cc, u);
  const passivedouble fact = (d[0] + beta * d[n - 1] / gamma) / (1.0 + u[0] + beta * u[n - 1] / gamma);
  for (unsigned long i = 0; i < n; ++i) d[i] -= fact * u[i];
}

}  // namespace

CReferenceWall::CReferenceWall(const CSimplexMesh& mesh, const std::vector<std::string>& markers,
                               passivedouble cornerAngle_)
    : cornerAngle(cornerAngle_) {
  if (mesh.nDim != 2) SU2_MPI::Error("The reference wall is only implemented in 2D.", CURRENT_FUNCTION);
  const passivedouble cornerTurn = cornerAngle * M_PI / 180.0;
  auto point = [&](unsigned long i) { return Vec2{mesh.coord[2 * i], mesh.coord[2 * i + 1]}; };

  for (const auto& name : markers) {
    const auto* marker = mesh.FindMarker(name);
    if (marker == nullptr || marker->elem.empty()) continue;

    /*--- Points of the other markers break the chains (marker junctions). ---*/
    std::set<unsigned long> otherPoints;
    for (const auto& other : mesh.markers)
      if (other.name != name) otherPoints.insert(other.elem.begin(), other.elem.end());

    std::map<unsigned long, std::vector<std::pair<unsigned long, unsigned long>>> adjacency;  // point -> (point, line)
    const auto nLine = marker->GetnElem(2);
    for (unsigned long iLine = 0; iLine < nLine; ++iLine) {
      const auto a = marker->elem[2 * iLine], b = marker->elem[2 * iLine + 1];
      adjacency[a].push_back({b, iLine});
      adjacency[b].push_back({a, iLine});
    }
    auto isBreak = [&](unsigned long p) { return adjacency[p].size() != 2 || otherPoints.count(p) > 0; };

    /*--- Chains: from break points first, then the remaining closed loops. ---*/
    std::vector<bool> used(nLine, false);
    std::vector<std::pair<std::vector<unsigned long>, bool>> chains;  // points, closed
    auto walk = [&](unsigned long start, unsigned long firstLine, unsigned long next) {
      std::vector<unsigned long> chain = {start};
      used[firstLine] = true;
      auto prev = start, cur = next;
      while (true) {
        chain.push_back(cur);
        if (cur == start || isBreak(cur)) break;
        bool advanced = false;
        for (const auto& nb : adjacency[cur]) {
          if (used[nb.second]) continue;
          used[nb.second] = true;
          prev = cur;
          cur = nb.first;
          advanced = true;
          break;
        }
        if (!advanced) break;
      }
      (void)prev;
      const bool closed = chain.size() > 2 && chain.back() == start;
      return std::make_pair(chain, closed);
    };
    for (const auto& entry : adjacency) {
      if (!isBreak(entry.first)) continue;
      for (const auto& nb : entry.second) {
        if (used[nb.second]) continue;
        chains.push_back(walk(entry.first, nb.second, nb.first));
      }
    }
    for (const auto& entry : adjacency) {
      for (const auto& nb : entry.second) {
        if (used[nb.second]) continue;
        chains.push_back(walk(entry.first, nb.second, nb.first));
      }
    }

    /*--- Split the chains at sharp vertices into segments. ---*/
    for (auto& chainEntry : chains) {
      auto chain = chainEntry.first;  // closed chains repeat the first point at the end
      const bool closed = chainEntry.second;
      const auto n = chain.size();
      std::vector<bool> sharp(n, false);
      for (unsigned long i = 0; i < n; ++i) {
        if (!closed && (i == 0 || i == n - 1)) continue;
        if (closed && i == n - 1) continue;
        const auto prevPoint = closed ? chain[(i + n - 2) % (n - 1)] : chain[i - 1];
        const auto nextPoint = chain[i + 1];
        sharp[i] = Turn(point(prevPoint), point(chain[i]), point(nextPoint)) > cornerTurn;
      }
      if (closed) {
        /*--- Rotate a closed chain to start at a sharp vertex (if any). ---*/
        long first = -1;
        for (unsigned long i = 0; i + 1 < n; ++i)
          if (sharp[i]) { first = i; break; }
        if (first < 0) {
          Segment seg;
          seg.marker = name;
          seg.closed = true;
          for (const auto p : chain) seg.x.push_back(point(p));
          Fit(seg);
          segments.push_back(seg);
          continue;
        }
        std::vector<unsigned long> rotated;
        std::vector<bool> rotatedSharp;
        for (unsigned long k = 0; k + 1 < n; ++k) {
          rotated.push_back(chain[(first + k) % (n - 1)]);
          rotatedSharp.push_back(sharp[(first + k) % (n - 1)]);
        }
        rotated.push_back(rotated.front());
        rotatedSharp.push_back(true);
        chain = rotated;
        sharp = rotatedSharp;
      }
      Segment seg;
      seg.marker = name;
      seg.sharpStart = closed || sharp[0];
      seg.x.push_back(point(chain[0]));
      for (unsigned long i = 1; i < chain.size(); ++i) {
        seg.x.push_back(point(chain[i]));
        const bool last = (i == chain.size() - 1);
        if (sharp[i] || last) {
          seg.sharpEnd = sharp[i];
          Fit(seg);
          segments.push_back(seg);
          seg = Segment();
          seg.marker = name;
          seg.sharpStart = sharp[i];
          seg.x.push_back(point(chain[i]));
        }
      }
    }
  }
}

void CReferenceWall::Fit(Segment& seg) const {
  const auto nKnot = seg.x.size();
  seg.s.assign(nKnot, 0.0);
  for (unsigned long i = 1; i < nKnot; ++i) seg.s[i] = seg.s[i - 1] + Dist(seg.x[i - 1], seg.x[i]);
  seg.m.assign(nKnot, Vec2{0.0, 0.0});
  const auto nInt = nKnot - 1;
  std::vector<passivedouble> h(nInt);
  for (unsigned long i = 0; i < nInt; ++i) h[i] = seg.s[i + 1] - seg.s[i];

  if (seg.closed && nInt >= 3) {
    /*--- Periodic: unknowns m_0..m_{n-1}, m_n = m_0. ---*/
    const auto n = nInt;
    for (unsigned short d = 0; d < 2; ++d) {
      std::vector<passivedouble> a(n), b(n), c(n), r(n);
      for (unsigned long i = 0; i < n; ++i) {
        const auto hm = h[(i + n - 1) % n], hp = h[i];
        const auto xm = seg.x[(i + n - 1) % n][d], x0 = seg.x[i][d], xp = seg.x[i + 1][d];
        a[i] = hm;
        b[i] = 2.0 * (hm + hp);
        c[i] = hp;
        r[i] = 6.0 * ((xp - x0) / hp - (x0 - xm) / hm);
      }
      CyclicThomas(a, b, c, r);
      for (unsigned long i = 0; i < n; ++i) seg.m[i][d] = r[i];
      seg.m[n][d] = r[0];
    }
  } else if (nInt >= 2) {
    /*--- Natural ends: m_0 = m_n = 0. ---*/
    const auto n = nInt - 1;  // interior knots
    for (unsigned short d = 0; d < 2; ++d) {
      std::vector<passivedouble> a(n), b(n), c(n), r(n);
      for (unsigned long k = 0; k < n; ++k) {
        const auto i = k + 1;
        a[k] = h[i - 1];
        b[k] = 2.0 * (h[i - 1] + h[i]);
        c[k] = h[i];
        r[k] = 6.0 * ((seg.x[i + 1][d] - seg.x[i][d]) / h[i] - (seg.x[i][d] - seg.x[i - 1][d]) / h[i - 1]);
      }
      Thomas(a, b, c, r);
      for (unsigned long k = 0; k < n; ++k) seg.m[k + 1][d] = r[k];
    }
  }

  /*--- Turning curvature of the polyline at the knots. ---*/
  seg.turnCurvature.assign(nKnot, 0.0);
  for (unsigned long i = 0; i < nKnot; ++i) {
    long ip, in;
    if (seg.closed) {
      ip = (i == 0) ? nInt - 1 : i - 1;
      in = (i == nKnot - 1) ? 1 : i + 1;
    } else {
      if (i == 0 || i == nKnot - 1) continue;
      ip = i - 1;
      in = i + 1;
    }
    const auto turn = Turn(seg.x[ip], seg.x[i], seg.x[in]);
    const auto len = 0.5 * (Dist(seg.x[ip], seg.x[i]) + Dist(seg.x[i], seg.x[in]));
    if (len > 0.0) seg.turnCurvature[i] = 2.0 * std::sin(0.5 * turn) / len;
  }
  if (!seg.closed && nKnot > 2) {
    seg.turnCurvature[0] = seg.turnCurvature[1];
    seg.turnCurvature[nKnot - 1] = seg.turnCurvature[nKnot - 2];
  }
}

void CReferenceWall::Evaluate(const Segment& seg, passivedouble s, passivedouble* x, passivedouble* dx,
                              passivedouble* ddx) const {
  const auto nInt = seg.s.size() - 1;
  if (seg.closed) {
    const auto len = seg.s.back();
    s = std::fmod(s, len);
    if (s < 0.0) s += len;
  }
  s = std::max(seg.s.front(), std::min(seg.s.back(), s));
  unsigned long i = std::upper_bound(seg.s.begin(), seg.s.end(), s) - seg.s.begin();
  i = (i == 0) ? 0 : i - 1;
  if (i >= nInt) i = nInt - 1;
  const auto h = seg.s[i + 1] - seg.s[i];
  const auto A = seg.s[i + 1] - s, B = s - seg.s[i];
  for (unsigned short d = 0; d < 2; ++d) {
    const auto m0 = seg.m[i][d], m1 = seg.m[i + 1][d], x0 = seg.x[i][d], x1 = seg.x[i + 1][d];
    x[d] = m0 * A * A * A / (6 * h) + m1 * B * B * B / (6 * h) + (x0 / h - m0 * h / 6) * A + (x1 / h - m1 * h / 6) * B;
    if (dx) dx[d] = -m0 * A * A / (2 * h) + m1 * B * B / (2 * h) - (x0 / h - m0 * h / 6) + (x1 / h - m1 * h / 6);
    if (ddx) ddx[d] = m0 * A / h + m1 * B / h;
  }
}

passivedouble CReferenceWall::Curvature(const Segment& seg, passivedouble s) const {
  passivedouble x[2], dx[2], ddx[2];
  Evaluate(seg, s, x, dx, ddx);
  const auto speed = std::hypot(dx[0], dx[1]);
  const auto spline = (speed > 0.0) ? std::fabs(dx[0] * ddx[1] - dx[1] * ddx[0]) / (speed * speed * speed) : 0.0;
  const auto nInt = seg.s.size() - 1;
  unsigned long i = std::upper_bound(seg.s.begin(), seg.s.end(), s) - seg.s.begin();
  i = (i == 0) ? 0 : i - 1;
  if (i >= nInt) i = nInt - 1;
  const auto turn = std::max(seg.turnCurvature[i], seg.turnCurvature[i + 1]);
  return std::max(spline, turn);
}

void CReferenceWall::ProjectOnInterval(const Segment& seg, unsigned long i, const passivedouble* p, Projection& best,
                                       long iSeg) const {
  const auto s0 = seg.s[i], s1 = seg.s[i + 1];
  /*--- Start: projection on the chord. ---*/
  const passivedouble u[2] = {seg.x[i + 1][0] - seg.x[i][0], seg.x[i + 1][1] - seg.x[i][1]};
  const auto len2 = u[0] * u[0] + u[1] * u[1];
  passivedouble t = (len2 > 0.0) ? ((p[0] - seg.x[i][0]) * u[0] + (p[1] - seg.x[i][1]) * u[1]) / len2 : 0.0;
  t = std::max(0.0, std::min(1.0, t));
  passivedouble s = s0 + t * (s1 - s0);
  for (int iter = 0; iter < 30; ++iter) {
    passivedouble x[2], dx[2], ddx[2];
    Evaluate(seg, s, x, dx, ddx);
    const passivedouble r[2] = {x[0] - p[0], x[1] - p[1]};
    const auto f = r[0] * dx[0] + r[1] * dx[1];
    auto fp = dx[0] * dx[0] + dx[1] * dx[1] + r[0] * ddx[0] + r[1] * ddx[1];
    if (fp <= 0.0) fp = dx[0] * dx[0] + dx[1] * dx[1];
    const auto step = f / fp;
    const auto sNew = std::max(s0, std::min(s1, s - step));
    if (std::fabs(sNew - s) <= 1e-15 * (1.0 + std::fabs(s1))) {
      s = sNew;
      break;
    }
    s = sNew;
  }
  for (const auto sTry : {s, s0, s1}) {
    passivedouble x[2], dx[2];
    Evaluate(seg, sTry, x, dx);
    const auto dist = std::hypot(x[0] - p[0], x[1] - p[1]);
    if (best.segment < 0 || dist < best.distance) {
      best.segment = iSeg;
      best.distance = dist;
      best.s = sTry;
      best.x[0] = x[0];
      best.x[1] = x[1];
      const auto speed = std::hypot(dx[0], dx[1]);
      best.tangent[0] = dx[0] / speed;
      best.tangent[1] = dx[1] / speed;
    }
  }
}

CReferenceWall::Projection CReferenceWall::Project(const passivedouble* point, const std::string& marker,
                                                   long onlySegment) const {
  /*--- Candidate intervals: the three closest chords of the marker or segment (and their neighbours). ---*/
  std::vector<std::tuple<passivedouble, long, unsigned long>> chords;  // distance, segment, interval
  for (unsigned long iSeg = 0; iSeg < segments.size(); ++iSeg) {
    const auto& seg = segments[iSeg];
    if (seg.marker != marker) continue;
    if (onlySegment >= 0 && static_cast<long>(iSeg) != onlySegment) continue;
    for (unsigned long i = 0; i + 1 < seg.x.size(); ++i) {
      const passivedouble u[2] = {seg.x[i + 1][0] - seg.x[i][0], seg.x[i + 1][1] - seg.x[i][1]};
      const auto len2 = u[0] * u[0] + u[1] * u[1];
      passivedouble t = (len2 > 0.0) ? ((point[0] - seg.x[i][0]) * u[0] + (point[1] - seg.x[i][1]) * u[1]) / len2 : 0;
      t = std::max(0.0, std::min(1.0, t));
      const auto d = std::hypot(seg.x[i][0] + t * u[0] - point[0], seg.x[i][1] + t * u[1] - point[1]);
      chords.emplace_back(d, iSeg, i);
    }
  }
  Projection best;
  if (chords.empty()) return best;
  const auto nCand = std::min<std::size_t>(3, chords.size());
  std::partial_sort(chords.begin(), chords.begin() + nCand, chords.end());
  std::set<std::pair<long, unsigned long>> done;
  for (std::size_t k = 0; k < nCand; ++k) {
    const auto iSeg = std::get<1>(chords[k]);
    const auto& seg = segments[iSeg];
    const long nInt = seg.x.size() - 1;
    for (long di = -1; di <= 1; ++di) {
      long i = static_cast<long>(std::get<2>(chords[k])) + di;
      if (seg.closed) i = (i + nInt) % nInt;
      if (i < 0 || i >= nInt) continue;
      if (!done.insert({iSeg, i}).second) continue;
      ProjectOnInterval(seg, i, point, best, iSeg);
    }
  }
  best.curvature = Curvature(segments[best.segment], best.s);
  return best;
}

std::string CReferenceWall::Fingerprint() const {
  unsigned long nKnot = 0;
  passivedouble hash = 0.0;
  for (const auto& seg : segments)
    for (const auto& x : seg.x) {
      ++nKnot;
      hash += (x[0] * 1.2345678901 + x[1] * 7.6543210987) * static_cast<passivedouble>(nKnot % 1009 + 1);
    }
  std::ostringstream text;
  text << segments.size() << " " << nKnot << " " << std::setprecision(15) << std::scientific << hash << " "
       << cornerAngle;
  /*--- The feature settings of every segment: marker, closed, sharp start, sharp end. ---*/
  for (const auto& seg : segments) text << " " << seg.marker << ":" << seg.closed << seg.sharpStart << seg.sharpEnd;
  return text.str();
}

void CReferenceWall::Write(const std::string& filename) const {
  std::ofstream file(filename);
  if (!file) SU2_MPI::Error("Could not write the reference wall " + filename + ".", CURRENT_FUNCTION);
  file << "SU2_BL_REFERENCE_WALL 2\n";
  file << "CORNER_ANGLE " << std::setprecision(17) << cornerAngle << "\n";
  file << "NSEGMENT " << segments.size() << "\n";
  for (const auto& seg : segments) {
    file << "SEGMENT " << seg.marker << " " << seg.closed << " " << seg.sharpStart << " " << seg.sharpEnd << " "
         << seg.x.size() << "\n";
    for (const auto& x : seg.x) file << std::setprecision(17) << x[0] << " " << x[1] << "\n";
  }
  file << "FINGERPRINT " << Fingerprint() << "\n";
}

bool CReferenceWall::Read(const std::string& filename, std::string* error) {
  auto fail = [&](const std::string& why) {
    if (error != nullptr) *error = why;
    return false;
  };
  std::ifstream file(filename);
  if (!file) return fail("does not exist or cannot be opened");
  /*--- Read into a copy: this object changes only if the whole file is valid. ---*/
  CReferenceWall read;
  std::string word;
  int version = 0;
  if (!(file >> word >> version) || word != "SU2_BL_REFERENCE_WALL")
    return fail("is not a reference wall file (header)");
  if (version != 2) return fail("has the unsupported format version " + std::to_string(version));
  if (!(file >> word >> read.cornerAngle) || word != "CORNER_ANGLE" || !std::isfinite(read.cornerAngle))
    return fail("is malformed (CORNER_ANGLE)");
  long nSegment = 0;
  if (!(file >> word >> nSegment) || word != "NSEGMENT" || nSegment < 1 || nSegment > 10000000)
    return fail("is malformed (NSEGMENT)");
  for (long iSeg = 0; iSeg < nSegment; ++iSeg) {
    Segment seg;
    long nKnot = 0;
    if (!(file >> word >> seg.marker >> seg.closed >> seg.sharpStart >> seg.sharpEnd >> nKnot) || word != "SEGMENT" ||
        nKnot < 2 || nKnot > 1000000000)
      return fail("is truncated or malformed (segment " + std::to_string(iSeg) + ")");
    seg.x.resize(nKnot);
    for (long k = 0; k < nKnot; ++k) {
      auto& x = seg.x[k];
      if (!(file >> x[0] >> x[1]) || !std::isfinite(x[0]) || !std::isfinite(x[1]))
        return fail("is truncated or malformed (segment " + std::to_string(iSeg) + ", knot " + std::to_string(k) + ")");
      if (k > 0 && Dist(seg.x[k - 1], x) <= 0.0)
        return fail("is malformed (repeated knot in segment " + std::to_string(iSeg) + ")");
    }
    /*--- A periodic segment is a smooth closed loop: it ends where it starts and has no sharp ends. ---*/
    if (seg.closed && (nKnot < 4 || Dist(seg.x.front(), seg.x.back()) > 0.0 || seg.sharpStart || seg.sharpEnd))
      return fail("is malformed (closed segment " + std::to_string(iSeg) + ")");
    read.Fit(seg);
    read.segments.push_back(seg);
  }
  std::string fingerprint;
  if (!(file >> word) || word != "FINGERPRINT") return fail("is truncated (no FINGERPRINT line)");
  std::getline(file, fingerprint);
  if (!fingerprint.empty() && fingerprint[0] == ' ') fingerprint.erase(0, 1);
  if (fingerprint != read.Fingerprint()) return fail("does not match its fingerprint (edited or corrupted)");
  *this = std::move(read);
  return true;
}

passivedouble CReferenceWall::MaxDistance(const CSimplexMesh& mesh, const std::vector<std::string>& markers) const {
  passivedouble maxDist = 0.0;
  for (const auto& name : markers) {
    const auto* marker = mesh.FindMarker(name);
    if (marker == nullptr) continue;
    std::set<unsigned long> points(marker->elem.begin(), marker->elem.end());
    for (const auto p : points) {
      const auto proj = Project(&mesh.coord[2 * p], name);
      if (proj.segment < 0) return std::numeric_limits<passivedouble>::infinity();
      maxDist = std::max(maxDist, proj.distance);
    }
  }
  return maxDist;
}

namespace {

/*--- Real roots in (lo, hi) of k1 + 2 k2 B + 3 k3 B^2 (the derivative of the cubic k0 + k1 B + k2 B^2 + k3 B^3). ---*/
void DerivativeRoots(const passivedouble* k, passivedouble lo, passivedouble hi, std::vector<passivedouble>& out) {
  const passivedouble A = 3.0 * k[3], B = 2.0 * k[2], C = k[1];
  auto add = [&](passivedouble r) {
    if (std::isfinite(r) && r > lo && r < hi) out.push_back(r);
  };
  if (A == 0.0) {
    if (B != 0.0) add(-C / B);
    return;
  }
  const passivedouble disc = B * B - 4.0 * A * C;
  if (disc < 0.0) return;
  const passivedouble q = -0.5 * (B + std::copysign(std::sqrt(disc), B));
  add(q / A);
  if (q != 0.0) add(C / q);
}

passivedouble Cubic(const passivedouble* k, passivedouble B) { return ((k[3] * B + k[2]) * B + k[1]) * B + k[0]; }

}  // namespace

passivedouble CReferenceWall::ArcChordDistance(const Segment& seg, passivedouble s0, passivedouble s1,
                                               const passivedouble* a, const passivedouble* b) const {
  const passivedouble u[2] = {b[0] - a[0], b[1] - a[1]};
  const passivedouble len = std::hypot(u[0], u[1]);
  if (!(len > 0.0)) return std::numeric_limits<passivedouble>::infinity();
  const auto nInt = seg.s.size() - 1;
  const passivedouble length = seg.s.back();
  passivedouble lo = std::min(s0, s1), hi = std::max(s0, s1);

  /*--- Parameter ranges inside [0, length]: a closed arc is shifted into one period and split at its end. ---*/
  std::vector<std::array<passivedouble, 2>> ranges;
  if (seg.closed) {
    hi = std::min(hi, lo + length);
    const passivedouble shift = std::floor(lo / length) * length;
    lo -= shift;
    hi -= shift;
    if (hi > length) {
      ranges.push_back({lo, length});
      ranges.push_back({0.0, std::min(hi - length, length)});
    } else {
      ranges.push_back({lo, hi});
    }
  } else {
    lo = std::max(lo, seg.s.front());
    hi = std::min(hi, seg.s.back());
    ranges.push_back({lo, std::max(lo, hi)});
  }

  passivedouble bound = 0.0;
  /*--- Rounding margin: the representation of the absolute coordinates (the differences x - a lose up to a few ulps of
   *    them, 64 eps) plus the evaluation of the local cubics (1e-12, about 4500 eps, of their term magnitudes). It does
   *    not grow with a translation of the mesh beyond the representation error itself. ---*/
  passivedouble absScale = std::max({std::fabs(a[0]), std::fabs(a[1]), std::fabs(b[0]), std::fabs(b[1])});
  passivedouble localScale = len;
  std::vector<passivedouble> candidates;
  for (const auto& range : ranges) {
    for (unsigned long i = 0; i < nInt; ++i) {
      const passivedouble si = seg.s[i], sj = seg.s[i + 1];
      /*--- Pieces that meet the range (a point range meets the piece that contains it). ---*/
      if (sj < range[0] || si > range[1]) continue;
      if (sj == range[0] && i + 1 < nInt && range[1] > range[0]) continue;
      const passivedouble h = sj - si;
      const passivedouble B0 = std::max(range[0], si) - si, B1 = std::max(B0, std::min(range[1], sj) - si);

      /*--- x(B) = k0 + k1 B + k2 B^2 + k3 B^3 per coordinate, B = s - s_i (the cubic of Evaluate). ---*/
      passivedouble k[2][4];
      for (unsigned short d = 0; d < 2; ++d) {
        const auto m0 = seg.m[i][d], m1 = seg.m[i + 1][d], x0 = seg.x[i][d], x1 = seg.x[i + 1][d];
        k[d][0] = x0 - a[d];
        k[d][1] = (x1 - x0) / h - h * (2.0 * m0 + m1) / 6.0;
        k[d][2] = 0.5 * m0;
        k[d][3] = (m1 - m0) / (6.0 * h);
        absScale = std::max(absScale, std::fabs(x0));
        passivedouble size = std::fabs(k[d][0]);
        for (int j = 1; j < 4; ++j) size += std::fabs(k[d][j]) * std::pow(B1, j);
        localScale = std::max(localScale, size);
      }
      /*--- Signed line distance c(B) and chord parameter u(B). ---*/
      passivedouble c[4], w[4];
      for (int j = 0; j < 4; ++j) {
        c[j] = (u[0] * k[1][j] - u[1] * k[0][j]) / len;
        w[j] = (u[0] * k[0][j] + u[1] * k[1][j]) / (len * len);
      }
      candidates = {B0, B1};
      DerivativeRoots(c, B0, B1, candidates);
      passivedouble maxC = 0.0;
      for (const auto B : candidates) maxC = std::max(maxC, std::fabs(Cubic(c, B)));
      candidates = {B0, B1};
      DerivativeRoots(w, B0, B1, candidates);
      passivedouble minU = std::numeric_limits<passivedouble>::max(), maxU = -minU;
      for (const auto B : candidates) {
        const auto value = Cubic(w, B);
        minU = std::min(minU, value);
        maxU = std::max(maxU, value);
      }
      const passivedouble e = len * std::max({0.0, -minU, maxU - 1.0});
      bound = std::max(bound, std::sqrt(maxC * maxC + e * e));
    }
  }
  return bound + 64.0 * std::numeric_limits<passivedouble>::epsilon() * absScale + 1e-12 * localScale;
}

namespace {
/*--- Knuth's two-sum: x + y = a + b exactly. SU2 is built with -ffast-math, which would simplify the error term to 0:
 *    every intermediate goes through a volatile, so each operation is evaluated as written (IEEE round to nearest). ---*/
inline void TwoSum(passivedouble a, passivedouble b, passivedouble& x, passivedouble& y) {
  volatile passivedouble sum = a + b;
  volatile passivedouble bVirtual = sum - a;
  volatile passivedouble aVirtual = sum - bVirtual;
  volatile passivedouble bRound = b - bVirtual;
  volatile passivedouble aRound = a - aVirtual;
  x = sum;
  y = aRound + bRound;
}
/*--- Shewchuk's GROW-EXPANSION: e (nonoverlapping, increasing magnitude) += b, exactly. ---*/
void Grow(std::vector<passivedouble>& e, passivedouble b) {
  passivedouble q = b;
  for (auto& component : e) {
    passivedouble sum, err;
    TwoSum(q, component, sum, err);
    component = err;
    q = sum;
  }
  e.push_back(q);
}
}  // namespace

int BLWallRule::Orientation(const passivedouble* a, const passivedouble* b, const passivedouble* c) {
  const passivedouble left = (b[0] - a[0]) * (c[1] - a[1]), right = (b[1] - a[1]) * (c[0] - a[0]);
  const passivedouble det = left - right;
  const passivedouble bound = 3.3306690738754716e-16 * (std::fabs(left) + std::fabs(right));  // (3 + 16 eps) eps
  if (det > bound) return 1;
  if (-det > bound) return -1;
  /*--- Exact: det = bx cy - bx ay - ax cy - by cx + by ax + ay cx, each product exact as hi + lo (fma). ---*/
  const passivedouble terms[6][2] = {{b[0], c[1]}, {-b[0], a[1]}, {-a[0], c[1]},
                                     {-b[1], c[0]}, {b[1], a[0]}, {a[1], c[0]}};
  std::vector<passivedouble> expansion;
  expansion.reserve(12);
  for (const auto& term : terms) {
    volatile passivedouble hi = term[0] * term[1];
    const passivedouble lo = std::fma(term[0], term[1], -static_cast<passivedouble>(hi));
    Grow(expansion, lo);
    Grow(expansion, hi);
  }
  /*--- The sign of a nonoverlapping expansion is that of its largest (last nonzero) component. ---*/
  for (auto it = expansion.rbegin(); it != expansion.rend(); ++it) {
    if (*it > 0.0) return 1;
    if (*it < 0.0) return -1;
  }
  return 0;
}

passivedouble BLWallRule::SizeAt(const SizeSamples& samples, passivedouble s) {
  const auto& S = samples.s;
  if (S.empty()) return 0.0;
  if (s <= S.front()) return samples.size.front();
  if (s >= S.back()) return samples.size.back();
  const auto i = std::upper_bound(S.begin(), S.end(), s) - S.begin() - 1;
  const auto w = (s - S[i]) / (S[i + 1] - S[i]);
  return (1.0 - w) * samples.size[i] + w * samples.size[i + 1];
}

void BLWallRule::Grade(SizeSamples& out, passivedouble gradation, bool closed) {
  const auto n = out.s.size();
  if (n < 2) return;
  const unsigned short nRound = closed ? 2 : 1;
  for (unsigned short round = 0; round < nRound; ++round) {
    for (unsigned long i = 1; i < n; ++i)
      out.size[i] = std::min(out.size[i], out.size[i - 1] + gradation * (out.s[i] - out.s[i - 1]));
    for (unsigned long i = n - 1; i > 0; --i)
      out.size[i - 1] = std::min(out.size[i - 1], out.size[i] + gradation * (out.s[i] - out.s[i - 1]));
    if (closed) {
      const auto t = std::min(out.size.front(), out.size.back());
      out.size.front() = out.size.back() = t;
    }
  }
}

std::vector<BLWallRule::Corner> BLWallRule::FindCorners(const CReferenceWall& wall) {
  struct End {
    unsigned long seg;
    bool atEnd;
    bool sharp;
    Vec2 x;
    Vec2 inward;  // unit tangent pointing into the segment
  };
  std::vector<End> ends;
  const auto& segments = wall.GetSegments();
  for (unsigned long iSeg = 0; iSeg < segments.size(); ++iSeg) {
    const auto& seg = segments[iSeg];
    if (seg.closed || seg.x.size() < 2) continue;
    for (const bool atEnd : {false, true}) {
      passivedouble x[2], dx[2];
      wall.Evaluate(seg, atEnd ? seg.s.back() : seg.s.front(), x, dx);
      const auto norm = std::hypot(dx[0], dx[1]);
      const passivedouble sign = (atEnd ? -1.0 : 1.0) / norm;
      ends.push_back({iSeg, atEnd, atEnd ? seg.sharpEnd : seg.sharpStart, atEnd ? seg.x.back() : seg.x.front(),
                      Vec2{sign * dx[0], sign * dx[1]}});
    }
  }
  /*--- Group the ends by their knot; exactly two ends make a corner if both are sharp, or if they belong to
   *    different markers and the wall turns there by more than the corner angle. ---*/
  const passivedouble cornerTurn = wall.GetCornerAngle() * M_PI / 180.0;
  std::map<Vec2, std::vector<unsigned long>> atPoint;
  for (unsigned long i = 0; i < ends.size(); ++i) atPoint[ends[i].x].push_back(i);
  std::vector<Corner> corners;
  for (const auto& entry : atPoint) {
    if (entry.second.size() != 2) continue;
    const auto& e0 = ends[entry.second[0]];
    const auto& e1 = ends[entry.second[1]];
    bool corner = e0.sharp && e1.sharp;
    if (!corner && segments[e0.seg].marker != segments[e1.seg].marker) {
      /*--- Turn of the wall = pi - the angle between the two inward tangents. ---*/
      const auto angle = std::atan2(std::fabs(e0.inward[0] * e1.inward[1] - e0.inward[1] * e1.inward[0]),
                                    e0.inward[0] * e1.inward[0] + e0.inward[1] * e1.inward[1]);
      corner = M_PI - angle > cornerTurn;
    }
    if (!corner) continue;
    Corner c;
    c.seg[0] = e0.seg;
    c.atEnd[0] = e0.atEnd;
    c.seg[1] = e1.seg;
    c.atEnd[1] = e1.atEnd;
    c.x[0] = entry.first[0];
    c.x[1] = entry.first[1];
    corners.push_back(c);
  }
  return corners;
}

passivedouble BLWallRule::CornerFloor(passivedouble requested, passivedouble hmax, passivedouble length,
                                      passivedouble tmin, passivedouble gradation) {
  if (!(requested > 0.0)) return 0.0;
  const auto t = std::min({requested, hmax, (0.5 * gradation * length + tmin) / (1.0 + gradation)});
  return (t > tmin) ? t : 0.0;
}

passivedouble BLWallRule::CornerReach(const Corner& corner, passivedouble gradation) {
  if (!corner.convex || !(corner.floor > 0.0)) return 0.0;
  return corner.floor + (corner.floor - corner.tmin) / gradation;
}

namespace {
/*--- Distance of a sample from one end of its segment, and the size at a distance from that end. ---*/
passivedouble FromEnd(const BLWallRule::SizeSamples& smp, bool atEnd, unsigned long j) {
  return atEnd ? smp.s.back() - smp.s[j] : smp.s[j] - smp.s.front();
}
passivedouble SizeFromEnd(const BLWallRule::SizeSamples& smp, bool atEnd, passivedouble r) {
  return BLWallRule::SizeAt(smp, atEnd ? smp.s.back() - r : smp.s.front() + r);
}
/*--- Insert a sample at the distance r from one end (its size interpolated), unless one is there within roundoff. ---*/
bool InsertFromEnd(BLWallRule::SizeSamples& smp, bool atEnd, passivedouble r) {
  const auto s = atEnd ? smp.s.back() - r : smp.s.front() + r;
  if (!(s > smp.s.front() && s < smp.s.back())) return false;
  const auto it = std::lower_bound(smp.s.begin(), smp.s.end(), s);
  const auto tol = 8.0 * std::numeric_limits<passivedouble>::epsilon() * (smp.s.back() - smp.s.front());
  if (*it - s <= tol || (it != smp.s.begin() && s - *(it - 1) <= tol)) return false;
  const auto i = it - smp.s.begin();
  const auto t = BLWallRule::SizeAt(smp, s);
  smp.s.insert(smp.s.begin() + i, s);
  smp.size.insert(smp.size.begin() + i, t);
  return true;
}
}  // namespace

BLWallRule::CornerChanges BLWallRule::ApplyCorners(const CReferenceWall& wall, const std::vector<Corner>& corners,
                                                   passivedouble gradation, std::vector<SizeSamples>& samples) {
  CornerChanges changes;
  changes.changed.assign(corners.size(), 0.0);
  const auto& segments = wall.GetSegments();
  const auto original = samples;

  /*--- Samples at the break points of the floor profiles (r = t_c and r = reach), so that the piecewise linear sizes
   *    hold the profile exactly (the maximum at the samples, interpolated, is at least the linear profile between
   *    them). This does not change the sizes. ---*/
  for (const auto& corner : corners) {
    const auto reach = CornerReach(corner, gradation);
    if (!(reach > 0.0)) continue;
    for (unsigned short k = 0; k < 2; ++k) {
      if (samples[corner.seg[k]].s.empty()) continue;
      InsertFromEnd(samples[corner.seg[k]], corner.atEnd[k], corner.floor);
      InsertFromEnd(samples[corner.seg[k]], corner.atEnd[k], reach);
    }
  }

  /*--- Symmetry: both sides of a corner take the smaller size at the same distance from it, never below their own
   *    t_min (markers with different h0). Both sides first get samples at the same distances within the reach, so the
   *    two piecewise linear sizes are equal there; repeat grid synchronisation until no samples are inserted, to
   *    propagate break points through overlapping windows. Repeat symmetry from the current sizes and grading of the
   *    touched segments until no sizes change, propagating reductions through overlapping windows. Each round reads
   *    the same snapshot, so the order of the corners does not matter; both steps only lower sizes. ---*/
  std::vector<passivedouble> reachOf;
  for (const auto& corner : corners) {
    const auto& a = samples[corner.seg[0]];
    const auto& b = samples[corner.seg[1]];
    if (a.s.empty() || b.s.empty()) {
      reachOf.push_back(0.0);
      continue;
    }
    const auto reach = std::max({2.0 * SizeFromEnd(a, corner.atEnd[0], 0.0), 2.0 * SizeFromEnd(b, corner.atEnd[1], 0.0),
                                 CornerReach(corner, gradation)});
    reachOf.push_back(reach);
  }
  bool inserted;
  do {
    inserted = false;
    for (unsigned long iCorner = 0; iCorner < corners.size(); ++iCorner) {
      const auto& corner = corners[iCorner];
      const auto reach = reachOf[iCorner];
      if (!(reach > 0.0)) continue;
      const auto& a = samples[corner.seg[0]];
      const auto& b = samples[corner.seg[1]];
      const auto tol = 8.0 * std::numeric_limits<passivedouble>::epsilon() *
                       std::max(a.s.back() - a.s.front(), b.s.back() - b.s.front());
      std::vector<passivedouble> distances = {std::min({reach, a.s.back() - a.s.front(), b.s.back() - b.s.front()})};
      for (unsigned short k = 0; k < 2; ++k) {
        const auto& smp = samples[corner.seg[k]];
        for (unsigned long j = 0; j < smp.s.size(); ++j) {
          const auto r = FromEnd(smp, corner.atEnd[k], j);
          if (r <= reach + tol) distances.push_back(std::min(r, reach));
        }
      }
      for (unsigned short k = 0; k < 2; ++k)
        for (const auto r : distances) inserted |= InsertFromEnd(samples[corner.seg[k]], corner.atEnd[k], r);
    }
  } while (inserted);
  std::set<unsigned long> touched;
  unsigned long rounds = 0;  // a closure of minima (finite); the bound only guards against a defect
  do {
    const auto before = samples;
    touched.clear();
    for (unsigned long iCorner = 0; iCorner < corners.size(); ++iCorner) {
      const auto& corner = corners[iCorner];
      const auto reach = reachOf[iCorner];
      if (!(reach > 0.0)) continue;
      for (unsigned short k = 0; k < 2; ++k) {
        const auto& other = before[corner.seg[1 - k]];
        const auto otherLength = other.s.back() - other.s.front();
        auto& own = samples[corner.seg[k]];
        const auto tol = 8.0 * std::numeric_limits<passivedouble>::epsilon() *
                         std::max(own.s.back() - own.s.front(), otherLength);
        for (unsigned long j = 0; j < own.s.size(); ++j) {
          auto r = FromEnd(own, corner.atEnd[k], j);
          if (r > reach + tol || r > otherLength + tol) continue;
          r = std::max(0.0, std::min({r, reach, otherLength}));
          const auto t = std::max(own.tmin, SizeFromEnd(other, corner.atEnd[1 - k], r));
          if (t < own.size[j]) {
            own.size[j] = t;
            changes.nSymmetry++;
            touched.insert(corner.seg[k]);
          }
        }
      }
    }
    for (const auto iSeg : touched) Grade(samples[iSeg], gradation, segments[iSeg].closed);
  } while (!touched.empty() && ++rounds < 10000);

  /*--- Floor at convex corners: t >= f(r) = max(t_min, t_c - gradation max(0, r - t_c)). Applied last; conflicting
   *    constraints can break a neighbouring corner's symmetry when its window overlaps the floor support.
   *    The floor is an experimental option, off by default. ---*/
  for (const auto& corner : corners) {
    if (!corner.convex || !(corner.floor > 0.0)) continue;
    for (unsigned short k = 0; k < 2; ++k) {
      auto& own = samples[corner.seg[k]];
      for (unsigned long j = 0; j < own.s.size(); ++j) {
        const auto r = FromEnd(own, corner.atEnd[k], j);
        const auto f = std::max(corner.tmin, corner.floor - gradation * std::max(0.0, r - corner.floor));
        if (f > own.size[j]) {
          changes.nFloorRaised++;
          changes.maxFloorRatio = std::max(changes.maxFloorRatio, f / own.size[j]);
          own.size[j] = f;
        }
      }
    }
  }
  /*--- Extent of the changes: each changed sample counts for the nearest corner of its segment. ---*/
  for (unsigned long iSeg = 0; iSeg < samples.size(); ++iSeg) {
    const auto& smp = samples[iSeg];
    for (unsigned long j = 0; j < smp.s.size(); ++j) {
      const auto before = SizeAt(original[iSeg], smp.s[j]);
      if (!(std::fabs(smp.size[j] - before) > 1e-12 * before)) continue;
      long nearest = -1;
      passivedouble rNearest = std::numeric_limits<passivedouble>::max();
      for (unsigned long iCorner = 0; iCorner < corners.size(); ++iCorner)
        for (unsigned short k = 0; k < 2; ++k) {
          if (corners[iCorner].seg[k] != iSeg) continue;
          const auto r = FromEnd(smp, corners[iCorner].atEnd[k], j);
          if (r < rNearest) {
            rNearest = r;
            nearest = iCorner;
          }
        }
      if (nearest >= 0) changes.changed[nearest] = std::max(changes.changed[nearest], rNearest);
    }
  }
  return changes;
}

std::vector<passivedouble> BLWallRule::NormalExtent(const CSimplexMesh& mesh, const std::string& name,
                                                    passivedouble cornerAngle) {
  std::vector<passivedouble> extent;
  const auto* marker = mesh.FindMarker(name);
  if (marker == nullptr || mesh.nDim != 2) return extent;
  const auto nLine = marker->GetnElem(2);
  std::map<unsigned long, std::vector<unsigned long>> lines;  // point -> lines
  for (unsigned long iLine = 0; iLine < nLine; ++iLine) {
    lines[marker->elem[2 * iLine]].push_back(iLine);
    lines[marker->elem[2 * iLine + 1]].push_back(iLine);
  }
  auto point = [&](unsigned long i) { return Vec2{mesh.coord[2 * i], mesh.coord[2 * i + 1]}; };
  auto other = [&](unsigned long iLine, unsigned long p) {
    return marker->elem[2 * iLine] == p ? marker->elem[2 * iLine + 1] : marker->elem[2 * iLine];
  };
  std::map<unsigned long, passivedouble> turn;
  const passivedouble cornerTurn = cornerAngle * M_PI / 180.0;
  for (const auto& entry : lines) {
    if (entry.second.size() != 2) continue;
    const auto p = entry.first;
    const auto t = Turn(point(other(entry.second[0], p)), point(p), point(other(entry.second[1], p)));
    turn[p] = (t > cornerTurn) ? 0.0 : t;
  }
  for (unsigned long iLine = 0; iLine < nLine; ++iLine) {
    const auto a = marker->elem[2 * iLine], b = marker->elem[2 * iLine + 1];
    const auto t = std::max(turn.count(a) ? turn[a] : 0.0, turn.count(b) ? turn[b] : 0.0);
    extent.push_back(Dist(point(a), point(b)) * std::sin(0.5 * t));
  }
  return extent;
}
