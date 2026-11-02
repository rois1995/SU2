/*!
 * \file p1_flux_variants.cpp
 * \brief Identities of the P1 flux variants, on small meshes (MPI safe, checks owned nodes only).
 * \version 8.6.0 "Harrier"
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

#include "catch.hpp"
#include <array>
#include <fstream>
#include <memory>
#include <random>
#include <sstream>
#include <vector>
#include "../../Common/include/CConfig.hpp"
#include "../../Common/include/geometry/CPhysicalGeometry.hpp"
#include "../../Common/include/toolboxes/boundary_closure.hpp"
#include "../../Common/include/toolboxes/p1_simplex.hpp"

namespace {

/*--- Node index on a structured lattice. ---*/
inline unsigned long Lattice(unsigned long i, unsigned long j, unsigned long k, unsigned long n) {
  return (i * (n + 1) + j) * (n + 1) + k;
}

/*!
 * \brief Write a jittered triangle (2D) or Kuhn tet (3D) mesh of the unit square/cube. Interior nodes move,
 *        boundary nodes stay on the box, the z scale stretches the cells. Markers: bottom, rest.
 */
void WriteSimplexMesh(const std::string& name, int nDim, unsigned long n, double jitter, double stretch) {
  if (SU2_MPI::GetRank() != MASTER_NODE) return;

  std::mt19937 gen(1234);
  std::uniform_real_distribution<double> rnd(-0.5, 0.5);
  std::ofstream f(name);
  f.precision(17);
  std::ostringstream elems, points;
  unsigned long nElem = 0, nPoint = 0;

  if (nDim == 2) {
    auto id = [&](unsigned long i, unsigned long j) { return i * (n + 1) + j; };
    for (auto i = 0ul; i < n; ++i) {
      for (auto j = 0ul; j < n; ++j) {
        const unsigned long q[] = {id(i, j), id(i + 1, j), id(i + 1, j + 1), id(i, j + 1)};
        if ((i + j) % 2 == 0) {
          elems << "5 " << q[0] << " " << q[1] << " " << q[2] << "\n5 " << q[0] << " " << q[2] << " " << q[3] << "\n";
        } else {
          elems << "5 " << q[0] << " " << q[1] << " " << q[3] << "\n5 " << q[1] << " " << q[2] << " " << q[3] << "\n";
        }
        nElem += 2;
      }
    }
    for (auto i = 0ul; i <= n; ++i) {
      for (auto j = 0ul; j <= n; ++j) {
        const bool inner = (i > 0) && (j > 0) && (i < n) && (j < n);
        const double x = (i + (inner ? jitter * rnd(gen) : 0.0)) / n;
        const double y = (j + (inner ? jitter * rnd(gen) : 0.0)) / n;
        points << x << " " << y * stretch << "\n";
        ++nPoint;
      }
    }
    f << "NDIME= 2\nNELEM= " << nElem << "\n" << elems.str() << "NPOIN= " << nPoint << "\n" << points.str();
    f << "NMARK= 2\nMARKER_TAG= bottom\nMARKER_ELEMS= " << n << "\n";
    for (auto i = 0ul; i < n; ++i) f << "3 " << id(i, 0) << " " << id(i + 1, 0) << "\n";
    f << "MARKER_TAG= rest\nMARKER_ELEMS= " << 3 * n << "\n";
    for (auto i = 0ul; i < n; ++i) f << "3 " << id(i + 1, n) << " " << id(i, n) << "\n";
    for (auto j = 0ul; j < n; ++j) f << "3 " << id(0, j + 1) << " " << id(0, j) << "\n";
    for (auto j = 0ul; j < n; ++j) f << "3 " << id(n, j) << " " << id(n, j + 1) << "\n";
    return;
  }

  static const int kuhn[6][4] = {{0, 1, 2, 6}, {0, 2, 3, 6}, {0, 3, 7, 6}, {0, 7, 4, 6}, {0, 4, 5, 6}, {0, 5, 1, 6}};
  for (auto i = 0ul; i < n; ++i) {
    for (auto j = 0ul; j < n; ++j) {
      for (auto k = 0ul; k < n; ++k) {
        const unsigned long h[] = {Lattice(i, j, k, n),         Lattice(i + 1, j, k, n),
                                   Lattice(i + 1, j + 1, k, n), Lattice(i, j + 1, k, n),
                                   Lattice(i, j, k + 1, n),     Lattice(i + 1, j, k + 1, n),
                                   Lattice(i + 1, j + 1, k + 1, n), Lattice(i, j + 1, k + 1, n)};
        for (const auto& t : kuhn) {
          elems << "10 " << h[t[0]] << " " << h[t[1]] << " " << h[t[2]] << " " << h[t[3]] << "\n";
          ++nElem;
        }
      }
    }
  }
  for (auto i = 0ul; i <= n; ++i) {
    for (auto j = 0ul; j <= n; ++j) {
      for (auto k = 0ul; k <= n; ++k) {
        const bool inner = (i > 0) && (j > 0) && (k > 0) && (i < n) && (j < n) && (k < n);
        const double x = (i + (inner ? jitter * rnd(gen) : 0.0)) / n;
        const double y = (j + (inner ? jitter * rnd(gen) : 0.0)) / n;
        const double z = (k + (inner ? jitter * rnd(gen) : 0.0)) / n;
        points << x << " " << y << " " << z * stretch << "\n";
        ++nPoint;
      }
    }
  }
  f << "NDIME= 3\nNELEM= " << nElem << "\n" << elems.str() << "NPOIN= " << nPoint << "\n" << points.str();

  /*--- The two triangles of each box face follow the diagonal of the Kuhn split. ---*/
  std::ostringstream bottom, rest;
  auto quad = [&](std::ostringstream& os, unsigned long a, unsigned long b, unsigned long c, unsigned long d) {
    os << "5 " << a << " " << b << " " << c << "\n5 " << a << " " << c << " " << d << "\n";
  };
  for (auto u = 0ul; u < n; ++u) {
    for (auto v = 0ul; v < n; ++v) {
      quad(bottom, Lattice(u, v, 0, n), Lattice(u, v + 1, 0, n), Lattice(u + 1, v + 1, 0, n), Lattice(u + 1, v, 0, n));
      quad(rest, Lattice(u, v, n, n), Lattice(u + 1, v, n, n), Lattice(u + 1, v + 1, n, n), Lattice(u, v + 1, n, n));
      quad(rest, Lattice(u, 0, v, n), Lattice(u + 1, 0, v, n), Lattice(u + 1, 0, v + 1, n), Lattice(u, 0, v + 1, n));
      quad(rest, Lattice(u, n, v, n), Lattice(u, n, v + 1, n), Lattice(u + 1, n, v + 1, n), Lattice(u + 1, n, v, n));
      quad(rest, Lattice(0, u, v, n), Lattice(0, u, v + 1, n), Lattice(0, u + 1, v + 1, n), Lattice(0, u + 1, v, n));
      quad(rest, Lattice(n, u, v, n), Lattice(n, u + 1, v, n), Lattice(n, u + 1, v + 1, n), Lattice(n, u, v + 1, n));
    }
  }
  f << "NMARK= 2\nMARKER_TAG= bottom\nMARKER_ELEMS= " << 2 * n * n << "\n" << bottom.str();
  f << "MARKER_TAG= rest\nMARKER_ELEMS= " << 10 * n * n << "\n" << rest.str();
}

/*!
 * \brief Write a jittered quadrilateral (2D) mesh, or a 3D mesh of hexahedra (kind 0), prisms (1) or pyramids (2).
 *        The pyramids of a lattice cell share a new center point. Markers: bottom, rest.
 */
void WriteGeneralMesh(const std::string& name, int nDim, int kind, unsigned long n, double jitter, double stretch) {
  if (SU2_MPI::GetRank() != MASTER_NODE) return;

  std::mt19937 gen(4321);
  std::uniform_real_distribution<double> rnd(-0.5, 0.5);
  std::ofstream f(name);
  f.precision(17);
  std::ostringstream elems, points, bottom, rest;
  unsigned long nElem = 0, nPoint = 0, nBottom = 0, nRest = 0;

  if (nDim == 2) {
    auto id = [&](unsigned long i, unsigned long j) { return i * (n + 1) + j; };
    for (auto i = 0ul; i < n; ++i) {
      for (auto j = 0ul; j < n; ++j) {
        elems << "9 " << id(i, j) << " " << id(i + 1, j) << " " << id(i + 1, j + 1) << " " << id(i, j + 1) << "\n";
        ++nElem;
      }
    }
    for (auto i = 0ul; i <= n; ++i) {
      for (auto j = 0ul; j <= n; ++j) {
        const bool inner = (i > 0) && (j > 0) && (i < n) && (j < n);
        points << (i + (inner ? jitter * rnd(gen) : 0.0)) / n << " "
               << (j + (inner ? jitter * rnd(gen) : 0.0)) / n * stretch << "\n";
        ++nPoint;
      }
    }
    f << "NDIME= 2\nNELEM= " << nElem << "\n" << elems.str() << "NPOIN= " << nPoint << "\n" << points.str();
    f << "NMARK= 2\nMARKER_TAG= bottom\nMARKER_ELEMS= " << n << "\n";
    for (auto i = 0ul; i < n; ++i) f << "3 " << id(i, 0) << " " << id(i + 1, 0) << "\n";
    f << "MARKER_TAG= rest\nMARKER_ELEMS= " << 3 * n << "\n";
    for (auto i = 0ul; i < n; ++i) f << "3 " << id(i + 1, n) << " " << id(i, n) << "\n";
    for (auto j = 0ul; j < n; ++j) f << "3 " << id(0, j + 1) << " " << id(0, j) << "\n";
    for (auto j = 0ul; j < n; ++j) f << "3 " << id(n, j) << " " << id(n, j + 1) << "\n";
    return;
  }

  std::vector<std::array<double, 3>> coord;
  for (auto i = 0ul; i <= n; ++i) {
    for (auto j = 0ul; j <= n; ++j) {
      for (auto k = 0ul; k <= n; ++k) {
        const bool inner = (i > 0) && (j > 0) && (k > 0) && (i < n) && (j < n) && (k < n);
        coord.push_back({(i + (inner ? jitter * rnd(gen) : 0.0)) / n, (j + (inner ? jitter * rnd(gen) : 0.0)) / n,
                         (k + (inner ? jitter * rnd(gen) : 0.0)) / n * stretch});
      }
    }
  }

  static const int pyramids[6][4] = {{0, 1, 2, 3}, {4, 7, 6, 5}, {0, 4, 5, 1}, {1, 5, 6, 2}, {2, 6, 7, 3}, {3, 7, 4, 0}};
  for (auto i = 0ul; i < n; ++i) {
    for (auto j = 0ul; j < n; ++j) {
      for (auto k = 0ul; k < n; ++k) {
        const unsigned long h[] = {Lattice(i, j, k, n),             Lattice(i + 1, j, k, n),
                                   Lattice(i + 1, j + 1, k, n),     Lattice(i, j + 1, k, n),
                                   Lattice(i, j, k + 1, n),         Lattice(i + 1, j, k + 1, n),
                                   Lattice(i + 1, j + 1, k + 1, n), Lattice(i, j + 1, k + 1, n)};
        if (kind == 0) {
          elems << "12";
          for (const auto node : h) elems << " " << node;
          elems << "\n";
          ++nElem;
        } else if (kind == 1) {
          elems << "13 " << h[0] << " " << h[1] << " " << h[2] << " " << h[4] << " " << h[5] << " " << h[6] << "\n";
          elems << "13 " << h[0] << " " << h[2] << " " << h[3] << " " << h[4] << " " << h[6] << " " << h[7] << "\n";
          nElem += 2;
        } else {
          std::array<double, 3> center = {0.0, 0.0, 0.0};
          for (const auto node : h) {
            for (int iDim = 0; iDim < 3; ++iDim) center[iDim] += coord[node][iDim] / 8;
          }
          const auto apex = coord.size();
          coord.push_back(center);
          for (const auto& p : pyramids) {
            elems << "14 " << h[p[0]] << " " << h[p[1]] << " " << h[p[2]] << " " << h[p[3]] << " " << apex << "\n";
            ++nElem;
          }
        }
      }
    }
  }
  for (const auto& x : coord) points << x[0] << " " << x[1] << " " << x[2] << "\n";
  nPoint = coord.size();

  auto quad = [&](std::ostringstream& os, unsigned long& count, bool tris, unsigned long a, unsigned long b,
                  unsigned long c, unsigned long d) {
    if (tris) {
      os << "5 " << a << " " << b << " " << c << "\n5 " << a << " " << c << " " << d << "\n";
      count += 2;
    } else {
      os << "9 " << a << " " << b << " " << c << " " << d << "\n";
      ++count;
    }
  };
  const bool prism = (kind == 1);
  for (auto u = 0ul; u < n; ++u) {
    for (auto v = 0ul; v < n; ++v) {
      quad(bottom, nBottom, prism, Lattice(u, v, 0, n), Lattice(u, v + 1, 0, n), Lattice(u + 1, v + 1, 0, n), Lattice(u + 1, v, 0, n));
      quad(rest, nRest, prism, Lattice(u, v, n, n), Lattice(u + 1, v, n, n), Lattice(u + 1, v + 1, n, n), Lattice(u, v + 1, n, n));
      quad(rest, nRest, false, Lattice(u, 0, v, n), Lattice(u + 1, 0, v, n), Lattice(u + 1, 0, v + 1, n), Lattice(u, 0, v + 1, n));
      quad(rest, nRest, false, Lattice(u, n, v, n), Lattice(u, n, v + 1, n), Lattice(u + 1, n, v + 1, n), Lattice(u + 1, n, v, n));
      quad(rest, nRest, false, Lattice(0, u, v, n), Lattice(0, u, v + 1, n), Lattice(0, u + 1, v + 1, n), Lattice(0, u + 1, v, n));
      quad(rest, nRest, false, Lattice(n, u, v, n), Lattice(n, u + 1, v, n), Lattice(n, u + 1, v + 1, n), Lattice(n, u, v + 1, n));
    }
  }
  f << "NDIME= 3\nNELEM= " << nElem << "\n" << elems.str() << "NPOIN= " << nPoint << "\n" << points.str();
  f << "NMARK= 2\nMARKER_TAG= bottom\nMARKER_ELEMS= " << nBottom << "\n" << bottom.str();
  f << "MARKER_TAG= rest\nMARKER_ELEMS= " << nRest << "\n" << rest.str();
}

/*!
 * \brief Config and partitioned geometry of a mesh file.
 */
struct MeshCase {
  std::unique_ptr<CConfig> config;
  std::unique_ptr<CGeometry> geometry;

  explicit MeshCase(const std::string& mesh, const std::string& extra = "") {
    SU2_MPI::Barrier(SU2_MPI::GetComm());
    auto* buf = std::cout.rdbuf();
    std::cout.rdbuf(nullptr);

    std::stringstream options;
    options << "SOLVER= EULER\nMESH_FORMAT= SU2\nMESH_FILENAME= " << mesh << "\n"
            << "MARKER_EULER= ( bottom )\nMARKER_FAR= ( rest )\n"
            << "MACH_NUMBER= 0.5\nINIT_OPTION= TD_CONDITIONS\n" << extra;
    config = std::unique_ptr<CConfig>(new CConfig(options, SU2_COMPONENT::SU2_CFD, false));
    {
      auto aux = std::unique_ptr<CGeometry>(new CPhysicalGeometry(config.get(), 0, 1));
      geometry = std::unique_ptr<CGeometry>(new CPhysicalGeometry(aux.get(), config.get()));
    }
    geometry->SetSendReceive(config.get());
    geometry->SetBoundaries(config.get());
    geometry->SetPoint_Connectivity();
    geometry->SetElement_Connectivity();
    geometry->SetBoundVolume();
    geometry->Check_IntElem_Orientation(config.get());
    geometry->Check_BoundElem_Orientation(config.get());
    geometry->SetEdges();
    geometry->SetVertex(config.get());
    geometry->SetControlVolume(config.get(), ALLOCATE);
    geometry->SetBoundControlVolume(config.get(), ALLOCATE);
    geometry->SetBoundCVCoeffs(config.get(), ALLOCATE);

    std::cout.rdbuf(buf);
  }
};

/*--- A linear flux F(x) = A x + b for one equation, its divergence is the trace of A. ---*/
struct LinearFlux {
  su2double A[3][3] = {{1.3, -0.4, 0.7}, {0.2, 2.1, -0.9}, {-0.6, 0.5, 0.8}};
  su2double b[3] = {0.3, -1.1, 0.6};
  su2double Eval(const CGeometry& geometry, unsigned long iPoint, unsigned short iDim) const {
    su2double val = b[iDim];
    for (auto jDim = 0u; jDim < geometry.GetnDim(); ++jDim) val += A[iDim][jDim] * geometry.nodes->GetCoord(iPoint, jDim);
    return val;
  }
  su2double Div(unsigned short nDim) const {
    su2double div = 0.0;
    for (auto iDim = 0u; iDim < nDim; ++iDim) div += A[iDim][iDim];
    return div;
  }
};

/*--- Edge sum of the central flux for one equation, added to R at both end points. ---*/
std::vector<su2double> EdgeSum(const CGeometry& geometry, const LinearFlux& flux) {
  std::vector<su2double> R(geometry.GetnPoint(), 0.0);
  for (auto iEdge = 0ul; iEdge < geometry.GetnEdge(); ++iEdge) {
    const auto iPoint = geometry.edges->GetNode(iEdge, 0);
    const auto jPoint = geometry.edges->GetNode(iEdge, 1);
    const auto* normal = geometry.edges->GetNormal(iEdge);
    su2double phi = 0.0;
    for (auto iDim = 0u; iDim < geometry.GetnDim(); ++iDim) {
      phi += 0.5 * (flux.Eval(geometry, iPoint, iDim) + flux.Eval(geometry, jPoint, iDim)) * normal[iDim];
    }
    R[iPoint] += phi;
    R[jPoint] -= phi;
  }
  return R;
}

/*--- Largest relative error of R/V against the divergence, over owned nodes of all ranks. ---*/
su2double MaxDivError(const CGeometry& geometry, const std::vector<su2double>& R, su2double div) {
  su2double local = 0.0, global = 0.0;
  for (auto iPoint = 0ul; iPoint < geometry.GetnPointDomain(); ++iPoint) {
    local = std::max(local, fabs(R[iPoint] / geometry.nodes->GetVolume(iPoint) - div) / fabs(div));
  }
  SU2_MPI::Allreduce(&local, &global, 1, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
  return global;
}

}  // namespace

TEST_CASE("Boundary closure makes the edge sum exact for a linear flux", "[P1Flux]") {
  for (int nDim : {2, 3}) {
    const std::string mesh = "p1_closure_" + std::to_string(nDim) + "d.su2";
    WriteSimplexMesh(mesh, nDim, (nDim == 2) ? 12 : 5, 0.4, 1.0);
    MeshCase test(mesh);
    const auto& geometry = *test.geometry;
    LinearFlux flux;

    auto closure = EdgeSum(geometry, flux);
    auto plain = closure;

    for (auto iMarker = 0u; iMarker < geometry.GetnMarker(); ++iMarker) {
      if (test.config->GetMarker_All_KindBC(iMarker) == SEND_RECEIVE) continue;

      for (auto iDim = 0u; iDim < geometry.GetnDim(); ++iDim) {
        su2activematrix face(geometry.GetnVertex(iMarker), geometry.GetnDim());
        face = su2double(0.0);
        auto field = [&](unsigned long iPoint) { return flux.Eval(geometry, iPoint, iDim); };
        BoundaryClosure::AddMarkerFlux(geometry, iMarker, field, face);

        for (auto iVertex = 0ul; iVertex < geometry.GetnVertex(iMarker); ++iVertex) {
          const auto iPoint = geometry.vertex[iMarker][iVertex]->GetNode();
          closure[iPoint] += face(iVertex, iDim);
          /*--- The vertex normal points into the domain. ---*/
          plain[iPoint] -= flux.Eval(geometry, iPoint, iDim) * geometry.vertex[iMarker][iVertex]->GetNormal()[iDim];
        }
      }
    }
    const auto div = flux.Div(nDim);
    INFO("nDim " << nDim);
    CHECK(MaxDivError(geometry, closure, div) < 1e-10);
    CHECK(MaxDivError(geometry, plain, div) > 1e-2);
  }
}

TEST_CASE("Median edge sum is the P1 integral of the group flux at interior nodes", "[P1Flux]") {
  for (int nDim : {2, 3}) {
    for (double stretch : {1.0, 1e-3}) {
      const std::string mesh = "p1_group_" + std::to_string(nDim) + "d.su2";
      WriteSimplexMesh(mesh, nDim, (nDim == 2) ? 12 : 5, 0.4, stretch);
      MeshCase test(mesh);
      const auto& geometry = *test.geometry;

      /*--- A nonlinear nodal flux, the identity holds for any nodal values. ---*/
      auto F = [&](unsigned long iPoint, unsigned short iDim) {
        const auto* x = geometry.nodes->GetCoord(iPoint);
        return sin(3.0 * x[0] + iDim) * cos(2.0 * x[1]) + x[nDim - 1] * x[nDim - 1] * (iDim + 1);
      };

      std::vector<su2double> edgeSum(geometry.GetnPoint(), 0.0), cellSum(geometry.GetnPoint(), 0.0);

      for (auto iEdge = 0ul; iEdge < geometry.GetnEdge(); ++iEdge) {
        const auto iPoint = geometry.edges->GetNode(iEdge, 0);
        const auto jPoint = geometry.edges->GetNode(iEdge, 1);
        const auto* normal = geometry.edges->GetNormal(iEdge);
        su2double phi = 0.0;
        for (auto iDim = 0; iDim < nDim; ++iDim) phi += 0.5 * (F(iPoint, iDim) + F(jPoint, iDim)) * normal[iDim];
        edgeSum[iPoint] += phi;
        edgeSum[jPoint] -= phi;
      }

      for (auto iElem = 0ul; iElem < geometry.GetnElem(); ++iElem) {
        auto* elem = geometry.elem[iElem];
        const su2double* x[4];
        for (auto iNode = 0u; iNode <= unsigned(nDim); ++iNode) x[iNode] = geometry.nodes->GetCoord(elem->GetNode(iNode));
        su2double grad[4][3];
        const auto size = fabs(P1Simplex::ShapeGradients(nDim, x, grad));
        su2double div = 0.0;
        for (auto iNode = 0u; iNode <= unsigned(nDim); ++iNode) {
          for (auto iDim = 0; iDim < nDim; ++iDim) div += F(elem->GetNode(iNode), iDim) * grad[iNode][iDim];
        }
        for (auto iNode = 0u; iNode <= unsigned(nDim); ++iNode) cellSum[elem->GetNode(iNode)] += size / (nDim + 1) * div;
      }

      su2double local = 0.0, global = 0.0;
      for (auto iPoint = 0ul; iPoint < geometry.GetnPointDomain(); ++iPoint) {
        if (geometry.nodes->GetPhysicalBoundary(iPoint)) continue;
        local = std::max(local, fabs(edgeSum[iPoint] - cellSum[iPoint]) / (fabs(cellSum[iPoint]) + geometry.nodes->GetVolume(iPoint)));
      }
      SU2_MPI::Allreduce(&local, &global, 1, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
      INFO("nDim " << nDim << " stretch " << stretch);
      CHECK(global < 1e-10);
    }
  }
}

TEST_CASE("P1 edge form on the modified dual", "[P1Flux]") {
  for (int nDim : {2, 3}) {
    for (double stretch : {1.0, 1e-3}) {
      const std::string mesh = "p1_edgeform_" + std::to_string(nDim) + "d.su2";
      WriteSimplexMesh(mesh, nDim, (nDim == 2) ? 12 : 5, 0.4, stretch);
      MeshCase test(mesh, "MOD_CENTROIDS= YES\nCONV_NUM_METHOD_FLOW= JST\nEDGE_FORM_P1= YES\n");
      auto& geometry = *test.geometry;
      geometry.SetEdgeFormP1();

      LinearFlux flux;
      std::vector<su2double> R(geometry.GetnPoint(), 0.0);

      for (auto iEdge = 0ul; iEdge < geometry.GetnEdge(); ++iEdge) {
        const auto iPoint = geometry.edges->GetNode(iEdge, 0);
        const auto jPoint = geometry.edges->GetNode(iEdge, 1);
        su2double phi = 0.0;
        for (auto iDim = 0; iDim < nDim; ++iDim) {
          const auto Fi = flux.Eval(geometry, iPoint, iDim);
          const auto Fj = flux.Eval(geometry, jPoint, iDim);
          phi += 0.5 * (Fi + Fj) * geometry.edges->GetFormNormal()(iEdge, iDim);
          phi += (Fj - Fi) * geometry.edges->GetFormE()(iEdge, iDim);
        }
        R[iPoint] += phi;
        R[jPoint] -= phi;
      }

      su2double local = 0.0, global = 0.0;
      const auto div = flux.Div(nDim);
      for (auto iPoint = 0ul; iPoint < geometry.GetnPointDomain(); ++iPoint) {
        if (geometry.nodes->GetPhysicalBoundary(iPoint)) continue;
        local = std::max(local, fabs(R[iPoint] / geometry.nodes->GetVolume(iPoint) - div) / fabs(div));
      }
      SU2_MPI::Allreduce(&local, &global, 1, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
      INFO("nDim " << nDim << " stretch " << stretch);
      CHECK(global < 1e-9);
    }
  }
}

TEST_CASE("Boundary closure makes the P1 edge form exact at all nodes", "[P1Flux]") {
  for (int nDim : {2, 3}) {
    for (double stretch : {1.0, 1e-3}) {
      const std::string mesh = "p1_edgeform_bc_" + std::to_string(nDim) + "d.su2";
      WriteSimplexMesh(mesh, nDim, (nDim == 2) ? 12 : 5, 0.4, stretch);
      MeshCase test(mesh, "MOD_CENTROIDS= YES\nCONV_NUM_METHOD_FLOW= JST\nEDGE_FORM_P1= YES\nBC_WEAK_P1= YES\n");
      auto& geometry = *test.geometry;
      geometry.SetEdgeFormP1();

      LinearFlux flux;
      std::vector<su2double> R(geometry.GetnPoint(), 0.0);
      for (auto iEdge = 0ul; iEdge < geometry.GetnEdge(); ++iEdge) {
        const auto iPoint = geometry.edges->GetNode(iEdge, 0);
        const auto jPoint = geometry.edges->GetNode(iEdge, 1);
        su2double phi = 0.0;
        for (auto iDim = 0; iDim < nDim; ++iDim) {
          const auto Fi = flux.Eval(geometry, iPoint, iDim);
          const auto Fj = flux.Eval(geometry, jPoint, iDim);
          phi += 0.5 * (Fi + Fj) * geometry.edges->GetFormNormal()(iEdge, iDim);
          phi += (Fj - Fi) * geometry.edges->GetFormE()(iEdge, iDim);
        }
        R[iPoint] += phi;
        R[jPoint] -= phi;
      }

      for (auto iMarker = 0u; iMarker < geometry.GetnMarker(); ++iMarker) {
        if (test.config->GetMarker_All_KindBC(iMarker) == SEND_RECEIVE) continue;
        for (auto iDim = 0; iDim < nDim; ++iDim) {
          su2activematrix face(geometry.GetnVertex(iMarker), nDim);
          face = su2double(0.0);
          auto field = [&](unsigned long iPoint) { return flux.Eval(geometry, iPoint, iDim); };
          BoundaryClosure::AddMarkerFlux(geometry, iMarker, field, face);
          for (auto iVertex = 0ul; iVertex < geometry.GetnVertex(iMarker); ++iVertex) {
            R[geometry.vertex[iMarker][iVertex]->GetNode()] += face(iVertex, iDim);
          }
        }
      }
      INFO("nDim " << nDim << " stretch " << stretch);
      CHECK(MaxDivError(geometry, R, flux.Div(nDim)) < 1e-9);
    }
  }
}

TEST_CASE("P1 edge form reduces to the median normal on the median dual", "[P1Flux]") {
  for (int nDim : {2, 3}) {
    const std::string mesh = "p1_edgeform_median_" + std::to_string(nDim) + "d.su2";
    WriteSimplexMesh(mesh, nDim, (nDim == 2) ? 12 : 5, 0.4, 1.0);
    MeshCase test(mesh);
    auto& geometry = *test.geometry;
    geometry.SetEdgeFormP1();

    su2double maxE = 0.0, maxDiff = 0.0;
    for (auto iEdge = 0ul; iEdge < geometry.GetnEdge(); ++iEdge) {
      if (geometry.nodes->GetDomain(geometry.edges->GetNode(iEdge, 0)) ||
          geometry.nodes->GetDomain(geometry.edges->GetNode(iEdge, 1))) {
        for (auto iDim = 0; iDim < nDim; ++iDim) {
          maxE = std::max(maxE, fabs(geometry.edges->GetFormE()(iEdge, iDim)));
          maxDiff = std::max(maxDiff, fabs(geometry.edges->GetFormNormal()(iEdge, iDim) - geometry.edges->GetNormal(iEdge)[iDim]));
        }
      }
    }
    INFO("nDim " << nDim);
    CHECK(maxE < 1e-13);
    CHECK(maxDiff < 1e-13);
  }
}

/*--- Sum of the split flux of one equation at each node, with the true edges and the extra pairs. ---*/
std::vector<su2double> SplitSum(const CGeometry& geometry, const LinearFlux& flux, bool constant) {
  const auto nDim = geometry.GetnDim();
  std::vector<su2double> R(geometry.GetnPoint(), 0.0);
  auto add = [&](unsigned long iPoint, unsigned long jPoint, const su2double* normal, const su2double* E) {
    su2double phi = 0.0;
    for (auto iDim = 0u; iDim < nDim; ++iDim) {
      if (constant) {
        phi += flux.b[iDim] * normal[iDim];
      } else {
        const auto Fi = flux.Eval(geometry, iPoint, iDim);
        const auto Fj = flux.Eval(geometry, jPoint, iDim);
        phi += 0.5 * (Fi + Fj) * normal[iDim] + (Fj - Fi) * E[iDim];
      }
    }
    R[iPoint] += phi;
    R[jPoint] -= phi;
  };
  for (auto iEdge = 0ul; iEdge < geometry.GetnEdge(); ++iEdge) {
    add(geometry.edges->GetNode(iEdge, 0), geometry.edges->GetNode(iEdge, 1), geometry.edges->GetFormNormal()[iEdge],
        geometry.edges->GetFormE()[iEdge]);
  }
  const auto& pairs = geometry.edges->GetSplitNodes();
  for (auto iSplit = 0ul; iSplit < pairs.size(); ++iSplit) {
    add(pairs[iSplit][0], pairs[iSplit][1], geometry.edges->GetSplitNormal()[iSplit], geometry.edges->GetSplitE()[iSplit]);
  }
  return R;
}

TEST_CASE("Split edges on general cells", "[P1Flux]") {
  struct Kind {
    int nDim, kind;
    unsigned long n;
  };
  for (const auto& mesh : {Kind{2, 0, 10}, Kind{3, 0, 5}, Kind{3, 1, 5}, Kind{3, 2, 4}}) {
    for (double stretch : {1.0, 1e-3}) {
      const std::string name = "p1_split_" + std::to_string(mesh.nDim) + "_" + std::to_string(mesh.kind) + ".su2";
      WriteGeneralMesh(name, mesh.nDim, mesh.kind, mesh.n, 0.2, stretch);
      MeshCase test(name, "CONV_NUM_METHOD_FLOW= JST\nSPLIT_EDGE_FLUX= YES\n");
      auto& geometry = *test.geometry;
      geometry.SetSplitEdges();
      INFO("nDim " << mesh.nDim << " kind " << mesh.kind << " stretch " << stretch);

      /*--- The split volumes fill the box. ---*/

      su2double local[4] = {}, global[4] = {};
      for (auto iPoint = 0ul; iPoint < geometry.GetnPointDomain(); ++iPoint) {
        local[0] += geometry.edges->GetSplitVolume()[iPoint];
      }

      /*--- A linear flux is exact at interior nodes. ---*/

      LinearFlux flux;
      const auto div = flux.Div(mesh.nDim);
      auto R = SplitSum(geometry, flux, false);
      for (auto iPoint = 0ul; iPoint < geometry.GetnPointDomain(); ++iPoint) {
        if (geometry.nodes->GetPhysicalBoundary(iPoint)) continue;
        local[1] = std::max(local[1], fabs(R[iPoint] / geometry.edges->GetSplitVolume()[iPoint] - div) / fabs(div));
      }

      /*--- A constant flux closes at every node with the vertex normals, which point into the domain. ---*/

      R = SplitSum(geometry, flux, true);
      for (auto iMarker = 0u; iMarker < geometry.GetnMarker(); ++iMarker) {
        if (test.config->GetMarker_All_KindBC(iMarker) == SEND_RECEIVE) continue;
        for (auto iVertex = 0ul; iVertex < geometry.GetnVertex(iMarker); ++iVertex) {
          const auto iPoint = geometry.vertex[iMarker][iVertex]->GetNode();
          const auto* normal = geometry.vertex[iMarker][iVertex]->GetNormal();
          for (auto iDim = 0u; iDim < geometry.GetnDim(); ++iDim) R[iPoint] -= flux.b[iDim] * normal[iDim];
        }
      }
      for (auto iPoint = 0ul; iPoint < geometry.GetnPointDomain(); ++iPoint) {
        if (mesh.nDim == 3) {
          for (auto iDim = 0u; iDim < 3; ++iDim) R[iPoint] += flux.b[iDim] * geometry.edges->GetSplitBoundNormal()(iPoint, iDim);
        }
        local[2] = std::max(local[2], fabs(R[iPoint]) / geometry.nodes->GetVolume(iPoint));
        local[3] = std::max(local[3], fabs(geometry.edges->GetSplitVolume()[iPoint] / geometry.nodes->GetVolume(iPoint) - 1.0));
      }

      SU2_MPI::Allreduce(local, global, 1, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
      SU2_MPI::Allreduce(local + 1, global + 1, 3, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
      CHECK(fabs(global[0] - stretch) < 1e-10 * stretch);
      CHECK(global[1] < 1e-8);
      CHECK(global[2] < 1e-8 / stretch);
      CHECK(global[3] < 1e-8);
    }
  }
}

TEST_CASE("Split coefficients follow a moved mesh", "[P1Flux]") {
  struct Kind {
    int nDim, kind;
    unsigned long n;
  };
  for (const auto& mesh : {Kind{2, 0, 10}, Kind{3, 0, 5}, Kind{3, 1, 5}, Kind{3, 2, 4}}) {
    const std::string name = "p1_split_move_" + std::to_string(mesh.nDim) + "_" + std::to_string(mesh.kind) + ".su2";
    WriteGeneralMesh(name, mesh.nDim, mesh.kind, mesh.n, 0.2, 1.0);
    MeshCase test(name, "CONV_NUM_METHOD_FLOW= JST\nSPLIT_EDGE_FLUX= YES\n");
    auto& geometry = *test.geometry;
    geometry.UpdateP1FluxCoefficients(test.config.get());

    /*--- A smooth motion of all nodes, the same on every rank. ---*/

    for (auto iPoint = 0ul; iPoint < geometry.GetnPoint(); ++iPoint) {
      const su2double x = geometry.nodes->GetCoord(iPoint, 0), y = geometry.nodes->GetCoord(iPoint, 1);
      geometry.nodes->SetCoord(iPoint, 0, x + 0.1 * y * y);
      geometry.nodes->SetCoord(iPoint, 1, y + 0.05 * sin(3.0 * x));
    }
    geometry.SetControlVolume(test.config.get(), UPDATE);
    geometry.SetBoundControlVolume(test.config.get(), UPDATE);
    geometry.SetBoundCVCoeffs(test.config.get(), UPDATE);
    geometry.UpdateP1FluxCoefficients(test.config.get());

    LinearFlux flux;
    const auto div = flux.Div(mesh.nDim);
    const auto R = SplitSum(geometry, flux, false);
    su2double local[2] = {}, global[2] = {};
    for (auto iPoint = 0ul; iPoint < geometry.GetnPointDomain(); ++iPoint) {
      const auto volume = geometry.nodes->GetVolume(iPoint);
      local[0] = std::max(local[0], fabs(geometry.edges->GetSplitVolume()[iPoint] / volume - 1.0));
      if (!geometry.nodes->GetPhysicalBoundary(iPoint)) {
        local[1] = std::max(local[1], fabs(R[iPoint] / volume - div) / fabs(div));
      }
    }
    SU2_MPI::Allreduce(local, global, 2, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
    INFO("nDim " << mesh.nDim << " kind " << mesh.kind);
    CHECK(global[0] < 1e-3);
    CHECK(global[1] < 1e-3);
  }
}

TEST_CASE("Split edges on simplices reduce to the median normal", "[P1Flux]") {
  for (int nDim : {2, 3}) {
    const std::string mesh = "p1_split_simplex_" + std::to_string(nDim) + "d.su2";
    WriteSimplexMesh(mesh, nDim, (nDim == 2) ? 12 : 5, 0.4, 1.0);
    MeshCase test(mesh, "CONV_NUM_METHOD_FLOW= JST\nSPLIT_EDGE_FLUX= YES\n");
    auto& geometry = *test.geometry;
    geometry.SetSplitEdges();

    su2double maxDiff = 0.0;
    for (auto iEdge = 0ul; iEdge < geometry.GetnEdge(); ++iEdge) {
      if (geometry.nodes->GetDomain(geometry.edges->GetNode(iEdge, 0)) ||
          geometry.nodes->GetDomain(geometry.edges->GetNode(iEdge, 1))) {
        for (auto iDim = 0; iDim < nDim; ++iDim) {
          maxDiff = std::max(maxDiff, fabs(geometry.edges->GetFormNormal()(iEdge, iDim) - geometry.edges->GetNormal(iEdge)[iDim]));
        }
      }
    }
    INFO("nDim " << nDim);
    CHECK(geometry.edges->GetSplitNodes().empty());
    CHECK(maxDiff < 1e-13);
  }
}

namespace {

/*--- Options of a moving mesh, grid velocities are only allocated with them. ---*/
const std::string GridMotion =
    "TIME_DOMAIN= YES\nTIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\nTIME_STEP= 0.01\nGRID_MOVEMENT= RIGID_MOTION\n";

/*--- Rate of a motion x = X + s d(X), nonlinear, with boundary nodes moving along and across the boundary. ---*/
void Displacement(unsigned short nDim, const su2double* X, su2double* d) {
  d[0] = 0.08 * X[1] * X[1] + 0.05 * sin(2.0 * X[0]);
  d[1] = 0.05 * sin(3.0 * X[0]) + 0.06 * X[1] * X[0];
  if (nDim == 3) {
    d[0] += 0.04 * X[2] * X[1];
    d[1] += 0.03 * cos(2.0 * X[2]);
    d[2] = 0.05 * sin(2.0 * X[1]) * X[0] + 0.04 * X[2] * X[2];
  }
}

/*--- Reference coordinates, so that all ranks and all halos follow the same motion. ---*/
std::vector<std::array<su2double, 3>> StoreCoord(const CGeometry& geometry) {
  std::vector<std::array<su2double, 3>> X0(geometry.GetnPoint(), std::array<su2double, 3>{});
  for (auto iPoint = 0ul; iPoint < geometry.GetnPoint(); ++iPoint) {
    for (auto iDim = 0u; iDim < geometry.GetnDim(); ++iDim) X0[iPoint][iDim] = geometry.nodes->GetCoord(iPoint, iDim);
  }
  return X0;
}

void MoveMesh(CGeometry& geometry, CConfig* config, const std::vector<std::array<su2double, 3>>& X0, su2double s) {
  const auto nDim = geometry.GetnDim();
  for (auto iPoint = 0ul; iPoint < geometry.GetnPoint(); ++iPoint) {
    su2double d[3] = {};
    Displacement(nDim, X0[iPoint].data(), d);
    for (auto iDim = 0u; iDim < nDim; ++iDim) geometry.nodes->SetCoord(iPoint, iDim, X0[iPoint][iDim] + s * d[iDim]);
  }
  geometry.SetControlVolume(config, UPDATE);
  geometry.SetBoundControlVolume(config, UPDATE);
  geometry.SetBoundCVCoeffs(config, UPDATE);
  geometry.UpdateP1FluxCoefficients(config);
}

su2double GlobalMax(su2double local) {
  su2double global = 0.0;
  SU2_MPI::Allreduce(&local, &global, 1, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
  return global;
}

su2double GlobalSum(su2double local) {
  su2double global = 0.0;
  SU2_MPI::Allreduce(&local, &global, 1, MPI_DOUBLE, MPI_SUM, SU2_MPI::GetComm());
  return global;
}

su2double OwnedVolume(const CGeometry& geometry) {
  su2double volume = 0.0;
  for (auto iPoint = 0ul; iPoint < geometry.GetnPointDomain(); ++iPoint) volume += geometry.nodes->GetVolume(iPoint);
  return GlobalSum(volume);
}

/*--- Grid flux on the mesh at s = 0.5, and dV/dt from the two neighbouring configurations. ---*/
void GridFluxAndVolumeChange(CGeometry& geometry, CConfig* config, std::vector<su2double>& dVdt, su2activevector& G) {
  const auto X0 = StoreCoord(geometry);
  const su2double s = 0.5, eps = 1e-4;
  dVdt.assign(geometry.GetnPointDomain(), 0.0);

  MoveMesh(geometry, config, X0, s + eps);
  for (auto iPoint = 0ul; iPoint < dVdt.size(); ++iPoint) dVdt[iPoint] = geometry.nodes->GetVolume(iPoint) / (2 * eps);
  MoveMesh(geometry, config, X0, s - eps);
  for (auto iPoint = 0ul; iPoint < dVdt.size(); ++iPoint) dVdt[iPoint] -= geometry.nodes->GetVolume(iPoint) / (2 * eps);
  MoveMesh(geometry, config, X0, s);

  for (auto iPoint = 0ul; iPoint < geometry.GetnPoint(); ++iPoint) {
    su2double d[3] = {};
    Displacement(geometry.GetnDim(), X0[iPoint].data(), d);
    for (auto iDim = 0u; iDim < geometry.GetnDim(); ++iDim) geometry.nodes->SetGridVel(iPoint, iDim, d[iDim]);
  }
  geometry.SetGridFluxP1(config, G);
}

}  // namespace

TEST_CASE("Grid flux of the median dual with the weak weights is the volume change", "[P1Flux]") {
  for (int nDim : {2, 3}) {
    const std::string mesh = "p1_gcl_median_" + std::to_string(nDim) + "d.su2";
    WriteSimplexMesh(mesh, nDim, (nDim == 2) ? 12 : 5, 0.4, 1.0);
    MeshCase test(mesh, GridMotion + "CONV_NUM_METHOD_FLOW= JST\nBC_WEAK_P1= YES\n");
    auto& geometry = *test.geometry;

    std::vector<su2double> dVdt;
    su2activevector G;
    GridFluxAndVolumeChange(geometry, test.config.get(), dVdt, G);

    su2double scale = 0.0, error = 0.0;
    for (auto iPoint = 0ul; iPoint < dVdt.size(); ++iPoint) {
      scale = std::max(scale, fabs(dVdt[iPoint]));
      error = std::max(error, fabs(G[iPoint] - dVdt[iPoint]));
    }
    INFO("nDim " << nDim);
    CHECK(GlobalMax(error) < 1e-7 * GlobalMax(scale));
  }
}

TEST_CASE("Grid flux of the P1 forms on moving meshes", "[P1Flux]") {
  struct Kind {
    int nDim, kind;  // kind -1: simplex mesh.
    unsigned long n;
    std::string options;
    bool weak;
  };
  const std::string edge = "MOD_CENTROIDS= YES\nCONV_NUM_METHOD_FLOW= JST\nEDGE_FORM_P1= YES\n";
  const std::string split = "CONV_NUM_METHOD_FLOW= JST\nSPLIT_EDGE_FLUX= YES\n";
  const std::vector<Kind> cases = {
      Kind{2, -1, 12, edge + "BC_WEAK_P1= YES\n", true}, Kind{3, -1, 5, edge + "BC_WEAK_P1= YES\n", true},
      Kind{2, -1, 12, edge, false},                      Kind{2, -1, 12, split, false},
      Kind{3, -1, 5, split, false},                      Kind{2, 0, 10, split, false},
      Kind{3, 0, 5, split, false},                       Kind{3, 1, 5, split, false},
      Kind{3, 2, 4, split, false}};

  for (const auto& mesh : cases) {
    const std::string name = "p1_gcl_" + std::to_string(mesh.nDim) + "_" + std::to_string(mesh.kind) + ".su2";
    if (mesh.kind < 0) {
      WriteSimplexMesh(name, mesh.nDim, mesh.n, 0.4, 1.0);
    } else {
      WriteGeneralMesh(name, mesh.nDim, mesh.kind, mesh.n, 0.2, 1.0);
    }
    MeshCase test(name, GridMotion + mesh.options);
    auto& geometry = *test.geometry;
    const auto nDim = geometry.GetnDim();
    INFO("nDim " << mesh.nDim << " kind " << mesh.kind << " weak " << mesh.weak << " " << mesh.options);

    /*--- The total grid flux is the total volume change: it must reach all pairs and all boundaries once. ---*/

    const auto X0 = StoreCoord(geometry);
    const su2double s = 0.5, eps = 1e-4;
    MoveMesh(geometry, test.config.get(), X0, s + eps);
    const auto volumeP = OwnedVolume(geometry);
    MoveMesh(geometry, test.config.get(), X0, s - eps);
    const auto volumeM = OwnedVolume(geometry);
    MoveMesh(geometry, test.config.get(), X0, 0.0);

    std::vector<su2double> dVdt;
    su2activevector G;
    GridFluxAndVolumeChange(geometry, test.config.get(), dVdt, G);

    su2double total = 0.0, scale = 0.0;
    for (auto iPoint = 0ul; iPoint < dVdt.size(); ++iPoint) {
      total += G[iPoint];
      scale = std::max(scale, fabs(dVdt[iPoint]));
    }
    CHECK(fabs(GlobalSum(total) - (volumeP - volumeM) / (2 * eps)) < 1e-7 * GlobalMax(scale));

    /*--- A translation gives no grid flux at any node, the pairs close with the boundary terms. ---*/

    const su2double translation[3] = {0.7, -0.4, 0.3};
    for (auto iPoint = 0ul; iPoint < geometry.GetnPoint(); ++iPoint) {
      for (auto iDim = 0u; iDim < nDim; ++iDim) geometry.nodes->SetGridVel(iPoint, iDim, translation[iDim]);
    }
    geometry.SetGridFluxP1(test.config.get(), G);
    su2double closure = 0.0;
    for (auto iPoint = 0ul; iPoint < geometry.GetnPointDomain(); ++iPoint) {
      closure = std::max(closure, fabs(G[iPoint]) / geometry.nodes->GetVolume(iPoint));
    }
    CHECK(GlobalMax(closure) < 1e-9);

    /*--- A linear velocity gives its divergence at interior nodes, and at all nodes with the weak weights. ---*/

    LinearFlux flux;
    for (auto iPoint = 0ul; iPoint < geometry.GetnPoint(); ++iPoint) {
      for (auto iDim = 0u; iDim < nDim; ++iDim) {
        geometry.nodes->SetGridVel(iPoint, iDim, flux.Eval(geometry, iPoint, iDim));
      }
    }
    geometry.SetGridFluxP1(test.config.get(), G);
    su2double linear = 0.0;
    const auto div = flux.Div(nDim);
    for (auto iPoint = 0ul; iPoint < geometry.GetnPointDomain(); ++iPoint) {
      if (!mesh.weak && geometry.nodes->GetPhysicalBoundary(iPoint)) continue;
      linear = std::max(linear, fabs(G[iPoint] / geometry.nodes->GetVolume(iPoint) - div) / fabs(div));
    }
    CHECK(GlobalMax(linear) < 1e-8);
  }
}

TEST_CASE("GCL_EXACT_P1 makes the grid flux the BDF difference of the volumes", "[P1Flux]") {
  struct Kind {
    int nDim, kind;  // kind -1: simplex mesh.
    unsigned long n;
    std::string options;
  };
  const std::string edge = "MOD_CENTROIDS= YES\nCONV_NUM_METHOD_FLOW= JST\nEDGE_FORM_P1= YES\n";
  const std::string split = "CONV_NUM_METHOD_FLOW= JST\nSPLIT_EDGE_FLUX= YES\n";
  const std::vector<Kind> cases = {
      Kind{2, -1, 12, edge + "BC_WEAK_P1= YES\n"}, Kind{3, -1, 5, edge + "BC_WEAK_P1= YES\n"}, Kind{2, -1, 12, edge},
      Kind{2, -1, 12, split}, Kind{3, -1, 5, split}, Kind{2, 0, 10, split}, Kind{3, 0, 5, split}, Kind{3, 1, 5, split},
      Kind{3, 2, 4, split}};

  for (int order : {1, 2}) {
    for (const auto& mesh : cases) {
      const std::string name = "p1_gcl_exact_" + std::to_string(mesh.nDim) + "_" + std::to_string(mesh.kind) + ".su2";
      if (mesh.kind < 0) {
        WriteSimplexMesh(name, mesh.nDim, mesh.n, 0.4, 1.0);
      } else {
        WriteGeneralMesh(name, mesh.nDim, mesh.kind, mesh.n, 0.2, 1.0);
      }
      const std::string motion = "TIME_DOMAIN= YES\nTIME_MARCHING= DUAL_TIME_STEPPING-" +
                                 std::string(order == 1 ? "1ST" : "2ND") +
                                 "_ORDER\nTIME_STEP= 0.01\nGRID_MOVEMENT= RIGID_MOTION\n";
      MeshCase test(name, motion + mesh.options + "GCL_EXACT_P1= YES\n");
      auto& geometry = *test.geometry;
      const auto config = test.config.get();
      const auto nDim = geometry.GetnDim();
      INFO("BDF" << order << " nDim " << mesh.nDim << " kind " << mesh.kind << " " << mesh.options);

      /*--- A large step, so that the BDF differences are far from the time derivatives. ---*/

      const su2double dt = 0.01, step = 0.1;
      config->SetDelta_UnstTimeND(dt);
      const auto X0 = StoreCoord(geometry);

      MoveMesh(geometry, config, X0, 0.5 - 2 * step);
      geometry.nodes->SetVolume_n();
      geometry.nodes->SetCoord_n();
      MoveMesh(geometry, config, X0, 0.5 - step);
      geometry.nodes->SetVolume_nM1();
      geometry.nodes->SetVolume_n();
      geometry.nodes->SetCoord_n1();
      geometry.nodes->SetCoord_n();
      MoveMesh(geometry, config, X0, 0.5);

      for (auto iPoint = 0ul; iPoint < geometry.GetnPoint(); ++iPoint) {
        const auto* x = geometry.nodes->GetCoord(iPoint);
        const auto* x_n = geometry.nodes->GetCoord_n(iPoint);
        const auto* x_n1 = geometry.nodes->GetCoord_n1(iPoint);
        for (auto iDim = 0u; iDim < nDim; ++iDim) {
          const su2double vel = (order == 1) ? (x[iDim] - x_n[iDim]) / dt
                                             : (1.5 * x[iDim] - 2.0 * x_n[iDim] + 0.5 * x_n1[iDim]) / dt;
          geometry.nodes->SetGridVel(iPoint, iDim, vel);
        }
      }

      geometry.UpdateSweptFluxP1(config);
      su2activevector G;
      geometry.SetGridFluxP1(config, G);

      su2double error = 0.0, scale = 0.0, total = 0.0, totalDV = 0.0;
      for (auto iPoint = 0ul; iPoint < geometry.GetnPointDomain(); ++iPoint) {
        const auto V = geometry.nodes->GetVolume(iPoint);
        const auto V_n = geometry.nodes->GetVolume_n(iPoint);
        const auto V_n1 = geometry.nodes->GetVolume_nM1(iPoint);
        const su2double dVdt = (order == 1) ? (V - V_n) / dt : (1.5 * V - 2.0 * V_n + 0.5 * V_n1) / dt;
        error = std::max(error, fabs(G[iPoint] - dVdt));
        scale = std::max(scale, fabs(dVdt));
        total += G[iPoint];
        totalDV += dVdt;
      }
      CHECK(GlobalMax(error) < 1e-10 * GlobalMax(scale));
      CHECK(fabs(GlobalSum(total) - GlobalSum(totalDV)) < 1e-9 * GlobalMax(scale));
    }
  }
}
