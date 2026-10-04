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

CReferenceWall::Projection CReferenceWall::Project(const passivedouble* point, const std::string& marker) const {
  /*--- Candidate intervals: the three closest chords of the marker (and their neighbours). ---*/
  std::vector<std::tuple<passivedouble, long, unsigned long>> chords;  // distance, segment, interval
  for (unsigned long iSeg = 0; iSeg < segments.size(); ++iSeg) {
    const auto& seg = segments[iSeg];
    if (seg.marker != marker) continue;
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
  return text.str();
}

void CReferenceWall::Write(const std::string& filename) const {
  std::ofstream file(filename);
  if (!file) SU2_MPI::Error("Could not write the reference wall " + filename + ".", CURRENT_FUNCTION);
  file << "SU2_BL_REFERENCE_WALL 1\n";
  file << "CORNER_ANGLE " << std::setprecision(17) << cornerAngle << "\n";
  file << "NSEGMENT " << segments.size() << "\n";
  for (const auto& seg : segments) {
    file << "SEGMENT " << seg.marker << " " << seg.closed << " " << seg.sharpStart << " " << seg.sharpEnd << " "
         << seg.x.size() << "\n";
    for (const auto& x : seg.x) file << std::setprecision(17) << x[0] << " " << x[1] << "\n";
  }
  file << "FINGERPRINT " << Fingerprint() << "\n";
}

bool CReferenceWall::Read(const std::string& filename) {
  std::ifstream file(filename);
  if (!file) return false;
  std::string word;
  int version = 0;
  file >> word >> version;
  if (word != "SU2_BL_REFERENCE_WALL" || version != 1) return false;
  unsigned long nSegment = 0;
  file >> word >> cornerAngle >> word >> nSegment;
  segments.clear();
  for (unsigned long iSeg = 0; iSeg < nSegment; ++iSeg) {
    Segment seg;
    unsigned long nKnot = 0;
    file >> word >> seg.marker >> seg.closed >> seg.sharpStart >> seg.sharpEnd >> nKnot;
    if (!file || word != "SEGMENT") return false;
    seg.x.resize(nKnot);
    for (auto& x : seg.x) file >> x[0] >> x[1];
    Fit(seg);
    segments.push_back(seg);
  }
  std::string fingerprint;
  file >> word;
  std::getline(file, fingerprint);
  if (!fingerprint.empty() && fingerprint[0] == ' ') fingerprint.erase(0, 1);
  return word == "FINGERPRINT" && fingerprint == Fingerprint();
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

passivedouble BLWallRule::SizeAt(const SizeSamples& samples, passivedouble s) {
  const auto& S = samples.s;
  if (S.empty()) return 0.0;
  if (s <= S.front()) return samples.size.front();
  if (s >= S.back()) return samples.size.back();
  const auto i = std::upper_bound(S.begin(), S.end(), s) - S.begin() - 1;
  const auto w = (s - S[i]) / (S[i + 1] - S[i]);
  return (1.0 - w) * samples.size[i] + w * samples.size[i + 1];
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
