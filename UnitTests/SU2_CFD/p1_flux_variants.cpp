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
#include <algorithm>
#include <cmath>
#include <limits>
#include <array>
#include <map>
#include <fstream>
#include <memory>
#include <random>
#include <sstream>
#include <vector>
#include "../../Common/include/CConfig.hpp"
#include "../../Common/include/geometry/CPhysicalGeometry.hpp"
#include "../../Common/include/toolboxes/boundary_closure.hpp"
#include "../../Common/include/toolboxes/p1_simplex.hpp"
#include "../../Common/include/toolboxes/p1_mixed.hpp"
#include "../../Common/include/geometry/primal_grid/CHexahedron.hpp"
#include "../../Common/include/geometry/primal_grid/CPrism.hpp"
#include "../../Common/include/geometry/primal_grid/CPyramid.hpp"
#include "../../Common/include/geometry/primal_grid/CQuadrilateral.hpp"
#include "../../SU2_CFD/include/numerics/flow/p1_viscous_entropy.hpp"
#include "../../SU2_CFD/include/numerics/flow/p1_ale_entropy.hpp"
#include "../../SU2_CFD/include/numerics/flow/p1_positive_update.hpp"
#include "../../SU2_CFD/include/numerics/flow/p1_boundary_ghost.hpp"
#include "../../SU2_CFD/include/numerics/flow/p1_boundary_closure.hpp"
#include "../../SU2_CFD/include/numerics/flow/jst_weak_wall.hpp"
#include "../../SU2_CFD/include/numerics/flow/p1_sst_matched.hpp"
#include "../../SU2_CFD/include/numerics/flow/jst_entropy.hpp"
#include "../../SU2_CFD/include/numerics_simd/flow/convection/common.hpp"
#include "../../SU2_CFD/include/numerics_simd/flow/variables.hpp"
#include "tecno_adjoint_checks.hpp"

namespace {

/*--- Node index on a structured lattice. ---*/
inline unsigned long Lattice(unsigned long i, unsigned long j, unsigned long k, unsigned long n) {
  return (i * (n + 1) + j) * (n + 1) + k;
}

/*!
 * \brief Write a jittered triangle (2D) or Kuhn tet (3D) mesh of the unit square/cube. Interior nodes move,
 *        boundary nodes stay on the box, the z scale stretches the cells. Markers: bottom, rest. An optional map
 *        moves every point (x, y, z) after the stretch (e.g. bends the box so that the bottom wall is curved).
 */
void WriteSimplexMesh(const std::string& name, int nDim, unsigned long n, double jitter, double stretch,
                      void (*map)(double*) = nullptr) {
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
        double p[3] = {x, y * stretch, 0.0};
        if (map) map(p);
        points << p[0] << " " << p[1] << "\n";
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
        double p[3] = {x, y, z * stretch};
        if (map) map(p);
        points << p[0] << " " << p[1] << " " << p[2] << "\n";
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

namespace {

namespace pve = P1ViscousEntropy;

constexpr su2double kGamma = 1.4, kGas = 0.7;

/*--- Random simplex (z or y stretched by "stretch") and random nodal data of VISC_P1_ENTROPY. ---*/
void RandomElement(std::mt19937& gen, int nDim, double stretch, su2double x[4][3], pve::NodeState* state) {
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  while (true) {
    for (int a = 0; a <= nDim; ++a) {
      for (int i = 0; i < 3; ++i) x[a][i] = (i < nDim) ? u(gen) : 0.0;
      x[a][nDim - 1] *= stretch;
    }
    su2double g[4][3];
    if (fabs(P1Simplex::ShapeGradients(nDim, x, g)) > 1e-3 * stretch) break;
  }
  for (int a = 0; a <= nDim; ++a) {
    auto& S = state[a];
    S.rho = exp(u(gen));
    S.p = S.rho * kGas * exp(u(gen));
    for (int i = 0; i < 3; ++i) S.vel[i] = (i < nDim) ? 1.5 * u(gen) : 0.0;
    S.mu = exp(2.0 * u(gen) - 1.0);
    S.kappa = exp(2.0 * u(gen));
  }
}

/*--- Full entropy variables of the closure C helpers, w = (w_0, theta v, -theta). ---*/
void FullEntropyVariables(int nDim, const pve::NodeState& S, su2double* w) {
  su2double vel2 = 0.0;
  for (int i = 0; i < nDim; ++i) vel2 += S.vel[i] * S.vel[i];
  const su2double theta = S.rho / S.p, s = log(S.p) - kGamma * log(S.rho);
  w[0] = (kGamma - s) / (kGamma - 1.0) - 0.5 * theta * vel2;
  for (int i = 0; i < nDim; ++i) w[i + 1] = theta * S.vel[i];
  w[nDim + 1] = -theta;
}

/*--- Element flux with the frozen state of E and the nodal entropy variables of "state" (Picard). ---*/
pve::Element Frozen(const pve::Element& E, const pve::NodeState* state) {
  pve::Element F = E;
  su2double gT[3] = {}, gM[3][3] = {};
  for (int a = 0; a < E.nNodes; ++a) {
    F.thetaNode[a] = state[a].rho / state[a].p;
    for (int i = 0; i < E.nDim; ++i) F.wMom[a][i] = F.thetaNode[a] * state[a].vel[i];
    for (int j = 0; j < E.nDim; ++j) {
      gT[j] += F.thetaNode[a] * E.grad[a][j];
      for (int i = 0; i < E.nDim; ++i) gM[i][j] += F.wMom[a][i] * E.grad[a][j];
    }
  }
  F.Flux(gT, gM, F.flux);
  return F;
}

void Perturb(int nDim, pve::NodeState& S, int jVar, su2double h) {
  su2double U[5] = {S.rho, 0.0, 0.0, 0.0, 0.0}, vel2 = 0.0;
  for (int i = 0; i < nDim; ++i) {
    U[i + 1] = S.rho * S.vel[i];
    vel2 += S.vel[i] * S.vel[i];
  }
  U[nDim + 1] = S.p / (kGamma - 1.0) + 0.5 * S.rho * vel2;
  U[jVar] += h;
  S.rho = U[0];
  vel2 = 0.0;
  for (int i = 0; i < nDim; ++i) {
    S.vel[i] = U[i + 1] / U[0];
    vel2 += S.vel[i] * S.vel[i];
  }
  S.p = (kGamma - 1.0) * (U[nDim + 1] - 0.5 * S.rho * vel2);
}

}  // namespace

TEST_CASE("VISC_P1_ENTROPY element: entropy dissipation, P1_CELL limit, Picard Jacobian", "[P1Flux]") {
  std::mt19937 gen(4321);
  for (int nDim : {2, 3}) {
    const int nNodes = nDim + 1, nVar = nDim + 2;
    for (int trial = 0; trial < 200; ++trial) {
      const double stretch = (trial % 2) ? 1.0 : 1e-3;
      su2double x[4][3];
      pve::NodeState state[4];
      RandomElement(gen, nDim, stretch, x, state);
      pve::Element E;
      REQUIRE(E.Set(nDim, x, state, kGas) > 0.0);
      su2double res[4][5];
      E.Residual(res);

      /*--- sum_a w_a . R_a = |T| G:K G >= 0, and the residual is conservative with no mass row. ---*/
      su2double rate = 0.0, scale = 0.0, sum[5] = {};
      for (int a = 0; a < nNodes; ++a) {
        su2double w[5];
        FullEntropyVariables(nDim, state[a], w);
        for (int v = 0; v < nVar; ++v) {
          rate += w[v] * res[a][v];
          scale += fabs(w[v] * res[a][v]);
          sum[v] += res[a][v];
        }
        CHECK(res[a][0] == 0.0);
      }
      CHECK(rate >= -1e-13 * scale);
      su2double maxRes = 0.0;
      for (int a = 0; a < nNodes; ++a)
        for (int v = 0; v < nVar; ++v) maxRes = std::max(maxRes, fabs(res[a][v]));
      for (int v = 0; v < nVar; ++v) CHECK(fabs(sum[v]) <= 1e-12 * maxRes);

      /*--- Constant temperature and linear velocity: the primitive P1 cell residual (old P1_CELL). ---*/
      pve::NodeState iso[4];
      for (int a = 0; a < nNodes; ++a) {
        iso[a] = state[a];
        iso[a].p = iso[a].rho * kGas * 1.3;
      }
      pve::Element Eiso;
      Eiso.Set(nDim, x, iso, kGas);
      su2double resIso[4][5], gradVel[3][3] = {}, tau[3][3] = {}, meanVel[3] = {}, mu = 0.0;
      Eiso.Residual(resIso);
      for (int a = 0; a < nNodes; ++a) {
        mu += iso[a].mu / nNodes;
        for (int i = 0; i < nDim; ++i) {
          meanVel[i] += iso[a].vel[i] / nNodes;
          for (int j = 0; j < nDim; ++j) gradVel[i][j] += iso[a].vel[i] * Eiso.grad[a][j];
        }
      }
      CNumerics::ComputeStressTensor(nDim, tau, gradVel, mu);
      su2double err = 0.0, ref = 0.0;
      for (int a = 0; a < nNodes; ++a) {
        su2double energy = 0.0;
        for (int i = 0; i < nDim; ++i) {
          su2double mom = 0.0;
          for (int j = 0; j < nDim; ++j) {
            mom += Eiso.size * tau[i][j] * Eiso.grad[a][j];
            energy += Eiso.size * meanVel[j] * tau[j][i] * Eiso.grad[a][i];
          }
          err = std::max(err, fabs(mom - resIso[a][i + 1]));
          ref = std::max(ref, fabs(mom));
        }
        err = std::max(err, fabs(energy - resIso[a][nDim + 1]));
      }
      CHECK(err <= 1e-10 * ref);

      /*--- Picard Jacobian against central differences of the residual with K frozen. ---*/
      su2double dwdu[4][5][5], jac[4][4][5][5], jacMax = 0.0, jacErr = 0.0;
      for (int a = 0; a < nNodes; ++a) pve::EntropyVariableJacobian(nDim, kGamma, state[a], dwdu[a]);
      E.Jacobian(dwdu, jac);
      for (int b = 0; b < nNodes; ++b) {
        for (int j = 0; j < nVar; ++j) {
          const su2double h = 1e-6;
          pve::NodeState plus[4], minus[4];
          for (int a = 0; a < nNodes; ++a) plus[a] = minus[a] = state[a];
          Perturb(nDim, plus[b], j, h);
          Perturb(nDim, minus[b], j, -h);
          su2double rp[4][5], rm[4][5];
          Frozen(E, plus).Residual(rp);
          Frozen(E, minus).Residual(rm);
          for (int a = 0; a < nNodes; ++a) {
            for (int v = 0; v < nVar; ++v) {
              jacErr = std::max(jacErr, fabs((rp[a][v] - rm[a][v]) / (2 * h) - jac[a][b][v][j]));
              jacMax = std::max(jacMax, fabs(jac[a][b][v][j]));
            }
          }
        }
      }
      CHECK(jacErr <= 1e-6 * jacMax);
    }
  }
}

TEST_CASE("BC_WALL_ENTROPY_P1 on one wall element: entropy bound, rest state, Jacobian", "[P1Flux]") {
  std::mt19937 gen(8765);
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  for (int nDim : {2, 3}) {
    const int nNodes = nDim + 1, nVar = nDim + 2;
    const unsigned short faceNodes[3] = {1, 2, 3};
    for (int trial = 0; trial < 200; ++trial) {
      const bool isothermal = (trial % 2 == 1);
      const su2double sipg = (trial % 4 < 2) ? 1.0 : 0.0;
      su2double x[4][3];
      pve::NodeState state[4];
      RandomElement(gen, nDim, (trial % 3) ? 1.0 : 1e-3, x, state);
      pve::Element E;
      REQUIRE(E.Set(nDim, x, state, kGas) > 0.0);

      /*--- Outward normal of the face opposite node 0. ---*/
      su2double N[3] = {};
      if (nDim == 2) {
        N[0] = x[2][1] - x[1][1];
        N[1] = -(x[2][0] - x[1][0]);
      } else {
        const su2double a[3] = {x[2][0] - x[1][0], x[2][1] - x[1][1], x[2][2] - x[1][2]};
        const su2double b[3] = {x[3][0] - x[1][0], x[3][1] - x[1][1], x[3][2] - x[1][2]};
        N[0] = 0.5 * (a[1] * b[2] - a[2] * b[1]);
        N[1] = 0.5 * (a[2] * b[0] - a[0] * b[2]);
        N[2] = 0.5 * (a[0] * b[1] - a[1] * b[0]);
      }
      su2double orient = 0.0;
      for (int i = 0; i < nDim; ++i) orient += N[i] * (x[1][i] - x[0][i]);
      if (orient < 0.0) for (int i = 0; i < nDim; ++i) N[i] = -N[i];

      const su2double wallT = exp(u(gen)), wallTheta = 1.0 / (kGas * wallT);

      /*--- Theorem N6 on one element (m_T = 1): sum_a (w_a - g).(R_a + b_a) >= 0 for c >= 1. ---*/
      su2double res[4][5], wall[4][5];
      E.Residual(res);
      pve::NoSlipWall(E, nDim, faceNodes, N, isothermal, wallTheta, 1, sipg, 2.5, wall);
      su2double rate = 0.0, scale = 0.0, resScale = 0.0;
      for (int a = 0; a < nNodes; ++a) {
        su2double w[5];
        FullEntropyVariables(nDim, state[a], w);
        if (isothermal) w[nDim + 1] += wallTheta;
        for (int v = 0; v < nVar; ++v) {
          rate += w[v] * (res[a][v] + wall[a][v]);
          scale += fabs(w[v] * (res[a][v] + wall[a][v]));
          resScale = std::max(resScale, fabs(res[a][v]) + fabs(wall[a][v]));
        }
      }
      CHECK(rate >= -1e-12 * scale);

      /*--- Rest state at the wall temperature: no residual. ---*/
      pve::NodeState rest[4];
      for (int a = 0; a < nNodes; ++a) {
        rest[a] = state[a];
        for (int i = 0; i < 3; ++i) rest[a].vel[i] = 0.0;
        rest[a].p = rest[a].rho * kGas * wallT;
      }
      pve::Element Erest;
      Erest.Set(nDim, x, rest, kGas);
      su2double resRest[4][5], wallRest[4][5];
      Erest.Residual(resRest);
      pve::NoSlipWall(Erest, nDim, faceNodes, N, true, wallTheta, 1, sipg, 2.5, wallRest);
      for (int a = 0; a < nNodes; ++a)
        for (int v = 0; v < nVar; ++v) CHECK(fabs(resRest[a][v] + wallRest[a][v]) < 1e-9 * resScale);

      /*--- Picard Jacobian (K and sigma frozen) against central differences. ---*/
      su2double dwdu[4][5][5], jac[4][4][5][5], jacMax = 0.0, jacErr = 0.0;
      for (int a = 0; a < nNodes; ++a) pve::EntropyVariableJacobian(nDim, kGamma, state[a], dwdu[a]);
      pve::NoSlipWall(E, nDim, faceNodes, N, isothermal, wallTheta, 1, sipg, 2.5, wall, dwdu, jac);
      for (int b = 0; b < nNodes; ++b) {
        for (int j = 0; j < nVar; ++j) {
          const su2double h = 1e-6;
          pve::NodeState plus[4], minus[4];
          for (int a = 0; a < nNodes; ++a) plus[a] = minus[a] = state[a];
          Perturb(nDim, plus[b], j, h);
          Perturb(nDim, minus[b], j, -h);
          su2double rp[4][5], rm[4][5];
          pve::NoSlipWall(Frozen(E, plus), nDim, faceNodes, N, isothermal, wallTheta, 1, sipg, 2.5, rp);
          pve::NoSlipWall(Frozen(E, minus), nDim, faceNodes, N, isothermal, wallTheta, 1, sipg, 2.5, rm);
          for (int a = 0; a < nNodes; ++a) {
            for (int v = 0; v < nVar; ++v) {
              jacErr = std::max(jacErr, fabs((rp[a][v] - rm[a][v]) / (2 * h) - jac[a][b][v][j]));
              jacMax = std::max(jacMax, fabs(jac[a][b][v][j]));
            }
          }
        }
      }
      CHECK(jacErr <= 1e-6 * jacMax);
    }
  }
}

namespace {

/*--- MOD_CENTROIDS face points of a tetrahedron written out independently of CPrimalGrid::getFaceCG:
 * p_f = sum_e w_e m_e / sum_e w_e over the three edges of the face, w_e = (L_e/L_max)^p (P1_VISCOUS_MOVED.md, 1.1). ---*/
struct ModCentroidTet {
  static constexpr unsigned short faces[4][3] = {{0, 2, 1}, {0, 1, 3}, {0, 3, 2}, {1, 2, 3}};
  double exponent = 2.0;
  unsigned short GetFaces(unsigned short iFace, unsigned short k) const { return faces[iFace][k]; }
  template <class Coords>
  void getFaceCG(unsigned short iFace, unsigned short, const Coords& x, su2double* cg) const {
    su2double L[3], mid[3][3], Lmax = 0.0;
    for (int e = 0; e < 3; ++e) {
      const auto a = faces[iFace][e], b = faces[iFace][(e + 1) % 3];
      su2double L2 = 0.0;
      for (int i = 0; i < 3; ++i) {
        mid[e][i] = 0.5 * (x[a][i] + x[b][i]);
        L2 += (x[b][i] - x[a][i]) * (x[b][i] - x[a][i]);
      }
      L[e] = sqrt(L2);
      Lmax = std::max(Lmax, L[e]);
    }
    su2double num[3] = {}, den = 0.0;
    for (int e = 0; e < 3; ++e) {
      const su2double w = pow(L[e] / Lmax, exponent);
      den += w;
      for (int i = 0; i < 3; ++i) num[i] += w * mid[e][i];
    }
    for (int i = 0; i < 3; ++i) cg[i] = num[i] / den;
  }
};
constexpr unsigned short ModCentroidTet::faces[4][3];

/*--- Y_T (row major) and the face shifts of a tetrahedron with the MOD_CENTROIDS points above. ---*/
void MovedGeometry(const su2double x[4][3], su2double Y[9], su2double xi[4][3]) {
  ModCentroidTet tet;
  su2double grad[4][3], Ym[3][3];
  const su2double size = fabs(P1Simplex::ShapeGradients(3, x, grad));
  P1Simplex::FaceShifts(tet, x, xi);
  P1Simplex::MovedFaceTensor(size, grad, xi, Ym);
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j) Y[3 * i + j] = Ym[i][j];
}

su2double TetVolume(const su2double* a, const su2double* b, const su2double* c, const su2double* d) {
  su2double u[3], v[3], w[3], n[3];
  for (int i = 0; i < 3; ++i) {
    u[i] = b[i] - a[i];
    v[i] = c[i] - a[i];
    w[i] = d[i] - a[i];
  }
  GeometryToolbox::CrossProduct(u, v, n);
  return GeometryToolbox::DotProduct(3, n, w) / 6.0;
}

/*--- Nodal data of a linear velocity field v = v0 + L x at constant temperature (L = rotation + dilatation + eps strain
 * is close to the kernel of K, where the moved form is negative, Theorem MV3). ---*/
void NearKernelStates(std::mt19937& gen, const su2double x[4][3], pve::NodeState* state) {
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  su2double L[3][3], S[3][3], v0[3];
  const su2double dil = u(gen), eps = pow(10.0, -3.0 + 2.0 * u(gen));
  for (int i = 0; i < 3; ++i) {
    v0[i] = u(gen);
    for (int j = 0; j < 3; ++j) S[i][j] = u(gen);
  }
  for (int i = 0; i < 3; ++i) {
    for (int j = 0; j < 3; ++j) L[i][j] = (i == j) ? dil : 0.0;
  }
  const su2double w[3] = {u(gen), u(gen), u(gen)};
  L[0][1] -= w[2]; L[1][0] += w[2]; L[0][2] += w[1]; L[2][0] -= w[1]; L[1][2] -= w[0]; L[2][1] += w[0];
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j) L[i][j] += eps * (S[i][j] + S[j][i]);
  const su2double mu = exp(2.0 * u(gen) - 1.0), kappa = exp(2.0 * u(gen));
  for (int a = 0; a < 4; ++a) {
    state[a].rho = 1.0;
    state[a].p = kGas;
    for (int i = 0; i < 3; ++i) {
      state[a].vel[i] = v0[i];
      for (int j = 0; j < 3; ++j) state[a].vel[i] += L[i][j] * x[a][j];
    }
    state[a].mu = mu;
    state[a].kappa = kappa;
  }
}

}  // namespace

TEST_CASE("VISC_P1_ENTROPY_MOVED: moved test vectors are the dual facets, wall shares", "[P1Flux]") {
  std::mt19937 gen(2468);
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  for (int trial = 0; trial < 400; ++trial) {
    su2double x[4][3];
    pve::NodeState state[4];
    RandomElement(gen, 3, pow(10.0, -3.0 * (trial % 4) / 3.0), x, state);
    pve::Element E;
    REQUIRE(E.Set(3, x, state, kGas) > 0.0);
    su2double Y[9], xi[4][3];
    MovedGeometry(x, Y, xi);
    E.SetMovedTest(Y, 1.0);

    /*--- Explicit dual facets (m_ab, p_o, c) with an arbitrary cell point c, oriented as on the median dual:
     * nu_a = -(outward facet areas of C_a in T), Lemma MV1(b, c). ---*/
    ModCentroidTet tet;
    su2double point[4][3], centroid[4][3], cell[3], cellMedian[3] = {}, nu[4][3] = {};
    for (int iFace = 0; iFace < 4; ++iFace) {
      int opposite = 6;
      for (int k = 0; k < 3; ++k) opposite -= tet.GetFaces(iFace, k);
      tet.getFaceCG(iFace, 3, x, point[opposite]);
      for (int i = 0; i < 3; ++i) {
        centroid[opposite][i] = 0.0;
        for (int k = 0; k < 3; ++k) centroid[opposite][i] += x[tet.GetFaces(iFace, k)][i] / 3.0;
      }
    }
    for (int a = 0; a < 4; ++a)
      for (int i = 0; i < 3; ++i) cellMedian[i] += x[a][i] / 4.0;
    for (int i = 0; i < 3; ++i) cell[i] = cellMedian[i] + 0.1 * u(gen) * (x[1][i] - x[0][i]);
    for (int a = 0; a < 4; ++a) {
      for (int b = a + 1; b < 4; ++b) {
        su2double m[3];
        for (int i = 0; i < 3; ++i) m[i] = 0.5 * (x[a][i] + x[b][i]);
        for (int o = 0; o < 4; ++o) {
          if (o == a || o == b) continue;
          const su2double sign = (TetVolume(x[a], m, centroid[o], cellMedian) > 0.0) ? 1.0 : -1.0;
          su2double r[3], s[3], n[3];
          for (int i = 0; i < 3; ++i) {
            r[i] = point[o][i] - m[i];
            s[i] = cell[i] - m[i];
          }
          GeometryToolbox::CrossProduct(r, s, n);
          for (int i = 0; i < 3; ++i) {
            nu[a][i] -= 0.5 * sign * n[i];
            nu[b][i] += 0.5 * sign * n[i];
          }
        }
      }
    }
    su2double scale = 0.0, err = 0.0, errShares = 0.0;
    for (int a = 0; a < 4; ++a)
      for (int i = 0; i < 3; ++i) scale = std::max(scale, fabs(E.size * E.grad[a][i]));
    for (int a = 0; a < 4; ++a)
      for (int i = 0; i < 3; ++i) err = std::max(err, fabs(nu[a][i] - E.test[a][i]));
    CHECK(err <= 1e-9 * scale);

    /*--- tr Y = 0, and nu_a = sum_{o != a} (a_a/|f_o|) N_o with the moved wall shares, N_o = -3|T| g_o (MV1(a)). ---*/
    CHECK(fabs(Y[0] + Y[4] + Y[8]) <= 1e-12 * E.size);
    su2double frac[4][4], nuShares[4][3] = {};
    for (int o = 0; o < 4; ++o) {
      P1Simplex::MovedFaceShares(E.grad, o, xi[o], frac[o]);
      su2double sum = 0.0;
      for (int a = 0; a < 4; ++a) {
        if (a == o) continue;
        sum += frac[o][a];
        CHECK(frac[o][a] >= 0.25 - 1e-12);
        CHECK(frac[o][a] <= 0.5 + 1e-12);
        for (int i = 0; i < 3; ++i) nuShares[a][i] += frac[o][a] * (-3.0 * E.size * E.grad[o][i]);
      }
      CHECK(fabs(sum - 1.0) < 1e-12);

      /*--- beta_a = (w_ab + w_ac)/(2 sum w) of the face weights; with p = 2, w_e is proportional to L_e^2. The face
       * opposite node o is face 3 - o of ModCentroidTet. ---*/
      const int iFace = 3 - o;
      su2double w[3], sumW = 0.0;
      for (int e = 0; e < 3; ++e) {
        w[e] = GeometryToolbox::SquaredDistance(3, x[tet.GetFaces(iFace, e)], x[tet.GetFaces(iFace, (e + 1) % 3)]);
        sumW += w[e];
      }
      for (int k = 0; k < 3; ++k) {
        const auto a = tet.GetFaces(iFace, k);
        /*--- Node k of the face is in edges k-1 and k. ---*/
        const su2double beta = 0.5 * (w[k] + w[(k + 2) % 3]) / sumW;
        errShares = std::max(errShares, fabs(frac[o][a] - 0.5 * (1.0 - beta)));
      }
    }
    CHECK(errShares < 1e-10);
    su2double errNu = 0.0;
    for (int a = 0; a < 4; ++a)
      for (int i = 0; i < 3; ++i) errNu = std::max(errNu, fabs(nuShares[a][i] - E.test[a][i]));
    CHECK(errNu <= 1e-9 * scale);
  }
}

TEST_CASE("VISC_P1_ENTROPY_MOVED element: production identity, conservation, Jacobian", "[P1Flux]") {
  std::mt19937 gen(1357);
  std::uniform_real_distribution<double> u(0.0, 1.0);
  for (int trial = 0; trial < 200; ++trial) {
    su2double x[4][3];
    pve::NodeState state[4];
    RandomElement(gen, 3, (trial % 2) ? 1.0 : 1e-3, x, state);
    if (trial % 3 == 0) NearKernelStates(gen, x, state);
    pve::Element E;
    REQUIRE(E.Set(3, x, state, kGas) > 0.0);
    su2double Y[9], xi[4][3];
    MovedGeometry(x, Y, xi);
    const su2double keep = (trial % 5 == 0) ? 1.0 : u(gen);
    const su2double Dgal = E.GalerkinDissipation(), Pv = E.MovedProduction(Y);
    CHECK(Dgal >= 0.0);
    E.SetMovedTest(Y, keep);

    /*--- sum_a w_a . R_a = D^gal - keep P^v (Lemma MV2), sum_a R_a = 0, no mass row. ---*/
    su2double res[4][5];
    E.Residual(res);
    su2double rate = 0.0, scale = 0.0, sum[5] = {}, maxRes = 0.0;
    for (int a = 0; a < 4; ++a) {
      su2double w[5];
      FullEntropyVariables(3, state[a], w);
      for (int v = 0; v < 5; ++v) {
        rate += w[v] * res[a][v];
        scale += fabs(w[v] * res[a][v]);
        sum[v] += res[a][v];
        maxRes = std::max(maxRes, fabs(res[a][v]));
      }
      CHECK(res[a][0] == 0.0);
    }
    CHECK(fabs(rate - (Dgal - keep * Pv)) <= 1e-11 * (scale + Dgal + fabs(Pv)));
    for (int v = 0; v < 5; ++v) CHECK(fabs(sum[v]) <= 1e-12 * maxRes);

    /*--- Picard Jacobian (K and theta_v frozen, not symmetric) against central differences. ---*/
    su2double dwdu[4][5][5], jac[4][4][5][5], jacMax = 0.0, jacErr = 0.0;
    for (int a = 0; a < 4; ++a) pve::EntropyVariableJacobian(3, kGamma, state[a], dwdu[a]);
    E.Jacobian(dwdu, jac);
    for (int b = 0; b < 4; ++b) {
      for (int j = 0; j < 5; ++j) {
        const su2double h = 1e-6;
        pve::NodeState plus[4], minus[4];
        for (int a = 0; a < 4; ++a) plus[a] = minus[a] = state[a];
        Perturb(3, plus[b], j, h);
        Perturb(3, minus[b], j, -h);
        su2double rp[4][5], rm[4][5];
        Frozen(E, plus).Residual(rp);
        Frozen(E, minus).Residual(rm);
        for (int a = 0; a < 4; ++a) {
          for (int v = 0; v < 5; ++v) {
            jacErr = std::max(jacErr, fabs((rp[a][v] - rm[a][v]) / (2 * h) - jac[a][b][v][j]));
            jacMax = std::max(jacMax, fabs(jac[a][b][v][j]));
          }
        }
      }
    }
    CHECK(jacErr <= 1e-6 * jacMax);
  }
}

TEST_CASE("VISC_P1_ENTROPY_MOVED one-budget blend: theta in [0,1], element production min(E,0)", "[P1Flux]") {
  std::mt19937 gen(9753);
  std::uniform_real_distribution<double> u(0.0, 1.0);
  int nFiredV = 0, nFiredC = 0;
  for (int trial = 0; trial < 4000; ++trial) {
    /*--- P^c <= D^c (Lemma 3); P^v, D^gal from real elements (near-kernel states every other trial, where the moved
     * form is negative), or synthetic; W_T <= D^gal/c (MV12). ---*/
    su2double Pv, Dgal;
    if (trial % 2 == 0) {
      su2double x[4][3], Y[9], xi[4][3];
      pve::NodeState state[4];
      RandomElement(gen, 3, (trial % 4) ? 1.0 : 1e-3, x, state);
      if (trial % 4 == 0) NearKernelStates(gen, x, state);
      pve::Element E;
      E.Set(3, x, state, kGas);
      MovedGeometry(x, Y, xi);
      Pv = E.MovedProduction(Y);
      Dgal = E.GalerkinDissipation();
    } else {
      Dgal = u(gen);
      Pv = 3.0 * (2.0 * u(gen) - 1.0) * Dgal;
    }
    const su2double scale = Dgal + fabs(Pv) + 1e-300;
    const su2double Dc = (trial % 7 == 0) ? 0.0 : scale * pow(10.0, 2.0 * u(gen) - 1.0);
    const su2double Pc = (2.0 * u(gen) - 1.0) * Dc;
    const su2double W = (trial % 3 == 0) ? (u(gen) * 1.4 - 1.0) * Dgal / pve::NOSLIP_PENALTY : 0.0;

    su2double thetaV = -1.0, thetaC = -1.0;
    pve::OneBudgetBlend(Pc, Dc, Pv, Dgal - W, thetaV, thetaC);
    CHECK(thetaV >= 0.0);
    CHECK(thetaV <= 1.0);
    CHECK(thetaC >= 0.0);
    CHECK(thetaC <= 1.0);
    nFiredV += (thetaV > 0.0);
    nFiredC += (thetaC > 0.0);

    const su2double excess = Pc + Pv - Dgal + W;
    const su2double net = Pc - thetaC * Dc + (1.0 - thetaV) * Pv - Dgal + W;
    CHECK(fabs(net - std::min(excess, su2double(0.0))) <= 1e-12 * (scale + Dc));

    /*--- Lemma MV7: never more convective floor than the B2 blend. ---*/
    if (Dc > 0.0) CHECK(thetaC <= std::max(Pc, su2double(0.0)) / Dc * (1.0 + 1e-12) + 1e-15);
  }
  CHECK(nFiredV > 0);
  CHECK(nFiredC > 0);
}

TEST_CASE("VISC_P1_ENTROPY_MOVED weak wall: moved shares, wall production W_T, Jacobian", "[P1Flux]") {
  std::mt19937 gen(3141);
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  const unsigned short faceNodes[3] = {1, 2, 3};
  for (int trial = 0; trial < 300; ++trial) {
    const bool isothermal = (trial % 2 == 1);
    su2double x[4][3];
    pve::NodeState state[4];
    RandomElement(gen, 3, (trial % 3) ? 1.0 : 1e-3, x, state);
    pve::Element E;
    REQUIRE(E.Set(3, x, state, kGas) > 0.0);
    su2double Y[9], xi[4][3], frac[4];
    MovedGeometry(x, Y, xi);
    P1Simplex::MovedFaceShares(E.grad, 0, xi[0], frac);
    const su2double share[3] = {frac[1], frac[2], frac[3]};
    const su2double keep = 0.5 * (1.0 + u(gen));

    su2double N[3];
    pve::WallFaceNormal(3, x, 0, faceNodes, N);
    su2double orient = 0.0;
    for (int i = 0; i < 3; ++i) orient += N[i] * (x[1][i] - x[0][i]);
    CHECK(orient > 0.0);
    const su2double wallTheta = 1.0 / (kGas * exp(u(gen)));

    /*--- -sum_c (w_c - g) . b_c = W_f exactly (the g terms vanish with w - g), and W_f <= D^gal / c (MV12, MV13). ---*/
    su2double wall[4][5], res[4][5];
    pve::NoSlipWall(E, 3, faceNodes, N, isothermal, wallTheta, 1, pve::NOSLIP_SIPG, pve::NOSLIP_PENALTY, wall, nullptr,
                    nullptr, share);
    const su2double W = pve::NoSlipWallProduction(E, 3, faceNodes, N, isothermal, wallTheta, 1, pve::NOSLIP_SIPG,
                                                  pve::NOSLIP_PENALTY, share);
    const su2double Dgal = E.GalerkinDissipation(), Pv = E.MovedProduction(Y);
    E.SetMovedTest(Y, keep);
    E.Residual(res);
    su2double rateWall = 0.0, rate = 0.0, scale = 0.0;
    for (int a = 0; a < 4; ++a) {
      su2double w[5];
      FullEntropyVariables(3, state[a], w);
      if (isothermal) w[4] += wallTheta;
      for (int v = 0; v < 5; ++v) {
        rateWall -= w[v] * wall[a][v];
        rate -= w[v] * (res[a][v] + wall[a][v]);
        scale += fabs(w[v] * wall[a][v]) + fabs(w[v] * res[a][v]);
      }
    }
    CHECK(fabs(rateWall - W) <= 1e-11 * scale);
    CHECK(W <= Dgal / pve::NOSLIP_PENALTY + 1e-11 * scale);
    CHECK(fabs(rate - (-Dgal + keep * Pv + W)) <= 1e-11 * (scale + Dgal + fabs(Pv)));

    /*--- Lumped shares 1/3 reproduce the default closure. ---*/
    const su2double third[3] = {1.0 / 3.0, 1.0 / 3.0, 1.0 / 3.0};
    su2double wallLumped[4][5], wallThird[4][5], maxWall = 0.0, errLumped = 0.0;
    pve::NoSlipWall(E, 3, faceNodes, N, isothermal, wallTheta, 1, pve::NOSLIP_SIPG, pve::NOSLIP_PENALTY, wallLumped);
    pve::NoSlipWall(E, 3, faceNodes, N, isothermal, wallTheta, 1, pve::NOSLIP_SIPG, pve::NOSLIP_PENALTY, wallThird,
                    nullptr, nullptr, third);
    for (int a = 0; a < 4; ++a) {
      for (int v = 0; v < 5; ++v) {
        maxWall = std::max(maxWall, fabs(wallLumped[a][v]));
        errLumped = std::max(errLumped, fabs(wallLumped[a][v] - wallThird[a][v]));
      }
    }
    CHECK(errLumped <= 1e-14 * maxWall);

    /*--- Picard Jacobian of the closure with the moved shares against central differences. ---*/
    su2double dwdu[4][5][5], jac[4][4][5][5], jacMax = 0.0, jacErr = 0.0;
    for (int a = 0; a < 4; ++a) pve::EntropyVariableJacobian(3, kGamma, state[a], dwdu[a]);
    pve::NoSlipWall(E, 3, faceNodes, N, isothermal, wallTheta, 1, pve::NOSLIP_SIPG, pve::NOSLIP_PENALTY, wall, dwdu,
                    jac, share);
    for (int b = 0; b < 4; ++b) {
      for (int j = 0; j < 5; ++j) {
        const su2double h = 1e-6;
        pve::NodeState plus[4], minus[4];
        for (int a = 0; a < 4; ++a) plus[a] = minus[a] = state[a];
        Perturb(3, plus[b], j, h);
        Perturb(3, minus[b], j, -h);
        su2double rp[4][5], rm[4][5];
        pve::NoSlipWall(Frozen(E, plus), 3, faceNodes, N, isothermal, wallTheta, 1, pve::NOSLIP_SIPG,
                        pve::NOSLIP_PENALTY, rp, nullptr, nullptr, share);
        pve::NoSlipWall(Frozen(E, minus), 3, faceNodes, N, isothermal, wallTheta, 1, pve::NOSLIP_SIPG,
                        pve::NOSLIP_PENALTY, rm, nullptr, nullptr, share);
        for (int a = 0; a < 4; ++a) {
          for (int v = 0; v < 5; ++v) {
            jacErr = std::max(jacErr, fabs((rp[a][v] - rm[a][v]) / (2 * h) - jac[a][b][v][j]));
            jacMax = std::max(jacMax, fabs(jac[a][b][v][j]));
          }
        }
      }
    }
    CHECK(jacErr <= 1e-6 * jacMax);
  }
}

TEST_CASE("VISC_P1_ENTROPY_MOVED off and Y = 0: the Galerkin operator", "[P1Flux]") {
  std::mt19937 gen(8642);
  std::uniform_real_distribution<double> u(0.0, 1.0);
  for (int nDim : {2, 3}) {
    const int nNodes = nDim + 1, nVar = nDim + 2;
    for (int trial = 0; trial < 50; ++trial) {
      su2double x[4][3];
      pve::NodeState state[4];
      RandomElement(gen, nDim, (trial % 2) ? 1.0 : 1e-3, x, state);
      pve::Element E;
      REQUIRE(E.Set(nDim, x, state, kGas) > 0.0);

      /*--- Off: R_a = |T| F_T . g_a exactly as before. ---*/
      su2double res[4][5], resMoved[4][5], maxRes = 0.0, err = 0.0;
      E.Residual(res);
      for (int a = 0; a < nNodes; ++a) {
        for (int v = 0; v < nVar; ++v) {
          su2double sum = 0.0;
          for (int j = 0; j < nDim; ++j) sum += E.flux[v][j] * E.grad[a][j];
          err = std::max(err, fabs(res[a][v] - E.size * sum));
          maxRes = std::max(maxRes, fabs(res[a][v]));
        }
      }

      CHECK(err <= 1e-15 * maxRes);
      err = 0.0;

      /*--- Y = 0 (median dual, 2D, equilateral faces): any theta_v gives the Galerkin residual. ---*/
      const su2double zero[9] = {};
      pve::Element Ezero = E;
      Ezero.SetMovedTest(zero, u(gen));
      Ezero.Residual(resMoved);
      for (int a = 0; a < nNodes; ++a)
        for (int v = 0; v < nVar; ++v) err = std::max(err, fabs(resMoved[a][v] - res[a][v]));
      CHECK(err <= 1e-14 * maxRes);
      CHECK(E.MovedProduction(zero) == 0.0);

      /*--- Lumped default shares in 2D and 3D. ---*/
      const unsigned short faceNodes[3] = {1, 2, 3};
      su2double N[3];
      pve::WallFaceNormal(nDim, x, 0, faceNodes, N);
      const su2double lumped[3] = {1.0 / nDim, 1.0 / nDim, 1.0 / nDim};
      su2double wallA[4][5], wallB[4][5], maxWall = 0.0, errWall = 0.0;
      pve::NoSlipWall(E, nDim, faceNodes, N, false, 1.0, 1, 1.0, 2.5, wallA);
      pve::NoSlipWall(E, nDim, faceNodes, N, false, 1.0, 1, 1.0, 2.5, wallB, nullptr, nullptr, lumped);
      for (int a = 0; a < nNodes; ++a) {
        for (int v = 0; v < nVar; ++v) {
          maxWall = std::max(maxWall, fabs(wallA[a][v]));
          errWall = std::max(errWall, fabs(wallA[a][v] - wallB[a][v]));
        }
      }
      CHECK(errWall <= 1e-14 * maxWall);
    }
  }

  /*--- Regular tetrahedron: all faces equilateral, the face points are the centroids, Y = 0 (MV1(d)). ---*/
  const su2double reg[4][3] = {{1, 1, 1}, {1, -1, -1}, {-1, 1, -1}, {-1, -1, 1}};
  su2double Y[9], xi[4][3];
  MovedGeometry(reg, Y, xi);
  for (int k = 0; k < 9; ++k) CHECK(fabs(Y[k]) < 1e-14);
}

TEST_CASE("VISC_P1_ENTROPY_MOVED: face tensor of the moved dual on a mesh, constant flux exactness", "[P1Flux]") {
  /*--- EDGE_FORM_P1 needs MOD_CENTROIDS (refused otherwise), so only the moved dual is tested on a mesh; Y = 0 for
   * centroid face points is covered by the regular tetrahedron in the "off and Y = 0" test. ---*/
  for (bool modCentroids : {true}) {
    for (double stretch : {1.0, 1e-3}) {
      const std::string mesh = "p1_visc_moved_3d.su2";
      WriteSimplexMesh(mesh, 3, 5, 0.4, stretch);
      MeshCase test(mesh, std::string(modCentroids ? "MOD_CENTROIDS= YES\n" : "") +
                              "CONV_NUM_METHOD_FLOW= JST\nEDGE_FORM_P1= YES\n");
      auto& geometry = *test.geometry;
      geometry.SetEdgeFormBlend(true);
      const auto& blendY = geometry.edges->GetBlendY();

      /*--- Stored Y_T against the independent MOD_CENTROIDS points, and the assembled moved test vectors: zero at
       * interior nodes (constant fluxes are exact, MV1(e)). ---*/
      std::vector<std::array<su2double, 3>> S(geometry.GetnPoint(), {0.0, 0.0, 0.0});
      su2double scale = 0.0, errY = 0.0, maxY = 0.0;
      for (auto iElem = 0ul; iElem < geometry.GetnElem(); ++iElem) {
        const auto* element = geometry.elem[iElem];
        su2double x[4][3], center[3] = {}, grad[4][3];
        for (int a = 0; a < 4; ++a)
          for (int i = 0; i < 3; ++i) center[i] += geometry.nodes->GetCoord(element->GetNode(a), i) / 4.0;
        for (int a = 0; a < 4; ++a)
          for (int i = 0; i < 3; ++i) x[a][i] = geometry.nodes->GetCoord(element->GetNode(a), i) - center[i];
        const su2double size = fabs(P1Simplex::ShapeGradients(3, x, grad));
        su2double Y[9], xi[4][3];
        MovedGeometry(x, Y, xi);
        for (int k = 0; k < 9; ++k) {
          if (modCentroids) errY = std::max(errY, fabs(blendY(iElem, k) - Y[k]) / size);
          maxY = std::max(maxY, fabs(blendY(iElem, k)) / size);
        }
        for (int a = 0; a < 4; ++a) {
          for (int j = 0; j < 3; ++j) {
            su2double nu = size * grad[a][j];
            for (int i = 0; i < 3; ++i) nu += blendY(iElem, 3 * i + j) * grad[a][i];
            S[element->GetNode(a)][j] += nu;
            scale = std::max(scale, fabs(size * grad[a][j]));
          }
        }
      }
      su2double local = 0.0, global = 0.0;
      for (auto iPoint = 0ul; iPoint < geometry.GetnPointDomain(); ++iPoint) {
        if (geometry.nodes->GetPhysicalBoundary(iPoint)) continue;
        for (int j = 0; j < 3; ++j) local = std::max(local, fabs(S[iPoint][j]) / scale);
      }
      SU2_MPI::Allreduce(&local, &global, 1, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
      /*--- The test geometry may sit on one rank only (empty on the others), so the maxima are reduced. ---*/
      su2double maxYAll = 0.0, errYAll = 0.0;
      SU2_MPI::Allreduce(&maxY, &maxYAll, 1, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
      SU2_MPI::Allreduce(&errY, &errYAll, 1, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
      INFO("MOD_CENTROIDS " << modCentroids << " stretch " << stretch);
      CHECK(global < 1e-10);
      CHECK(errYAll < 1e-10);
      if (modCentroids) CHECK(maxYAll > 1e-3);
      else CHECK(maxYAll < 1e-11);
    }
  }
}

/*--- GRID_ENTROPY_P1 (P1_ALE.md): kernels and the semi-discrete ALE entropy balance on moving meshes. The identities
 * that go through the logarithmic mean hold to its series error (u < 1e-2: 1e-9 relative), not to round-off. ---*/

namespace {

namespace ale = P1AleEntropy;

ale::State RandomAleState(std::mt19937& gen, int nDim, double spread = 0.5) {
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  ale::State S;
  S.rho = exp(2.0 * spread * u(gen));
  S.p = exp(2.0 * spread * u(gen));
  for (int i = 0; i < 3; ++i) S.vel[i] = (i < nDim) ? 0.8 * u(gen) : 0.0;
  return S;
}

/*--- Physical flux F(S).N. ---*/
void AleFluxN(int nDim, const ale::State& S, const su2double* N, su2double* F) {
  su2double vn = 0.0, vel2 = 0.0;
  for (int i = 0; i < nDim; ++i) {
    vn += S.vel[i] * N[i];
    vel2 += S.vel[i] * S.vel[i];
  }
  const su2double E = S.p / (kGamma - 1.0) + 0.5 * S.rho * vel2;
  F[0] = S.rho * vn;
  for (int i = 0; i < nDim; ++i) F[i + 1] = S.rho * vn * S.vel[i] + S.p * N[i];
  F[nDim + 1] = (E + S.p) * vn;
}

}  // namespace

TEST_CASE("GRID_ENTROPY_P1: temporal entropy conservative average and relative Lemma 3", "[P1Flux]") {
  std::mt19937 gen(2468);
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  for (int trial = 0; trial < 4000; ++trial) {
    const int nDim = 2 + trial % 2, nVar = nDim + 2;
    const auto a = RandomAleState(gen, nDim, 1.0), b = RandomAleState(gen, nDim, 1.0);
    su2double wa[5], wb[5], ubar[5], uaa[5], Ua[5], Ub[5];
    ale::EntropyVariables(nDim, kGamma, a, wa);
    ale::EntropyVariables(nDim, kGamma, b, wb);
    ale::TemporalAverage(nDim, kGamma, a, b, ubar);

    /*--- [[w]].ubar = [[rho]] (the temporal potential), ubar(U,U) = U. ---*/
    su2double dot = 0.0, scale = 0.0;
    for (int v = 0; v < nVar; ++v) {
      dot += (wb[v] - wa[v]) * ubar[v];
      scale += fabs((wb[v] - wa[v]) * ubar[v]);
    }
    CHECK(fabs(dot - (b.rho - a.rho)) <= 1e-8 * scale);
    ale::TemporalAverage(nDim, kGamma, a, a, uaa);
    ale::Conservative(nDim, kGamma, a, Ua);
    for (int v = 0; v < nVar; ++v) CHECK(fabs(uaa[v] - Ua[v]) <= 1e-14 * (1.0 + fabs(Ua[v])));

    /*--- The vectorized kernel of the edge loop gives the same average. ---*/
    if (nDim == 2) {
      CCompressiblePrimitives<2, 7> Vi, Vj;
      Vi.density() = a.rho; Vi.pressure() = a.p; Vj.density() = b.rho; Vj.pressure() = b.p;
      for (int i = 0; i < 2; ++i) { Vi.velocity(i) = a.vel[i]; Vj.velocity(i) = b.vel[i]; }
      const auto simd = ecTemporalAverage(Double(kGamma), Vi, Vj);
      for (int v = 0; v < nVar; ++v) CHECK(fabs(simd(v)[0] - ubar[v]) <= 1e-13 * (1.0 + fabs(ubar[v])));
    } else {
      CCompressiblePrimitives<3, 8> Vi, Vj;
      Vi.density() = a.rho; Vi.pressure() = a.p; Vj.density() = b.rho; Vj.pressure() = b.p;
      for (int i = 0; i < 3; ++i) { Vi.velocity(i) = a.vel[i]; Vj.velocity(i) = b.vel[i]; }
      const auto simd = ecTemporalAverage(Double(kGamma), Vi, Vj);
      for (int v = 0; v < nVar; ++v) CHECK(fabs(simd(v)[0] - ubar[v]) <= 1e-13 * (1.0 + fabs(ubar[v])));
    }

    /*--- Lemma A12: [[w]].((F_k - F_i).E - (vbar.E)(U_k - U_i)) <= lam_rel [[w]].[[U]],
     * lam_rel = max(0, max(u_i.E, u_k.E) - vbar.E + max(c_i, c_k)|E|), as in centered.hpp and SetEdgeBlendLambda. ---*/
    su2double E[3] = {0.0, 0.0, 0.0}, vbar[3] = {0.0, 0.0, 0.0}, Fa[5], Fb[5];
    for (int i = 0; i < nDim; ++i) {
      E[i] = u(gen);
      vbar[i] = 1.5 * u(gen);
    }
    AleFluxN(nDim, a, E, Fa);
    AleFluxN(nDim, b, E, Fb);
    ale::Conservative(nDim, kGamma, b, Ub);
    su2double areaE = 0.0, projA = 0.0, projB = 0.0, projV = 0.0;
    for (int i = 0; i < nDim; ++i) {
      areaE += E[i] * E[i];
      projA += a.vel[i] * E[i];
      projB += b.vel[i] * E[i];
      projV += vbar[i] * E[i];
    }
    areaE = sqrt(areaE);
    const su2double lam = std::max(su2double(0.0), std::max(projA, projB) - projV +
                                                       sqrt(kGamma * std::max(a.p / a.rho, b.p / b.rho)) * areaE);
    su2double production = 0.0, dissipation = 0.0;
    for (int v = 0; v < nVar; ++v) {
      production += (wb[v] - wa[v]) * ((Fb[v] - Fa[v]) - projV * (Ub[v] - Ua[v]));
      dissipation += (wb[v] - wa[v]) * (Ub[v] - Ua[v]);
    }
    CHECK(production <= lam * dissipation + 1e-12 * fabs(lam * dissipation));
  }
}

TEST_CASE("GRID_ENTROPY_P1: closure C-ALE on one face, moving wall and far field", "[P1Flux]") {
  std::mt19937 gen(1357);
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  for (int trial = 0; trial < 1000; ++trial) {
    const int nDim = 2 + trial % 2, nNodes = nDim, nVar = nDim + 2;
    su2double x[3][3] = {}, N[3] = {};
    for (int k = 0; k < nNodes; ++k)
      for (int i = 0; i < nDim; ++i) x[k][i] = u(gen);
    if (nDim == 2) {
      N[0] = x[1][1] - x[0][1];
      N[1] = -(x[1][0] - x[0][0]);
    } else {
      const su2double a[3] = {x[1][0] - x[0][0], x[1][1] - x[0][1], x[1][2] - x[0][2]};
      const su2double b[3] = {x[2][0] - x[0][0], x[2][1] - x[0][1], x[2][2] - x[0][2]};
      GeometryToolbox::CrossProduct(a, b, N);
      for (int i = 0; i < 3; ++i) N[i] *= 0.5;
    }
    ale::State S[3];
    su2double gridVel[3][3] = {}, w[3][5];
    for (int k = 0; k < nNodes; ++k) {
      S[k] = RandomAleState(gen, nDim);
      for (int i = 0; i < nDim; ++i) gridVel[k][i] = u(gen);
      ale::EntropyVariables(nDim, kGamma, S[k], w[k]);
    }
    const su2double kappa = 0.3 * u(gen);
    su2double phi[3][3];
    const su2double weight = ale::FaceRule(nNodes, phi);

    /*--- beta_i = sum_q phi_i(x_q) N_q and g^b_i = sum_q phi_i(x_q) gamma_iq of the face. ---*/
    su2double beta[3][3] = {}, gridFlux[3] = {};
    for (int q = 0; q < nNodes; ++q) {
      su2double vq[3] = {};
      for (int k = 0; k < nNodes; ++k)
        for (int i = 0; i < nDim; ++i) vq[i] += phi[q][k] * gridVel[k][i];
      for (int a = 0; a < nNodes; ++a) {
        su2double gamma = kappa;
        for (int i = 0; i < nDim; ++i) {
          beta[a][i] += phi[q][a] * weight * N[i];
          gamma += 0.5 * (gridVel[a][i] + vq[i]) * weight * N[i];
        }
        gridFlux[a] += phi[q][a] * gamma;
      }
    }
    auto theta = [&](const su2double (*res)[5], su2double& scale) {
      su2double value = 0.0;
      scale = 0.0;
      for (int a = 0; a < nNodes; ++a) {
        for (int i = 0; i < nDim; ++i) value += S[a].rho * S[a].vel[i] * beta[a][i];
        value -= S[a].rho * gridFlux[a];
        for (int v = 0; v < nVar; ++v) {
          value -= w[a][v] * res[a][v];
          scale += fabs(w[a][v] * res[a][v]);
        }
      }
      return value;
    };

    /*--- Moving pressure wall: Theta = 0 for any state (Theorem A13 ii, Lemma A14), no mass flux. ---*/
    su2double res[3][5], scale = 0.0;
    ale::FaceResidual(nDim, nNodes, kGamma, nullptr, N, S, gridVel, kappa, res);
    {
      const su2double thetaValue = theta(res, scale);
      CHECK(fabs(thetaValue) <= 1e-8 * scale);
    }
    su2double mass = 0.0;
    for (int a = 0; a < nNodes; ++a) mass += res[a][0];
    CHECK(fabs(mass) <= 1e-13 * scale);

    /*--- Free stream: b_i = F(U).beta_i - U g^b_i with the far field at the uniform state. ---*/
    const auto inf = RandomAleState(gen, nDim);
    const ale::State uniform[3] = {inf, inf, inf};
    ale::FaceResidual(nDim, nNodes, kGamma, &inf, N, uniform, gridVel, kappa, res);
    su2double Uinf[5];
    ale::Conservative(nDim, kGamma, inf, Uinf);
    for (int a = 0; a < nNodes; ++a) {
      su2double F[5];
      AleFluxN(nDim, inf, beta[a], F);
      for (int v = 0; v < nVar; ++v)
        CHECK(fabs(res[a][v] - (F[v] - Uinf[v] * gridFlux[a])) <= 1e-12 * (fabs(F[v]) + fabs(Uinf[v] * gridFlux[a]) + 1.0));
    }

    /*--- Far field at another state: Theta <= the relative entropy flux of the ghosts (Lemma A15), conservation. ---*/
    ale::FaceResidual(nDim, nNodes, kGamma, &inf, N, S, gridVel, kappa, res);
    su2double area = 0.0;
    for (int i = 0; i < nDim; ++i) area += N[i] * N[i];
    area = sqrt(area);
    su2double unit[3] = {}, Nq[3] = {}, bound = 0.0, sumFstar[5] = {};
    for (int i = 0; i < nDim; ++i) {
      unit[i] = N[i] / area;
      Nq[i] = weight * N[i];
    }
    for (int q = 0; q < nNodes; ++q) {
      su2double wq[5] = {}, vq[3] = {};
      for (int k = 0; k < nNodes; ++k) {
        for (int v = 0; v < nVar; ++v) wq[v] += phi[q][k] * w[k][v];
        for (int i = 0; i < nDim; ++i) vq[i] += phi[q][k] * gridVel[k][i];
      }
      ale::State sq, ghost;
      ale::StateFromEntropyVariables(nDim, kGamma, wq, sq);
      su2double gq = kappa;
      for (int i = 0; i < nDim; ++i) gq += vq[i] * Nq[i];
      const su2double speed = gq / (weight * area);
      ale::RelativeFarFieldGhost(nDim, kGamma, sq, inf, unit, speed, ghost);
      su2double F[5], Uq[5], Ug[5], wg[5];
      ale::AleFlux(nDim, kGamma, sq, ghost, Nq, gq, F);
      su2double vnq = -speed, vng = -speed;
      for (int i = 0; i < nDim; ++i) {
        vnq += sq.vel[i] * unit[i];
        vng += ghost.vel[i] * unit[i];
      }
      const su2double lam = std::max(fabs(vnq) + sqrt(kGamma * sq.p / sq.rho), fabs(vng) + sqrt(kGamma * ghost.p / ghost.rho));
      ale::Conservative(nDim, kGamma, sq, Uq);
      ale::Conservative(nDim, kGamma, ghost, Ug);
      ale::EntropyVariables(nDim, kGamma, ghost, wg);
      for (int v = 0; v < nVar; ++v) F[v] -= 0.5 * lam * weight * area * (Ug[v] - Uq[v]);
      for (int i = 0; i < nDim; ++i) bound += ghost.rho * ghost.vel[i] * Nq[i];
      bound -= ghost.rho * gq;
      for (int v = 0; v < nVar; ++v) {
        bound -= wg[v] * F[v];
        sumFstar[v] += F[v];
      }
    }
    const su2double th = theta(res, scale);
    CHECK(th <= bound + 1e-8 * scale);
    for (int v = 0; v < nVar; ++v) {
      su2double sum = 0.0;
      for (int a = 0; a < nNodes; ++a) sum += res[a][v];
      CHECK(fabs(sum - sumFstar[v]) <= 1e-12 * (fabs(sumFstar[v]) + scale));
    }
  }
}

TEST_CASE("GRID_ENTROPY_P1: moving weak no-slip wall, projector, entropy bound, rest state, Jacobian", "[P1Flux]") {
  std::mt19937 gen(97531);
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  for (int nDim : {2, 3}) {
    const int nNodes = nDim + 1, nVar = nDim + 2;
    const unsigned short faceNodes[3] = {1, 2, 3};
    for (int trial = 0; trial < 200; ++trial) {
      const bool isothermal = (trial % 2 == 1);
      const su2double sipg = (trial % 4 < 2) ? 1.0 : 0.0;
      su2double x[4][3];
      pve::NodeState state[4];
      RandomElement(gen, nDim, (trial % 3) ? 1.0 : 1e-3, x, state);
      pve::Element E;
      REQUIRE(E.Set(nDim, x, state, kGas) > 0.0);
      su2double N[3] = {};
      pve::WallFaceNormal(nDim, x, 0, faceNodes, N);

      /*--- Nodal wall velocities (deforming wall) for adiabatic walls, one (translating wall) for isothermal ones, where
       * the wall data g = (0, theta_w v_w, -theta_w) is then constant on the element. ---*/
      su2double wallVel[3][3] = {};
      for (int i = 0; i < nDim; ++i) wallVel[0][i] = 1.2 * u(gen);
      for (int k = 1; k < nDim; ++k)
        for (int i = 0; i < nDim; ++i) wallVel[k][i] = isothermal ? wallVel[0][i] : 1.2 * u(gen);

      /*--- P_C is symmetric, idempotent, and P_C w = 0 iff w_m + w_E v_w = 0. ---*/
      su2double P[5][5];
      for (int j = 0; j < nVar; ++j) {
        su2double e[5] = {}, Pe[5];
        e[j] = 1.0;
        pve::MovingWallProjector(nDim, wallVel[0], e, Pe);
        for (int i = 0; i < nVar; ++i) P[i][j] = Pe[i];
      }
      for (int i = 0; i < nVar; ++i) {
        for (int j = 0; j < nVar; ++j) {
          su2double PP = 0.0;
          for (int k = 0; k < nVar; ++k) PP += P[i][k] * P[k][j];
          CHECK(fabs(P[i][j] - P[j][i]) < 1e-14);
          CHECK(fabs(PP - P[i][j]) < 1e-14);
        }
      }
      su2double wWall[5] = {u(gen), 0.0, 0.0, 0.0, -exp(u(gen))}, Pw[5];
      for (int i = 0; i < nDim; ++i) wWall[i + 1] = -wWall[nDim + 1] * wallVel[0][i];
      pve::MovingWallProjector(nDim, wallVel[0], wWall, Pw);
      for (int i = 0; i < nVar; ++i) CHECK(fabs(Pw[i]) < 1e-14 * (1.0 + fabs(wWall[nDim + 1])));

      /*--- Theorem A18 on one element (m_T = 1): sum_a (w_a - g).(R_a + b_a) >= 0. ---*/
      const su2double wallT = exp(u(gen)), wallTheta = 1.0 / (kGas * wallT);
      su2double res[4][5], wall[4][5];
      E.Residual(res);
      pve::NoSlipWall(E, nDim, faceNodes, N, isothermal, wallTheta, 1, sipg, 2.5, wall, nullptr, nullptr, nullptr, wallVel);
      su2double rate = 0.0, scale = 0.0, resScale = 0.0, wallRate = 0.0, wallScale = 0.0;
      for (int a = 0; a < nNodes; ++a) {
        su2double w[5];
        FullEntropyVariables(nDim, state[a], w);
        for (int v = 0; v < nVar; ++v) {
          wallRate += w[v] * wall[a][v];
          wallScale += fabs(w[v] * wall[a][v]);
        }
        if (isothermal) {
          w[nDim + 1] += wallTheta;
          for (int i = 0; i < nDim; ++i) w[i + 1] -= wallTheta * wallVel[0][i];
        }
        for (int v = 0; v < nVar; ++v) {
          rate += w[v] * (res[a][v] + wall[a][v]);
          scale += fabs(w[v] * (res[a][v] + wall[a][v]));
          resScale = std::max(resScale, fabs(res[a][v]) + fabs(wall[a][v]));
        }
      }
      CHECK(rate >= -1e-12 * scale);

      /*--- Adiabatic: the production used by the element budget is -sum_c w_c . b_c. ---*/
      if (!isothermal) {
        const su2double production =
            pve::NoSlipWallProduction(E, nDim, faceNodes, N, false, wallTheta, 1, sipg, 2.5, nullptr, wallVel);
        CHECK(fabs(production + wallRate) <= 1e-12 * wallScale);
      }

      /*--- Rest in the wall frame (uniform v = v_w, T = T_w if isothermal): no residual. ---*/
      su2double restVel[3][3] = {};
      for (int k = 0; k < nDim; ++k)
        for (int i = 0; i < nDim; ++i) restVel[k][i] = wallVel[0][i];
      pve::NodeState rest[4];
      for (int a = 0; a < nNodes; ++a) {
        rest[a] = state[a];
        for (int i = 0; i < nDim; ++i) rest[a].vel[i] = wallVel[0][i];
        rest[a].p = rest[a].rho * kGas * (isothermal ? wallT : 0.8);
      }
      pve::Element Erest;
      Erest.Set(nDim, x, rest, kGas);
      su2double resRest[4][5], wallRest[4][5];
      Erest.Residual(resRest);
      pve::NoSlipWall(Erest, nDim, faceNodes, N, isothermal, wallTheta, 1, sipg, 2.5, wallRest, nullptr, nullptr, nullptr,
                      restVel);
      for (int a = 0; a < nNodes; ++a)
        for (int v = 0; v < nVar; ++v) CHECK(fabs(resRest[a][v] + wallRest[a][v]) < 1e-9 * resScale);

      /*--- v_w = 0 is the wall at rest. ---*/
      const su2double zeroVel[3][3] = {};
      su2double wallZero[4][5], wallStatic[4][5];
      pve::NoSlipWall(E, nDim, faceNodes, N, isothermal, wallTheta, 1, sipg, 2.5, wallZero, nullptr, nullptr, nullptr, zeroVel);
      pve::NoSlipWall(E, nDim, faceNodes, N, isothermal, wallTheta, 1, sipg, 2.5, wallStatic);
      for (int a = 0; a < nNodes; ++a)
        for (int v = 0; v < nVar; ++v) CHECK(fabs(wallZero[a][v] - wallStatic[a][v]) <= 1e-13 * resScale);

      /*--- Picard Jacobian (K and sigma frozen) against central differences. ---*/
      su2double dwdu[4][5][5], jac[4][4][5][5], jacMax = 0.0, jacErr = 0.0;
      for (int a = 0; a < nNodes; ++a) pve::EntropyVariableJacobian(nDim, kGamma, state[a], dwdu[a]);
      pve::NoSlipWall(E, nDim, faceNodes, N, isothermal, wallTheta, 1, sipg, 2.5, wall, dwdu, jac, nullptr, wallVel);
      for (int b = 0; b < nNodes; ++b) {
        for (int j = 0; j < nVar; ++j) {
          const su2double h = 1e-6;
          pve::NodeState plus[4], minus[4];
          for (int a = 0; a < nNodes; ++a) plus[a] = minus[a] = state[a];
          Perturb(nDim, plus[b], j, h);
          Perturb(nDim, minus[b], j, -h);
          su2double rp[4][5], rm[4][5];
          pve::NoSlipWall(Frozen(E, plus), nDim, faceNodes, N, isothermal, wallTheta, 1, sipg, 2.5, rp, nullptr, nullptr,
                          nullptr, wallVel);
          pve::NoSlipWall(Frozen(E, minus), nDim, faceNodes, N, isothermal, wallTheta, 1, sipg, 2.5, rm, nullptr, nullptr,
                          nullptr, wallVel);
          for (int a = 0; a < nNodes; ++a) {
            for (int v = 0; v < nVar; ++v) {
              jacErr = std::max(jacErr, fabs((rp[a][v] - rm[a][v]) / (2 * h) - jac[a][b][v][j]));
              jacMax = std::max(jacMax, fabs(jac[a][b][v][j]));
            }
          }
        }
      }
      CHECK(jacErr <= 1e-6 * jacMax);
    }
  }
}

namespace {

/*--- Nodal state from the global index, the same on all ranks and halos. ---*/
ale::State GlobalState(unsigned long globalIndex, int nDim) {
  std::mt19937 gen(1000 + globalIndex);
  return RandomAleState(gen, nDim, 0.4);
}

}  // namespace

TEST_CASE("GRID_ENTROPY_P1 on a closed moving domain: exact GCL, swept remainder, entropy balance, free stream", "[P1Flux]") {
  struct Kind {
    int nDim;
    unsigned long n;
    std::string options;
    bool form;
  };
  const std::string common = "CONV_NUM_METHOD_FLOW= JST\nEDGE_FORM_EC_FLUX= YES\nBC_WEAK_P1= YES\nGCL_EXACT_P1= YES\nGRID_ENTROPY_P1= YES\n";
  const std::string moved = "MOD_CENTROIDS= YES\nEDGE_FORM_P1= YES\n" + common;
  const std::string median = "EDGE_DISS_PROJECTED= YES\n" + common;
  const std::vector<Kind> cases = {Kind{2, 8, moved, true}, Kind{3, 3, moved, true}, Kind{2, 8, median, false},
                                   Kind{3, 3, median, false}};

  for (int order : {1, 2}) {
    for (const auto& mesh : cases) {
      const std::string name = "p1_grid_entropy_" + std::to_string(mesh.nDim) + "d.su2";
      WriteSimplexMesh(name, mesh.nDim, mesh.n, 0.4, 1.0);
      const std::string motion = "TIME_DOMAIN= YES\nTIME_MARCHING= DUAL_TIME_STEPPING-" +
                                 std::string(order == 1 ? "1ST" : "2ND") +
                                 "_ORDER\nTIME_STEP= 0.01\nGRID_MOVEMENT= RIGID_MOTION\n";
      MeshCase test(name, motion + mesh.options);
      auto& geometry = *test.geometry;
      const auto config = test.config.get();
      const auto nDim = geometry.GetnDim();
      const unsigned short nVar = nDim + 2;
      INFO("BDF" << order << " nDim " << mesh.nDim << " " << (mesh.form ? "moved dual" : "median dual"));

      /*--- Three levels of a large step of a rotation (angle 0.8 s about the box centre, axis z) composed with the
       * nonlinear deformation of MoveMesh; grid velocity = BDF difference of the coordinates. ---*/
      const su2double dt = 0.01, step = 0.1;
      config->SetDelta_UnstTimeND(dt);
      const auto X0 = StoreCoord(geometry);
      auto moveRotating = [&](su2double s) {
        const su2double c = cos(0.8 * s), sn = sin(0.8 * s);
        for (auto iPoint = 0ul; iPoint < geometry.GetnPoint(); ++iPoint) {
          su2double d[3] = {}, y[3] = {};
          Displacement(nDim, X0[iPoint].data(), d);
          for (auto iDim = 0u; iDim < nDim; ++iDim) y[iDim] = X0[iPoint][iDim] + s * d[iDim] - 0.5;
          geometry.nodes->SetCoord(iPoint, 0, 0.5 + c * y[0] - sn * y[1]);
          geometry.nodes->SetCoord(iPoint, 1, 0.5 + sn * y[0] + c * y[1]);
          if (nDim == 3) geometry.nodes->SetCoord(iPoint, 2, 0.5 + y[2]);
        }
        geometry.SetControlVolume(config, UPDATE);
        geometry.SetBoundControlVolume(config, UPDATE);
        geometry.SetBoundCVCoeffs(config, UPDATE);
        geometry.UpdateP1FluxCoefficients(config);
      };
      moveRotating(0.5 - 2 * step);
      geometry.nodes->SetVolume_n();
      geometry.nodes->SetCoord_n();
      moveRotating(0.5 - step);
      geometry.nodes->SetVolume_nM1();
      geometry.nodes->SetVolume_n();
      geometry.nodes->SetCoord_n1();
      geometry.nodes->SetCoord_n();
      moveRotating(0.5);
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

      /*--- (a) G = D_t V on both duals; (b) the nodal remainder is the sum of kappa_f of the boundary faces. ---*/
      std::vector<su2double> kappaSum(geometry.GetnPoint(), 0.0);
      for (auto iMarker = 0u; iMarker < geometry.GetnMarker(); ++iMarker) {
        if (config->GetMarker_All_KindBC(iMarker) == SEND_RECEIVE) continue;
        for (auto iElem = 0ul; iElem < geometry.GetnElem_Bound(iMarker); ++iElem) {
          const auto* face = geometry.bound[iMarker][iElem];
          unsigned long points[3] = {};
          for (unsigned short k = 0; k < face->GetnNodes(); ++k) points[k] = face->GetNode(k);
          const su2double kappa = geometry.BoundaryFaceKappa(config, face->GetnNodes(), points, face->getNormal());
          for (unsigned short k = 0; k < face->GetnNodes(); ++k) kappaSum[points[k]] += kappa;
        }
      }
      su2double gclError = 0.0, kappaError = 0.0, scale = 0.0, sigmaMax = 0.0;
      for (auto iPoint = 0ul; iPoint < geometry.GetnPointDomain(); ++iPoint) {
        const auto V = geometry.nodes->GetVolume(iPoint);
        const auto V_n = geometry.nodes->GetVolume_n(iPoint);
        const auto V_n1 = geometry.nodes->GetVolume_nM1(iPoint);
        const su2double dVdt = (order == 1) ? (V - V_n) / dt : (1.5 * V - 2.0 * V_n + 0.5 * V_n1) / dt;
        gclError = std::max(gclError, fabs(G[iPoint] - dVdt));
        scale = std::max(scale, fabs(dVdt));
        const su2double sigma = geometry.edges->GetBoundSwept()[iPoint];
        kappaError = std::max(kappaError, fabs(sigma - kappaSum[iPoint]));
        sigmaMax = std::max(sigmaMax, fabs(sigma));
      }
      CHECK(GlobalMax(gclError) < 1e-10 * GlobalMax(scale));
      CHECK(GlobalMax(kappaError) < 1e-10 * GlobalMax(scale));
      CHECK(GlobalMax(sigmaMax) > 1e-6 * GlobalMax(scale));  /*--- The remainder is not trivially zero. ---*/

      /*--- Residual of the entropy conservative central part, the grid term -ubar^EC g_ik of centered.hpp and closure
       * C-ALE at all boundary faces (walls or far field), without the E part (its production is the P_E of the blend).
       * (c) Walls: -sum_i w_i.(R_i + b_i) - sum_i rho_i G_i = 0 (Theorem A10: all grid terms telescope).
       * (d) Far field at the uniform state: R_i + b_i + U G_i = 0 (free stream with the dual time source U G). ---*/
      for (int farField = 0; farField < 2; ++farField) {
        std::vector<ale::State> state(geometry.GetnPoint());
        const auto uniform = GlobalState(12345, nDim);
        for (auto iPoint = 0ul; iPoint < geometry.GetnPoint(); ++iPoint) {
          state[iPoint] = farField ? uniform : GlobalState(geometry.nodes->GetGlobalIndex(iPoint), nDim);
        }
        std::vector<std::array<su2double, 5>> R(geometry.GetnPoint(), std::array<su2double, 5>{});

        for (auto iEdge = 0ul; iEdge < geometry.GetnEdge(); ++iEdge) {
          const auto iPoint = geometry.edges->GetNode(iEdge, 0), kPoint = geometry.edges->GetNode(iEdge, 1);
          const su2double* normal = mesh.form ? geometry.edges->GetFormNormal()[iEdge] : geometry.edges->GetNormal(iEdge);
          const su2double zero[3] = {0.0, 0.0, 0.0};
          const su2double* formE = mesh.form ? geometry.edges->GetFormE()[iEdge] : zero;
          const auto* v_i = geometry.nodes->GetGridVel(iPoint);
          const auto* v_k = geometry.nodes->GetGridVel(kPoint);
          su2double gridFlux = geometry.edges->GetFormSwept()[iEdge];
          for (auto iDim = 0u; iDim < nDim; ++iDim)
            gridFlux += 0.5 * (v_i[iDim] + v_k[iDim]) * normal[iDim] + (v_k[iDim] - v_i[iDim]) * formE[iDim];
          su2double flux[5];
          ale::AleFlux(nDim, kGamma, state[iPoint], state[kPoint], normal, gridFlux, flux);
          for (unsigned short v = 0; v < nVar; ++v) {
            R[iPoint][v] += flux[v];
            R[kPoint][v] -= flux[v];
          }
        }

        const auto inf = uniform;
        for (auto iMarker = 0u; iMarker < geometry.GetnMarker(); ++iMarker) {
          if (config->GetMarker_All_KindBC(iMarker) == SEND_RECEIVE) continue;
          for (auto iElem = 0ul; iElem < geometry.GetnElem_Bound(iMarker); ++iElem) {
            const auto* face = geometry.bound[iMarker][iElem];
            const auto nFaceNodes = face->GetnNodes();
            unsigned long points[3] = {};
            ale::State faceState[3];
            su2double faceVel[3][3] = {}, faceRes[3][5];
            for (unsigned short k = 0; k < nFaceNodes; ++k) {
              points[k] = face->GetNode(k);
              faceState[k] = state[points[k]];
              for (auto iDim = 0u; iDim < nDim; ++iDim) faceVel[k][iDim] = geometry.nodes->GetGridVel(points[k])[iDim];
            }
            const su2double kappa = geometry.BoundaryFaceKappa(config, nFaceNodes, points, face->getNormal());
            ale::FaceResidual(nDim, nFaceNodes, kGamma, farField ? &inf : nullptr, face->getNormal(), faceState, faceVel,
                              kappa, faceRes);
            for (unsigned short k = 0; k < nFaceNodes; ++k)
              for (unsigned short v = 0; v < nVar; ++v) R[points[k]][v] += faceRes[k][v];
          }
        }

        su2double rate = 0.0, rateScale = 0.0, freeStream = 0.0, freeScale = 0.0;
        for (auto iPoint = 0ul; iPoint < geometry.GetnPointDomain(); ++iPoint) {
          su2double w[5], U[5];
          ale::EntropyVariables(nDim, kGamma, state[iPoint], w);
          ale::Conservative(nDim, kGamma, state[iPoint], U);
          rate -= state[iPoint].rho * G[iPoint];
          rateScale += fabs(state[iPoint].rho * G[iPoint]);
          for (unsigned short v = 0; v < nVar; ++v) {
            rate -= w[v] * R[iPoint][v];
            rateScale += fabs(w[v] * R[iPoint][v]);
            freeStream = std::max(freeStream, fabs(R[iPoint][v] + U[v] * G[iPoint]));
            freeScale = std::max(freeScale, fabs(U[v] * G[iPoint]) + fabs(R[iPoint][v]));
          }
        }
        if (farField) {
          CHECK(GlobalMax(freeStream) < 1e-10 * GlobalMax(freeScale));
        } else {
          CHECK(fabs(GlobalSum(rate)) < 1e-8 * GlobalSum(rateScale));
        }
      }
    }
  }
}

/*--- EDGE_FORM_P1_MIXED (P1_MIXED.md): split averages of quadrilaterals, hexahedra, prisms and pyramids. ---*/

namespace {

/*!
 * \brief Write a conforming mixed mesh. 2D: quadrilaterals and triangle pairs in a checkerboard. 3D (n >= 3): columns
 *        of hexahedra ((i+j) even) and of prism pairs below z = (n-2)/n, a transition layer of pyramids with 4
 *        tetrahedra on the hexahedra and of Kuhn cells on the prisms, Kuhn tetrahedra on top. So every cell type and
 *        every interface (hex-prism quads, hex-pyramid quads, prism-tet and pyramid-tet triangles) occurs. Interior
 *        nodes are jittered, the z scale stretches the cells. Markers: bottom (quads and triangles), rest.
 */
void WriteMixedMesh(const std::string& name, int nDim, unsigned long n, double jitter, double stretch) {
  if (SU2_MPI::GetRank() != MASTER_NODE) return;

  std::mt19937 gen(2718);
  std::uniform_real_distribution<double> rnd(-0.5, 0.5);
  std::ofstream f(name);
  f.precision(17);
  std::ostringstream elems, points, bottom, rest;
  unsigned long nElem = 0, nBottom = 0, nRest = 0;

  if (nDim == 2) {
    auto id = [&](unsigned long i, unsigned long j) { return i * (n + 1) + j; };
    for (auto i = 0ul; i < n; ++i) {
      for (auto j = 0ul; j < n; ++j) {
        const unsigned long q[] = {id(i, j), id(i + 1, j), id(i + 1, j + 1), id(i, j + 1)};
        if ((i + j) % 2 == 0) {
          elems << "9 " << q[0] << " " << q[1] << " " << q[2] << " " << q[3] << "\n";
          ++nElem;
        } else {
          elems << "5 " << q[0] << " " << q[1] << " " << q[2] << "\n5 " << q[0] << " " << q[2] << " " << q[3] << "\n";
          nElem += 2;
        }
      }
    }
    for (auto i = 0ul; i <= n; ++i) {
      for (auto j = 0ul; j <= n; ++j) {
        const bool inner = (i > 0) && (j > 0) && (i < n) && (j < n);
        points << (i + (inner ? jitter * rnd(gen) : 0.0)) / n << " "
               << (j + (inner ? jitter * rnd(gen) : 0.0)) / n * stretch << "\n";
      }
    }
    f << "NDIME= 2\nNELEM= " << nElem << "\n" << elems.str() << "NPOIN= " << (n + 1) * (n + 1) << "\n" << points.str();
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
      const bool hexColumn = ((i + j) % 2 == 0);
      for (auto k = 0ul; k < n; ++k) {
        const unsigned long h[] = {Lattice(i, j, k, n),             Lattice(i + 1, j, k, n),
                                   Lattice(i + 1, j + 1, k, n),     Lattice(i, j + 1, k, n),
                                   Lattice(i, j, k + 1, n),         Lattice(i + 1, j, k + 1, n),
                                   Lattice(i + 1, j + 1, k + 1, n), Lattice(i, j + 1, k + 1, n)};
        auto tets = [&](int first) {
          for (int t = first; t < 6; ++t) {
            elems << "10 " << h[kuhn[t][0]] << " " << h[kuhn[t][1]] << " " << h[kuhn[t][2]] << " " << h[kuhn[t][3]] << "\n";
            ++nElem;
          }
        };
        if (k + 2 < n) {
          if (hexColumn) {
            elems << "12";
            for (const auto node : h) elems << " " << node;
            elems << "\n";
            ++nElem;
          } else {
            elems << "13 " << h[0] << " " << h[1] << " " << h[2] << " " << h[4] << " " << h[5] << " " << h[6] << "\n";
            elems << "13 " << h[0] << " " << h[2] << " " << h[3] << " " << h[4] << " " << h[6] << " " << h[7] << "\n";
            nElem += 2;
          }
        } else if ((k + 2 == n) && hexColumn) {
          /*--- The Kuhn tets 0-1-2-6 and 0-2-3-6 merged into the pyramid on the quad face below. ---*/
          elems << "14 " << h[0] << " " << h[1] << " " << h[2] << " " << h[3] << " " << h[6] << "\n";
          ++nElem;
          tets(2);
        } else {
          tets(0);
        }
      }
    }
  }
  for (auto i = 0ul; i <= n; ++i) {
    for (auto j = 0ul; j <= n; ++j) {
      for (auto k = 0ul; k <= n; ++k) {
        const bool inner = (i > 0) && (j > 0) && (k > 0) && (i < n) && (j < n) && (k < n);
        points << (i + (inner ? jitter * rnd(gen) : 0.0)) / n << " " << (j + (inner ? jitter * rnd(gen) : 0.0)) / n << " "
               << (k + (inner ? jitter * rnd(gen) : 0.0)) / n * stretch << "\n";
      }
    }
  }

  /*--- Triangles follow the diagonal of the Kuhn split (or of the prism pair at the bottom). ---*/
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
  for (auto u = 0ul; u < n; ++u) {
    for (auto v = 0ul; v < n; ++v) {
      const bool side = (v + 2 >= n);
      quad(bottom, nBottom, (u + v) % 2 == 1, Lattice(u, v, 0, n), Lattice(u, v + 1, 0, n), Lattice(u + 1, v + 1, 0, n),
           Lattice(u + 1, v, 0, n));
      quad(rest, nRest, true, Lattice(u, v, n, n), Lattice(u + 1, v, n, n), Lattice(u + 1, v + 1, n, n), Lattice(u, v + 1, n, n));
      quad(rest, nRest, side, Lattice(u, 0, v, n), Lattice(u + 1, 0, v, n), Lattice(u + 1, 0, v + 1, n), Lattice(u, 0, v + 1, n));
      quad(rest, nRest, side, Lattice(u, n, v, n), Lattice(u, n, v + 1, n), Lattice(u + 1, n, v + 1, n), Lattice(u + 1, n, v, n));
      quad(rest, nRest, side, Lattice(0, u, v, n), Lattice(0, u, v + 1, n), Lattice(0, u + 1, v + 1, n), Lattice(0, u + 1, v, n));
      quad(rest, nRest, side, Lattice(n, u, v, n), Lattice(n, u + 1, v, n), Lattice(n, u + 1, v + 1, n), Lattice(n, u, v + 1, n));
    }
  }
  f << "NDIME= 3\nNELEM= " << nElem << "\n" << elems.str() << "NPOIN= " << (n + 1) * (n + 1) * (n + 1) << "\n" << points.str();
  f << "NMARK= 2\nMARKER_TAG= bottom\nMARKER_ELEMS= " << nBottom << "\n" << bottom.str();
  f << "MARKER_TAG= rest\nMARKER_ELEMS= " << nRest << "\n" << rest.str();
}

const std::string MixedOptions =
    "MOD_CENTROIDS= YES\nCONV_NUM_METHOD_FLOW= JST\nEDGE_FORM_P1= YES\nEDGE_FORM_P1_MIXED= YES\nBC_WEAK_P1= YES\n";

/*--- A single cell of a VTK type with local node IDs, and its reference coordinates. ---*/
std::unique_ptr<CPrimalGrid> MakeCell(int vtk) {
  if (vtk == HEXAHEDRON) return std::unique_ptr<CPrimalGrid>(new CHexahedron(0, 1, 2, 3, 4, 5, 6, 7));
  if (vtk == PRISM) return std::unique_ptr<CPrimalGrid>(new CPrism(0, 1, 2, 3, 4, 5));
  if (vtk == PYRAMID) return std::unique_ptr<CPrimalGrid>(new CPyramid(0, 1, 2, 3, 4));
  return std::unique_ptr<CPrimalGrid>(new CQuadrilateral(0, 1, 2, 3));
}

/*--- Reference cell, then a non-affine perturbation, a stretch of the last axis and a rotation. ---*/
std::vector<std::array<su2double, 3>> CellCoords(std::mt19937& gen, int vtk, double perturbation, double stretch) {
  std::vector<std::array<su2double, 3>> x;
  if (vtk == HEXAHEDRON) x = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};
  else if (vtk == PRISM) x = {{0, 0, 0}, {1, 0, 0}, {0, 1, 0}, {0, 0, 1}, {1, 0, 1}, {0, 1, 1}};
  else if (vtk == PYRAMID) x = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}, {0.5, 0.5, 0.6}};
  else x = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0}};
  const int nDim = (vtk == QUADRILATERAL) ? 2 : 3;
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  const double a = 0.7, b = 0.4;
  for (auto& p : x) {
    for (int i = 0; i < nDim; ++i) p[i] += perturbation * u(gen);
    p[nDim - 1] *= stretch;
    const su2double x0 = p[0], x1 = p[1];
    p[0] = cos(a) * x0 - sin(a) * x1;
    p[1] = sin(a) * x0 + cos(a) * x1;
    if (nDim == 3) {
      const su2double y = p[1], z = p[2];
      p[1] = cos(b) * y - sin(b) * z;
      p[2] = sin(b) * y + cos(b) * z;
    }
  }
  return x;
}

/*--- The kept (neighbour-fan) pulling splits of a cell type, each as a list of weight-1 sub-simplices. ---*/
std::vector<std::vector<P1Mixed::Simplex>> KeptSplits(CPhysicalGeometry& geometry, const CPrimalGrid& cell, int nDim) {
  std::vector<std::vector<P1Mixed::Simplex>> splits;
  for (const auto& pattern : geometry.GetSplitPatterns(&cell)) {
    if (!P1Mixed::NeighbourFan(cell, nDim, pattern.simplices)) continue;
    std::vector<P1Mixed::Simplex> split;
    for (const auto& t : pattern.simplices) {
      P1Mixed::Simplex entry;
      entry.nodes = t;
      entry.weight = 1.0;
      split.push_back(entry);
    }
    splits.push_back(split);
  }
  return splits;
}

/*--- Sorted node tuple of a sub-simplex. ---*/
std::array<unsigned short, 4> SimplexKey(const P1Mixed::Simplex& t, int nDim) {
  std::array<unsigned short, 4> key = {{0, 0, 0, 0}};
  for (int a = 0; a <= nDim; ++a) key[a] = t.nodes[a];
  std::sort(key.begin(), key.begin() + nDim + 1);
  return key;
}

/*--- Sizes (oriented like the cell) and gradients of the sub-simplices of a list on coordinates x. ---*/
struct SubSimplices {
  std::vector<su2double> size;
  std::vector<std::array<std::array<su2double, 3>, 4>> grad;
  SubSimplices(int nDim, const std::vector<P1Mixed::Simplex>& list, const std::vector<std::array<su2double, 3>>& x) {
    su2double total = 0.0;
    for (const auto& t : list) {
      const su2double* X[4] = {nullptr, nullptr, nullptr, nullptr};
      for (int a = 0; a <= nDim; ++a) X[a] = x[t.nodes[a]].data();
      su2double g[4][3] = {};
      size.push_back(P1Simplex::ShapeGradients(nDim, X, g));
      std::array<std::array<su2double, 3>, 4> G;
      for (int a = 0; a < 4; ++a)
        for (int i = 0; i < 3; ++i) G[a][i] = g[a][i];
      grad.push_back(G);
      total += t.weight * size.back();
    }
    for (auto& s : size) s *= (total < 0.0) ? -1.0 : 1.0;
  }
};

using PairMap = std::map<std::pair<int, int>, std::array<su2double, 3>>;

/*--- n_ik (from i to k, i < k) of the M0 operator of a list: sum_t omega_t m_t (g_k - g_i), and the masses. With
 * delta != nullptr, the E part sum_t omega_t (delta_i g_k + delta_k g_i)/2 of the weights W = m_t + delta instead. ---*/
PairMap MixedPairs(int nDim, const std::vector<P1Mixed::Simplex>& list, const std::vector<std::array<su2double, 3>>& x,
                   std::vector<su2double>* mass = nullptr,
                   const std::map<std::array<unsigned short, 4>, std::array<su2double, 4>>* delta = nullptr) {
  SubSimplices sub(nDim, list, x);
  PairMap pairs;
  for (size_t t = 0; t < list.size(); ++t) {
    const su2double m = sub.size[t] / (nDim + 1);
    const std::array<su2double, 4>* d = delta ? &delta->at(SimplexKey(list[t], nDim)) : nullptr;
    for (int a = 0; a <= nDim; ++a) {
      if (mass) (*mass)[list[t].nodes[a]] += list[t].weight * m;
      for (int b = a + 1; b <= nDim; ++b) {
        int i = list[t].nodes[a], k = list[t].nodes[b], ia = a, kb = b;
        if (i > k) {
          std::swap(i, k);
          std::swap(ia, kb);
        }
        auto& n = pairs[{i, k}];
        for (int j = 0; j < nDim; ++j) {
          if (d) {
            /*--- delta of each node is stored in the sorted order of the key. ---*/
            const auto key = SimplexKey(list[t], nDim);
            su2double di = 0.0, dk = 0.0;
            for (int c = 0; c <= nDim; ++c) {
              if (key[c] == i) di = (*d)[c];
              if (key[c] == k) dk = (*d)[c];
            }
            n[j] += list[t].weight * 0.5 * (di * sub.grad[t][kb][j] + dk * sub.grad[t][ia][j]);
          } else {
            n[j] += list[t].weight * m * (sub.grad[t][kb][j] - sub.grad[t][ia][j]);
          }
        }
      }
    }
  }
  return pairs;
}

/*--- Lemma 3 speed of a pair piece E and the production / dissipation terms of the pair. ---*/
void PairEntropy(int nDim, const ale::State& Si, const ale::State& Sk, const su2double* E, su2double& production,
                 su2double& dissipation) {
  su2double Fi[5], Fk[5], wi[5], wk[5], Ui[5], Uk[5];
  AleFluxN(nDim, Si, E, Fi);
  AleFluxN(nDim, Sk, E, Fk);
  ale::EntropyVariables(nDim, kGamma, Si, wi);
  ale::EntropyVariables(nDim, kGamma, Sk, wk);
  ale::Conservative(nDim, kGamma, Si, Ui);
  ale::Conservative(nDim, kGamma, Sk, Uk);
  su2double area = 0.0, ui = 0.0, uk = 0.0;
  for (int j = 0; j < nDim; ++j) {
    area += E[j] * E[j];
    ui += Si.vel[j] * E[j];
    uk += Sk.vel[j] * E[j];
  }
  area = sqrt(area);
  const su2double c = sqrt(kGamma * std::max(Si.p / Si.rho, Sk.p / Sk.rho));
  const su2double lam = std::max(su2double(0.0), std::max(ui, uk) + c * area);
  production = 0.0;
  dissipation = 0.0;
  for (int v = 0; v < nDim + 2; ++v) {
    production += (wk[v] - wi[v]) * (Fk[v] - Fi[v]);
    dissipation += lam * (wk[v] - wi[v]) * (Uk[v] - Ui[v]);
  }
}

constexpr int MixedTypes[4] = {HEXAHEDRON, PRISM, PYRAMID, QUADRILATERAL};

}  // namespace

TEST_CASE("EDGE_FORM_P1_MIXED: split averages per cell type, face balance, symmetry, linear exactness", "[P1Flux]") {
  std::mt19937 gen(1357);
  for (int nDim : {2, 3}) {
    const std::string mesh = "p1_mixed_cells_" + std::to_string(nDim) + "d.su2";
    WriteMixedMesh(mesh, nDim, (nDim == 2) ? 8 : 4, 0.0, 1.0);
    MeshCase test(mesh, MixedOptions);
    auto& geometry = static_cast<CPhysicalGeometry&>(*test.geometry);

    for (const int vtk : MixedTypes) {
      if ((vtk == QUADRILATERAL) != (nDim == 2)) continue;
      const auto cell = MakeCell(vtk);
      const auto& list = geometry.SetMixedSimplices(cell.get());
      INFO("VTK type " << vtk);

      /*--- Sizes and weights (P1_MIXED.md, 8.1). ---*/
      const size_t expected = (vtk == HEXAHEDRON) ? 24 : (vtk == PRISM) ? 12 : 4;
      CHECK(list.size() == expected);
      su2double sumWeight = 0.0;
      for (const auto& t : list) {
        sumWeight += t.weight;
        if (vtk == PRISM) CHECK(((fabs(t.weight - 1.0 / 6.0) < 1e-14) || (fabs(t.weight - 1.0 / 3.0) < 1e-14)));
        else CHECK(fabs(t.weight - ((vtk == HEXAHEDRON) ? 0.25 : 0.5)) < 1e-14);
      }
      const auto splits = KeptSplits(geometry, *cell, nDim);
      CHECK(splits.size() == ((vtk == HEXAHEDRON) ? 8u : (vtk == PRISM) ? 12u : (vtk == PYRAMID) ? 6u : 4u));
      CHECK(fabs(sumWeight - splits[0].size()) < 1e-12);

      /*--- Face condition (Lemma X2): each sub-triangle of a quad face has weight 1/2, every other face weight 1. ---*/
      auto faceWeight = [&](const std::vector<unsigned short>& face) {
        su2double w = 0.0;
        for (const auto& t : list) {
          size_t found = 0;
          for (const auto node : face)
            for (int a = 0; a <= nDim; ++a) found += (t.nodes[a] == node);
          if (found == face.size()) w += t.weight;
        }
        return w;
      };
      for (unsigned short iFace = 0; iFace < cell->GetnFaces(); ++iFace) {
        const auto nFaceNodes = (nDim == 2) ? 2 : cell->GetnNodesFace(iFace);
        std::vector<unsigned short> face;
        for (unsigned short k = 0; k < nFaceNodes; ++k) face.push_back(cell->GetFaces(iFace, k));
        if (nFaceNodes == 4) {
          for (const auto& tri : P1Mixed::QuadTriangles) {
            CHECK(fabs(faceWeight({face[tri[0]], face[tri[1]], face[tri[2]]}) - 0.5) < 1e-14);
          }
        } else {
          CHECK(fabs(faceWeight(face) - 1.0) < 1e-14);
        }
      }

      /*--- Node-ID independence: the list is invariant under the symmetries of the reference cell. ---*/
      std::vector<std::vector<unsigned short>> maps;
      if (vtk == HEXAHEDRON) maps = {{1, 2, 3, 0, 5, 6, 7, 4}, {4, 5, 6, 7, 0, 1, 2, 3}, {3, 2, 6, 7, 0, 1, 5, 4}};
      if (vtk == PRISM) maps = {{1, 2, 0, 4, 5, 3}, {3, 4, 5, 0, 1, 2}, {0, 2, 1, 3, 5, 4}};
      if (vtk == PYRAMID) maps = {{1, 2, 3, 0, 4}, {0, 3, 2, 1, 4}};
      if (vtk == QUADRILATERAL) maps = {{1, 2, 3, 0}, {0, 3, 2, 1}};
      std::map<std::array<unsigned short, 4>, su2double> weights;
      for (const auto& t : list) weights[SimplexKey(t, nDim)] = t.weight;
      for (const auto& map : maps) {
        for (const auto& t : list) {
          P1Mixed::Simplex image = t;
          for (int a = 0; a <= nDim; ++a) image.nodes[a] = map[t.nodes[a]];
          const auto found = weights.find(SimplexKey(image, nDim));
          REQUIRE(found != weights.end());
          CHECK(fabs(found->second - t.weight) < 1e-14);
        }
      }

      /*--- Single cells (affine, stretched, non-affine), Lemma X1: sum_t omega_t |t| is the mean volume of the kept
       * splits; c_ik = sum_t omega_t m_t g^t_k (rows i of t) is linearly exact, sum_k c_ik (x) x_k = H_i I, with
       * zero row sums; n = c - c^T has sum_k n_ik = -beta_i with sum_i beta_i = 0 (closed cell). ---*/
      for (int draw = 0; draw < 20; ++draw) {
        const auto x = CellCoords(gen, vtk, (draw < 4) ? 0.0 : 0.12, (draw % 2) ? 1e-3 : 1.0);
        const auto nv = x.size();
        SubSimplices sub(nDim, list, x);
        std::vector<su2double> mass(nv, 0.0);
        std::vector<std::array<su2double, 3>> c(nv * nv, {0.0, 0.0, 0.0});
        for (size_t t = 0; t < list.size(); ++t) {
          CHECK(sub.size[t] > 0.0);
          const su2double m = sub.size[t] / (nDim + 1);
          for (int a = 0; a <= nDim; ++a) {
            mass[list[t].nodes[a]] += list[t].weight * m;
            for (int b = 0; b <= nDim; ++b)
              for (int j = 0; j < nDim; ++j) c[list[t].nodes[a] * nv + list[t].nodes[b]][j] += list[t].weight * m * sub.grad[t][b][j];
          }
        }
        su2double volume = 0.0, splitVolume = 0.0;
        for (const auto value : mass) volume += value;
        for (const auto& split : splits) {
          SubSimplices subSplit(nDim, split, x);
          for (const auto s : subSplit.size) splitVolume += s / splits.size();
        }
        CHECK(fabs(volume - splitVolume) < 1e-12 * fabs(volume));

        su2double exactness = 0.0, rowSum = 0.0, gradScale = 0.0;
        for (size_t i = 0; i < nv; ++i) {
          for (int p = 0; p < nDim; ++p) {
            su2double sum = 0.0;
            for (size_t k = 0; k < nv; ++k) {
              sum += c[i * nv + k][p];
              gradScale = std::max(gradScale, fabs(c[i * nv + k][p]));
            }
            rowSum = std::max(rowSum, fabs(sum));
            for (int q = 0; q < nDim; ++q) {
              su2double cx = 0.0;
              for (size_t k = 0; k < nv; ++k) cx += c[i * nv + k][p] * x[k][q];
              exactness = std::max(exactness, fabs(cx - ((p == q) ? mass[i] : 0.0)) / volume);
            }
          }
        }
        CHECK(exactness < 1e-11);
        CHECK(rowSum < 1e-12 * gradScale);

        const auto pairs = MixedPairs(nDim, list, x);
        std::vector<std::array<su2double, 3>> beta(nv, {0.0, 0.0, 0.0});
        su2double maxBeta = 0.0, sumBeta[3] = {0.0, 0.0, 0.0};
        for (const auto& pair : pairs) {
          for (int j = 0; j < nDim; ++j) {
            beta[pair.first.first][j] -= pair.second[j];
            beta[pair.first.second][j] += pair.second[j];
            /*--- n_ik = c_ik - c_ki from the operator above. ---*/
            const auto i = pair.first.first, k = pair.first.second;
            CHECK(fabs(pair.second[j] - (c[i * nv + k][j] - c[k * nv + i][j])) < 1e-12 * gradScale);
          }
        }
        for (const auto& b : beta) {
          for (int j = 0; j < nDim; ++j) {
            sumBeta[j] += b[j];
            maxBeta = std::max(maxBeta, fabs(b[j]));
          }
        }
        for (int j = 0; j < nDim; ++j) CHECK(fabs(sumBeta[j]) < 1e-13 * maxBeta);
      }
    }
  }
}

TEST_CASE("EDGE_FORM_P1_MIXED on a mixed mesh: linear exactness with the form mass, closure on quad faces", "[P1Flux]") {
  for (int nDim : {2, 3}) {
    for (double stretch : {1.0, 1e-3}) {
      const std::string mesh = "p1_mixed_mesh_" + std::to_string(nDim) + "d.su2";
      WriteMixedMesh(mesh, nDim, (nDim == 2) ? 10 : 5, 0.3, stretch);
      MeshCase test(mesh, MixedOptions);
      auto& geometry = *test.geometry;
      geometry.SetEdgeFormP1(true);
      const auto& mass = geometry.edges->GetFormMass();
      INFO("nDim " << nDim << " stretch " << stretch);

      /*--- The masses H_i fill the box (sum over the split averages, faces shared consistently). ---*/
      su2double localVolume = 0.0;
      for (auto iPoint = 0ul; iPoint < geometry.GetnPointDomain(); ++iPoint) localVolume += mass[iPoint];
      CHECK(fabs(GlobalSum(localVolume) - stretch) < 1e-10 * stretch);

      /*--- A linear flux is exact at interior nodes, R_i = H_i div F (true edges and diagonals), and at every node
       * with the boundary closure (quad faces with both diagonals at 1/2). E = 0 on the non-simplex cells. ---*/
      LinearFlux flux;
      const auto div = flux.Div(nDim);
      auto R = SplitSum(geometry, flux, false);
      su2double interior = 0.0;
      for (auto iPoint = 0ul; iPoint < geometry.GetnPointDomain(); ++iPoint) {
        if (geometry.nodes->GetPhysicalBoundary(iPoint)) continue;
        interior = std::max(interior, fabs(R[iPoint] / mass[iPoint] - div) / fabs(div));
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
      su2double all = 0.0;
      for (auto iPoint = 0ul; iPoint < geometry.GetnPointDomain(); ++iPoint) {
        all = std::max(all, fabs(R[iPoint] / mass[iPoint] - div) / fabs(div));
      }
      CHECK(GlobalMax(interior) < 1e-9);
      CHECK(GlobalMax(all) < 1e-9);

      /*--- The diagonals carry no E part (M0) and exist on every mesh with non-simplex cells. ---*/
      su2double maxE = 0.0;
      for (auto iSplit = 0ul; iSplit < geometry.edges->GetSplitNodes().size(); ++iSplit)
        for (auto iDim = 0; iDim < nDim; ++iDim) maxE = std::max(maxE, fabs(geometry.edges->GetSplitE()(iSplit, iDim)));
      CHECK(maxE == 0.0);
      CHECK(GlobalSum(geometry.edges->GetSplitNodes().size()) > 0.0);
    }
  }
}

TEST_CASE("EDGE_FORM_P1_MIXED: moved viscous test vectors of tetrahedra next to prisms and pyramids", "[P1Flux]") {
  /*--- P1_MIXED.md 5.3: Y_T with xi_o = 0 on the faces a tetrahedron shares with a prism or a pyramid. Then the
   * assembled test vectors, (|T| I + Y_T^T) g_a on the tetrahedra and omega_t |t| g_a on the sub-simplices of the
   * split averages, sum to zero at interior nodes (constant fluxes are exact, MV1(e)). ---*/
  for (double stretch : {1.0, 1e-3}) {
    const std::string mesh = "p1_mixed_moved_3d.su2";
    WriteMixedMesh(mesh, 3, 5, 0.3, stretch);
    MeshCase test(mesh, MixedOptions);
    auto& geometry = *test.geometry;
    geometry.SetEdgeFormP1(true);
    geometry.SetEdgeFormBlend(true, true);
    const auto& blendY = geometry.edges->GetBlendY();

    std::vector<std::array<su2double, 3>> S(geometry.GetnPoint(), {0.0, 0.0, 0.0});
    su2double scale = 0.0, maxY = 0.0;
    for (auto iElem = 0ul; iElem < geometry.GetnElem(); ++iElem) {
      const auto* element = geometry.elem[iElem];
      const auto nNodes = element->GetnNodes();
      if (nNodes == 4) {
        su2double x[4][3], grad[4][3];
        for (int a = 0; a < 4; ++a)
          for (int i = 0; i < 3; ++i) x[a][i] = geometry.nodes->GetCoord(element->GetNode(a), i);
        const su2double size = fabs(P1Simplex::ShapeGradients(3, x, grad));
        for (int k = 0; k < 9; ++k) maxY = std::max(maxY, fabs(blendY(iElem, k)) / size);
        for (int a = 0; a < 4; ++a) {
          for (int j = 0; j < 3; ++j) {
            su2double nu = size * grad[a][j];
            for (int i = 0; i < 3; ++i) nu += blendY(iElem, 3 * i + j) * grad[a][i];
            S[element->GetNode(a)][j] += nu;
            scale = std::max(scale, fabs(size * grad[a][j]));
          }
        }
        continue;
      }
      for (const auto& t : geometry.GetMixedSimplices(element->GetVTK_Type())) {
        su2double x[4][3], grad[4][3];
        for (int a = 0; a < 4; ++a)
          for (int i = 0; i < 3; ++i) x[a][i] = geometry.nodes->GetCoord(element->GetNode(t.nodes[a]), i);
        const su2double size = fabs(P1Simplex::ShapeGradients(3, x, grad));
        for (int a = 0; a < 4; ++a)
          for (int j = 0; j < 3; ++j) S[element->GetNode(t.nodes[a])][j] += t.weight * size * grad[a][j];
      }
    }
    su2double local = 0.0;
    for (auto iPoint = 0ul; iPoint < geometry.GetnPointDomain(); ++iPoint) {
      if (geometry.nodes->GetPhysicalBoundary(iPoint)) continue;
      for (int j = 0; j < 3; ++j) local = std::max(local, fabs(S[iPoint][j]));
    }
    INFO("stretch " << stretch);
    CHECK(GlobalMax(local) < 1e-10 * GlobalMax(scale));
    CHECK(GlobalMax(maxY) > 1e-3);
  }
}

TEST_CASE("EDGE_FORM_P1_MIXED: central part and entropy production of the average are the means of the splits", "[P1Flux]") {
  /*--- Theorem X4: Ismail-Roe is linear in its normal, the E production is linear in E, so on one cell the averaged
   * operator gives the weighted mean of the split residuals and productions, for any W (here random delta). ---*/
  std::mt19937 gen(9753);
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  for (int nDim : {2, 3}) {
    const std::string mesh = "p1_mixed_x4_" + std::to_string(nDim) + "d.su2";
    WriteMixedMesh(mesh, nDim, (nDim == 2) ? 4 : 3, 0.0, 1.0);
    MeshCase test(mesh, MixedOptions);
    auto& geometry = static_cast<CPhysicalGeometry&>(*test.geometry);
    for (const int vtk : MixedTypes) {
      if ((vtk == QUADRILATERAL) != (nDim == 2)) continue;
      const auto cell = MakeCell(vtk);
      const auto& list = geometry.SetMixedSimplices(cell.get());
      const auto splits = KeptSplits(geometry, *cell, nDim);
      INFO("VTK type " << vtk);
      for (int draw = 0; draw < 40; ++draw) {
        const auto x = CellCoords(gen, vtk, 0.12, (draw % 2) ? 1e-2 : 1.0);
        std::vector<ale::State> state(x.size());
        for (auto& S : state) S = RandomAleState(gen, nDim);

        /*--- Random W = m_t + delta per distinct sub-simplex, sum_a delta_a = 0. ---*/
        std::map<std::array<unsigned short, 4>, std::array<su2double, 4>> delta;
        SubSimplices sub(nDim, list, x);
        for (size_t t = 0; t < list.size(); ++t) {
          std::array<su2double, 4> d = {0.0, 0.0, 0.0, 0.0};
          su2double mean = 0.0;
          for (int a = 0; a <= nDim; ++a) {
            d[a] = 0.3 * u(gen) * sub.size[t] / (nDim + 1);
            mean += d[a] / (nDim + 1);
          }
          for (int a = 0; a <= nDim; ++a) d[a] -= mean;
          delta[SimplexKey(list[t], nDim)] = d;
        }

        auto residual = [&](const PairMap& pairs, std::vector<std::array<su2double, 5>>& R) {
          for (const auto& pair : pairs) {
            su2double F[5];
            ale::IsmailRoeFlux(nDim, kGamma, state[pair.first.first], state[pair.first.second], pair.second.data(), F);
            for (int v = 0; v < nDim + 2; ++v) {
              R[pair.first.first][v] += F[v];
              R[pair.first.second][v] -= F[v];
            }
          }
        };
        auto production = [&](const PairMap& pieces) {
          su2double P = 0.0;
          for (const auto& pair : pieces) {
            su2double p = 0.0, d = 0.0;
            PairEntropy(nDim, state[pair.first.first], state[pair.first.second], pair.second.data(), p, d);
            P += p;
          }
          return P;
        };

        std::vector<std::array<su2double, 5>> Ravg(x.size(), {0, 0, 0, 0, 0}), Rmean(x.size(), {0, 0, 0, 0, 0});
        residual(MixedPairs(nDim, list, x), Ravg);
        const su2double Pavg = production(MixedPairs(nDim, list, x, nullptr, &delta));
        su2double Pmean = 0.0, Pscale = 0.0;
        for (const auto& split : splits) {
          std::vector<std::array<su2double, 5>> Rs(x.size(), {0, 0, 0, 0, 0});
          residual(MixedPairs(nDim, split, x), Rs);
          for (size_t i = 0; i < x.size(); ++i)
            for (int v = 0; v < nDim + 2; ++v) Rmean[i][v] += Rs[i][v] / splits.size();
          const su2double Ps = production(MixedPairs(nDim, split, x, nullptr, &delta));
          Pmean += Ps / splits.size();
          Pscale += fabs(Ps) / splits.size();
        }
        su2double err = 0.0, rScale = 0.0;
        for (size_t i = 0; i < x.size(); ++i) {
          for (int v = 0; v < nDim + 2; ++v) {
            err = std::max(err, fabs(Ravg[i][v] - Rmean[i][v]));
            rScale = std::max(rScale, fabs(Rmean[i][v]));
          }
        }
        CHECK(err < 1e-12 * rScale);
        CHECK(fabs(Pavg - Pmean) < 1e-12 * (Pscale + fabs(Pmean)));

        /*--- M0 (delta = 0): no E part, no production. ---*/
        for (auto& entry : delta) entry.second = {0.0, 0.0, 0.0, 0.0};
        CHECK(fabs(production(MixedPairs(nDim, list, x, nullptr, &delta))) == 0.0);
      }
    }
  }
}

TEST_CASE("EDGE_FORM_P1_MIXED: one theta per cell on the averaged pieces", "[P1Flux]") {
  /*--- Theorem X5 for any W: theta_C = P_C^+ / D_C in [0,1], cell production P_C - theta_C D_C = min(P_C, 0); the
   * averaged floor and the positive part are below the means of the split ones. With M0 the code has P_C = 0. ---*/
  std::mt19937 gen(8642);
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  for (int nDim : {2, 3}) {
    const std::string mesh = "p1_mixed_x5_" + std::to_string(nDim) + "d.su2";
    WriteMixedMesh(mesh, nDim, (nDim == 2) ? 4 : 3, 0.0, 1.0);
    MeshCase test(mesh, MixedOptions);
    auto& geometry = static_cast<CPhysicalGeometry&>(*test.geometry);
    for (const int vtk : MixedTypes) {
      if ((vtk == QUADRILATERAL) != (nDim == 2)) continue;
      const auto cell = MakeCell(vtk);
      const auto& list = geometry.SetMixedSimplices(cell.get());
      const auto splits = KeptSplits(geometry, *cell, nDim);
      INFO("VTK type " << vtk);
      int nActive = 0;
      for (int draw = 0; draw < 200; ++draw) {
        const auto x = CellCoords(gen, vtk, 0.12, (draw % 2) ? 1e-2 : 1.0);
        std::vector<ale::State> state(x.size());
        for (auto& S : state) S = RandomAleState(gen, nDim, 0.3);
        std::map<std::array<unsigned short, 4>, std::array<su2double, 4>> delta;
        SubSimplices sub(nDim, list, x);
        for (size_t t = 0; t < list.size(); ++t) {
          std::array<su2double, 4> d = {0.0, 0.0, 0.0, 0.0};
          su2double mean = 0.0;
          for (int a = 0; a <= nDim; ++a) {
            d[a] = 0.5 * u(gen) * sub.size[t] / (nDim + 1);
            mean += d[a] / (nDim + 1);
          }
          for (int a = 0; a <= nDim; ++a) d[a] -= mean;
          delta[SimplexKey(list[t], nDim)] = d;
        }
        auto budget = [&](const std::vector<P1Mixed::Simplex>& simplices, su2double& P, su2double& D) {
          P = 0.0;
          D = 0.0;
          for (const auto& pair : MixedPairs(nDim, simplices, x, nullptr, &delta)) {
            su2double p = 0.0, d = 0.0;
            PairEntropy(nDim, state[pair.first.first], state[pair.first.second], pair.second.data(), p, d);
            P += p;
            D += d;
          }
        };
        su2double P = 0.0, D = 0.0, meanD = 0.0, meanPplus = 0.0;
        budget(list, P, D);
        for (const auto& split : splits) {
          su2double Ps = 0.0, Ds = 0.0;
          budget(split, Ps, Ds);
          CHECK(Ps <= Ds * (1.0 + 1e-12));
          meanD += Ds / splits.size();
          meanPplus += std::max(Ps, su2double(0.0)) / splits.size();
        }
        CHECK(P <= D * (1.0 + 1e-12));
        const su2double theta = (P > 0.0 && D > 0.0) ? std::min(P / D, su2double(1.0)) : 0.0;
        nActive += (theta > 0.0);
        CHECK(theta >= 0.0);
        CHECK(theta <= 1.0);
        CHECK(fabs((P - theta * D) - std::min(P, su2double(0.0))) <= 1e-12 * (fabs(P) + D));
        CHECK(D <= meanD * (1.0 + 1e-12));
        CHECK(std::max(P, su2double(0.0)) <= meanPplus * (1.0 + 1e-12) + 1e-14 * D);
      }
      CHECK(nActive > 0);
    }
  }
}

TEST_CASE("EDGE_FORM_P1_MIXED: closure C on quad faces, closed cells are entropy conservative", "[P1Flux]") {
  /*--- Theorem X10: interior Ismail-Roe on the averaged n_ik (E = 0), closure C with the pressure flux on every face,
   * quad faces on their four sub-triangles with A_tau/2 (P1Mixed::QuadSubNormals). The entropy rate is zero (to the
   * series error of the logarithmic mean), the rest state is exact, and the linear analog on a parallelogram has the
   * weights 9:1:1:1/48. ---*/
  std::mt19937 gen(3141);
  const std::string mesh = "p1_mixed_x10_3d.su2";
  WriteMixedMesh(mesh, 3, 3, 0.0, 1.0);
  MeshCase test(mesh, MixedOptions);
  auto& geometry = static_cast<CPhysicalGeometry&>(*test.geometry);
  for (const int vtk : {HEXAHEDRON, PRISM, PYRAMID}) {
    const auto cell = MakeCell(vtk);
    const auto& list = geometry.SetMixedSimplices(cell.get());
    INFO("VTK type " << vtk);
    for (int draw = 0; draw < 30; ++draw) {
      const auto x = CellCoords(gen, vtk, (draw < 3) ? 0.0 : 0.15, (draw % 2) ? 1e-2 : 1.0);
      const auto nv = x.size();
      const auto pairs = MixedPairs(3, list, x);
      su2double center[3] = {0.0, 0.0, 0.0};
      for (const auto& p : x)
        for (int i = 0; i < 3; ++i) center[i] += p[i] / nv;

      for (const bool rest : {false, true}) {
        std::vector<ale::State> state(nv);
        for (auto& S : state) {
          S = RandomAleState(gen, 3);
          if (rest) S = ale::State{1.2, {0.0, 0.0, 0.0}, 0.8};
        }
        std::vector<std::array<su2double, 5>> R(nv, {0, 0, 0, 0, 0});
        for (const auto& pair : pairs) {
          su2double F[5];
          ale::IsmailRoeFlux(3, kGamma, state[pair.first.first], state[pair.first.second], pair.second.data(), F);
          for (int v = 0; v < 5; ++v) {
            R[pair.first.first][v] += F[v];
            R[pair.first.second][v] -= F[v];
          }
        }
        const su2double zeroVel[3][3] = {{0.0}};
        for (unsigned short iFace = 0; iFace < cell->GetnFaces(); ++iFace) {
          const auto nf = cell->GetnNodesFace(iFace);
          unsigned short fn[4];
          su2double fc[3] = {0.0, 0.0, 0.0}, A[3];
          for (unsigned short k = 0; k < nf; ++k) {
            fn[k] = cell->GetFaces(iFace, k);
            for (int i = 0; i < 3; ++i) fc[i] += x[fn[k]][i] / nf;
          }
          const auto& x0 = x[fn[0]];
          const auto& x1 = x[fn[1]];
          const auto& x2 = x[fn[2]];
          const auto& x3 = x[fn[(nf == 4) ? 3 : 0]];
          su2double a[3], b[3];
          for (int i = 0; i < 3; ++i) {
            a[i] = (nf == 4) ? x2[i] - x0[i] : x1[i] - x0[i];
            b[i] = (nf == 4) ? x3[i] - x1[i] : x2[i] - x0[i];
          }
          A[0] = 0.5 * (a[1] * b[2] - a[2] * b[1]);
          A[1] = 0.5 * (a[2] * b[0] - a[0] * b[2]);
          A[2] = 0.5 * (a[0] * b[1] - a[1] * b[0]);
          su2double orient = 0.0;
          for (int i = 0; i < 3; ++i) orient += A[i] * (fc[i] - center[i]);
          if (orient < 0.0)
            for (auto& value : A) value = -value;

          if (nf == 3) {
            ale::State fs[3] = {state[fn[0]], state[fn[1]], state[fn[2]]};
            su2double res[3][ale::MAXNVAR];
            ale::FaceResidual(3, 3, kGamma, nullptr, A, fs, zeroVel, 0.0, res);
            for (int k = 0; k < 3; ++k)
              for (int v = 0; v < 5; ++v) R[fn[k]][v] += res[k][v];
            continue;
          }
          std::array<const su2double*, 4> X = {x[fn[0]].data(), x[fn[1]].data(), x[fn[2]].data(), x[fn[3]].data()};
          su2double sub[4][3];
          P1Mixed::QuadSubNormals(X, A, sub);

          /*--- The pairs of sub-normals sum to the face area vector; on a parallelogram the linear analog is 9:1:1:1/48. ---*/
          su2double weights[4][4] = {{0.0}}, area = 0.0;
          for (int i = 0; i < 3; ++i) area += A[i] * A[i];
          area = sqrt(area);
          for (int i = 0; i < 3; ++i) {
            CHECK(fabs(sub[0][i] + sub[1][i] - 0.5 * A[i]) < 1e-14 * area);
            CHECK(fabs(sub[2][i] + sub[3][i] - 0.5 * A[i]) < 1e-14 * area);
          }
          for (int tri = 0; tri < 4; ++tri) {
            ale::State fs[3];
            int nodes[3];
            su2double subArea = 0.0;
            for (int i = 0; i < 3; ++i) subArea += sub[tri][i] * sub[tri][i];
            for (int k = 0; k < 3; ++k) {
              nodes[k] = fn[P1Mixed::QuadTriangles[tri][k]];
              fs[k] = state[nodes[k]];
            }
            for (int k = 0; k < 3; ++k)
              for (int l = 0; l < 3; ++l)
                weights[P1Mixed::QuadTriangles[tri][k]][P1Mixed::QuadTriangles[tri][l]] += ((k == l) ? 6.0 : 1.0) / 24.0 * sqrt(subArea) / area;
            su2double res[3][ale::MAXNVAR];
            ale::FaceResidual(3, 3, kGamma, nullptr, sub[tri], fs, zeroVel, 0.0, res);
            for (int k = 0; k < 3; ++k)
              for (int v = 0; v < 5; ++v) R[nodes[k]][v] += res[k][v];
          }
          if (draw < 3) {
            for (int k = 0; k < 4; ++k)
              for (int l = 0; l < 4; ++l) CHECK(fabs(weights[k][l] - ((k == l) ? 9.0 : 1.0) / 48.0) < 1e-13);
          }
        }

        su2double rate = 0.0, rateScale = 0.0, maxR = 0.0, scale = 0.0;
        for (size_t i = 0; i < nv; ++i) {
          su2double w[5];
          ale::EntropyVariables(3, kGamma, state[i], w);
          for (int v = 0; v < 5; ++v) {
            rate += w[v] * R[i][v];
            rateScale += fabs(w[v] * R[i][v]);
            maxR = std::max(maxR, fabs(R[i][v]));
          }
          scale = std::max(scale, state[i].p);
        }
        if (rest) {
          su2double faceScale = 0.0;
          for (const auto& pair : pairs)
            for (int i = 0; i < 3; ++i) faceScale = std::max(faceScale, fabs(pair.second[i]));
          CHECK(maxR < 1e-13 * scale * faceScale);
        } else {
          CHECK(fabs(rate) < 1e-9 * rateScale);
        }
      }
    }
  }
}

TEST_CASE("EDGE_FORM_P1_MIXED: weak no-slip SIPG bound on quad wall faces", "[P1Flux]") {
  /*--- P1_MIXED.md 6.2: SIPG on each sub-simplex t over a wall sub-triangle, with omega_t, h = d|t|/|tau| and m_t the
   * wall faces of t. W_t <= D_t/c holds per sub-simplex (Theorem N6), so for the cell sum_t omega_t W_t <= D/c, D the
   * omega-weighted Galerkin dissipation of the cell. The wall sub-triangles carry total weight 1/2 per diagonal. ---*/
  std::mt19937 gen(2357);
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  const std::string mesh = "p1_mixed_wall_3d.su2";
  WriteMixedMesh(mesh, 3, 3, 0.0, 1.0);
  MeshCase test(mesh, MixedOptions);
  auto& geometry = static_cast<CPhysicalGeometry&>(*test.geometry);
  const su2double c = pve::NOSLIP_PENALTY;
  for (const int vtk : {HEXAHEDRON, PRISM}) {
    const auto cell = MakeCell(vtk);
    const auto& list = geometry.SetMixedSimplices(cell.get());
    /*--- A quad face of the cell as the wall: hexahedron bottom 0-3-2-1, prism side 3-4-1-0. ---*/
    const unsigned short wallFace = (vtk == HEXAHEDRON) ? 4 : 0;
    std::vector<unsigned short> wall;
    for (unsigned short k = 0; k < 4; ++k) wall.push_back(cell->GetFaces(wallFace, k));
    auto onWall = [&](unsigned short node) { return std::find(wall.begin(), wall.end(), node) != wall.end(); };
    INFO("VTK type " << vtk);

    su2double worst = -1e300;
    for (int trial = 0; trial < 300; ++trial) {
      const bool isothermal = (trial % 2 == 1);
      const auto x = CellCoords(gen, vtk, (trial % 3 == 0) ? 0.0 : 0.15, (trial % 3 == 2) ? 1e-3 : 1.0);
      std::vector<pve::NodeState> cellState(x.size());
      for (auto& S : cellState) {
        S.rho = exp(u(gen));
        S.p = S.rho * kGas * exp(u(gen));
        for (int i = 0; i < 3; ++i) S.vel[i] = 1.5 * u(gen);
        S.mu = exp(2.0 * u(gen) - 1.0);
        S.kappa = exp(2.0 * u(gen));
      }
      const su2double wallTheta = 1.0 / (kGas * exp(u(gen)));

      su2double W = 0.0, D = 0.0, wallWeight = 0.0;
      for (const auto& t : list) {
        const su2double* coord[4];
        pve::NodeState state[4];
        for (int a = 0; a < 4; ++a) {
          coord[a] = x[t.nodes[a]].data();
          state[a] = cellState[t.nodes[a]];
        }
        pve::Element E;
        REQUIRE(E.Set(3, coord, state, kGas) > 0.0);
        const su2double Dt = E.GalerkinDissipation();
        D += t.weight * Dt;

        unsigned short nWallFaces = 0;
        for (unsigned short opp = 0; opp < 4; ++opp) {
          bool all = true;
          for (unsigned short a = 0; a < 4; ++a)
            if (a != opp) all = all && onWall(t.nodes[a]);
          nWallFaces += all;
        }
        su2double Wt = 0.0;
        for (unsigned short opp = 0; opp < 4; ++opp) {
          unsigned short faceNodes[3], k = 0;
          bool all = true;
          for (unsigned short a = 0; a < 4; ++a) {
            if (a == opp) continue;
            all = all && onWall(t.nodes[a]);
            faceNodes[k++] = a;
          }
          if (!all) continue;
          if (trial == 0) wallWeight += t.weight;
          su2double N[3];
          pve::WallFaceNormal(3, coord, opp, faceNodes, N);
          Wt += pve::NoSlipWallProduction(E, 3, faceNodes, N, isothermal, wallTheta, nWallFaces, pve::NOSLIP_SIPG, c);
        }
        CHECK(Wt <= Dt / c * (1.0 + 1e-10) + 1e-14);
        W += t.weight * Wt;
      }
      if (trial == 0) CHECK(fabs(wallWeight - 2.0) < 1e-14);  /*--- four sub-triangles, 1/2 each. ---*/
      CHECK(W <= D / c * (1.0 + 1e-10));
      worst = std::max(worst, (W - D / c) / D);
    }
    CHECK(worst <= 0.0);
  }
}

TEST_CASE("EDGE_FORM_P1_MIXED: the Kuhn subset of a brick has dihedral angles <= 90 degrees", "[P1Flux]") {
  /*--- Lemma X9(e): on bricks of any axis ratios the 24 sub-tetrahedra of the hexahedron list are orthoschemes. The
   * full pulling family holds slivers (Lemma X8). ---*/
  std::mt19937 gen(1729);
  std::uniform_real_distribution<double> u(-1.0, 1.0), logScale(-3.0, 3.0);
  const std::string mesh = "p1_mixed_kuhn_3d.su2";
  WriteMixedMesh(mesh, 3, 3, 0.0, 1.0);
  MeshCase test(mesh, MixedOptions);
  auto& geometry = static_cast<CPhysicalGeometry&>(*test.geometry);
  const auto cell = MakeCell(HEXAHEDRON);
  const auto& list = geometry.SetMixedSimplices(cell.get());

  auto maxDihedral = [](const su2double* const* p) {
    su2double n[4][3];
    for (int o = 0; o < 4; ++o) {
      int f[3], k = 0;
      for (int a = 0; a < 4; ++a)
        if (a != o) f[k++] = a;
      su2double a[3], b[3];
      for (int i = 0; i < 3; ++i) {
        a[i] = p[f[1]][i] - p[f[0]][i];
        b[i] = p[f[2]][i] - p[f[0]][i];
      }
      su2double cr[3] = {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
      su2double dir = 0.0;
      for (int i = 0; i < 3; ++i) dir += cr[i] * (p[f[0]][i] - p[o][i]);
      const su2double len = sqrt(cr[0] * cr[0] + cr[1] * cr[1] + cr[2] * cr[2]) * ((dir < 0.0) ? -1.0 : 1.0);
      for (int i = 0; i < 3; ++i) n[o][i] = cr[i] / len;
    }
    su2double worst = 0.0;
    for (int a = 0; a < 4; ++a) {
      for (int b = a + 1; b < 4; ++b) {
        su2double dot = 0.0;
        for (int i = 0; i < 3; ++i) dot += n[a][i] * n[b][i];
        worst = std::max(worst, 180.0 - acos(std::max(su2double(-1.0), std::min(su2double(1.0), dot))) * 180.0 / M_PI);
      }
    }
    return worst;
  };

  su2double worstKuhn = 0.0, worstAll = 0.0;
  for (int trial = 0; trial < 2000; ++trial) {
    const su2double s[3] = {pow(10.0, logScale(gen)), pow(10.0, logScale(gen)), pow(10.0, logScale(gen))};
    const su2double angle[2] = {3.0 * u(gen), 3.0 * u(gen)};
    std::vector<std::array<su2double, 3>> x = {{0, 0, 0}, {1, 0, 0}, {1, 1, 0}, {0, 1, 0},
                                                {0, 0, 1}, {1, 0, 1}, {1, 1, 1}, {0, 1, 1}};
    for (auto& p : x) {
      for (int i = 0; i < 3; ++i) p[i] *= s[i];
      su2double y = cos(angle[0]) * p[0] - sin(angle[0]) * p[1], z = sin(angle[0]) * p[0] + cos(angle[0]) * p[1];
      p[0] = y;
      p[1] = z;
      y = cos(angle[1]) * p[1] - sin(angle[1]) * p[2];
      z = sin(angle[1]) * p[1] + cos(angle[1]) * p[2];
      p[1] = y;
      p[2] = z;
    }
    for (const auto& t : list) {
      const su2double* p[4] = {x[t.nodes[0]].data(), x[t.nodes[1]].data(), x[t.nodes[2]].data(), x[t.nodes[3]].data()};
      worstKuhn = std::max(worstKuhn, maxDihedral(p));
    }
    for (const auto& pattern : geometry.GetSplitPatterns(cell.get())) {
      for (const auto& t : pattern.simplices) {
        const su2double* p[4] = {x[t[0]].data(), x[t[1]].data(), x[t[2]].data(), x[t[3]].data()};
        worstAll = std::max(worstAll, maxDihedral(p));
      }
    }
  }
  CHECK(worstKuhn <= 90.0 + 1e-6);
  CHECK(worstAll > 170.0);
}

/*--- EDGE_FORM_TECNO_ADJOINT and EDGE_FORM_EC_EXACT_JACOBIAN (P1_TECNO_CONVERGENCE.md, 3.2 and 5.1). The residual of
 * TeCNOAdjointChecks::Residual is the one of the edge loop (Delta part) plus the transpose loop of the solver, on all
 * nodes, so these cases run in serial only (no halo copies of G, Y and phi). ---*/

namespace {
template <size_t nDim>
void TeCNOAdjointEntropyCase() {
  const std::string mesh = "tecno_adj_entropy_" + std::to_string(nDim) + "d.su2";
  WriteSimplexMesh(mesh, nDim, (nDim == 2) ? 6 : 3, 0.4, 1.0);
  MeshCase test(mesh);
  const auto& geometry = *test.geometry;
  const auto r = TeCNOAdjointChecks::EntropyCheck<nDim>(geometry, geometry.GetnPoint(), 100, 5);
  INFO("nDim " << nDim << ": identity " << r.identity << ", production " << r.production << ", conservation "
               << r.conservation);
  CHECK(r.identity < 1e-12);
  CHECK(r.production < 0.0);
  CHECK(r.conservation < 1e-12);
}

template <size_t nDim>
void TeCNOAdjointLinearizationCase() {
  const std::string mesh = "tecno_adj_lin_" + std::to_string(nDim) + "d.su2";
  WriteSimplexMesh(mesh, nDim, (nDim == 2) ? 3 : 2, 0.4, 1.0);
  MeshCase test(mesh);
  const auto& geometry = *test.geometry;
  const auto r = TeCNOAdjointChecks::LinearizationCheck<nDim>(geometry, geometry.GetnPoint(), 8, 9);
  INFO("nDim " << nDim << ": asymmetry " << r.asymmetry << ", negative " << r.negative << ", majorant " << r.majorant
               << ", first ring " << r.firstRing << " (edge part " << r.firstRingEdge << ", spread " << r.firstRingSpread
               << ")");
  CHECK(r.asymmetry < 1e-12);
  CHECK(r.negative < 1e-12);
  CHECK(r.majorant < 1e-12);
  CHECK(r.firstRing < 1e-12);
  CHECK(r.firstRingEdge < 1e-12);
}

template <size_t nDim>
void TeCNOAdjointJumpCase() {
  /*--- Linear w: L_e = 0 between interior nodes of a jittered median dual (GG is linear exact there). ---*/
  {
    const std::string mesh = "tecno_adj_linear_" + std::to_string(nDim) + "d.su2";
    WriteSimplexMesh(mesh, nDim, (nDim == 2) ? 8 : 4, 0.4, 1.0);
    MeshCase test(mesh);
    const auto& geometry = *test.geometry;
    su2activematrix w(geometry.GetnPoint(), nDim + 2);
    for (auto i = 0ul; i < geometry.GetnPoint(); ++i) {
      for (size_t v = 0; v < nDim + 2; ++v) {
        su2double val = 0.3 + v;
        for (size_t d = 0; d < nDim; ++d) val += (1.0 + v - 0.7 * d) * geometry.nodes->GetCoord(i, d);
        w(i, v) = val;
      }
    }
    auto interior = [&](unsigned long i) { return !geometry.nodes->GetBoundary(i); };
    const auto err = TeCNOAdjointChecks::MaxJump<nDim>(geometry, w, interior, true);
    INFO("nDim " << nDim << ": linear field, max |L|/|dw| " << err);
    CHECK(err < 1e-10);
  }
  /*--- Smooth w on lattice meshes (centrally symmetric stencils): L_e = O(h^3). ---*/
  std::vector<double> maxJump;
  for (unsigned long n : (nDim == 2) ? std::vector<unsigned long>{8, 16, 32} : std::vector<unsigned long>{4, 8}) {
    const std::string mesh = "tecno_adj_order_" + std::to_string(nDim) + "d_" + std::to_string(n) + ".su2";
    WriteSimplexMesh(mesh, nDim, n, 0.0, 1.0);
    MeshCase test(mesh);
    const auto& geometry = *test.geometry;
    su2activematrix w(geometry.GetnPoint(), nDim + 2);
    for (auto i = 0ul; i < geometry.GetnPoint(); ++i) {
      const auto* x = geometry.nodes->GetCoord(i);
      for (size_t v = 0; v < nDim + 2; ++v) {
        w(i, v) = sin(2.0 * x[0] + 0.5 * v) * cos(1.5 * x[1] - 0.3 * v) + 0.2 * v * x[0] * x[1] +
                  ((nDim == 3) ? sin(x[nDim - 1] + 0.2 * v) : 0.0);
      }
    }
    auto interior = [&](unsigned long i) { return !geometry.nodes->GetBoundary(i); };
    maxJump.push_back(TeCNOAdjointChecks::MaxJump<nDim>(geometry, w, interior, false));
  }
  const double order = log2(maxJump[maxJump.size() - 2] / maxJump.back());
  INFO("nDim " << nDim << ": order of the jump " << order);
  CHECK(order > 2.6);
}
}  // namespace

TEST_CASE("EDGE_FORM_TECNO_ADJOINT: entropy identity, sign and conservation for random states and weights", "[P1Flux]") {
  if (SU2_MPI::GetSize() > 1) return;
  TeCNOAdjointEntropyCase<2>();
  TeCNOAdjointEntropyCase<3>();
}

TEST_CASE("EDGE_FORM_TECNO_ADJOINT: symmetric PSD linearization and the Lemma A4 majorant", "[P1Flux]") {
  if (SU2_MPI::GetSize() > 1) return;
  TeCNOAdjointLinearizationCase<2>();
  TeCNOAdjointLinearizationCase<3>();
}

TEST_CASE("EDGE_FORM_TECNO_ADJOINT: the jump C_e w is linear exact and third order", "[P1Flux]") {
  if (SU2_MPI::GetSize() > 1) return;
  TeCNOAdjointJumpCase<2>();
  TeCNOAdjointJumpCase<3>();
}

/*--- TECNO_ADJOINT_GRADIENT= P1_AVERAGE (P1_TECNO_ADJ_GRADIENT.md, G2/G3): weights and volumes, linear exactness at every
 * node of jittered, stretched and curved-wall meshes on both duals (1/2/4 ranks, owned nodes and the edges between
 * owned nodes), the median-dual identity with Green-Gauss, the transpose, Theorem A1 and Lemma A4 (serial). ---*/

namespace {
const char* const P1AverageMoved = "MOD_CENTROIDS= YES\nCONV_NUM_METHOD_FLOW= JST\nEDGE_FORM_P1= YES\n";

/*--- Bend the box: x -> angle, the bottom (y = 0 in 2D, z = 0 in 3D) becomes an arc of radius 2. ---*/
void BendMesh2D(double* p) {
  const double angle = 0.8 * (p[0] - 0.5), r = 2.0 + p[1];
  p[0] = r * sin(angle);
  p[1] = r * cos(angle) - 2.0;
}
void BendMesh3D(double* p) {
  const double angle = 0.8 * (p[0] - 0.5), r = 2.0 + p[2];
  p[0] = r * sin(angle);
  p[2] = r * cos(angle) - 2.0;
}

struct P1AverageErrors {
  double volume = 0.0;   /*!< \brief max |Vhat - V| / V. */
  double identity = 0.0; /*!< \brief max |sum_k a_ik (x) e_ik - Vhat I| / Vhat. */
  double linear = 0.0;   /*!< \brief max |G w - A| / max|A| for w = A x + b. */
  double constant = 0.0; /*!< \brief max |G 1|. */
  double jump = 0.0;     /*!< \brief max |L_e| / max |w_k - w_i| for the linear w, edges between owned nodes. */
  double minShare = 0.0; /*!< \brief min Vhat / V (positivity of the shares). */
};

template <size_t nDim>
P1AverageErrors P1AverageExactness(CGeometry& geometry) {
  constexpr size_t nVar = nDim + 2;
  const TeCNOAdjointChecks::P1Average p1(geometry);
  const auto nPoint = geometry.GetnPoint();
  P1AverageErrors local;
  local.minShare = 1e300;

  su2double A[nVar][nDim];
  for (size_t v = 0; v < nVar; ++v)
    for (size_t d = 0; d < nDim; ++d) A[v][d] = 1.0 + v - 0.7 * d + 0.3 * v * d;
  su2activematrix w(nPoint, nVar), one(nPoint, nVar);
  double maxA = 0.0;
  for (size_t v = 0; v < nVar; ++v)
    for (size_t d = 0; d < nDim; ++d) maxA = std::max(maxA, fabs(A[v][d]));
  for (auto i = 0ul; i < nPoint; ++i) {
    for (size_t v = 0; v < nVar; ++v) {
      su2double val = 0.3 + v;
      for (size_t d = 0; d < nDim; ++d) val += A[v][d] * geometry.nodes->GetCoord(i, d);
      w(i, v) = val;
      one(i, v) = 1.0;
    }
  }
  CVectorOfMatrix grad(nPoint, nVar, nDim, 0.0), gradOne(nPoint, nVar, nDim, 0.0);
  TeCNOAdjoint::GradientP1(geometry, nDim, nVar, p1.weights, p1.volume, w, grad);
  TeCNOAdjoint::GradientP1(geometry, nDim, nVar, p1.weights, p1.volume, one, gradOne);

  for (auto i = 0ul; i < geometry.GetnPointDomain(); ++i) {
    const su2double V = geometry.nodes->GetVolume(i), Vhat = p1.volume(i);
    local.volume = std::max(local.volume, fabs(Vhat - V) / V);
    local.minShare = std::min(local.minShare, Vhat / V);
    su2double M[3][3] = {{0.0}};
    for (auto iEdge : geometry.nodes->GetEdges(i)) {
      const bool first = (geometry.edges->GetNode(iEdge, 0) == i);
      const auto k = geometry.edges->GetNode(iEdge, first ? 1 : 0);
      for (size_t a = 0; a < nDim; ++a)
        for (size_t b = 0; b < nDim; ++b)
          M[a][b] += p1.weights(iEdge, (first ? 0 : nDim) + a) *
                     (geometry.nodes->GetCoord(k, b) - geometry.nodes->GetCoord(i, b));
    }
    for (size_t a = 0; a < nDim; ++a)
      for (size_t b = 0; b < nDim; ++b) local.identity = std::max(local.identity, fabs(M[b][a] - (a == b) * Vhat) / Vhat);
    for (size_t v = 0; v < nVar; ++v) {
      for (size_t d = 0; d < nDim; ++d) {
        local.linear = std::max(local.linear, fabs(grad(i, v, d) - A[v][d]) / maxA);
        local.constant = std::max(local.constant, fabs(gradOne(i, v, d)));
      }
    }
  }
  for (auto e = 0ul; e < geometry.GetnEdge(); ++e) {
    const auto i = geometry.edges->GetNode(e, 0), k = geometry.edges->GetNode(e, 1);
    if (!geometry.nodes->GetDomain(i) || !geometry.nodes->GetDomain(k)) continue;
    Vector<su2double, nVar> dw, L;
    Vector<su2double, nDim> edge;
    double maxDw = 0.0;
    for (size_t v = 0; v < nVar; ++v) {
      dw(v) = w(k, v) - w(i, v);
      maxDw = std::max(maxDw, fabs(dw(v)));
    }
    for (size_t d = 0; d < nDim; ++d) edge(d) = geometry.nodes->GetCoord(k)[d] - geometry.nodes->GetCoord(i)[d];
    TeCNOAdjoint::EdgeJump(nDim, nVar, dw, grad[i], grad[k], su2double(1.0), su2double(1.0), edge, 1.0, 0.0, L);
    for (size_t v = 0; v < nVar; ++v) local.jump = std::max(local.jump, fabs(L(v)) / maxDw);
  }
  P1AverageErrors global;
  SU2_MPI::Allreduce(&local.volume, &global.volume, 1, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
  SU2_MPI::Allreduce(&local.identity, &global.identity, 1, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
  SU2_MPI::Allreduce(&local.linear, &global.linear, 1, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
  SU2_MPI::Allreduce(&local.constant, &global.constant, 1, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
  SU2_MPI::Allreduce(&local.jump, &global.jump, 1, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
  SU2_MPI::Allreduce(&local.minShare, &global.minShare, 1, MPI_DOUBLE, MPI_MIN, SU2_MPI::GetComm());
  return global;
}

template <size_t nDim>
void P1AverageExactnessCase() {
  struct Mesh {
    const char* tag;
    unsigned long n;
    double jitter, stretch;
    void (*map)(double*);
  };
  const std::vector<Mesh> meshes = {{"jitter", (nDim == 2) ? 8ul : 3ul, 0.4, 1.0, nullptr},
                                    {"stretch", (nDim == 2) ? 8ul : 3ul, 0.3, 1e-3, nullptr},
                                    {"curved", (nDim == 2) ? 8ul : 3ul, 0.3, 0.1, (nDim == 2) ? BendMesh2D : BendMesh3D}};
  for (const auto& m : meshes) {
    for (const bool moved : {false, true}) {
      const std::string mesh = "p1avg_" + std::string(m.tag) + "_" + std::to_string(nDim) + "d.su2";
      WriteSimplexMesh(mesh, nDim, m.n, m.jitter, m.stretch, m.map);
      MeshCase test(mesh, moved ? P1AverageMoved : "");
      const auto r = P1AverageExactness<nDim>(*test.geometry);
      INFO("nDim " << nDim << " mesh " << m.tag << " moved " << moved << ": volume " << r.volume << ", identity "
                   << r.identity << ", linear " << r.linear << ", constant " << r.constant << ", jump " << r.jump
                   << ", min share " << r.minShare);
      CHECK(r.volume < 1e-12);
      CHECK(r.identity < 1e-10);
      CHECK(r.linear < 1e-9);
      CHECK(r.constant == 0.0);
      CHECK(r.jump < 1e-9);
      CHECK(r.minShare > 0.5);
    }
  }
}

/*--- Median dual: G = Green-Gauss at interior nodes for any nodal data (G2 (d)); boundary nodes differ. ---*/
template <size_t nDim>
void P1AverageMedianIdentityCase() {
  constexpr size_t nVar = nDim + 2;
  const std::string mesh = "p1avg_median_" + std::to_string(nDim) + "d.su2";
  WriteSimplexMesh(mesh, nDim, (nDim == 2) ? 8 : 4, 0.4, 1.0);
  MeshCase test(mesh);
  auto& geometry = *test.geometry;
  const TeCNOAdjointChecks::P1Average p1(geometry);
  const auto nPoint = geometry.GetnPoint();
  su2activematrix w(nPoint, nVar);
  for (auto i = 0ul; i < nPoint; ++i) {
    const auto* x = geometry.nodes->GetCoord(i);
    for (size_t v = 0; v < nVar; ++v) w(i, v) = sin(3.0 * x[0] + v) * cos(2.0 * x[1] - 0.5 * v) + x[nDim - 1] * x[0];
  }
  CVectorOfMatrix g1(nPoint, nVar, nDim, 0.0), g2(nPoint, nVar, nDim, 0.0);
  TeCNOAdjoint::GradientP1(geometry, nDim, nVar, p1.weights, p1.volume, w, g1);
  TeCNOAdjoint::Gradient(geometry, nDim, nVar, w, g2);
  double interior = 0.0, boundary = 0.0, scale = 0.0;
  for (auto i = 0ul; i < geometry.GetnPointDomain(); ++i) {
    for (size_t v = 0; v < nVar; ++v) {
      for (size_t d = 0; d < nDim; ++d) {
        scale = std::max(scale, fabs(g2(i, v, d)));
        auto& worst = geometry.nodes->GetBoundary(i) ? boundary : interior;
        worst = std::max(worst, fabs(g1(i, v, d) - g2(i, v, d)));
      }
    }
  }
  double global[3], local[3] = {interior, boundary, scale};
  SU2_MPI::Allreduce(local, global, 3, MPI_DOUBLE, MPI_MAX, SU2_MPI::GetComm());
  INFO("nDim " << nDim << ": interior " << global[0] / global[2] << ", boundary " << global[1] / global[2]);
  CHECK(global[0] / global[2] < 1e-12);
  CHECK(global[1] / global[2] > 1e-3);
}

/*--- <G w, Z> = <w, G^T Z> with G^T Z = AddTransposeP1(Y = Z / Vhat, gam = 1), serial. ---*/
template <size_t nDim>
void P1AverageTransposeCase() {
  constexpr size_t nVar = nDim + 2;
  for (const bool moved : {false, true}) {
    const std::string mesh = "p1avg_transpose_" + std::to_string(nDim) + "d.su2";
    WriteSimplexMesh(mesh, nDim, (nDim == 2) ? 6 : 3, 0.4, 0.2, (nDim == 2) ? BendMesh2D : BendMesh3D);
    MeshCase test(mesh, moved ? P1AverageMoved : "");
    auto& geometry = *test.geometry;
    const TeCNOAdjointChecks::P1Average p1(geometry);
    const auto nPoint = geometry.GetnPoint();
    std::mt19937 gen(17);
    std::uniform_real_distribution<double> r(-1.0, 1.0);
    su2activematrix w(nPoint, nVar), R(nPoint, nVar);
    CVectorOfMatrix Z(nPoint, nVar, nDim, 0.0), Y(nPoint, nVar, nDim, 0.0), G(nPoint, nVar, nDim, 0.0);
    R = su2double(0.0);
    for (auto i = 0ul; i < nPoint; ++i) {
      for (size_t v = 0; v < nVar; ++v) {
        w(i, v) = r(gen);
        for (size_t d = 0; d < nDim; ++d) {
          Z(i, v, d) = r(gen);
          Y(i, v, d) = Z(i, v, d) / p1.volume(i);
        }
      }
    }
    TeCNOAdjoint::GradientP1(geometry, nDim, nVar, p1.weights, p1.volume, w, G);
    TeCNOAdjoint::AddTransposeP1(geometry, nDim, nVar, p1.weights, Y, 1.0, R);
    double lhs = 0.0, rhs = 0.0, absLhs = 0.0;
    for (auto i = 0ul; i < nPoint; ++i) {
      for (size_t v = 0; v < nVar; ++v) {
        rhs += w(i, v) * R(i, v);
        for (size_t d = 0; d < nDim; ++d) {
          lhs += G(i, v, d) * Z(i, v, d);
          absLhs += fabs(G(i, v, d) * Z(i, v, d));
        }
      }
    }
    INFO("nDim " << nDim << " moved " << moved << ": <Gw,Z> " << lhs << ", <w,G^T Z> " << rhs);
    CHECK(fabs(lhs - rhs) < 1e-12 * absLhs);
  }
}

template <size_t nDim>
void P1AverageEntropyCase() {
  for (const bool moved : {false, true}) {
    const std::string mesh = "p1avg_entropy_" + std::to_string(nDim) + "d.su2";
    WriteSimplexMesh(mesh, nDim, (nDim == 2) ? 6 : 3, 0.4, 0.2, (nDim == 2) ? BendMesh2D : BendMesh3D);
    MeshCase test(mesh, moved ? P1AverageMoved : "");
    auto& geometry = *test.geometry;
    const TeCNOAdjointChecks::P1Average p1(geometry);
    const auto r = TeCNOAdjointChecks::EntropyCheck<nDim>(geometry, geometry.GetnPoint(), 60, 7, &p1);
    INFO("nDim " << nDim << " moved " << moved << ": identity " << r.identity << ", production " << r.production
                 << ", conservation " << r.conservation);
    CHECK(r.identity < 1e-12);
    CHECK(r.production < 0.0);
    CHECK(r.conservation < 1e-12);
  }
}

template <size_t nDim>
void P1AverageLinearizationCase() {
  for (const bool moved : {false, true}) {
    const std::string mesh = "p1avg_lin_" + std::to_string(nDim) + "d.su2";
    WriteSimplexMesh(mesh, nDim, (nDim == 2) ? 3 : 2, 0.4, 0.2, (nDim == 2) ? BendMesh2D : BendMesh3D);
    MeshCase test(mesh, moved ? P1AverageMoved : "");
    auto& geometry = *test.geometry;
    const TeCNOAdjointChecks::P1Average p1(geometry);
    const auto r = TeCNOAdjointChecks::LinearizationCheck<nDim>(geometry, geometry.GetnPoint(), 6, 11, &p1);
    INFO("nDim " << nDim << " moved " << moved << ": asymmetry " << r.asymmetry << ", negative " << r.negative
                 << ", majorant " << r.majorant << ", first ring " << r.firstRing << " (edge part " << r.firstRingEdge
                 << ", spread " << r.firstRingSpread << ")");
    CHECK(r.asymmetry < 1e-12);
    CHECK(r.negative < 1e-12);
    CHECK(r.majorant < 1e-12);
    CHECK(r.firstRing < 1e-12);
    CHECK(r.firstRingEdge < 1e-12);
  }
}
}  // namespace

TEST_CASE("TECNO_ADJOINT_GRADIENT= P1_AVERAGE: volumes, weights and linear exactness at every node", "[P1Flux]") {
  P1AverageExactnessCase<2>();
  P1AverageExactnessCase<3>();
}

TEST_CASE("TECNO_ADJOINT_GRADIENT= P1_AVERAGE: equals Green-Gauss at interior nodes of the median dual", "[P1Flux]") {
  P1AverageMedianIdentityCase<2>();
  P1AverageMedianIdentityCase<3>();
}

TEST_CASE("TECNO_ADJOINT_GRADIENT= P1_AVERAGE: transpose, Theorem A1, conservation, Lemma A4 majorant", "[P1Flux]") {
  if (SU2_MPI::GetSize() > 1) return;
  P1AverageTransposeCase<2>();
  P1AverageTransposeCase<3>();
  P1AverageEntropyCase<2>();
  P1AverageEntropyCase<3>();
  P1AverageLinearizationCase<2>();
  P1AverageLinearizationCase<3>();
}

/*--- TECNO_ADJOINT_LIMITER= EDGE_ENTROPY_VA (P1_TECNO_ADJ_LIMITER.md): Theorem A1 with the per-edge-end limiter of the
 * kernel function, linear data and zero fields give phi = 1, Phi = I gives tecnoDissipation, the L4 range. ---*/
namespace {
template <size_t nDim>
void EdgeEntropyVAEntropyCase() {
  for (const bool moved : {false, true}) {
    for (const bool p1Average : {false, true}) {
      const std::string mesh = "tecno_va_entropy_" + std::to_string(nDim) + "d.su2";
      WriteSimplexMesh(mesh, nDim, (nDim == 2) ? 6 : 3, 0.4, 0.2, (nDim == 2) ? BendMesh2D : BendMesh3D);
      MeshCase test(mesh, moved ? P1AverageMoved : "");
      auto& geometry = *test.geometry;
      const TeCNOAdjointChecks::P1Average p1(geometry);
      const auto r = TeCNOAdjointChecks::EntropyCheckVA<nDim>(geometry, geometry.GetnPoint(), 40, 13,
                                                              p1Average ? &p1 : nullptr);
      INFO("nDim " << nDim << " moved " << moved << " P1 average " << p1Average << ": identity " << r.identity
                   << ", production " << r.production << ", conservation " << r.conservation << ", phi in ["
                   << r.phiMin << ", " << r.phiMax << "]");
      CHECK(r.identity < 1e-12);
      CHECK(r.production < 0.0);
      CHECK(r.conservation < 1e-12);
      CHECK(r.phiMax > 1.0);  /*--- Unclipped: values outside [0, 1] occur for random data. ---*/
      CHECK(r.phiMin < 0.0);
    }
  }
}

template <size_t nDim>
void EdgeEntropyVAPropertiesCase() {
  constexpr size_t nVar = nDim + 2;
  std::mt19937 gen(29);
  std::uniform_real_distribution<double> r(-1.0, 1.0);
  double worstLinear = 0.0, worstZero = 0.0, worstIdentity = 0.0;
  for (int t = 0; t < 200; ++t) {
    double u_i[3] = {0.0}, u_k[3] = {0.0};
    for (size_t d = 0; d < nDim; ++d) {
      u_i[d] = 0.5 * r(gen);
      u_k[d] = 0.5 * r(gen);
    }
    CPair<CCompressiblePrimitives<nDim, nDim + 4>> V;
    V.i = TeCNOAdjointChecks::Primitives<nDim>(exp(r(gen)), u_i, exp(r(gen)));
    V.j = TeCNOAdjointChecks::Primitives<nDim>(exp(r(gen)), u_k, exp(r(gen)));
    VectorDbl<nDim> unit;
    double norm = 0.0, n[3];
    for (size_t d = 0; d < nDim; ++d) {
      n[d] = r(gen);
      norm += n[d] * n[d];
    }
    for (size_t d = 0; d < nDim; ++d) unit(d) = n[d] / sqrt(norm);
    const auto avg = roeAveragedVariables(Double(TeCNOAdjointChecks::GAMMA), V, unit);
    const Double lamM = abs(avg.projVel - avg.speedSound) + 0.1, lam0 = abs(avg.projVel) + 0.1;
    const Double lamP = abs(avg.projVel + avg.speedSound) + 0.1, scale = 0.7;
    const double ramp = (t % 2) ? 1.0 : 0.6, kappa = (t % 3 == 0) ? 1.0 / 3.0 : 0.0;
    VectorDbl<nVar> dw, zero, q, q_i, q_k;
    for (size_t v = 0; v < nVar; ++v) {
      dw(v) = r(gen);
      zero(v) = 0.0;
    }
    Double phi[8];
    /*--- Linear data (ramp 1): the slopes of both ends equal the jump, so alpha = Delta and phi = 1. ---*/
    tecnoAdjointEdgeVA(Double(TeCNOAdjointChecks::GAMMA), avg, unit, lamM, lam0, lamP, scale, dw, dw, dw, V.i, V.j,
                       1.0, kappa, q, q_i, q_k, phi);
    for (int a = 0; a < 8; ++a) worstLinear = std::max(worstLinear, fabs(phi[a][0] - 1.0));
    /*--- Zero slopes, kappa = 0: alpha = 0, phi = 1 exactly, q_i = q_k = q = tecnoDissipation(dw). ---*/
    tecnoAdjointEdgeVA(Double(TeCNOAdjointChecks::GAMMA), avg, unit, lamM, lam0, lamP, scale, dw, zero, zero, V.i, V.j,
                       ramp, 0.0, q, q_i, q_k, phi);
    for (int a = 0; a < 8; ++a) worstZero = std::max(worstZero, fabs(phi[a][0] - 1.0));
    const auto ref = tecnoDissipation(Double(TeCNOAdjointChecks::GAMMA), avg, unit, lamM, lam0, lamP, scale, dw);
    double refMax = 1e-300;
    for (size_t v = 0; v < nVar; ++v) refMax = std::max(refMax, fabs(ref(v)[0]));
    for (size_t v = 0; v < nVar; ++v) {
      worstIdentity = std::max(worstIdentity, fabs(q(v)[0] - ref(v)[0]) / refMax);
      worstIdentity = std::max(worstIdentity, fabs(q_i(v)[0] - q(v)[0]) / refMax);
      worstIdentity = std::max(worstIdentity, fabs(q_k(v)[0] - q(v)[0]) / refMax);
    }
    /*--- Zero field: tau/tau = 1, q = 0. ---*/
    tecnoAdjointEdgeVA(Double(TeCNOAdjointChecks::GAMMA), avg, unit, lamM, lam0, lamP, scale, zero, zero, zero, V.i,
                       V.j, ramp, kappa, q, q_i, q_k, phi);
    for (int a = 0; a < 8; ++a) worstZero = std::max(worstZero, fabs(phi[a][0] - 1.0));
    for (size_t v = 0; v < nVar; ++v) worstZero = std::max(worstZero, fabs(q(v)[0]) + fabs(q_i(v)[0]));
  }
  INFO("nDim " << nDim << ": linear |phi - 1| " << worstLinear << ", zero field " << worstZero << ", Phi = I "
               << worstIdentity);
  CHECK(worstLinear < 1e-12);
  CHECK(worstZero == 0.0);
  CHECK(worstIdentity < 1e-12);

  /*--- The computed guard: positive at a uniform state at rest (w_m = u = 0), phi = 1 there for zero data, and
   * rotation invariant (tau and phi of rotated states, normal and momentum components). ---*/
  {
    const double rest[3] = {0.0, 0.0, 0.0};
    CPair<CCompressiblePrimitives<nDim, nDim + 4>> V;
    V.i = TeCNOAdjointChecks::Primitives<nDim>(1.3, rest, 0.8);
    V.j = V.i;
    VectorDbl<nDim> unit;
    for (size_t d = 0; d < nDim; ++d) unit(d) = (d == 0) ? 1.0 : 0.0;
    const auto avg = roeAveragedVariables(Double(TeCNOAdjointChecks::GAMMA), V, unit);
    VectorDbl<nVar> zero, q, q_i, q_k;
    for (size_t v = 0; v < nVar; ++v) zero(v) = 0.0;
    Double phi[8], tau[2];
    tecnoAdjointEdgeVA(Double(TeCNOAdjointChecks::GAMMA), avg, unit, Double(1.0), Double(1.0), Double(1.0), Double(0.5),
                       zero, zero, zero, V.i, V.j, 1.0, 0.0, q, q_i, q_k, phi, nullptr, tau);
    double worst = 0.0;
    for (int a = 0; a < 8; ++a) worst = std::max(worst, fabs(phi[a][0] - 1.0));
    INFO("nDim " << nDim << ": rest state tau " << tau[0][0] << " " << tau[1][0] << ", |phi - 1| " << worst);
    CHECK(tau[0][0] > 0.0);
    CHECK(tau[1][0] > 0.0);
    CHECK(worst == 0.0);
  }
  {
    double Qr[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    if (nDim == 2) {
      Qr[0][0] = cos(0.7); Qr[0][1] = -sin(0.7); Qr[1][0] = sin(0.7); Qr[1][1] = cos(0.7);
    } else {
      const double ax[3] = {1.0 / sqrt(14.0), 2.0 / sqrt(14.0), 3.0 / sqrt(14.0)}, c = cos(0.7), sn = sin(0.7);
      for (int a = 0; a < 3; ++a)
        for (int b = 0; b < 3; ++b) Qr[a][b] = c * (a == b) + (1.0 - c) * ax[a] * ax[b];
      Qr[0][1] -= sn * ax[2]; Qr[1][0] += sn * ax[2];
      Qr[0][2] += sn * ax[1]; Qr[2][0] -= sn * ax[1];
      Qr[1][2] -= sn * ax[0]; Qr[2][1] += sn * ax[0];
    }
    auto rotate = [&](const double* x, double* y) {
      for (size_t a = 0; a < nDim; ++a) {
        y[a] = 0.0;
        for (size_t b = 0; b < nDim; ++b) y[a] += Qr[a][b] * x[b];
      }
    };
    double worstPhi = 0.0, worstTau = 0.0;
    for (int t = 0; t < 50; ++t) {
      double u[2][3] = {{0.0}}, ur[2][3] = {{0.0}}, n[3] = {0.0}, nr[3] = {0.0}, norm = 0.0;
      for (size_t d = 0; d < nDim; ++d) {
        u[0][d] = 0.6 * r(gen);
        u[1][d] = 0.6 * r(gen);
        n[d] = r(gen);
        norm += n[d] * n[d];
      }
      for (size_t d = 0; d < nDim; ++d) n[d] /= sqrt(norm);
      rotate(u[0], ur[0]);
      rotate(u[1], ur[1]);
      rotate(n, nr);
      const double rho[2] = {exp(r(gen)), exp(r(gen))}, p[2] = {exp(r(gen)), exp(r(gen))};
      VectorDbl<nVar> dw[2], pa[2], pb[2];
      for (size_t v = 0; v < nVar; ++v) {
        dw[0](v) = r(gen);
        pa[0](v) = r(gen);
        pb[0](v) = r(gen);
      }
      double m[3][3], mr[3][3];
      for (size_t d = 0; d < nDim; ++d) {
        m[0][d] = dw[0](d + 1)[0];
        m[1][d] = pa[0](d + 1)[0];
        m[2][d] = pb[0](d + 1)[0];
      }
      for (int a = 0; a < 3; ++a) rotate(m[a], mr[a]);
      dw[1] = dw[0];
      pa[1] = pa[0];
      pb[1] = pb[0];
      for (size_t d = 0; d < nDim; ++d) {
        dw[1](d + 1) = mr[0][d];
        pa[1](d + 1) = mr[1][d];
        pb[1](d + 1) = mr[2][d];
      }
      Double phi[2][8], tau[2][2];
      for (int side = 0; side < 2; ++side) {
        CPair<CCompressiblePrimitives<nDim, nDim + 4>> V;
        V.i = TeCNOAdjointChecks::Primitives<nDim>(rho[0], side ? ur[0] : u[0], p[0]);
        V.j = TeCNOAdjointChecks::Primitives<nDim>(rho[1], side ? ur[1] : u[1], p[1]);
        VectorDbl<nDim> unit;
        for (size_t d = 0; d < nDim; ++d) unit(d) = side ? nr[d] : n[d];
        const auto avg = roeAveragedVariables(Double(TeCNOAdjointChecks::GAMMA), V, unit);
        VectorDbl<nVar> q, q_i, q_k;
        tecnoAdjointEdgeVA(Double(TeCNOAdjointChecks::GAMMA), avg, unit, Double(1.0), Double(1.0), Double(1.0),
                           Double(0.5), dw[side], pa[side], pb[side], V.i, V.j, 1.0, 0.0, q, q_i, q_k, phi[side],
                           nullptr, tau[side]);
      }
      for (int a = 0; a < 8; ++a) worstPhi = std::max(worstPhi, fabs(phi[1][a][0] - phi[0][a][0]));
      for (int a = 0; a < 2; ++a) worstTau = std::max(worstTau, fabs(tau[1][a][0] - tau[0][a][0]) / tau[0][a][0]);
    }
    INFO("nDim " << nDim << ": rotation, max |dphi| " << worstPhi << ", max |dtau|/tau " << worstTau);
    CHECK(worstPhi < 1e-10);
    CHECK(worstTau < 1e-12);
  }
}
}  // namespace

TEST_CASE("TECNO_ADJOINT_LIMITER= EDGE_ENTROPY_VA: Theorem A1 and conservation with the per-edge-end limiter", "[P1Flux]") {
  if (SU2_MPI::GetSize() > 1) return;
  EdgeEntropyVAEntropyCase<2>();
  EdgeEntropyVAEntropyCase<3>();
}

TEST_CASE("TECNO_ADJOINT_LIMITER= EDGE_ENTROPY_VA: phi = 1 for linear data and zero fields, Phi = I, range of L4", "[P1Flux]") {
  EdgeEntropyVAPropertiesCase<2>();
  EdgeEntropyVAPropertiesCase<3>();
  /*--- L4: phi alpha = Delta g(t), t = alpha / Delta, g = t(1+t)/(1+t^2) in [(1-sqrt2)/2, (1+sqrt2)/2] (tau = 0). ---*/
  double gMin = 1e300, gMax = -1e300;
  for (int k = -40000; k <= 40000; ++k) {
    const double t = k / 1000.0;
    const double g = t * TeCNOAdjoint::EdgeVanAlbada(1.0, t, t * t, 0.0);
    gMin = std::min(gMin, g);
    gMax = std::max(gMax, g);
  }
  INFO("L4 range [" << gMin << ", " << gMax << "]");
  CHECK(fabs(gMin - 0.5 * (1.0 - sqrt(2.0))) < 1e-6);
  CHECK(fabs(gMax - 0.5 * (1.0 + sqrt(2.0))) < 1e-6);

  /*--- L3: smooth through Delta = alpha = 0 with tau > 0: phi(h d, h a) = 1 - h^2 (a^2 - a d)/tau + O(h^4), so the
   * deviation from 1 falls by 100 when h falls by 10, for any direction (d, a). ---*/
  const double tau = 1e-30;
  double worstRatio = 0.0;
  for (int k = 0; k < 16; ++k) {
    const double d = cos(0.4 * k), a = 1.7 * sin(0.4 * k + 0.3);
    if (fabs(a * a - a * d) < 1e-3) continue;
    auto dev = [&](double h) {
      const double D = h * d, A = h * a;
      return 1.0 - TeCNOAdjoint::EdgeVanAlbada(D * D, A * D, A * A, tau);
    };
    const double h1 = 1e-3 * sqrt(tau), h2 = 1e-4 * sqrt(tau);
    const double expected = h1 * h1 * (a * a - a * d) / tau;
    worstRatio = std::max(worstRatio, fabs(dev(h1) / expected - 1.0));
    worstRatio = std::max(worstRatio, fabs(dev(h1) / dev(h2) / 100.0 - 1.0));
  }
  INFO("L3 smoothness at the origin, worst relative deviation " << worstRatio);
  CHECK(worstRatio < 1e-3);
}

TEST_CASE("EDGE_FORM_EC_EXACT_JACOBIAN: exact Ismail-Roe Jacobian and dw/dU against finite differences", "[P1Flux]") {
  std::mt19937 gen(3);
  double far2 = 0.0, near2 = 0.0, far3 = 0.0, near3 = 0.0, equal = 0.0, hessian = 0.0;
  for (int t = 0; t < 100; ++t) {
    far2 = std::max(far2, TeCNOAdjointChecks::IsmailRoeJacobianError<2>(gen, 1.0));
    near2 = std::max(near2, TeCNOAdjointChecks::IsmailRoeJacobianError<2>(gen, 0.01));
    far3 = std::max(far3, TeCNOAdjointChecks::IsmailRoeJacobianError<3>(gen, 1.0));
    near3 = std::max(near3, TeCNOAdjointChecks::IsmailRoeJacobianError<3>(gen, 0.01));
    equal = std::max(equal, TeCNOAdjointChecks::IsmailRoeJacobianEqualStates<2>(gen));
    equal = std::max(equal, TeCNOAdjointChecks::IsmailRoeJacobianEqualStates<3>(gen));
    hessian = std::max(hessian, TeCNOAdjointChecks::EntropyHessianError(gen, 2));
    hessian = std::max(hessian, TeCNOAdjointChecks::EntropyHessianError(gen, 3));
  }
  INFO("far " << far2 << " " << far3 << ", near " << near2 << " " << near3 << ", equal " << equal << ", dw/dU " << hessian);
  CHECK(std::max(std::max(far2, far3), std::max(near2, near3)) < 1e-6);
  CHECK(equal < 1e-12);
  CHECK(hessian < 1e-6);
}

/*--- POSITIVE_UPDATE_FLOW (P1_POSITIVITY.md): step lengths of P1PositiveUpdate, no solver needed. ---*/

namespace {

namespace ppu = P1PositiveUpdate;

/*--- Factor of CEulerSolver::ComputeUnderRelaxationFactor for one node (the formula of this branch). ---*/
double SU2UnderRelaxation(int nDim, const double* U, const double* dU, double allowableRatio) {
  const int nVar = nDim + 2;
  double ratio = std::fabs(dU[0]) / std::max(U[0], EPS);
  double e_old = U[nVar - 1], e_new = U[nVar - 1] + dU[nVar - 1];
  const double rho_old = std::max(U[0], EPS);
  const double rho_new = std::max(rho_old + dU[0], EPS);
  for (int j = 1; j <= nDim; ++j) {
    e_old -= 0.5 * std::pow(U[j], 2) / rho_old;
    e_new -= 0.5 * std::pow(U[j] + dU[j], 2) / rho_new;
  }
  ratio = std::fmax(ratio, std::fabs(e_new - e_old) / std::max(e_old, EPS));
  double factor = std::fmin(allowableRatio / std::fmax(ratio, EPS), 1.0);
  if (factor < 1e-10) factor = 0.0;
  return factor;
}

/*--- The margins hold up to rounding of the comparison form. ---*/
bool KeepsMargins(int nDim, const double* U0, const double* U, double k) {
  const double q0 = ppu::InternalEnergy(nDim, U0, k);
  const double q = ppu::InternalEnergy(nDim, U, k);
  return (U[0] > 0.0) && (q > 0.0) && (U[0] >= ppu::EPS_RHO * U0[0] * (1.0 - 1e-12)) &&
         (q >= ppu::EPS_Q * q0 * (1.0 - 1e-12));
}

/*--- Brute force: feasible(a) on the exact conditions, a* by bisection (the feasible set is [0, a*]). ---*/
double BruteForceStep(int nDim, const double* U0, const double* dU, double k, double alpha0) {
  const double rho0 = U0[0], q0 = ppu::InternalEnergy(nDim, U0, k);
  auto feasible = [&](double a) {
    double U[ppu::MAXNVAR];
    for (int i = 0; i < nDim + 2; ++i) U[i] = U0[i] + a * dU[i];
    return (U[0] >= ppu::EPS_RHO * rho0) && (ppu::InternalEnergy(nDim, U, k) >= ppu::EPS_Q * q0);
  };
  if (feasible(alpha0)) return alpha0;
  double lo = 0.0, hi = alpha0;
  for (int it = 0; it < 200; ++it) {
    const double mid = 0.5 * (lo + hi);
    if (feasible(mid)) lo = mid; else hi = mid;
  }
  return lo;
}

/*--- Random admissible state: rho, velocity, pressure (gamma = 1.4), k a fraction of q/rho (0 for no SST). ---*/
void RandomState(std::mt19937& gen, int nDim, bool sst, double* U, double& k) {
  std::uniform_real_distribution<double> logu(-3.0, 1.0), vel(-3.0, 3.0), frac(0.0, 0.9);
  const double rho = std::pow(10.0, logu(gen)), p = std::pow(10.0, logu(gen));
  double v2 = 0.0;
  U[0] = rho;
  for (int i = 0; i < nDim; ++i) {
    const double v = vel(gen);
    U[i + 1] = rho * v;
    v2 += v * v;
  }
  const double q = p / 0.4;
  k = sst ? frac(gen) * q / rho : 0.0;
  U[nDim + 1] = q + rho * k + 0.5 * rho * v2;
}

void RandomIncrement(std::mt19937& gen, int nDim, const double* U, double* dU) {
  std::uniform_real_distribution<double> logs(-3.0, 1.5), unit(-1.0, 1.0);
  const double scale = std::pow(10.0, logs(gen));
  for (int i = 0; i < nDim + 2; ++i) dU[i] = scale * unit(gen) * (std::fabs(U[i]) + U[0]);
}

}  // namespace

TEST_CASE("POSITIVE_UPDATE_FLOW: the counterexample of the SU2 factor gives an admissible state", "[P1Flux]") {
  const double U0[3] = {1.0, 1.0, 0.51}, dU[3] = {-2.0, -1.0, -0.5};
  const double zero = 0.0;

  /*--- SU2's factor: rho e < 0 at the accepted state. ---*/
  const double alphaSU2 = SU2UnderRelaxation(1, U0, dU, 0.2);
  CHECK(alphaSU2 == Approx(0.1));
  double U[3];
  for (int i = 0; i < 3; ++i) U[i] = U0[i] + alphaSU2 * dU[i];
  CHECK(ppu::InternalEnergy(1, U, zero) == Approx(-0.04625));
  CHECK_FALSE(ppu::Admissible(1, U, zero, zero, zero));

  /*--- The positive limit: the smallest root of P, 0.0176761..., or its half after the rounded check. ---*/
  const double alpha = ppu::FlowStepLimit(1, U0, dU, zero, alphaSU2);
  INFO("alpha " << alpha);
  CHECK(alpha <= 0.017676104908030614 * (1.0 + 1e-12));
  CHECK(alpha >= 0.5 * 0.017676104908030614 * (1.0 - 1e-12));
  for (int i = 0; i < 3; ++i) U[i] = U0[i] + alpha * dU[i];
  CHECK(ppu::Admissible(1, U, zero, ppu::EPS_RHO * U0[0], ppu::EPS_Q * ppu::InternalEnergy(1, U0, zero)));
}

TEST_CASE("POSITIVE_UPDATE_FLOW: linear and constant cases of P and the stable root formula", "[P1Flux]") {
  /*--- Roots of c0 + c1 a + c2 a^2, c0 > 0. ---*/
  CHECK(ppu::SmallestPositiveRoot(1.0, -2.0, 0.0) == Approx(0.5));
  CHECK(ppu::SmallestPositiveRoot(1.0, 2.0, 0.0) < 0.0);
  CHECK(ppu::SmallestPositiveRoot(1.0, 0.0, 0.0) < 0.0);
  CHECK(ppu::SmallestPositiveRoot(1.0, -3.0, 2.0) == Approx(0.5));
  CHECK(ppu::SmallestPositiveRoot(1.0, 0.0, -1.0) == Approx(1.0));
  CHECK(ppu::SmallestPositiveRoot(1.0, 5.0, -1.0) == Approx((5.0 + std::sqrt(29.0)) / 2.0));
  CHECK(ppu::SmallestPositiveRoot(1.0, 1.0, 1.0) < 0.0);
  CHECK(ppu::SmallestPositiveRoot(1.0, 3.0, 2.0) < 0.0);
  /*--- Tiny c2: the stable form keeps the root of the linear part. ---*/
  CHECK(ppu::SmallestPositiveRoot(1.0, -2.0, 1e-300) == Approx(0.5));

  const double zero = 0.0;

  /*--- Linear P (d rho = 0, d m = 0): q(a) = q0 + a dE >= 0.1 q0 gives a = 0.9 q0 / 2. ---*/
  const double U0[3] = {1.0, 0.5, 1.0};
  const double q0 = ppu::InternalEnergy(1, U0, zero);
  CHECK(q0 == Approx(0.875));
  const double dLin[3] = {0.0, 0.0, -2.0};
  const double aLin = ppu::FlowStepLimit(1, U0, dLin, zero, 1.0);
  CHECK(aLin <= 0.9 * q0 / 2.0 * (1.0 + 1e-12));
  CHECK(aLin >= 0.45 * q0 / 2.0 * (1.0 - 1e-12));

  /*--- Constant P (dU = 0) and increasing linear P (dE > 0): the factor stays alpha0. ---*/
  const double dZero[3] = {0.0, 0.0, 0.0}, dUp[3] = {0.0, 0.0, 3.0};
  CHECK(ppu::FlowStepLimit(1, U0, dZero, zero, 0.7) == 0.7);
  CHECK(ppu::FlowStepLimit(1, U0, dUp, zero, 0.7) == 0.7);

  /*--- Density only: the linear step 1, rho >= 0.1 rho0. ---*/
  const double U1[3] = {1.0, 0.0, 1.0}, dRho[3] = {-2.0, 0.0, 0.0};
  const double aRho = ppu::FlowStepLimit(1, U1, dRho, zero, 1.0);
  CHECK(aRho <= 0.45 * (1.0 + 1e-12));
  CHECK(aRho >= 0.225 * (1.0 - 1e-12));

  /*--- A zero factor stays zero. ---*/
  CHECK(ppu::FlowStepLimit(1, U0, dLin, zero, 0.0) == 0.0);
}

TEST_CASE("POSITIVE_UPDATE_FLOW: a nonfinite increment gives a zero factor", "[P1Flux]") {
  const double zero = 0.0;
  const double U0[4] = {1.0, 0.2, -0.1, 2.5};
  const double nan = std::numeric_limits<double>::quiet_NaN(), inf = std::numeric_limits<double>::infinity();
  const double dNan[4] = {0.0, nan, 0.0, 0.0}, dInf[4] = {0.0, 0.0, 0.0, -inf};
  CHECK(ppu::FlowStepLimit(2, U0, dNan, zero, 1.0) == 0.0);
  CHECK(ppu::FlowStepLimit(2, U0, dInf, zero, 1.0) == 0.0);
  CHECK(ppu::TkeStepLimit(1.0, 1.0, 1.0, 0.1, nan, 1.0, 1e-10, 1e10) == 0.0);
  CHECK(ppu::TkeStepLimit(1.0, 1.0, 1.0, 0.1, inf, 1.0, 1e-10, 1e10) == 0.0);
}

TEST_CASE("POSITIVE_UPDATE_FLOW: random states and increments never give an inadmissible accepted state", "[P1Flux]") {
  std::mt19937 gen(20261106);
  std::uniform_real_distribution<double> unit(0.0, 1.0);
  int limited = 0, failed = 0, notMaximal = 0, zeroSteps = 0;
  for (int nDim = 1; nDim <= 3; ++nDim) {
    for (int sst = 0; sst < 2; ++sst) {
      for (int t = 0; t < 4000; ++t) {
        double U0[ppu::MAXNVAR], dU[ppu::MAXNVAR], U[ppu::MAXNVAR], k = 0.0;
        RandomState(gen, nDim, sst == 1, U0, k);
        RandomIncrement(gen, nDim, U0, dU);
        const double alpha0 = (t % 3 == 0) ? 1.0 : unit(gen);

        const double alpha = ppu::FlowStepLimit(nDim, U0, dU, k, alpha0);
        for (int i = 0; i < nDim + 2; ++i) U[i] = U0[i] + alpha * dU[i];

        if (alpha < alpha0) limited++;
        if (!(alpha >= 0.0 && alpha <= alpha0) || !KeepsMargins(nDim, U0, U, k)) failed++;
        if (alpha0 > 0.0 && !(alpha > 0.0)) zeroSteps++;

        /*--- Largest step up to the one halving of the rounded check. ---*/
        const double aStar = BruteForceStep(nDim, U0, dU, k, alpha0);
        if (alpha > aStar * (1.0 + 1e-9) + 1e-300 || alpha < 0.5 * aStar * (1.0 - 1e-9)) notMaximal++;
      }
    }
  }
  INFO("limited " << limited << ", failed " << failed << ", not maximal " << notMaximal << ", zero " << zeroSteps);
  CHECK(limited > 0);
  CHECK(failed == 0);
  CHECK(notMaximal == 0);
  CHECK(zeroSteps == 0);
}

TEST_CASE("POSITIVE_UPDATE_FLOW: SST flow step and k limit keep q - rho k positive", "[P1Flux]") {
  const double kLower = 1e-10, kUpper = 1e10;

  /*--- Deterministic k limit: rho = 1, q = 1, k0 = 0.1, d(rho k) = 2: room 0.9 - 0.1 gives beta = 0.4. ---*/
  const double beta = ppu::TkeStepLimit(1.0, 1.0, 1.0, 0.1, 2.0, 1.0, kLower, kUpper);
  CHECK(beta <= 0.4 * (1.0 + 1e-12));
  CHECK(beta >= 0.2 * (1.0 - 1e-12));
  CHECK(1.0 - ppu::ClippedTke(1.0, 0.1, 2.0, beta, kLower, kUpper) > 0.0);
  /*--- Without it the full step gives q - rho k = 1 - 2.1 < 0. ---*/
  CHECK(1.0 - ppu::ClippedTke(1.0, 0.1, 2.0, 1.0, kLower, kUpper) < 0.0);
  /*--- A decreasing k keeps the factor, no room for a growing k cancels it. ---*/
  CHECK(ppu::TkeStepLimit(1.0, 1.0, 1.0, 0.1, -0.05, 0.8, kLower, kUpper) == 0.8);
  CHECK(ppu::TkeStepLimit(1.0, 1.0, 1.0, 0.95, 0.1, 0.8, kLower, kUpper) == 0.0);

  /*--- Random flow step with k, then the k step on the accepted flow state. ---*/
  std::mt19937 gen(611);
  std::uniform_real_distribution<double> unit(0.0, 1.0), signedUnit(-1.0, 1.0), logs(-3.0, 1.5);
  int failed = 0, limitedK = 0;
  for (int nDim = 2; nDim <= 3; ++nDim) {
    for (int t = 0; t < 5000; ++t) {
      double U0[ppu::MAXNVAR], dU[ppu::MAXNVAR], U[ppu::MAXNVAR], k0 = 0.0;
      RandomState(gen, nDim, true, U0, k0);
      RandomIncrement(gen, nDim, U0, dU);

      const double alpha = ppu::FlowStepLimit(nDim, U0, dU, k0, unit(gen));
      for (int i = 0; i < nDim + 2; ++i) U[i] = U0[i] + alpha * dU[i];
      if (!KeepsMargins(nDim, U0, U, k0)) failed++;

      /*--- k step: q of the updated flow state without k, d(rho k) random of either sign. ---*/
      /*--- The k update uses the old (primitive) density rhoP, the pressure the new density rho. ---*/
      const double rho = U[0], rhoP = U0[0], zero = 0.0;
      const double q = ppu::InternalEnergy(nDim, U, zero);
      const double dK = std::pow(10.0, logs(gen)) * signedUnit(gen) * (rho * k0 + q);
      const double beta0 = unit(gen);
      const double b = ppu::TkeStepLimit(rho, rhoP, q, k0, dK, beta0, kLower, kUpper);
      const double kNew = ppu::ClippedTke(rhoP, k0, dK, b, kLower, kUpper);
      if (b < beta0) limitedK++;
      /*--- b = 0 cancels the k update (k0 up to rounding), q - rho k0 > 0 from the flow step. ---*/
      const bool bound = (b == 0.0) || (rho * kNew <= (1.0 - ppu::EPS_Q) * q * (1.0 + 1e-12)) || (kNew <= k0);
      if (!(b >= 0.0 && b <= beta0) || !bound || !(q - rho * kNew * (1.0 - 1e-12) > 0.0)) failed++;
    }
  }
  INFO("failed " << failed << ", k limited " << limitedK);
  CHECK(failed == 0);
  CHECK(limitedK > 0);
}

/*--- P1_WALL_FORCES: the wall forces of the weak no-slip wall (weighted -Fhat) and of closure C (pressure shares). ---*/

namespace {

/*--- Local index of the marker "tag", -1 if it is not on this rank. ---*/
int MarkerIndex(const CConfig& config, const CGeometry& geometry, const std::string& tag) {
  for (unsigned short iMarker = 0; iMarker < geometry.GetnMarker(); ++iMarker)
    if (config.GetMarker_All_TagBound(iMarker) == tag) return iMarker;
  return -1;
}

/*--- Volume simplex of a boundary face and the element-local index of its node opposite the face. ---*/
bool FaceElement(const CGeometry& geometry, const CPrimalGrid& face, unsigned long& iElem, unsigned short& opposite) {
  const auto p0 = face.GetNode(0);
  for (unsigned short i = 0; i < geometry.nodes->GetnElem(p0); ++i) {
    const auto jElem = geometry.nodes->GetElem(p0, i);
    const auto* elem = geometry.elem[jElem];
    unsigned short found = 0, other = 0;
    for (unsigned short c = 0; c < elem->GetnNodes(); ++c) {
      bool onFace = false;
      for (unsigned short k = 0; k < face.GetnNodes(); ++k) onFace = onFace || (elem->GetNode(c) == face.GetNode(k));
      if (onFace) ++found;
      else other = c;
    }
    if (found == face.GetnNodes()) {
      iElem = jElem;
      opposite = other;
      return true;
    }
  }
  return false;
}

/*--- Smooth nodal data from the coordinates (the same on every rank), with a nonzero wall velocity. ---*/
pve::NodeState WallTestState(const su2double* x, int nDim) {
  pve::NodeState S;
  S.rho = 1.0 + 0.3 * sin(2.0 * x[0] + x[1]);
  S.p = 0.8 + 0.2 * cos(x[0] - 3.0 * x[nDim - 1]);
  for (int i = 0; i < nDim; ++i) S.vel[i] = 0.4 * sin(1.0 + i + 3.0 * x[(i + 1) % nDim]) + 0.2 * x[nDim - 1];
  S.mu = 0.05 * (1.0 + 0.5 * x[0]);
  S.kappa = 0.07 * (1.0 + 0.3 * x[nDim - 1]);
  return S;
}

/*--- Wall element of a bottom face: nodes, coordinates, states of "field", face nodes and outward area vector. ---*/
template <class Field>
void WallTestElement(const CGeometry& geometry, const CPrimalGrid& face, int nDim, const Field& field,
                     const su2double* coord[4], pve::NodeState* state, unsigned short* faceNodes, su2double* N,
                     unsigned long* point) {
  unsigned long iElem = 0;
  unsigned short opposite = 0;
  REQUIRE(FaceElement(geometry, face, iElem, opposite));
  const auto* elem = geometry.elem[iElem];
  for (unsigned short c = 0, k = 0; c <= nDim; ++c) {
    point[c] = elem->GetNode(c);
    coord[c] = geometry.nodes->GetCoord(point[c]);
    state[c] = field(coord[c]);
    if (c != opposite) faceNodes[k++] = c;
  }
  pve::WallFaceNormal(nDim, coord, opposite, faceNodes, N);
}

}  // namespace

TEST_CASE("P1_WALL_FORCES: weak wall rows sum to the weighted traction without the adjoint term", "[P1Flux]") {
  /*--- Over all rows, R with the weak no-slip wall minus R without it is the sum of NoSlipWall over the wall faces. The
   * adjoint term sums to zero over each element (sum_c g_c = 0), so this equals the sum of the weighted -Fhat of the
   * face nodes that P1_WALL_FORCES reports (lumped and moved shares, adiabatic and isothermal). The output argument
   * leaves the residual unchanged. Faces counted on the rank owning their first node, sums MPI-reduced. ---*/
  for (int nDim : {2, 3}) {
    const int nVar = nDim + 2;
    const std::string mesh = "p1_wall_forces_" + std::to_string(nDim) + "d.su2";
    WriteSimplexMesh(mesh, nDim, (nDim == 2) ? 8 : 3, 0.4, 1.0);
    MeshCase test(mesh);
    const auto& geometry = *test.geometry;
    const int iMarker = MarkerIndex(*test.config, geometry, "bottom");
    const unsigned long nFace = (iMarker < 0) ? 0 : geometry.GetnElem_Bound(iMarker);
    auto field = [&](const su2double* x) { return WallTestState(x, nDim); };

    for (int variant = 0; variant < 4; ++variant) {
      const bool isothermal = (variant % 2 == 1);
      const bool moved = (variant >= 2) && (nDim == 3);
      const su2double wallTheta = 1.0 / (kGas * 1.1);
      const su2double shares[3] = {0.3, 0.45, 0.25};
      su2double rows[5] = {0.0}, fluxes[5] = {0.0}, scale = 0.0, changed = 0.0;

      for (unsigned long iFace = 0; iFace < nFace; ++iFace) {
        const auto& face = *geometry.bound[iMarker][iFace];
        if (!geometry.nodes->GetDomain(face.GetNode(0))) continue;
        const su2double* coord[4];
        pve::NodeState state[4];
        unsigned short faceNodes[3] = {0, 0, 0};
        unsigned long point[4];
        su2double N[3] = {0.0, 0.0, 0.0};
        WallTestElement(geometry, face, nDim, field, coord, state, faceNodes, N, point);
        pve::Element E;
        REQUIRE(E.Set(nDim, coord, state, kGas) > 0.0);

        su2double wall[4][5], plain[4][5], faceFlux[3][5];
        pve::NoSlipWall(E, nDim, faceNodes, N, isothermal, wallTheta, 1, pve::NOSLIP_SIPG, pve::NOSLIP_PENALTY, wall,
                        nullptr, nullptr, moved ? shares : nullptr, nullptr, faceFlux);
        pve::NoSlipWall(E, nDim, faceNodes, N, isothermal, wallTheta, 1, pve::NOSLIP_SIPG, pve::NOSLIP_PENALTY, plain,
                        nullptr, nullptr, moved ? shares : nullptr);
        for (int c = 0; c <= nDim; ++c) {
          for (int v = 0; v < nVar; ++v) {
            rows[v] += wall[c][v];
            scale = std::max(scale, fabs(wall[c][v]));
            changed = std::max(changed, fabs(wall[c][v] - plain[c][v]));
          }
        }
        for (int k = 0; k < nDim; ++k)
          for (int v = 0; v < nVar; ++v) fluxes[v] += faceFlux[k][v];
      }

      const su2double scaleAll = GlobalMax(scale);
      const su2double changedAll = GlobalMax(changed);
      INFO("nDim " << nDim << " variant " << variant);
      CHECK(scaleAll > 0.0);
      CHECK(changedAll <= 1e-14 * scaleAll);
      for (int v = 0; v < nVar; ++v) {
        const su2double rowSum = GlobalSum(rows[v]), fluxSum = GlobalSum(fluxes[v]);
        CHECK(fabs(rowSum - fluxSum) < 1e-12 * scaleAll);
      }
    }
  }
}

TEST_CASE("P1_WALL_FORCES: weak wall traction of a linear shear flow is the exact friction force", "[P1Flux]") {
  /*--- u = (a x_d, 0, 0) with uniform rho, p and mu: the P1 element flux is exact and the wall values vanish (no
   * penalty), so the weighted -Fhat over the bottom wall (x_d = 0, unit area, fluid above, adiabatic) is the force
   * (mu a, 0, 0) on the body and no heat flow. ---*/
  const su2double a = 0.7, mu = 0.03;
  for (int nDim : {2, 3}) {
    const int nVar = nDim + 2;
    const std::string mesh = "p1_wall_shear_" + std::to_string(nDim) + "d.su2";
    WriteSimplexMesh(mesh, nDim, (nDim == 2) ? 8 : 3, 0.4, 1.0);
    MeshCase test(mesh);
    const auto& geometry = *test.geometry;
    const int iMarker = MarkerIndex(*test.config, geometry, "bottom");
    const unsigned long nFace = (iMarker < 0) ? 0 : geometry.GetnElem_Bound(iMarker);
    auto field = [&](const su2double* x) {
      pve::NodeState S;
      S.rho = 1.2;
      S.p = 1.0;
      S.vel[0] = a * x[nDim - 1];
      S.mu = mu;
      S.kappa = 0.05;
      return S;
    };

    su2double force[5] = {0.0};
    for (unsigned long iFace = 0; iFace < nFace; ++iFace) {
      const auto& face = *geometry.bound[iMarker][iFace];
      if (!geometry.nodes->GetDomain(face.GetNode(0))) continue;
      const su2double* coord[4];
      pve::NodeState state[4];
      unsigned short faceNodes[3] = {0, 0, 0};
      unsigned long point[4];
      su2double N[3] = {0.0, 0.0, 0.0};
      WallTestElement(geometry, face, nDim, field, coord, state, faceNodes, N, point);
      pve::Element E;
      REQUIRE(E.Set(nDim, coord, state, kGas) > 0.0);
      su2double wall[4][5], faceFlux[3][5];
      pve::NoSlipWall(E, nDim, faceNodes, N, false, 1.0, 1, pve::NOSLIP_SIPG, pve::NOSLIP_PENALTY, wall, nullptr,
                      nullptr, nullptr, nullptr, faceFlux);
      for (int k = 0; k < nDim; ++k)
        for (int v = 0; v < nVar; ++v) force[v] += faceFlux[k][v];
    }
    INFO("nDim " << nDim);
    CHECK(fabs(GlobalSum(force[1]) - mu * a) < 1e-12 * mu * a);
    for (int v = 2; v < nVar; ++v) CHECK(fabs(GlobalSum(force[v])) < 1e-12 * mu * a);
    CHECK(GlobalSum(force[0]) == 0.0);
  }
}

TEST_CASE("P1_WALL_FORCES AXISYMMETRIC: sum_i 2 pi y_i share_i is the face rule of int 2 pi y (p~ - pRef) n", "[P1Flux]") {
  /*--- 2D segments with random end points and states: y is linear on the face, the share of node i is
   * sum_q phi_i(x_q) a_q (p~_q - pRef), so sum_i y_i share_i = sum_q y(x_q) a_q (p~_q - pRef) exactly (the 2 pi y_i weight
   * of Pressure_Forces applied to the closure C shares is the face rule of the axisymmetric pressure force). Also the hoop
   * part of the production: Q_axi - Q_planar = 4/3 (v/y)(v/y - div_pl) for random velocity gradients. ---*/
  std::mt19937 gen(7314);
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  const int nDim = 2;
  for (int trial = 0; trial < 100; ++trial) {
    ale::State fs[2];
    for (int k = 0; k < 2; ++k) fs[k] = RandomAleState(gen, nDim, (trial % 2) ? 0.5 : 0.05);
    const su2double y[2] = {0.05 + exp(u(gen)), 0.05 + exp(u(gen))};
    const su2double pRef = exp(u(gen));
    su2double share[2];
    ale::WallPressureShares(nDim, 2, kGamma, fs, pRef, share);
    su2double phi[3][3] = {{0.0}};
    const su2double weight = ale::FaceRule(2, phi);
    su2double w[2][ale::MAXNVAR] = {{0.0}};
    for (int k = 0; k < 2; ++k) ale::EntropyVariables(nDim, kGamma, fs[k], w[k]);
    su2double lhs = y[0] * share[0] + y[1] * share[1], rhs = 0.0, scale = pRef;
    for (int q = 0; q < 2; ++q) {
      su2double wq[ale::MAXNVAR] = {0.0};
      for (int k = 0; k < 2; ++k)
        for (int v = 0; v < nDim + 2; ++v) wq[v] += phi[q][k] * w[k][v];
      ale::State sq;
      ale::StateFromEntropyVariables(nDim, kGamma, wq, sq);
      rhs += (phi[q][0] * y[0] + phi[q][1] * y[1]) * weight * (sq.p - pRef);
      scale = std::max(scale, sq.p);
    }
    CHECK(fabs(lhs - rhs) <= 1e-13 * scale * (y[0] + y[1]));

    /*--- Hoop production: 2 S:S - 2/3 div^2 with the hoop strain v/y. ---*/
    su2double G[2][2] = {{u(gen), u(gen)}, {u(gen), u(gen)}};
    const su2double vy = u(gen);
    const su2double s01 = 0.5 * (G[0][1] + G[1][0]), divPl = G[0][0] + G[1][1];
    const su2double qPl = 2.0 * (G[0][0] * G[0][0] + G[1][1] * G[1][1] + 2.0 * s01 * s01) - 2.0 / 3.0 * divPl * divPl;
    const su2double qAxi = 2.0 * (G[0][0] * G[0][0] + G[1][1] * G[1][1] + vy * vy + 2.0 * s01 * s01) -
                           2.0 / 3.0 * (divPl + vy) * (divPl + vy);
    const su2double divAxi = divPl + vy;
    CHECK(fabs((qAxi - qPl) - 4.0 / 3.0 * vy * (2.0 * vy - divAxi)) <= 1e-14 * (1.0 + fabs(qAxi)));
  }
}

TEST_CASE("P1_WALL_FORCES: closure C pressure shares are its wall force and the nodal force for a uniform state", "[P1Flux]") {
  /*--- (i) Random faces: sum_i share_i A_f nu + pRef A_f nu is the momentum part of sum_i b_i of closure C at a wall
   * (FaceResidual with v = 0 and kappa = 0 is the static closure). (ii) Uniform state on the mesh: at every owned vertex
   * of the bottom wall the shares of its faces give -(p - pRef) n_v, the nodal force of Pressure_Forces (n_v points
   * into the domain), so the closure C force equals the nodal one. ---*/
  std::mt19937 gen(4242);
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  const su2double zeroVel[3][3] = {{0.0}};
  for (int nDim : {2, 3}) {
    for (int trial = 0; trial < 100; ++trial) {
      ale::State fs[3];
      for (int k = 0; k < nDim; ++k) fs[k] = RandomAleState(gen, nDim, (trial % 2) ? 0.5 : 0.05);
      su2double N[3] = {0.0, 0.0, 0.0}, normN = 0.0;
      for (int i = 0; i < nDim; ++i) {
        N[i] = u(gen);
        normN += N[i] * N[i];
      }
      normN = sqrt(normN);
      const su2double pRef = exp(u(gen));
      su2double res[3][ale::MAXNVAR], share[3];
      ale::FaceResidual(nDim, nDim, kGamma, nullptr, N, fs, zeroVel, 0.0, res);
      ale::WallPressureShares(nDim, nDim, kGamma, fs, pRef, share);
      su2double scale = pRef;
      for (int k = 0; k < nDim; ++k) {
        su2double vel2 = 0.0;
        for (int i = 0; i < nDim; ++i) vel2 += fs[k].vel[i] * fs[k].vel[i];
        scale = std::max(scale, fs[k].p + fs[k].rho * vel2);
      }
      for (int i = 0; i < nDim; ++i) {
        su2double sumRes = 0.0, sumShare = pRef * N[i];
        for (int k = 0; k < nDim; ++k) {
          sumRes += res[k][i + 1];
          sumShare += share[k] * N[i];
        }
        CHECK(fabs(sumRes - sumShare) < 1e-12 * scale * normN);
      }
    }

    const std::string mesh = "p1_wall_pressure_" + std::to_string(nDim) + "d.su2";
    WriteSimplexMesh(mesh, nDim, (nDim == 2) ? 8 : 3, 0.4, 1.0);
    MeshCase test(mesh);
    auto& geometry = *test.geometry;
    const int iMarker = MarkerIndex(*test.config, geometry, "bottom");
    const su2double pRef = 0.7;
    ale::State uniform;
    uniform.rho = 1.3;
    uniform.p = 0.9;
    uniform.vel[0] = 0.2;
    uniform.vel[1] = -0.1;
    uniform.vel[2] = (nDim == 3) ? 0.05 : 0.0;
    auto field = [&](const su2double*) {
      pve::NodeState S;
      S.rho = uniform.rho;
      S.p = uniform.p;
      for (int i = 0; i < 3; ++i) S.vel[i] = uniform.vel[i];
      return S;
    };

    su2double err = 0.0, scale = 0.0;
    if (iMarker >= 0) {
      std::vector<std::array<su2double, 3>> force(geometry.GetnVertex(iMarker), {0.0, 0.0, 0.0});
      for (unsigned long iFace = 0; iFace < geometry.GetnElem_Bound(iMarker); ++iFace) {
        const auto& face = *geometry.bound[iMarker][iFace];
        const su2double* coord[4];
        pve::NodeState state[4];
        unsigned short faceNodes[3] = {0, 0, 0};
        unsigned long point[4];
        su2double N[3] = {0.0, 0.0, 0.0};
        WallTestElement(geometry, face, nDim, field, coord, state, faceNodes, N, point);
        ale::State fs[3];
        for (int k = 0; k < nDim; ++k) fs[k] = uniform;
        su2double share[3];
        ale::WallPressureShares(nDim, nDim, kGamma, fs, pRef, share);
        for (int k = 0; k < nDim; ++k) {
          const long iVertex = geometry.nodes->GetVertex(point[faceNodes[k]], iMarker);
          REQUIRE(iVertex >= 0);
          for (int i = 0; i < nDim; ++i) force[iVertex][i] += share[k] * N[i];
        }
      }
      for (unsigned long iVertex = 0; iVertex < geometry.GetnVertex(iMarker); ++iVertex) {
        if (!geometry.nodes->GetDomain(geometry.vertex[iMarker][iVertex]->GetNode())) continue;
        const su2double* normal = geometry.vertex[iMarker][iVertex]->GetNormal();
        for (int i = 0; i < nDim; ++i) {
          const su2double nodal = -(uniform.p - pRef) * normal[i];
          err = std::max(err, fabs(force[iVertex][i] - nodal));
          scale = std::max(scale, fabs(nodal));
        }
      }
    }
    const su2double scaleAll = GlobalMax(scale);
    INFO("nDim " << nDim);
    CHECK(scaleAll > 0.0);
    CHECK(GlobalMax(err) < 1e-12 * scaleAll);
  }
}

namespace {

/*--- VISC_P1_TIME_STEP checks (P1_TIME_STEP.md). Full dw/du with the w_0 row by central differences of the entropy
 * variables: the inner product x.W y of the nodal entropy Hessians (EntropyVariableJacobian has no w_0 row). ---*/
void FullEntropyJacobian(int nDim, const pve::NodeState& S, su2double W[5][5]) {
  const int nVar = nDim + 2;
  const su2double h = 1e-6;
  for (int j = 0; j < nVar; ++j) {
    pve::NodeState plus = S, minus = S;
    Perturb(nDim, plus, j, h);
    Perturb(nDim, minus, j, -h);
    su2double wp[5], wm[5];
    FullEntropyVariables(nDim, plus, wp);
    FullEntropyVariables(nDim, minus, wm);
    for (int i = 0; i < nVar; ++i) W[i][j] = (wp[i] - wm[i]) / (2 * h);
  }
  for (int i = 0; i < nVar; ++i) {
    for (int j = 0; j < i; ++j) {
      const su2double mean = 0.5 * (W[i][j] + W[j][i]);
      W[i][j] = mean;
      W[j][i] = mean;
    }
  }
}

/*--- x.K(t,t)x of the frozen K of E, K(t,t)x = F(x (x) t) t. ---*/
su2double NormalForm(const pve::Element& E, const su2double* x, const su2double* t) {
  su2double F[5][3] = {};
  E.FluxOuter(x, t, F);
  su2double form = 0.0;
  for (int v = 0; v < E.nVar; ++v)
    for (int j = 0; j < E.nDim; ++j) form += x[v] * F[v][j] * t[j];
  return form;
}

void RandomNodeState(std::mt19937& gen, int nDim, pve::NodeState& S) {
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  S.rho = exp(u(gen));
  S.p = S.rho * kGas * exp(u(gen));
  for (int i = 0; i < 3; ++i) S.vel[i] = (i < nDim) ? 1.5 * u(gen) : 0.0;
  S.mu = exp(2.0 * u(gen) - 1.0);
  S.kappa = exp(2.0 * u(gen));
}

/*--- Small graded simplex mesh with a wall at the lowest level: 2D 4 x 3 squares split by parity, 3D 2 x 2 x 2 Kuhn
 * cubes, first heights 0.01 and 0.02, jittered off the wall. walls = (cell, node opposite the wall face). ---*/
struct TimeStepMesh {
  int nDim = 2;
  std::vector<std::array<su2double, 3>> x;
  std::vector<std::array<unsigned long, 4>> cells;
  std::vector<std::pair<unsigned long, unsigned short>> walls;
};

TimeStepMesh GradedWallMesh(int nDim, std::mt19937& gen) {
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  TimeStepMesh m;
  m.nDim = nDim;
  if (nDim == 2) {
    const int nx = 4, ny = 3;
    const double level[4] = {0.0, 0.01, 0.06, 0.3};
    auto id = [&](int i, int j) { return static_cast<unsigned long>(j * (nx + 1) + i); };
    for (int j = 0; j <= ny; ++j)
      for (int i = 0; i <= nx; ++i) m.x.push_back({(i + ((j > 0) ? 0.2 * u(gen) : 0.0)) / nx, level[j], 0.0});
    for (int j = 0; j < ny; ++j) {
      for (int i = 0; i < nx; ++i) {
        const auto a = id(i, j), b = id(i + 1, j), c = id(i, j + 1), d = id(i + 1, j + 1);
        if ((i + j) % 2 == 0) {
          m.cells.push_back({a, b, d, 0});
          m.cells.push_back({a, d, c, 0});
        } else {
          m.cells.push_back({a, b, c, 0});
          m.cells.push_back({b, d, c, 0});
        }
      }
    }
  } else {
    const unsigned long n = 2;
    const double level[3] = {0.0, 0.02, 0.2};
    for (unsigned long i = 0; i <= n; ++i)
      for (unsigned long j = 0; j <= n; ++j)
        for (unsigned long k = 0; k <= n; ++k) {
          const double jx = (k > 0) ? 0.15 * u(gen) : 0.0, jy = (k > 0) ? 0.15 * u(gen) : 0.0;
          m.x.push_back({(i + jx) / n, (j + jy) / n, level[k]});
        }
    static const int kuhn[6][4] = {{0, 1, 2, 6}, {0, 2, 3, 6}, {0, 3, 7, 6}, {0, 7, 4, 6}, {0, 4, 5, 6}, {0, 5, 1, 6}};
    for (unsigned long i = 0; i < n; ++i)
      for (unsigned long j = 0; j < n; ++j)
        for (unsigned long k = 0; k < n; ++k) {
          const unsigned long h[] = {Lattice(i, j, k, n),         Lattice(i + 1, j, k, n),
                                     Lattice(i + 1, j + 1, k, n), Lattice(i, j + 1, k, n),
                                     Lattice(i, j, k + 1, n),     Lattice(i + 1, j, k + 1, n),
                                     Lattice(i + 1, j + 1, k + 1, n), Lattice(i, j + 1, k + 1, n)};
          for (const auto& t : kuhn) m.cells.push_back({h[t[0]], h[t[1]], h[t[2]], h[t[3]]});
        }
  }
  for (unsigned long e = 0; e < m.cells.size(); ++e) {
    for (unsigned short opp = 0; opp <= nDim; ++opp) {
      bool wall = true;
      for (unsigned short c = 0; c <= nDim; ++c)
        if (c != opp) wall = wall && (m.x[m.cells[e][c]][nDim - 1] == 0.0);
      if (wall) m.walls.emplace_back(e, opp);
    }
  }
  return m;
}

/*--- Dense Picard Jacobian (conservative variables) of VISC_P1_ENTROPY and of the weak walls on the mesh, and the
 * row bounds B_i of VISC_P1_TIME_STEP, assembled as Viscous_Residual_P1Entropy and WeakNoSlipWallResidual do. ---*/
void AssembleViscousOperator(const TimeStepMesh& m, const std::vector<pve::NodeState>& states, bool isothermal,
                             const su2double& wallTheta, std::vector<su2double>& J, std::vector<su2double>& rows,
                             std::vector<su2double>& lumped) {
  const int nDim = m.nDim, nVar = nDim + 2, nNodes = nDim + 1;
  const unsigned long nPoint = m.x.size(), n = nPoint * nVar;
  J.assign(n * n, 0.0);
  rows.assign(nPoint, 0.0);
  lumped.assign(nPoint, 0.0);
  std::vector<unsigned short> nWall(m.cells.size(), 0);
  for (const auto& w : m.walls) ++nWall[w.first];

  auto setElement = [&](unsigned long e, pve::Element& E, pve::NodeState* st, su2double (*dwdu)[5][5]) {
    const su2double* coord[4];
    for (int a = 0; a < nNodes; ++a) {
      coord[a] = m.x[m.cells[e][a]].data();
      st[a] = states[m.cells[e][a]];
      pve::EntropyVariableJacobian(nDim, kGamma, st[a], dwdu[a]);
    }
    REQUIRE(E.Set(nDim, coord, st, kGas) > 0.0);
  };
  auto addBlocks = [&](unsigned long e, su2double (*jac)[4][5][5]) {
    for (int a = 0; a < nNodes; ++a)
      for (int b = 0; b < nNodes; ++b)
        for (int v = 0; v < nVar; ++v)
          for (int w = 0; w < nVar; ++w)
            J[(m.cells[e][a] * nVar + v) * n + m.cells[e][b] * nVar + w] += jac[a][b][v][w];
  };

  for (unsigned long e = 0; e < m.cells.size(); ++e) {
    pve::Element E;
    pve::NodeState st[4];
    su2double dwdu[4][5][5], jac[4][4][5][5], r[4] = {};
    setElement(e, E, st, dwdu);
    E.Jacobian(dwdu, jac);
    addBlocks(e, jac);
    E.RowBound(st, kGamma, r);
    for (int a = 0; a < nNodes; ++a) {
      rows[m.cells[e][a]] += r[a];
      lumped[m.cells[e][a]] += E.size / nNodes;
    }
  }
  for (const auto& w : m.walls) {
    const auto e = w.first;
    pve::Element E;
    pve::NodeState st[4];
    su2double dwdu[4][5][5], jac[4][4][5][5], res[4][5], r[4] = {}, N[3] = {};
    setElement(e, E, st, dwdu);
    const su2double* coord[4];
    for (int a = 0; a < nNodes; ++a) coord[a] = m.x[m.cells[e][a]].data();
    unsigned short faceNodes[3] = {0, 0, 0};
    for (unsigned short c = 0, k = 0; c < nNodes; ++c)
      if (c != w.second) faceNodes[k++] = c;
    pve::WallFaceNormal(nDim, coord, w.second, faceNodes, N);
    pve::NoSlipWall(E, nDim, faceNodes, N, isothermal, wallTheta, nWall[e], pve::NOSLIP_SIPG, pve::NOSLIP_PENALTY, res,
                    dwdu, jac);
    addBlocks(e, jac);
    pve::NoSlipWallRowBound(E, st, kGamma, nDim, faceNodes, N, isothermal, nWall[e], pve::NOSLIP_SIPG,
                            pve::NOSLIP_PENALTY, r);
    for (int a = 0; a < nNodes; ++a) rows[m.cells[e][a]] += r[a];
  }
}

}  // namespace

TEST_CASE("VISC_P1_TIME_STEP: Lambda bounds the normal form of K against the entropy Hessian and is attained", "[P1Flux]") {
  std::mt19937 gen(97531);
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  for (int nDim : {2, 3}) {
    const int nVar = nDim + 2;
    su2double worst = 0.0, worstWall = 0.0, worstMoving = 0.0;
    for (int trial = 0; trial < 300; ++trial) {
      su2double x[4][3];
      pve::NodeState state[4];
      RandomElement(gen, nDim, (trial % 2) ? 1.0 : 1e-3, x, state);
      pve::Element E;
      REQUIRE(E.Set(nDim, x, state, kGas) > 0.0);
      const int b = trial % (nDim + 1);
      su2double W[5][5];
      FullEntropyJacobian(nDim, state[b], W);
      su2double t[3] = {}, nu[3] = {}, t2 = 0.0, vw[3] = {};
      for (int i = 0; i < nDim; ++i) {
        t[i] = u(gen);
        t2 += t[i] * t[i];
        vw[i] = u(gen);
      }
      for (int i = 0; i < nDim; ++i) nu[i] = t[i] / sqrt(t2);
      const su2double lambda = E.SpectralFactor(state[b], kGamma);
      const su2double piAdiabatic = E.SpectralFactor(state[b], kGamma, false);
      const su2double piMoving = pve::MovingWallSpectralFactor(E, state[b], kGamma, vw);
      for (int sample = 0; sample < 20; ++sample) {
        /*--- x = W du, so x.H x = x.du. ---*/
        su2double du[5] = {}, dw[5] = {}, hx = 0.0;
        for (int v = 0; v < nVar; ++v) du[v] = u(gen);
        for (int v = 0; v < nVar; ++v)
          for (int k = 0; k < nVar; ++k) dw[v] += W[v][k] * du[k];
        for (int v = 0; v < nVar; ++v) hx += dw[v] * du[v];
        REQUIRE(hx > 0.0);
        const su2double q = NormalForm(E, dw, t) / (t2 * lambda * hx);
        worst = std::max(worst, q);

        su2double pAdi[5] = {}, pMov[5] = {};
        for (int i = 0; i < nDim; ++i) pAdi[i + 1] = dw[i + 1];
        pve::MovingWallProjector(nDim, vw, dw, pMov);
        const su2double qa = NormalForm(E, pAdi, nu) / (piAdiabatic * hx);
        const su2double qm = NormalForm(E, pMov, nu) / (piMoving * hx);
        worstWall = std::max(worstWall, qa);
        worstMoving = std::max(worstMoving, qm);
      }
    }
    INFO("nDim " << nDim);
    CHECK(worst <= 1.0 + 1e-8);
    CHECK(worstWall <= 1.0 + 1e-8);
    CHECK(worstMoving <= 1.0 + 1e-8);

    /*--- Attained (Lemma T1 is sharp): uniform state, viscous part larger than the thermal one, du = (0, rho dv,
     * rho v.dv) with dv || t gives the ratio 1. ---*/
    pve::NodeState uniform[4];
    su2double x[4][3] = {};
    for (int a = 0; a <= nDim; ++a) {
      uniform[a].rho = 1.3;
      uniform[a].p = 0.8;
      for (int i = 0; i < nDim; ++i) uniform[a].vel[i] = 0.4 - 0.3 * i;
      uniform[a].mu = 2.0;
      uniform[a].kappa = 0.1;
      if (a > 0) x[a][a - 1] = 1.0 + 0.2 * a;
    }
    pve::Element E;
    REQUIRE(E.Set(nDim, x, uniform, kGas) > 0.0);
    su2double W[5][5], t[3] = {0.3, -0.7, 0.5}, t2 = 0.0, du[5] = {}, dw[5] = {}, hx = 0.0;
    FullEntropyJacobian(nDim, uniform[0], W);
    for (int i = 0; i < nDim; ++i) {
      t2 += t[i] * t[i];
      du[i + 1] = uniform[0].rho * t[i];
      du[nDim + 1] += uniform[0].rho * uniform[0].vel[i] * t[i];
    }
    for (int v = 0; v < nVar; ++v)
      for (int k = 0; k < nVar; ++k) dw[v] += W[v][k] * du[k];
    for (int v = 0; v < nVar; ++v) hx += dw[v] * du[v];
    const su2double lambda = E.SpectralFactor(uniform[0], kGamma);
    CHECK(fabs(lambda - 4.0 * 2.0 / (3.0 * 1.3)) < 1e-12 * lambda);
    CHECK(fabs(NormalForm(E, dw, t) / (t2 * lambda * hx) - 1.0) < 1e-6);
  }
}

TEST_CASE("VISC_P1_TIME_STEP: the row bound dominates the spectral radius of the assembled viscous operator", "[P1Flux]") {
  std::mt19937 gen(24680);
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  for (int nDim : {2, 3}) {
    const int nVar = nDim + 2;
    const auto mesh = GradedWallMesh(nDim, gen);
    REQUIRE(!mesh.walls.empty());
    const unsigned long nPoint = mesh.x.size(), n = nPoint * nVar;
    for (int trial = 0; trial < 8; ++trial) {
      const bool isothermal = (trial % 2 == 1);
      std::vector<pve::NodeState> states(nPoint);
      for (auto& S : states) RandomNodeState(gen, nDim, S);
      const su2double wallTheta = 1.0 / (kGas * exp(u(gen)));
      std::vector<su2double> J, rows, lumped;
      AssembleViscousOperator(mesh, states, isothermal, wallTheta, J, rows, lumped);

      /*--- Lumped and random volumes: the bound max_i B_i/V_i holds for any V > 0. ---*/
      std::vector<su2double> V(nPoint);
      for (unsigned long i = 0; i < nPoint; ++i) V[i] = (trial < 4) ? lumped[i] : su2double(exp(2.0 * u(gen)));
      su2double bound = 0.0;
      for (unsigned long i = 0; i < nPoint; ++i) bound = std::max(bound, su2double(rows[i] / V[i]));

      /*--- Power iteration on V^-1 J, self-adjoint in <x,y> = sum_i V_i x_i.W_i y_i (W_i = dw/du, SIPG), positive
       * semidefinite (Theorem N6), so the Rayleigh quotient rises to the largest eigenvalue from below. ---*/
      std::vector<std::array<std::array<su2double, 5>, 5>> W(nPoint);
      for (unsigned long i = 0; i < nPoint; ++i) {
        su2double Wi[5][5];
        FullEntropyJacobian(nDim, states[i], Wi);
        for (int a = 0; a < 5; ++a)
          for (int b = 0; b < 5; ++b) W[i][a][b] = Wi[a][b];
      }
      auto inner = [&](const std::vector<su2double>& a, const std::vector<su2double>& b) {
        su2double sum = 0.0;
        for (unsigned long i = 0; i < nPoint; ++i)
          for (int v = 0; v < nVar; ++v)
            for (int w = 0; w < nVar; ++w) sum += V[i] * a[i * nVar + v] * W[i][v][w] * b[i * nVar + w];
        return sum;
      };
      std::vector<su2double> x(n), y(n);
      for (auto& xi : x) xi = u(gen);
      su2double rayleigh = 0.0;
      for (int it = 0; it < 3000; ++it) {
        for (unsigned long r = 0; r < n; ++r) {
          su2double sum = 0.0;
          for (unsigned long c = 0; c < n; ++c) sum += J[r * n + c] * x[c];
          y[r] = sum / V[r / nVar];
        }
        const su2double xx = inner(x, x);
        rayleigh = inner(x, y) / xx;
        const su2double scale = 1.0 / sqrt(inner(y, y));
        for (unsigned long r = 0; r < n; ++r) x[r] = y[r] * scale;
      }
      INFO("nDim " << nDim << " trial " << trial << " rayleigh " << rayleigh << " bound " << bound);
      CHECK(rayleigh <= bound * (1.0 + 1e-8));
      if (trial < 4) CHECK(rayleigh >= 0.1 * bound);  /*--- Not vacuous (lumped volumes; Python: 0.26-0.88). ---*/
    }
  }
}

TEST_CASE("VISC_P1_TIME_STEP: uniform simplex meshes against the SU2 edge formula", "[P1Flux]") {
  /*--- Uniform state: Lambda_b = max(4/3 mu, gamma kappa/cp)/rho, SU2's edge factor (4/3 mu + gamma kappa/cp)/rho.
   * Interior node: V_i B_i / Lambda_v(SU2) = geometric factor x max/sum (median dual, P1_TIME_STEP.md, section 4);
   * geometric factors 9/2 (equilateral), 9/4 + 3 sqrt(2)/2 (right triangles, one diagonal), 9.3254834 (Kuhn). ---*/
  const su2double cp = kGamma * kGas / (kGamma - 1.0), mu = 0.8, kappa = cp * mu / 0.72;
  pve::NodeState S;
  S.rho = 1.2;
  S.p = 0.9;
  S.vel[0] = 0.3;
  S.vel[1] = -0.2;
  S.vel[2] = 0.1;
  S.mu = mu;
  S.kappa = kappa;
  const su2double viscous = 4.0 * mu / 3.0, thermal = kGamma * kappa / cp;
  const su2double physical = std::max(viscous, thermal) / (viscous + thermal);

  struct Patch {
    int nDim;
    std::vector<std::array<su2double, 3>> x;
    std::vector<std::array<unsigned long, 4>> cells;
    unsigned long centre;
    su2double geometric;
  };
  std::vector<Patch> patches;
  {
    Patch p{2, {}, {}, 0, 4.5};
    p.x.push_back({0.0, 0.0, 0.0});
    for (int k = 0; k < 6; ++k) p.x.push_back({cos(k * M_PI / 3.0), sin(k * M_PI / 3.0), 0.0});
    for (unsigned long k = 1; k <= 6; ++k) p.cells.push_back({0, k, k % 6 + 1, 0});
    patches.push_back(p);
  }
  {
    Patch p{2, {}, {}, 4, 2.25 + 1.5 * sqrt(2.0)};
    for (int j = 0; j <= 2; ++j)
      for (int i = 0; i <= 2; ++i) p.x.push_back({0.5 * i, 0.5 * j, 0.0});
    for (unsigned long j = 0; j < 2; ++j)
      for (unsigned long i = 0; i < 2; ++i) {
        const unsigned long a = 3 * j + i;
        p.cells.push_back({a, a + 1, a + 4, 0});
        p.cells.push_back({a, a + 4, a + 3, 0});
      }
    patches.push_back(p);
  }
  {
    const unsigned long n = 2;
    Patch p{3, {}, {}, Lattice(1, 1, 1, n), 9.32548339959391};
    for (unsigned long i = 0; i <= n; ++i)
      for (unsigned long j = 0; j <= n; ++j)
        for (unsigned long k = 0; k <= n; ++k) p.x.push_back({0.7 * i, 0.7 * j, 0.7 * k});
    static const int kuhn[6][4] = {{0, 1, 2, 6}, {0, 2, 3, 6}, {0, 3, 7, 6}, {0, 7, 4, 6}, {0, 4, 5, 6}, {0, 5, 1, 6}};
    for (unsigned long i = 0; i < n; ++i)
      for (unsigned long j = 0; j < n; ++j)
        for (unsigned long k = 0; k < n; ++k) {
          const unsigned long h[] = {Lattice(i, j, k, n),         Lattice(i + 1, j, k, n),
                                     Lattice(i + 1, j + 1, k, n), Lattice(i, j + 1, k, n),
                                     Lattice(i, j, k + 1, n),     Lattice(i + 1, j, k + 1, n),
                                     Lattice(i + 1, j + 1, k + 1, n), Lattice(i, j + 1, k + 1, n)};
          for (const auto& t : kuhn) p.cells.push_back({h[t[0]], h[t[1]], h[t[2]], h[t[3]]});
        }
    patches.push_back(p);
  }

  for (const auto& p : patches) {
    const int nDim = p.nDim, nNodes = nDim + 1;
    su2double rowBound = 0.0, volume = 0.0;
    std::map<unsigned long, std::array<su2double, 3>> normals;
    for (const auto& t : p.cells) {
      int local = -1;
      for (int a = 0; a < nNodes; ++a)
        if (t[a] == p.centre) local = a;
      if (local < 0) continue;
      const su2double* coord[4];
      pve::NodeState st[4];
      for (int a = 0; a < nNodes; ++a) {
        coord[a] = p.x[t[a]].data();
        st[a] = S;
      }
      pve::Element E;
      REQUIRE(E.Set(nDim, coord, st, kGas) > 0.0);
      CHECK(fabs(E.SpectralFactor(S, kGamma) - std::max(viscous, thermal) / S.rho) < 1e-12 * thermal);
      su2double r[4] = {};
      E.RowBound(st, kGamma, r);
      rowBound += r[local];
      volume += E.size / nNodes;
      /*--- Median dual normal of edge (centre, b) in E: |T|/(d+1) (g_b - g_centre). ---*/
      for (int b = 0; b < nNodes; ++b) {
        if (b == local) continue;
        auto& nb = normals[t[b]];
        for (int i = 0; i < nDim; ++i) nb[i] += E.size / nNodes * (E.grad[b][i] - E.grad[local][i]);
      }
    }
    su2double lambdaSU2 = 0.0;
    for (const auto& entry : normals)
      for (int i = 0; i < nDim; ++i) lambdaSU2 += (viscous + thermal) / S.rho * entry.second[i] * entry.second[i];
    const su2double ratio = volume * rowBound / lambdaSU2;
    INFO("nDim " << nDim << " ratio " << ratio << " expected " << p.geometric * physical);
    CHECK(fabs(ratio - p.geometric * physical) < 1e-10 * ratio);
  }
}

namespace {

namespace pbg = P1BoundaryGhost;

/*--- The ghosts of the weak boundary residuals of CEulerSolver as they were before P1BoundaryGhost (transcribed from
 * BC_Far_Field_Residual, BC_Inlet_Residual and BC_Outlet_Residual at c922d6d5a4). Output: rho, v, p. ---*/
void OldFarField(int nDim, const ale::State& B, const ale::State& Inf, const su2double* n, const su2double* gridVel,
                 ale::State& G) {
  const su2double Gamma = kGamma, Gamma_Minus_One = kGamma - 1.0;
  su2double Vn_Bound = 0.0, Vn_Infty = 0.0, Velocity[3] = {0.0, 0.0, 0.0};
  for (int i = 0; i < nDim; i++) Vn_Bound += B.vel[i] * n[i];
  const su2double SoundSpeed_Bound = sqrt(Gamma * B.p / B.rho);
  const su2double Entropy_Bound = pow(B.rho, Gamma) / B.p;
  for (int i = 0; i < nDim; i++) Vn_Infty += Inf.vel[i] * n[i];
  const su2double SoundSpeed_Infty = sqrt(Gamma * Inf.p / Inf.rho);
  const su2double Entropy_Infty = pow(Inf.rho, Gamma) / Inf.p;
  su2double Qn_Infty = Vn_Infty;
  if (gridVel != nullptr)
    for (int i = 0; i < nDim; i++) Qn_Infty -= gridVel[i] * n[i];
  su2double RiemannPlus, RiemannMinus, Entropy;
  if (Qn_Infty > -SoundSpeed_Infty) RiemannPlus = Vn_Bound + 2.0 * SoundSpeed_Bound / Gamma_Minus_One;
  else RiemannPlus = Vn_Infty + 2.0 * SoundSpeed_Infty / Gamma_Minus_One;
  if (Qn_Infty > SoundSpeed_Infty) RiemannMinus = Vn_Bound - 2.0 * SoundSpeed_Bound / Gamma_Minus_One;
  else RiemannMinus = Vn_Infty - 2.0 * SoundSpeed_Infty / Gamma_Minus_One;
  const su2double Vn = 0.5 * (RiemannPlus + RiemannMinus);
  const su2double SoundSpeed = 0.25 * (RiemannPlus - RiemannMinus) * Gamma_Minus_One;
  if (Qn_Infty > 0.0) {
    for (int i = 0; i < nDim; i++) Velocity[i] = B.vel[i] + (Vn - Vn_Bound) * n[i];
    Entropy = Entropy_Bound;
  } else {
    for (int i = 0; i < nDim; i++) Velocity[i] = Inf.vel[i] + (Vn - Vn_Infty) * n[i];
    Entropy = Entropy_Infty;
  }
  G.rho = pow(Entropy * SoundSpeed * SoundSpeed / Gamma, 1.0 / Gamma_Minus_One);
  G.p = G.rho * SoundSpeed * SoundSpeed / Gamma;
  for (int i = 0; i < 3; i++) G.vel[i] = (i < nDim) ? Velocity[i] : 0.0;
}

void OldInlet(int nDim, bool massFlow, const ale::State& B, const su2double& dataT, const su2double& dataP,
              const su2double* dir, bool useNormal, const su2double* n, const su2double& refT, const su2double& refP,
              ale::State& G) {
  const su2double Gamma = kGamma, Gamma_Minus_One = kGamma - 1.0, Gas_Constant = kGas;
  const su2double Two_Gamma_M1 = 2.0 / Gamma_Minus_One;
  su2double vel2 = 0.0;
  for (int i = 0; i < nDim; i++) vel2 += B.vel[i] * B.vel[i];
  const su2double enthalpy = Gamma * B.p / (Gamma_Minus_One * B.rho) + 0.5 * vel2;
  su2double mag = 0.0, Flow_Dir[3] = {0.0, 0.0, 0.0}, Velocity[3] = {0.0, 0.0, 0.0};
  for (int i = 0; i < nDim; i++) mag += dir[i] * dir[i];
  mag = sqrt(mag);
  for (int i = 0; i < nDim; i++) Flow_Dir[i] = useNormal ? -n[i] : dir[i] / mag;
  if (!massFlow) {
    const su2double P_Total = dataP / refP, T_Total = dataT / refT;
    su2double Density = B.rho, Velocity2 = 0.0;
    for (int i = 0; i < nDim; i++) {
      Velocity[i] = B.vel[i];
      Velocity2 += Velocity[i] * Velocity[i];
    }
    const su2double Energy = enthalpy - B.p / B.rho;
    su2double Pressure = B.p;
    const su2double H_Total = (Gamma * Gas_Constant / Gamma_Minus_One) * T_Total;
    su2double SoundSpeed2 = Gamma * Pressure / Density;
    su2double Riemann = 2.0 * sqrt(SoundSpeed2) / Gamma_Minus_One;
    for (int i = 0; i < nDim; i++) Riemann += Velocity[i] * n[i];
    const su2double SoundSpeed_Total2 =
        Gamma_Minus_One * (H_Total - (Energy + Pressure / Density) + 0.5 * Velocity2) + SoundSpeed2;
    su2double alpha = 0.0;
    for (int i = 0; i < nDim; i++) alpha += n[i] * Flow_Dir[i];
    const su2double aa = 1.0 + 0.5 * Gamma_Minus_One * alpha * alpha;
    const su2double bb = -1.0 * Gamma_Minus_One * alpha * Riemann;
    const su2double cc = 0.5 * Gamma_Minus_One * Riemann * Riemann - 2.0 * SoundSpeed_Total2 / Gamma_Minus_One;
    su2double dd = bb * bb - 4.0 * aa * cc;
    dd = sqrt(std::max(su2double(0.0), dd));
    su2double Vel_Mag = (-bb + dd) / (2.0 * aa);
    Vel_Mag = std::max(su2double(0.0), Vel_Mag);
    Velocity2 = Vel_Mag * Vel_Mag;
    SoundSpeed2 = SoundSpeed_Total2 - 0.5 * Gamma_Minus_One * Velocity2;
    su2double Mach2 = Velocity2 / SoundSpeed2;
    Mach2 = std::min(su2double(1.0), Mach2);
    Velocity2 = Mach2 * SoundSpeed2;
    Vel_Mag = sqrt(Velocity2);
    SoundSpeed2 = SoundSpeed_Total2 - 0.5 * Gamma_Minus_One * Velocity2;
    for (int i = 0; i < nDim; i++) Velocity[i] = Vel_Mag * Flow_Dir[i];
    const su2double Temperature = SoundSpeed2 / (Gamma * Gas_Constant);
    Pressure = P_Total * pow((Temperature / T_Total), Gamma / Gamma_Minus_One);
    Density = Pressure / (Gas_Constant * Temperature);
    G.rho = Density;
    G.p = Pressure;
  } else {
    const su2double Density = dataT / refT, Vel_Mag = dataP / refP;
    su2double SoundSpeed2 = Gamma * B.p / B.rho;
    su2double Riemann = Two_Gamma_M1 * sqrt(SoundSpeed2);
    for (int i = 0; i < nDim; i++) Riemann += B.vel[i] * n[i];
    SoundSpeed2 = Riemann;
    for (int i = 0; i < nDim; i++) SoundSpeed2 -= Vel_Mag * Flow_Dir[i] * n[i];
    SoundSpeed2 = std::max(su2double(0.0), su2double(0.5 * Gamma_Minus_One * SoundSpeed2));
    SoundSpeed2 = SoundSpeed2 * SoundSpeed2;
    G.p = SoundSpeed2 * Density / Gamma;
    G.rho = Density;
    for (int i = 0; i < nDim; i++) Velocity[i] = Vel_Mag * Flow_Dir[i];
  }
  for (int i = 0; i < 3; i++) G.vel[i] = (i < nDim) ? Velocity[i] : 0.0;
}

void OldOutlet(int nDim, const ale::State& B, const su2double& P_Exit, const su2double* n, ale::State& G) {
  const su2double Gamma = kGamma, Gamma_Minus_One = kGamma - 1.0;
  su2double Density = B.rho, Velocity2 = 0.0, Vn = 0.0, Velocity[3] = {0.0, 0.0, 0.0};
  for (int i = 0; i < nDim; i++) {
    Velocity[i] = B.vel[i];
    Velocity2 += Velocity[i] * Velocity[i];
    Vn += Velocity[i] * n[i];
  }
  su2double Pressure = B.p;
  su2double SoundSpeed = sqrt(Gamma * Pressure / Density);
  const su2double Mach_Exit = sqrt(Velocity2) / SoundSpeed;
  if (Mach_Exit >= 1.0) {
    G = B;
    return;
  }
  const su2double Entropy = Pressure * pow(1.0 / Density, Gamma);
  const su2double Riemann = Vn + 2.0 * SoundSpeed / Gamma_Minus_One;
  Density = pow(P_Exit / Entropy, 1.0 / Gamma);
  Pressure = P_Exit;
  SoundSpeed = sqrt(Gamma * P_Exit / Density);
  const su2double Vn_Exit = Riemann - 2.0 * SoundSpeed / Gamma_Minus_One;
  for (int i = 0; i < nDim; i++) Velocity[i] = Velocity[i] + (Vn_Exit - Vn) * n[i];
  G.rho = Density;
  G.p = Pressure;
  for (int i = 0; i < 3; i++) G.vel[i] = (i < nDim) ? Velocity[i] : 0.0;
}

/*--- Random unit normal. ---*/
void RandomUnitNormal(std::mt19937& gen, int nDim, su2double* n) {
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  su2double norm = 0.0;
  while (norm < 1e-2) {
    norm = 0.0;
    for (int i = 0; i < 3; ++i) {
      n[i] = (i < nDim) ? u(gen) : 0.0;
      norm += n[i] * n[i];
    }
  }
  norm = sqrt(norm);
  for (int i = 0; i < nDim; ++i) n[i] /= norm;
}

/*--- Inlet data and builder calls on the state B. ---*/
struct InletCase {
  bool massFlow = false, useNormal = false;
  su2double dataT = 1.0, dataP = 1.0, dir[3] = {0.0, 0.0, 0.0}, refT = 1.0, refP = 1.0;
};

InletCase RandomInlet(std::mt19937& gen, int nDim, const ale::State& B, const su2double* n, int variant) {
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  InletCase c;
  c.massFlow = (variant % 2 == 1);
  c.useNormal = (variant % 4 >= 2);
  c.refT = exp(0.3 * u(gen));
  c.refP = exp(0.3 * u(gen));
  /*--- Flow direction into the domain (dir.n < 0), not normalized. ---*/
  su2double dn = 0.0;
  for (int i = 0; i < nDim; ++i) {
    c.dir[i] = u(gen) - 1.5 * n[i];
    dn += c.dir[i] * n[i];
  }
  if (dn > -0.1) for (int i = 0; i < nDim; ++i) c.dir[i] -= (dn + 0.5) * n[i];
  for (int i = 0; i < nDim; ++i) c.dir[i] *= exp(0.5 * u(gen));
  const su2double T = B.p / (kGas * B.rho);
  if (!c.massFlow) {
    c.dataT = c.refT * T * (1.0 + 0.3 * (1.0 + u(gen)));
    c.dataP = c.refP * B.p * exp(0.5 + 0.3 * u(gen));
  } else {
    c.dataT = c.refT * B.rho * exp(0.3 * u(gen));
    c.dataP = c.refP * sqrt(kGamma * B.p / B.rho) * 0.3 * (1.0 + u(gen));
  }
  return c;
}

ale::State InletGhost(int nDim, const ale::State& B, const InletCase& c, const su2double* n) {
  su2double vel2 = 0.0;
  for (int i = 0; i < nDim; ++i) vel2 += B.vel[i] * B.vel[i];
  const su2double enthalpy = kGamma * B.p / ((kGamma - 1.0) * B.rho) + 0.5 * vel2;
  pbg::InletState g;
  pbg::Inlet(nDim, c.massFlow, kGamma, kGas, B.rho, B.vel, B.p, enthalpy, c.dataT, c.dataP, c.dir, c.useNormal, n,
             c.refT, c.refP, g);
  ale::State G;
  G.rho = g.Density;
  G.p = g.Pressure;
  for (int i = 0; i < 3; ++i) G.vel[i] = (i < nDim) ? g.Velocity[i] : 0.0;
  return G;
}

ale::State FarFieldGhost(int nDim, const ale::State& B, const ale::State& Inf, const su2double* n,
                         const su2double* gridVel) {
  pbg::FarFieldState g;
  pbg::FarField(nDim, kGamma, B.rho, B.vel, B.p, Inf.rho, Inf.vel, Inf.p, n, gridVel, g);
  ale::State G;
  G.rho = g.Density;
  G.p = g.Pressure;
  for (int i = 0; i < 3; ++i) G.vel[i] = (i < nDim) ? g.Velocity[i] : 0.0;
  return G;
}

ale::State OutletGhost(int nDim, const ale::State& B, const su2double& pExit, const su2double* n) {
  pbg::OutletState g;
  pbg::Outlet(nDim, kGamma, B.rho, B.vel, B.p, pExit, n, g);
  ale::State G;
  G.rho = g.Density;
  G.p = g.Pressure;
  for (int i = 0; i < 3; ++i) G.vel[i] = (i < nDim) ? g.Velocity[i] : 0.0;
  return G;
}

/*--- Relative difference of two states. ---*/
su2double StateDiff(int nDim, const ale::State& A, const ale::State& B) {
  su2double vScale = sqrt(A.p / A.rho) + sqrt(fabs(B.p / B.rho));
  su2double d = std::max(fabs(A.rho - B.rho) / fabs(A.rho), fabs(A.p - B.p) / fabs(A.p));
  for (int i = 0; i < nDim; ++i) d = std::max(d, fabs(A.vel[i] - B.vel[i]) / vScale);
  return d;
}

/*--- Point flux of closure C at the far field, inlet and outlet: F* = F_IR(u, g; N) - lam/2 |N| (U(g) - U(u)). ---*/
void TadmorPointFlux(int nDim, const ale::State& S, const ale::State& G, const su2double* N, su2double* Fstar) {
  su2double area = 0.0;
  for (int i = 0; i < nDim; ++i) area += N[i] * N[i];
  area = sqrt(area);
  ale::IsmailRoeFlux(nDim, kGamma, S, G, N, Fstar);
  su2double vnS = 0.0, vnG = 0.0;
  for (int i = 0; i < nDim; ++i) {
    vnS += S.vel[i] * N[i] / area;
    vnG += G.vel[i] * N[i] / area;
  }
  const su2double lam = std::max(fabs(vnS) + sqrt(kGamma * S.p / S.rho), fabs(vnG) + sqrt(kGamma * G.p / G.rho));
  su2double US[5], UG[5];
  ale::Conservative(nDim, kGamma, S, US);
  ale::Conservative(nDim, kGamma, G, UG);
  for (int v = 0; v < nDim + 2; ++v) Fstar[v] -= 0.5 * lam * area * (UG[v] - US[v]);
}

}  // namespace

TEST_CASE("BC_ENTROPY_P1 viscous: ghost builders equal the old weak boundary states", "[P1Flux]") {
  /*--- P1BoundaryGhost against the transcribed ghosts of BC_Far_Field_Residual (static and moving grid),
   * BC_Inlet_Residual (total conditions, mass flow, flow direction or normal) and BC_Outlet_Residual (subsonic and
   * supersonic), on random states and normals. The operations are the same, so the states agree to round-off. ---*/
  std::mt19937 gen(5150);
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  for (int nDim : {2, 3}) {
    for (int trial = 0; trial < 400; ++trial) {
      su2double n[3];
      RandomUnitNormal(gen, nDim, n);
      ale::State B = RandomAleState(gen, nDim), Inf = RandomAleState(gen, nDim);
      if (trial % 5 == 0) for (int i = 0; i < nDim; ++i) Inf.vel[i] *= 4.0;  /*--- supersonic free streams ---*/
      su2double gridVel[3] = {0.0, 0.0, 0.0};
      for (int i = 0; i < nDim; ++i) gridVel[i] = 0.3 * u(gen);
      const su2double* gv = (trial % 2) ? gridVel : nullptr;

      ale::State Gnew = FarFieldGhost(nDim, B, Inf, n, gv), Gold;
      OldFarField(nDim, B, Inf, n, gv, Gold);
      INFO("far field nDim " << nDim << " trial " << trial);
      CHECK(StateDiff(nDim, Gnew, Gold) < 1e-13);

      const InletCase c = RandomInlet(gen, nDim, B, n, trial % 4);
      const ale::State Inew = InletGhost(nDim, B, c, n);
      ale::State Iold;
      OldInlet(nDim, c.massFlow, B, c.dataT, c.dataP, c.dir, c.useNormal, n, c.refT, c.refP, Iold);
      INFO("inlet variant " << trial % 4);
      CHECK(StateDiff(nDim, Inew, Iold) < 1e-13);

      ale::State Bout = B;
      if (trial % 3 == 0) for (int i = 0; i < nDim; ++i) Bout.vel[i] *= 5.0;  /*--- supersonic exits ---*/
      const su2double pExit = B.p * exp(0.5 * u(gen));
      const ale::State Onew = OutletGhost(nDim, Bout, pExit, n);
      ale::State Oold;
      OldOutlet(nDim, Bout, pExit, n, Oold);
      CHECK(StateDiff(nDim, Onew, Oold) < 1e-13);
    }
  }
}

TEST_CASE("BC_ENTROPY_P1 viscous: inlet outlet and far field points satisfy the Tadmor inequality of Lemma B7", "[P1Flux]") {
  /*--- At a face point with the projected state u and the ghost g of each kind: (w_g - w).F* <= (psi_g - psi).N with
   * psi = rho u, i.e. psi.N - w.F* <= psi_g.N - w_g.F* (Lemma B7), for F* = F_IR - lam/2 |N| (U_g - U). Inadmissible
   * ghosts are what the code refuses; for physical data they do not occur here. ---*/
  std::mt19937 gen(6061);
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  for (int nDim : {2, 3}) {
    int admissible = 0, total = 0;
    for (int trial = 0; trial < 600; ++trial) {
      su2double n[3], N[3] = {0.0, 0.0, 0.0};
      RandomUnitNormal(gen, nDim, n);
      const su2double area = exp(u(gen));
      for (int i = 0; i < nDim; ++i) N[i] = area * n[i];
      const ale::State S = RandomAleState(gen, nDim);
      ale::State G;
      const int kind = trial % 3;
      if (kind == 0) G = FarFieldGhost(nDim, S, RandomAleState(gen, nDim), n, nullptr);
      if (kind == 1) G = InletGhost(nDim, S, RandomInlet(gen, nDim, S, n, trial % 4), n);
      if (kind == 2) G = OutletGhost(nDim, S, S.p * exp(0.5 * u(gen)), n);
      ++total;
      if (!pbg::Admissible(G.rho, G.p)) continue;
      ++admissible;

      su2double Fstar[5], w[5], wg[5];
      TadmorPointFlux(nDim, S, G, N, Fstar);
      ale::EntropyVariables(nDim, kGamma, S, w);
      ale::EntropyVariables(nDim, kGamma, G, wg);
      su2double lhs = 0.0, psiN = 0.0, psiGN = 0.0, scale = 0.0;
      for (int v = 0; v < nDim + 2; ++v) {
        lhs += (wg[v] - w[v]) * Fstar[v];
        scale += fabs(wg[v] * Fstar[v]) + fabs(w[v] * Fstar[v]);
      }
      for (int i = 0; i < nDim; ++i) {
        psiN += S.rho * S.vel[i] * N[i];
        psiGN += G.rho * G.vel[i] * N[i];
      }
      scale += fabs(psiN) + fabs(psiGN);
      INFO("nDim " << nDim << " kind " << kind << " trial " << trial);
      CHECK(lhs <= (psiGN - psiN) + 1e-12 * scale);

      su2double wF = 0.0, wgF = 0.0;
      for (int v = 0; v < nDim + 2; ++v) {
        wF += w[v] * Fstar[v];
        wgF += wg[v] * Fstar[v];
      }
      CHECK(psiN - wF <= psiGN - wgF + 1e-12 * scale);
    }
    INFO("nDim " << nDim);
    CHECK(admissible >= 0.95 * total);
  }
}

TEST_CASE("BC_ENTROPY_P1 viscous: symmetry SAT entropy sign, normal force only, corner with a wall, Jacobian", "[P1Flux]") {
  /*--- Theorem S1 (P1_NOSLIP.md, 11.3): with P_s = nu nu^T on the momentum rows and g = 0, sum_a w_a.(R_a + b_a) >= 0 on
   * one element (m_T = 1), and with a wall face and a symmetry face on the same element (m_T = 2, 11.4). The SAT has no
   * mass and energy total and its momentum total is along nu. NoSlipWallProduction is -sum_c w_c.b_c. Picard Jacobian
   * against central differences. ---*/
  std::mt19937 gen(7377);
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  for (int nDim : {2, 3}) {
    const int nNodes = nDim + 1, nVar = nDim + 2;
    const unsigned short faceSym[3] = {1, 2, 3};   /*--- face opposite node 0 ---*/
    const unsigned short faceWall[3] = {0, 2, 3};  /*--- face opposite node 1 ---*/
    for (int trial = 0; trial < 200; ++trial) {
      const su2double sipg = (trial % 4 < 2) ? 1.0 : 0.0;
      su2double x[4][3];
      pve::NodeState state[4];
      RandomElement(gen, nDim, (trial % 3) ? 1.0 : 1e-3, x, state);
      pve::Element E;
      REQUIRE(E.Set(nDim, x, state, kGas) > 0.0);
      su2double Ns[3] = {0.0, 0.0, 0.0}, Nw[3] = {0.0, 0.0, 0.0};
      pve::WallFaceNormal(nDim, x, 0, faceSym, Ns);
      pve::WallFaceNormal(nDim, x, 1, faceWall, Nw);
      su2double area = 0.0, nu[3] = {0.0, 0.0, 0.0};
      for (int i = 0; i < nDim; ++i) area += Ns[i] * Ns[i];
      area = sqrt(area);
      for (int i = 0; i < nDim; ++i) nu[i] = Ns[i] / area;

      su2double res[4][5], sym[4][5], sym2[4][5], wall2[4][5];
      E.Residual(res);
      pve::NoSlipWall(E, nDim, faceSym, Ns, false, 1.0, 1, sipg, 2.5, sym, nullptr, nullptr, nullptr, nullptr, nullptr,
                      true);
      pve::NoSlipWall(E, nDim, faceSym, Ns, false, 1.0, 2, sipg, 2.5, sym2, nullptr, nullptr, nullptr, nullptr, nullptr,
                      true);
      pve::NoSlipWall(E, nDim, faceWall, Nw, false, 1.0, 2, sipg, 2.5, wall2);

      su2double rate = 0.0, rate2 = 0.0, scale = 0.0, wSym = 0.0, total[5] = {0.0}, totScale = 0.0;
      for (int a = 0; a < nNodes; ++a) {
        su2double w[5];
        FullEntropyVariables(nDim, state[a], w);
        for (int v = 0; v < nVar; ++v) {
          rate += w[v] * (res[a][v] + sym[a][v]);
          rate2 += w[v] * (res[a][v] + sym2[a][v] + wall2[a][v]);
          scale += fabs(w[v] * res[a][v]) + fabs(w[v] * sym[a][v]) + fabs(w[v] * sym2[a][v]) + fabs(w[v] * wall2[a][v]);
          wSym += w[v] * sym[a][v];
          total[v] += sym[a][v];
          totScale = std::max(totScale, fabs(sym[a][v]));
        }
      }
      INFO("nDim " << nDim << " trial " << trial);
      CHECK(rate >= -1e-12 * scale);
      CHECK(rate2 >= -1e-12 * scale);
      CHECK(fabs(pve::NoSlipWallProduction(E, nDim, faceSym, Ns, false, 1.0, 1, sipg, 2.5, nullptr, nullptr, true) + wSym)
            <= 1e-12 * scale);

      /*--- Totals: no mass, no energy, momentum along nu. ---*/
      CHECK(fabs(total[0]) <= 1e-12 * totScale);
      CHECK(fabs(total[nDim + 1]) <= 1e-12 * totScale);
      su2double tn = 0.0;
      for (int i = 0; i < nDim; ++i) tn += total[i + 1] * nu[i];
      for (int i = 0; i < nDim; ++i) CHECK(fabs(total[i + 1] - tn * nu[i]) <= 1e-12 * totScale);

      /*--- Picard Jacobian. ---*/
      su2double dwdu[4][5][5], jac[4][4][5][5], jacMax = 0.0, jacErr = 0.0;
      for (int a = 0; a < nNodes; ++a) pve::EntropyVariableJacobian(nDim, kGamma, state[a], dwdu[a]);
      pve::NoSlipWall(E, nDim, faceSym, Ns, false, 1.0, 1, sipg, 2.5, sym, dwdu, jac, nullptr, nullptr, nullptr, true);
      for (int b = 0; b < nNodes; ++b) {
        for (int j = 0; j < nVar; ++j) {
          const su2double h = 1e-6;
          pve::NodeState plus[4], minus[4];
          for (int a = 0; a < nNodes; ++a) plus[a] = minus[a] = state[a];
          Perturb(nDim, plus[b], j, h);
          Perturb(nDim, minus[b], j, -h);
          su2double rp[4][5], rm[4][5];
          pve::NoSlipWall(Frozen(E, plus), nDim, faceSym, Ns, false, 1.0, 1, sipg, 2.5, rp, nullptr, nullptr, nullptr,
                          nullptr, nullptr, true);
          pve::NoSlipWall(Frozen(E, minus), nDim, faceSym, Ns, false, 1.0, 1, sipg, 2.5, rm, nullptr, nullptr, nullptr,
                          nullptr, nullptr, true);
          for (int a = 0; a < nNodes; ++a) {
            for (int v = 0; v < nVar; ++v) {
              jacErr = std::max(jacErr, fabs((rp[a][v] - rm[a][v]) / (2 * h) - jac[a][b][v][j]));
              jacMax = std::max(jacMax, fabs(jac[a][b][v][j]));
            }
          }
        }
      }
      CHECK(jacErr <= 1e-6 * jacMax);
    }
  }
}

TEST_CASE("BC_ENTROPY_P1 viscous: free stream preservation of the ghosts and the symmetry SAT", "[P1Flux]") {
  /*--- A uniform state with consistent marker data is its own ghost: far field with the state as free stream, outlet
   * with p_e = p (subsonic) or any p_e (supersonic), inlet with the total conditions or the mass flow of the state.
   * With F* = F(u).N closure C keeps the free stream (Thm B9 iii). A uniform flow tangent to a symmetry face gives a
   * zero SAT (no gradient, u.n = 0). ---*/
  std::mt19937 gen(8181);
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  for (int nDim : {2, 3}) {
    for (int trial = 0; trial < 200; ++trial) {
      su2double n[3];
      RandomUnitNormal(gen, nDim, n);
      ale::State S = RandomAleState(gen, nDim);
      const su2double c = sqrt(kGamma * S.p / S.rho);
      INFO("nDim " << nDim << " trial " << trial);

      /*--- Far field, any Mach number. ---*/
      ale::State Sfast = S;
      if (trial % 4 == 0) for (int i = 0; i < nDim; ++i) Sfast.vel[i] *= 4.0;
      CHECK(StateDiff(nDim, FarFieldGhost(nDim, Sfast, Sfast, n, nullptr), Sfast) < 1e-12);

      /*--- Outlet: subsonic with p_e = p, supersonic with any p_e. ---*/
      su2double vel2 = 0.0;
      for (int i = 0; i < nDim; ++i) vel2 += S.vel[i] * S.vel[i];
      if (vel2 < c * c) CHECK(StateDiff(nDim, OutletGhost(nDim, S, S.p, n), S) < 1e-12);
      ale::State Ssup = S;
      const su2double factor = 1.5 * c / sqrt(vel2 + 1e-30);
      for (int i = 0; i < nDim; ++i) Ssup.vel[i] *= factor;
      CHECK(StateDiff(nDim, OutletGhost(nDim, Ssup, 0.3 * S.p, n), Ssup) < 1e-12);

      /*--- Inlet: subsonic inflow (v.n < 0) along the flow direction of the state. ---*/
      ale::State Sin = S;
      su2double vn = 0.0;
      for (int i = 0; i < nDim; ++i) vn += Sin.vel[i] * n[i];
      for (int i = 0; i < nDim; ++i) Sin.vel[i] -= (vn + 0.3 * c) * n[i];
      su2double v2 = 0.0;
      for (int i = 0; i < nDim; ++i) v2 += Sin.vel[i] * Sin.vel[i];
      if (v2 >= 0.81 * c * c) {
        for (int i = 0; i < nDim; ++i) Sin.vel[i] *= 0.9 * c / sqrt(v2);
        v2 = 0.81 * c * c;
      }
      InletCase total, mass;
      const su2double T = Sin.p / (kGas * Sin.rho), mach2 = v2 / (c * c);
      total.dataT = T * (1.0 + 0.5 * (kGamma - 1.0) * mach2);
      total.dataP = Sin.p * pow(total.dataT / T, kGamma / (kGamma - 1.0));
      mass.massFlow = true;
      mass.dataT = Sin.rho;
      mass.dataP = sqrt(v2);
      for (int i = 0; i < nDim; ++i) total.dir[i] = mass.dir[i] = Sin.vel[i];
      CHECK(StateDiff(nDim, InletGhost(nDim, Sin, total, n), Sin) < 1e-10);
      CHECK(StateDiff(nDim, InletGhost(nDim, Sin, mass, n), Sin) < 1e-10);

      /*--- Symmetry SAT of a uniform flow along the face. ---*/
      su2double x[4][3];
      pve::NodeState state[4];
      RandomElement(gen, nDim, 1.0, x, state);
      const unsigned short faceSym[3] = {1, 2, 3};
      su2double Ns[3] = {0.0, 0.0, 0.0}, area = 0.0;
      pve::WallFaceNormal(nDim, x, 0, faceSym, Ns);
      for (int i = 0; i < nDim; ++i) area += Ns[i] * Ns[i];
      area = sqrt(area);
      su2double tangent[3] = {0.0, 0.0, 0.0}, tn = 0.0;
      for (int i = 0; i < nDim; ++i) tangent[i] = S.vel[i];
      for (int i = 0; i < nDim; ++i) tn += tangent[i] * Ns[i] / area;
      for (int i = 0; i < nDim; ++i) tangent[i] -= tn * Ns[i] / area;
      for (int a = 0; a <= nDim; ++a) {
        state[a].rho = S.rho;
        state[a].p = S.p;
        for (int i = 0; i < 3; ++i) state[a].vel[i] = (i < nDim) ? tangent[i] : 0.0;
      }
      pve::Element E;
      REQUIRE(E.Set(nDim, x, state, kGas) > 0.0);
      su2double res[4][5], sym[4][5], scale = S.p * area;
      E.Residual(res);
      pve::NoSlipWall(E, nDim, faceSym, Ns, false, 1.0, 1, pve::NOSLIP_SIPG, pve::NOSLIP_PENALTY, sym, nullptr, nullptr,
                      nullptr, nullptr, nullptr, true);
      for (int a = 0; a <= nDim; ++a)
        for (int v = 0; v < nDim + 2; ++v) CHECK(fabs(res[a][v] + sym[a][v]) <= 1e-12 * scale);
    }
  }
}

namespace {

/*--- VISC_P1_JACOBIAN: Sutherland viscosity and constant Prandtl conductivity (non-dimensional), mu_t frozen (SA). ---*/
constexpr double kMuRef = 0.8, kTRef = 1.2, kSuth = 0.38, kPrLam = 0.72, kPrTurb = 0.9;

pve::TransportLaw SutherlandLaw() {
  pve::TransportLaw law;
  law.sutherland = true;
  law.sutherlandS = kSuth;
  law.constantPrandtl = true;
  law.prandtlLam = kPrLam;
  return law;
}

/*--- mu and kappa of a node recomputed from its temperature (as CNSVariable and GatherNode, mu_t given), and the
 * temperature derivatives of the code (TransportDerivatives). ---*/
void RecomputeTransport(pve::NodeState& S, const su2double& muTurb) {
  const su2double T = S.p / (S.rho * kGas), cp = kGamma * kGas / (kGamma - 1.0);
  const su2double muLam = kMuRef * pow(T / kTRef, 1.5) * (kTRef + kSuth) / (T + kSuth);
  S.mu = muLam + muTurb;
  S.kappa = cp * muLam / kPrLam + cp * muTurb / kPrTurb;
  pve::TransportDerivatives(muLam, cp, kGas, SutherlandLaw(), S);
}

/*--- Squared Frobenius norms per block type of a finite-difference Jacobian and of its part missing from an analytic
 * one, summed over the draws, and the largest relative missing part of one draw. ---*/
struct BlockNorms {
  su2double fd[3] = {0.0, 0.0, 0.0}, missing[3] = {0.0, 0.0, 0.0}, worst[3] = {0.0, 0.0, 0.0};
  su2double drawFd[3] = {0.0, 0.0, 0.0}, drawMissing[3] = {0.0, 0.0, 0.0};
  void Add(int type, const su2double& exact, const su2double& approx) {
    drawFd[type] += exact * exact;
    drawMissing[type] += (exact - approx) * (exact - approx);
  }
  void EndDraw() {
    for (int t = 0; t < 3; ++t) {
      fd[t] += drawFd[t];
      missing[t] += drawMissing[t];
      if (drawFd[t] > 0.0) worst[t] = std::max(worst[t], su2double(sqrt(drawMissing[t] / drawFd[t])));
      drawFd[t] = drawMissing[t] = 0.0;
    }
  }
  su2double Relative(int t) const { return (fd[t] > 0.0) ? su2double(sqrt(missing[t] / fd[t])) : su2double(0.0); }
  su2double Total() const {
    const su2double f = fd[0] + fd[1] + fd[2], m = missing[0] + missing[1] + missing[2];
    return (f > 0.0) ? su2double(sqrt(m / f)) : su2double(0.0);
  }
};

/*--- Report ||J_fd - J||/||J_fd|| per block type (INFO, and WARN on the master rank so that it is always printed). ---*/
void ReportMissing(const std::string& what, const BlockNorms& norms, const std::vector<std::string>& names) {
  std::ostringstream msg;
  msg.precision(3);
  msg << "VISC_P1_JACOBIAN " << what << ", ||J_fd - J||/||J_fd|| per block type:";
  for (size_t t = 0; t < names.size(); ++t)
    msg << " " << names[t] << " " << norms.Relative(static_cast<int>(t)) << " (largest draw " << norms.worst[t] << ");";
  msg << " all blocks " << norms.Total();
  INFO(msg.str());
  CHECK(std::isfinite(SU2_TYPE::GetValue(norms.Total())));
  if (SU2_MPI::GetRank() == MASTER_NODE) {
    WARN(msg.str());
  }
}

}  // namespace

TEST_CASE("VISC_P1_JACOBIAN: simplex element and weak wall Jacobians against finite differences with recomputed mu and kappa", "[P1Flux]") {
  /*--- The finite differences recompute every coefficient from the state: the frozen state theta*, v* and mu(T),
   * kappa(T) (Sutherland, constant Prandtl), mu_t frozen. The Picard Jacobian (K frozen) must match the differences
   * of the frozen-K residual; FULL (Picard + dK/du) must match the full differences. The missing part of Picard is
   * reported per block type. ---*/
  std::mt19937 gen(97531);
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  const unsigned short faceNodes[3] = {1, 2, 3};
  const std::vector<std::string> wallNames = {"adiabatic wall", "isothermal wall", "symmetry SAT"};
  for (int nDim : {2, 3}) {
    const int nNodes = nDim + 1, nVar = nDim + 2;
    BlockNorms elemNorms, wallNorms[3];
    for (int trial = 0; trial < 120; ++trial) {
      const double stretch = (trial % 2) ? 1.0 : 1e-3;
      su2double x[4][3];
      pve::NodeState state[4];
      RandomElement(gen, nDim, stretch, x, state);
      su2double muTurb[4] = {0.0, 0.0, 0.0, 0.0};
      for (int a = 0; a < nNodes; ++a) {
        if (trial % 3 != 0) muTurb[a] = exp(3.0 * u(gen));
        RecomputeTransport(state[a], muTurb[a]);
      }

      /*--- 3D: moved test vectors on every fourth element (theta_v frozen). ---*/
      const bool moved = (nDim == 3) && (trial % 4 == 3);
      su2double Y[9] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
      pve::Element E;
      REQUIRE(E.Set(nDim, x, state, kGas) > 0.0);
      if (moved) {
        for (auto& y : Y) y = 0.3 * E.size * u(gen);
        E.SetMovedTest(Y, 0.6);
      }
      auto recomputed = [&](const pve::NodeState* S) {
        pve::NodeState R[4];
        for (int a = 0; a < nNodes; ++a) {
          R[a] = S[a];
          RecomputeTransport(R[a], muTurb[a]);
        }
        pve::Element F;
        F.Set(nDim, x, R, kGas);
        if (moved) F.SetMovedTest(Y, 0.6);
        return F;
      };

      /*--- Wall face opposite node 0: adiabatic, isothermal or symmetry plane, SIPG or IIPG. ---*/
      const int kind = (trial / 2) % 3;
      const bool isothermal = (kind == 1), symmetry = (kind == 2);
      const su2double sipg = (trial % 4 < 2) ? 1.0 : 0.0, wallTheta = 1.0 / (kGas * exp(u(gen)));
      su2double N[3] = {0.0, 0.0, 0.0};
      pve::WallFaceNormal(nDim, x, 0, faceNodes, N);
      auto wall = [&](const pve::Element& F, su2double res[][5]) {
        pve::NoSlipWall(F, nDim, faceNodes, N, isothermal, wallTheta, 1, sipg, pve::NOSLIP_PENALTY, res, nullptr, nullptr,
                        nullptr, nullptr, nullptr, symmetry);
      };

      /*--- Analytic Jacobians: Picard and FULL. ---*/
      su2double dwdu[4][5][5], elemP[4][4][5][5], elemF[4][4][5][5], wallP[4][4][5][5], wallF[4][4][5][5], res[4][5];
      for (int a = 0; a < nNodes; ++a) pve::EntropyVariableJacobian(nDim, kGamma, state[a], dwdu[a]);
      E.Jacobian(dwdu, elemP);
      pve::NoSlipWall(E, nDim, faceNodes, N, isothermal, wallTheta, 1, sipg, pve::NOSLIP_PENALTY, res, dwdu, wallP,
                      nullptr, nullptr, nullptr, symmetry);
      for (int a = 0; a < nNodes; ++a)
        for (int b = 0; b < nNodes; ++b)
          for (int v = 0; v < nVar; ++v)
            for (int j = 0; j < nVar; ++j) {
              elemF[a][b][v][j] = elemP[a][b][v][j];
              wallF[a][b][v][j] = wallP[a][b][v][j];
            }
      E.AddStateJacobian(state, dwdu, elemF);
      pve::NoSlipWallStateJacobian(E, state, dwdu, nDim, faceNodes, N, isothermal, wallTheta, 1, sipg,
                                   pve::NOSLIP_PENALTY, wallF, nullptr, nullptr, symmetry);

      su2double errP = 0.0, maxP = 0.0, errF = 0.0, maxF = 0.0, wErrP = 0.0, wMaxP = 0.0, wErrF = 0.0, wMaxF = 0.0;
      for (int b = 0; b < nNodes; ++b) {
        for (int j = 0; j < nVar; ++j) {
          const su2double h = 1e-6;
          pve::NodeState plus[4], minus[4];
          for (int a = 0; a < nNodes; ++a) plus[a] = minus[a] = state[a];
          Perturb(nDim, plus[b], j, h);
          Perturb(nDim, minus[b], j, -h);

          /*--- K frozen: the Picard part. ---*/
          su2double rp[4][5], rm[4][5], wp[4][5], wm[4][5];
          const auto frozenPlus = Frozen(E, plus), frozenMinus = Frozen(E, minus);
          frozenPlus.Residual(rp);
          frozenMinus.Residual(rm);
          wall(frozenPlus, wp);
          wall(frozenMinus, wm);
          for (int a = 0; a < nNodes; ++a) {
            for (int v = 0; v < nVar; ++v) {
              errP = std::max(errP, su2double(fabs((rp[a][v] - rm[a][v]) / (2 * h) - elemP[a][b][v][j])));
              maxP = std::max(maxP, su2double(fabs(elemP[a][b][v][j])));
              wErrP = std::max(wErrP, su2double(fabs((wp[a][v] - wm[a][v]) / (2 * h) - wallP[a][b][v][j])));
              wMaxP = std::max(wMaxP, su2double(fabs(wallP[a][b][v][j])));
            }
          }

          /*--- Coefficients recomputed from the state: the exact Jacobian. ---*/
          const auto fullPlus = recomputed(plus), fullMinus = recomputed(minus);
          fullPlus.Residual(rp);
          fullMinus.Residual(rm);
          wall(fullPlus, wp);
          wall(fullMinus, wm);
          for (int a = 0; a < nNodes; ++a) {
            const int type = (a == b) ? 0 : 1;
            for (int v = 0; v < nVar; ++v) {
              const su2double fdElem = (rp[a][v] - rm[a][v]) / (2 * h), fdWall = (wp[a][v] - wm[a][v]) / (2 * h);
              errF = std::max(errF, su2double(fabs(fdElem - elemF[a][b][v][j])));
              maxF = std::max(maxF, su2double(fabs(elemF[a][b][v][j])));
              wErrF = std::max(wErrF, su2double(fabs(fdWall - wallF[a][b][v][j])));
              wMaxF = std::max(wMaxF, su2double(fabs(wallF[a][b][v][j])));
              elemNorms.Add(type, fdElem, elemP[a][b][v][j]);
              wallNorms[kind].Add(type, fdWall, wallP[a][b][v][j]);
            }
          }
        }
      }
      elemNorms.EndDraw();
      wallNorms[kind].EndDraw();
      INFO("nDim " << nDim << ", trial " << trial << ", wall kind " << kind << ", moved " << moved);
      CHECK(errP <= 1e-6 * maxP);
      CHECK(wErrP <= 1e-6 * wMaxP);
      CHECK(errF <= 1e-6 * maxF);
      CHECK(wErrF <= 1e-6 * wMaxF);
    }
    const std::vector<std::string> names = {"diagonal", "edge"};
    ReportMissing(std::to_string(nDim) + "D simplex element, Picard", elemNorms, names);
    for (int kind = 0; kind < 3; ++kind)
      ReportMissing(std::to_string(nDim) + "D " + wallNames[kind] + ", Picard", wallNorms[kind], names);
  }
}

TEST_CASE("VISC_P1_JACOBIAN: split averages of prisms and hexahedra and the dropped non-edge blocks", "[P1Flux]") {
  /*--- One cell, V-A on the sub-simplices of its split average with weights omega_t (CNSSolver::MixedViscousResidual).
   * The cell Jacobian has blocks between nodes that are not edges of the cell (face and body diagonals); they are not
   * in the edge sparsity pattern of the matrix, so the solver drops them with Picard and with FULL. Reported: the part
   * of the full finite-difference Jacobian missing from the assembled Picard and FULL Jacobians, per block type. ---*/
  std::mt19937 gen(24680);
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  const int nDim = 3, nVar = 5;
  const std::string mesh = "p1_visc_jacobian_mixed_3d.su2";
  WriteMixedMesh(mesh, nDim, 3, 0.0, 1.0);
  MeshCase test(mesh, MixedOptions);
  auto& geometry = static_cast<CPhysicalGeometry&>(*test.geometry);
  const std::vector<std::string> names = {"diagonal", "edge", "non-edge"};

  for (const int vtk : {PRISM, HEXAHEDRON}) {
    const auto cell = MakeCell(vtk);
    const auto& list = geometry.SetMixedSimplices(cell.get());
    const int nCell = cell->GetnNodes();
    INFO("VTK type " << vtk);

    /*--- 0 diagonal, 1 edge of the cell, 2 any other pair (not in the sparsity pattern). ---*/
    auto blockType = [&](int i, int k) {
      if (i == k) return 0;
      for (unsigned short n = 0; n < cell->GetnNeighbor_Nodes(i); ++n)
        if (cell->GetNeighbor_Nodes(i, n) == k) return 1;
      return 2;
    };
    auto at = [&](int i, int k, int v, int j) { return ((i * nCell + k) * nVar + v) * nVar + j; };

    BlockNorms picardNorms, fullNorms;
    for (int draw = 0; draw < 12; ++draw) {
      const auto x = CellCoords(gen, vtk, 0.12, (draw % 2) ? 1e-2 : 1.0);
      std::vector<pve::NodeState> state(nCell);
      std::vector<su2double> muTurb(nCell, 0.0);
      for (int i = 0; i < nCell; ++i) {
        auto& S = state[i];
        S.rho = exp(u(gen));
        S.p = S.rho * kGas * exp(u(gen));
        for (int d = 0; d < 3; ++d) S.vel[d] = 1.5 * u(gen);
        if (draw % 3 != 0) muTurb[i] = exp(3.0 * u(gen));
        RecomputeTransport(S, muTurb[i]);
      }

      /*--- Sub-simplices at the base state and the omega-weighted Picard and FULL cell Jacobians (all blocks). ---*/
      std::vector<pve::Element> base(list.size());
      std::vector<su2double> JP(nCell * nCell * nVar * nVar, 0.0), JF(nCell * nCell * nVar * nVar, 0.0);
      for (size_t t = 0; t < list.size(); ++t) {
        const su2double* X[4] = {nullptr, nullptr, nullptr, nullptr};
        pve::NodeState sub[4];
        for (int a = 0; a <= nDim; ++a) {
          X[a] = x[list[t].nodes[a]].data();
          sub[a] = state[list[t].nodes[a]];
        }
        REQUIRE(base[t].Set(nDim, X, sub, kGas) > 0.0);
        su2double dwdu[4][5][5], jacP[4][4][5][5], jacF[4][4][5][5];
        for (int a = 0; a <= nDim; ++a) pve::EntropyVariableJacobian(nDim, kGamma, sub[a], dwdu[a]);
        base[t].Jacobian(dwdu, jacP);
        for (int a = 0; a < 4; ++a)
          for (int b = 0; b < 4; ++b)
            for (int v = 0; v < 5; ++v)
              for (int j = 0; j < 5; ++j) jacF[a][b][v][j] = jacP[a][b][v][j];
        base[t].AddStateJacobian(sub, dwdu, jacF);
        for (int a = 0; a <= nDim; ++a)
          for (int b = 0; b <= nDim; ++b)
            for (int v = 0; v < nVar; ++v)
              for (int j = 0; j < nVar; ++j) {
                JP[at(list[t].nodes[a], list[t].nodes[b], v, j)] += list[t].weight * jacP[a][b][v][j];
                JF[at(list[t].nodes[a], list[t].nodes[b], v, j)] += list[t].weight * jacF[a][b][v][j];
              }
      }

      /*--- Cell residual, K of each sub-simplex frozen at the base state or recomputed from the state. ---*/
      auto residual = [&](const std::vector<pve::NodeState>& S, bool recompute) {
        std::vector<std::array<su2double, 5>> R(nCell, {0, 0, 0, 0, 0});
        for (size_t t = 0; t < list.size(); ++t) {
          const su2double* X[4] = {nullptr, nullptr, nullptr, nullptr};
          pve::NodeState sub[4];
          for (int a = 0; a <= nDim; ++a) {
            X[a] = x[list[t].nodes[a]].data();
            sub[a] = S[list[t].nodes[a]];
            if (recompute) RecomputeTransport(sub[a], muTurb[list[t].nodes[a]]);
          }
          pve::Element Et;
          if (recompute) Et.Set(nDim, X, sub, kGas);
          else Et = Frozen(base[t], sub);
          su2double r[4][5];
          Et.Residual(r);
          for (int a = 0; a <= nDim; ++a)
            for (int v = 0; v < nVar; ++v) R[list[t].nodes[a]][v] += list[t].weight * r[a][v];
        }
        return R;
      };

      su2double errP = 0.0, maxP = 0.0, errF = 0.0, maxF = 0.0;
      for (int k = 0; k < nCell; ++k) {
        for (int j = 0; j < nVar; ++j) {
          const su2double h = 1e-6;
          auto plus = state, minus = state;
          Perturb(nDim, plus[k], j, h);
          Perturb(nDim, minus[k], j, -h);
          const auto Rp = residual(plus, false), Rm = residual(minus, false);
          const auto Qp = residual(plus, true), Qm = residual(minus, true);
          for (int i = 0; i < nCell; ++i) {
            const int type = blockType(i, k);
            for (int v = 0; v < nVar; ++v) {
              const su2double fdFrozen = (Rp[i][v] - Rm[i][v]) / (2 * h), fdFull = (Qp[i][v] - Qm[i][v]) / (2 * h);
              const su2double picard = JP[at(i, k, v, j)], full = JF[at(i, k, v, j)];
              errP = std::max(errP, su2double(fabs(fdFrozen - picard)));
              maxP = std::max(maxP, su2double(fabs(picard)));
              errF = std::max(errF, su2double(fabs(fdFull - full)));
              maxF = std::max(maxF, su2double(fabs(full)));
              /*--- What the solver assembles: no blocks between non-edge pairs. ---*/
              picardNorms.Add(type, fdFull, (type == 2) ? su2double(0.0) : picard);
              fullNorms.Add(type, fdFull, (type == 2) ? su2double(0.0) : full);
            }
          }
        }
      }
      picardNorms.EndDraw();
      fullNorms.EndDraw();
      INFO("draw " << draw);
      CHECK(errP <= 1e-6 * maxP);
      CHECK(errF <= 1e-6 * maxF);
    }
    const std::string cellName = (vtk == PRISM) ? "prism" : "hexahedron";
    ReportMissing(cellName + " split average, assembled Picard", picardNorms, names);
    ReportMissing(cellName + " split average, assembled FULL (non-edge blocks dropped)", fullNorms, names);
  }
}

/*--- BC_ENTROPY_P1 Jacobian (P1_BOUNDARY.md, 11): EsFaceJacobian of closure C against central differences of
 * EsFaceResidual in the conservative variables of the face nodes, segments (2D) and triangles (3D). ---*/

namespace {

namespace pbc = P1BoundaryClosure;

/*--- Random admissible state with rho, p in exp(+-2 spread) and |u_i| <= vmax. ---*/
pbc::EsState JacRandomState(std::mt19937& gen, int nDim, double spread, double vmax) {
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  pbc::EsState S;
  S.rho = exp(2.0 * spread * u(gen));
  S.p = exp(2.0 * spread * u(gen));
  for (int i = 0; i < 3; ++i) S.vel[i] = (i < nDim) ? vmax * u(gen) : 0.0;
  return S;
}

/*--- Face node state near S: relative changes of rho and p, absolute of u, all of size eps. ---*/
pbc::EsState JacPerturb(std::mt19937& gen, int nDim, const pbc::EsState& S, double eps) {
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  pbc::EsState P = S;
  P.rho = S.rho * exp(eps * u(gen));
  P.p = S.p * exp(eps * u(gen));
  for (int i = 0; i < nDim; ++i) P.vel[i] = S.vel[i] + eps * u(gen);
  return P;
}

pbc::EsState JacStateOf(int nDim, const su2double* U) {
  pbc::EsState S;
  su2double vel2 = 0.0;
  S.rho = U[0];
  for (int i = 0; i < nDim; ++i) {
    S.vel[i] = U[i + 1] / U[0];
    vel2 += S.vel[i] * S.vel[i];
  }
  S.p = (kGamma - 1.0) * (U[nDim + 1] - 0.5 * U[0] * vel2);
  return S;
}

/*--- Random unit normal n, tangent t (t.n = 0) and face area vector N = A n. ---*/
void JacRandomNormal(std::mt19937& gen, int nDim, su2double* n, su2double* t, su2double* N) {
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  RandomUnitNormal(gen, nDim, n);
  su2double r[3] = {u(gen), u(gen), u(gen)}, rn = 0.0, norm = 0.0;
  if (nDim == 2) r[2] = 0.0;
  for (int i = 0; i < nDim; ++i) rn += r[i] * n[i];
  for (int i = 0; i < 3; ++i) {
    t[i] = (i < nDim) ? r[i] - rn * n[i] : 0.0;
    norm += t[i] * t[i];
  }
  norm = sqrt(norm);
  for (int i = 0; i < nDim; ++i) t[i] /= norm;
  const su2double area = exp(u(gen));
  for (int i = 0; i < 3; ++i) N[i] = (i < nDim) ? area * n[i] : 0.0;
}

/*--- State with density, pressure from JacRandomState and velocity (Mn n + Mt t) c. ---*/
pbc::EsState JacStateWithMach(std::mt19937& gen, int nDim, const su2double* n, const su2double* t, double Mn,
                              double Mt) {
  pbc::EsState S = JacRandomState(gen, nDim, 0.5, 0.0);
  const su2double c = sqrt(kGamma * S.p / S.rho);
  for (int i = 0; i < nDim; ++i) S.vel[i] = (Mn * n[i] + Mt * t[i]) * c;
  return S;
}

/*--- max |J - J_fd| / max |J_fd| over all blocks of one face (nNodes = nDim). ---*/
su2double FaceJacobianError(int nDim, const pbc::EsBoundaryData* data, const su2double* N, const pbc::EsState* states) {
  const int nNodes = nDim, nVar = nDim + 2;
  su2double jac[3][3][pbc::ES_NVAR][pbc::ES_NVAR];
  pbc::EsFaceJacobian(nDim, nNodes, kGamma, data, N, states, jac);
  su2double err = 0.0, ref = 0.0;
  for (int j = 0; j < nNodes; ++j) {
    su2double U[pbc::ES_NVAR] = {0.0};
    pbc::EsConservative(nDim, kGamma, states[j], U);
    for (int c = 0; c < nVar; ++c) {
      const su2double h = 1e-6 * std::max(su2double(1.0), su2double(fabs(U[c])));
      su2double Up[pbc::ES_NVAR], Um[pbc::ES_NVAR];
      for (int v = 0; v < nVar; ++v) Up[v] = Um[v] = U[v];
      Up[c] += h;
      Um[c] -= h;
      pbc::EsState sp[3], sm[3];
      for (int k = 0; k < nNodes; ++k) sp[k] = sm[k] = states[k];
      sp[j] = JacStateOf(nDim, Up);
      sm[j] = JacStateOf(nDim, Um);
      su2double rp[3][pbc::ES_NVAR] = {{0.0}}, rm[3][pbc::ES_NVAR] = {{0.0}};
      pbc::EsFaceResidual(nDim, nNodes, kGamma, data, N, sp, rp);
      pbc::EsFaceResidual(nDim, nNodes, kGamma, data, N, sm, rm);
      for (int i = 0; i < nNodes; ++i) {
        for (int a = 0; a < nVar; ++a) {
          const su2double fd = (rp[i][a] - rm[i][a]) / (2.0 * h);
          err = std::max(err, su2double(fabs(fd - jac[i][j][a][c])));
          ref = std::max(ref, su2double(fabs(fd)));
        }
      }
    }
  }
  return err / ref;
}

/*--- Scalar EsIsmailRoeJacobian against ismailRoeProjJac of EDGE_FORM_EC_EXACT_JACOBIAN. ---*/
template <size_t nDim>
su2double IsmailRoeScalarVersusVectorized(std::mt19937& gen) {
  constexpr size_t nVar = nDim + 2;
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  const pbc::EsState L = JacRandomState(gen, nDim, 0.5, 0.8), R = JacRandomState(gen, nDim, 0.5, 0.8);
  su2double n[3] = {0.0, 0.0, 0.0};
  VectorDbl<nDim> normal;
  for (size_t d = 0; d < nDim; ++d) {
    n[d] = u(gen);
    normal(d) = n[d];
  }
  su2double jL[pbc::ES_NVAR][pbc::ES_NVAR], jR[pbc::ES_NVAR][pbc::ES_NVAR];
  pbc::EsIsmailRoeJacobian(nDim, kGamma, L, R, n, jL, jR);
  const auto VL = TeCNOAdjointChecks::Primitives<nDim>(L.rho, L.vel, L.p);
  const auto VR = TeCNOAdjointChecks::Primitives<nDim>(R.rho, R.vel, R.p);
  MatrixDbl<nVar> jac_i, jac_k;
  ismailRoeProjJac(Double(TeCNOAdjointChecks::GAMMA), VL, VR, normal, jac_i, jac_k);
  su2double err = 0.0, ref = 0.0;
  for (size_t a = 0; a < nVar; ++a) {
    for (size_t b = 0; b < nVar; ++b) {
      err = std::max(err, su2double(fabs(jL[a][b] - jac_i(a, b)[0])));
      err = std::max(err, su2double(fabs(jR[a][b] - jac_k(a, b)[0])));
      ref = std::max(ref, su2double(fabs(jac_i(a, b)[0])));
      ref = std::max(ref, su2double(fabs(jac_k(a, b)[0])));
    }
  }
  return err / ref;
}

}  // namespace

TEST_CASE("BC_ENTROPY_P1 Jacobian: far field faces in the four characteristic regimes against finite differences", "[P1Flux]") {
  /*--- Free stream normal Mach number -1.6 (supersonic inflow), -0.5 (subsonic inflow), 0.5 (subsonic outflow), 1.6
   * (supersonic outflow), each with a tangential component; face states near a random interior state. ---*/
  std::mt19937 gen(9101);
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  const double regimes[4] = {-1.6, -0.5, 0.5, 1.6};
  for (int nDim : {2, 3}) {
    for (int regime = 0; regime < 4; ++regime) {
      su2double worst = 0.0;
      for (int trial = 0; trial < 40; ++trial) {
        su2double n[3], t[3], N[3];
        JacRandomNormal(gen, nDim, n, t, N);
        pbc::EsBoundaryData data;
        data.kind = FAR_FIELD;
        data.tag = "farfield";
        data.inf = JacStateWithMach(gen, nDim, n, t, regimes[regime] + 0.1 * u(gen), 0.5 * u(gen));
        const pbc::EsState base = JacRandomState(gen, nDim, 0.5, 0.8);
        const double eps = (trial % 2) ? 0.2 : 0.02;
        pbc::EsState states[3];
        for (int k = 0; k < nDim; ++k) states[k] = JacPerturb(gen, nDim, base, eps);
        worst = std::max(worst, FaceJacobianError(nDim, &data, N, states));
      }
      INFO("nDim " << nDim << " regime " << regimes[regime] << " worst relative error " << worst);
      CHECK(worst < 1e-6);
    }
  }
}

TEST_CASE("BC_ENTROPY_P1 Jacobian: inlet faces with total conditions and mass flow against finite differences", "[P1Flux]") {
  /*--- Subsonic inflow interior states (u.n about -0.4 c), nodal marker data as for the inlet ghost test: total
   * conditions above the static state, or density and velocity magnitude; flow direction or the face normal. ---*/
  std::mt19937 gen(9202);
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  for (int nDim : {2, 3}) {
    for (int variant = 0; variant < 4; ++variant) {
      su2double worst = 0.0;
      for (int trial = 0; trial < 40; ++trial) {
        su2double n[3], t[3], N[3];
        JacRandomNormal(gen, nDim, n, t, N);
        const pbc::EsState base = JacStateWithMach(gen, nDim, n, t, -0.4 + 0.2 * u(gen), 0.3 * u(gen));
        const double eps = (trial % 2) ? 0.2 : 0.02;
        pbc::EsState states[3];
        for (int k = 0; k < nDim; ++k) states[k] = JacPerturb(gen, nDim, base, eps);

        pbc::EsBoundaryData data;
        data.kind = INLET_FLOW;
        data.tag = "inlet";
        data.massFlow = (variant % 2 == 1);
        data.useNormal = (variant >= 2);
        data.gasConst = kGas;
        data.refT = exp(0.3 * u(gen));
        data.refP = exp(0.3 * u(gen));
        const su2double T = base.p / (kGas * base.rho), c = sqrt(kGamma * base.p / base.rho);
        for (int k = 0; k < nDim; ++k) {
          su2double dn = 0.0;
          for (int i = 0; i < nDim; ++i) {
            data.flowDir[k][i] = u(gen) - 1.5 * n[i];
            dn += data.flowDir[k][i] * n[i];
          }
          if (dn > -0.1) for (int i = 0; i < nDim; ++i) data.flowDir[k][i] -= (dn + 0.5) * n[i];
          if (!data.massFlow) {
            data.dataT[k] = data.refT * T * (1.0 + 0.3 * (1.0 + u(gen)));
            data.dataP[k] = data.refP * base.p * exp(0.5 + 0.3 * u(gen));
          } else {
            data.dataT[k] = data.refT * base.rho * exp(0.3 * u(gen));
            data.dataP[k] = data.refP * c * 0.3 * (1.0 + u(gen));
          }
        }
        worst = std::max(worst, FaceJacobianError(nDim, &data, N, states));
      }
      INFO("nDim " << nDim << " mass flow " << (variant % 2) << " normal " << (variant >= 2) << " worst " << worst);
      CHECK(worst < 1e-6);
    }
  }
}

TEST_CASE("P1 SST matched exact Jacobians: inlet k_bc(U) and the d mu/dT part of the element k diffusion, against FD", "[P1Flux]") {
  /*--- (a) k_bc = 3/2 I^2 |v_g|^2 of the nodal inlet ghost (EsInletKineGradient) as a function of the conservative U of
   * the node with k frozen: gradient against central differences, total conditions and mass flow, flow direction or
   * normal. (b) R_a = |T| D grad k . g_a with D the vertex mean of mu(T) + sigma mu_t, Sutherland mu: the state Jacobian
   * of KineDiffusionStateJacobian against central differences in U_b (k, mu_t, F1 frozen), strong rows skipped. ---*/
  std::mt19937 gen(7316);
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  for (int nDim : {2, 3}) {
    const int nVar = nDim + 2;
    su2double worstA = 0.0, worstB = 0.0;
    for (int trial = 0; trial < 40; ++trial) {
      su2double n[3], t[3], N[3];
      JacRandomNormal(gen, nDim, n, t, N);
      const pbc::EsState base = JacStateWithMach(gen, nDim, n, t, -0.4 + 0.2 * u(gen), 0.3 * u(gen));
      pbc::EsBoundaryData data;
      data.kind = INLET_FLOW;
      data.tag = "inlet";
      data.massFlow = (trial % 2 == 1);
      data.useNormal = ((trial / 2) % 2 == 1);
      data.gasConst = kGas;
      data.refT = exp(0.3 * u(gen));
      data.refP = exp(0.3 * u(gen));
      const su2double T = base.p / (kGas * base.rho), c = sqrt(kGamma * base.p / base.rho);
      su2double dn = 0.0;
      for (int i = 0; i < nDim; ++i) {
        data.flowDir[0][i] = u(gen) - 1.5 * n[i];
        dn += data.flowDir[0][i] * n[i];
      }
      if (dn > -0.1) for (int i = 0; i < nDim; ++i) data.flowDir[0][i] -= (dn + 0.5) * n[i];
      if (!data.massFlow) {
        data.dataT[0] = data.refT * T * (1.0 + 0.3 * (1.0 + u(gen)));
        data.dataP[0] = data.refP * base.p * exp(0.5 + 0.3 * u(gen));
      } else {
        data.dataT[0] = data.refT * base.rho * exp(0.3 * u(gen));
        data.dataP[0] = data.refP * c * 0.3 * (1.0 + u(gen));
      }
      const su2double kNode = 0.02 * exp(u(gen)), intensity = 0.05 * exp(0.5 * u(gen));
      su2double U[pbc::ES_NVAR] = {0.0};
      pbc::EsConservative(nDim, kGamma, base, U);
      U[nVar - 1] += base.rho * kNode;
      auto kbc = [&](const su2double* Ux, su2double* dk) {
        su2double Uh[pbc::ES_NVAR];
        for (int v = 0; v < nVar; ++v) Uh[v] = Ux[v];
        Uh[nVar - 1] -= Ux[0] * kNode;
        const pbc::EsState S = JacStateOf(nDim, Uh);
        su2double vel2 = 0.0;
        for (int i = 0; i < nDim; ++i) vel2 += S.vel[i] * S.vel[i];
        const su2double h = kGamma * S.p / ((kGamma - 1.0) * S.rho) + 0.5 * vel2;
        return pbc::EsInletKineGradient(nDim, kGamma, data, 0, S, kNode, h, n, intensity, dk);
      };
      su2double dk[pbc::ES_NVAR] = {0.0}, dummy[pbc::ES_NVAR];
      kbc(U, dk);
      su2double err = 0.0, ref = 0.0;
      for (int cIdx = 0; cIdx < nVar; ++cIdx) {
        const su2double h = 1e-6 * std::max(su2double(1.0), su2double(fabs(U[cIdx])));
        su2double Up[pbc::ES_NVAR], Um[pbc::ES_NVAR];
        for (int v = 0; v < nVar; ++v) Up[v] = Um[v] = U[v];
        Up[cIdx] += h;
        Um[cIdx] -= h;
        const su2double fd = (kbc(Up, dummy) - kbc(Um, dummy)) / (2.0 * h);
        err = std::max(err, su2double(fabs(fd - dk[cIdx])));
        ref = std::max(ref, su2double(fabs(fd)));
      }
      /*--- Mass flow: the ghost velocity is data, dk_bc/dU = 0 (absolute check). ---*/
      if (ref > 1e-12) worstA = std::max(worstA, err / ref);
      else CHECK(err <= 1e-12);

      /*--- (b) element k diffusion, Sutherland mu. ---*/
      const int nNodes = nDim + 1;
      su2double xyz[4][3] = {{0.0}};
      for (int a = 1; a < nNodes; ++a) xyz[a][a - 1] = 1.0;
      for (int a = 0; a < nNodes; ++a)
        for (int i = 0; i < nDim; ++i) xyz[a][i] += 0.15 * u(gen);
      const su2double* coord[pve::MAXNODE];
      pve::NodeState S[pve::MAXNODE];
      su2double muT[pve::MAXNODE], F1[pve::MAXNODE];
      for (int a = 0; a < nNodes; ++a) {
        coord[a] = xyz[a];
        const pbc::EsState P = JacRandomState(gen, nDim, 0.3, 0.5);
        S[a].rho = P.rho;
        S[a].p = P.p;
        for (int i = 0; i < 3; ++i) S[a].vel[i] = P.vel[i];
        S[a].k = 0.03 * exp(u(gen));
        muT[a] = 0.5 * exp(u(gen));
        F1[a] = 0.5 * (1.0 + u(gen));
        RecomputeTransport(S[a], muT[a]);
        S[a].mut = muT[a];
      }
      pve::Element simplex;
      REQUIRE(simplex.Set(nDim, coord, S, kGas) != 0.0);
      const bool addRow[4] = {true, (trial % 3) != 0, true, true};
      su2double jac[pve::MAXNODE][pve::MAXNODE][pve::MAXNVAR][pve::MAXNVAR] = {};
      P1SSTMatched::KineDiffusionStateJacobian(nDim, kGamma, kGas, simplex.size, simplex.grad, S, addRow, jac);
      auto energyRows = [&](const pve::NodeState* St, su2double* res) {
        su2double mu[pve::MAXNODE], k[pve::MAXNODE];
        for (int a = 0; a < nNodes; ++a) {
          const su2double Tn = St[a].p / (St[a].rho * kGas);
          mu[a] = kMuRef * pow(Tn / kTRef, 1.5) * (kTRef + kSuth) / (Tn + kSuth);
          k[a] = St[a].k;
        }
        const su2double D = P1SSTMatched::ElementKineDiffusivity(nNodes, mu, muT, F1);
        P1SSTMatched::KineDiffusion(nDim, simplex.size, simplex.grad, k, D, res);
      };
      su2double errB = 0.0, refB = 0.0;
      for (int b = 0; b < nNodes; ++b) {
        pbc::EsState Pb;
        Pb.rho = S[b].rho;
        Pb.p = S[b].p;
        for (int i = 0; i < 3; ++i) Pb.vel[i] = S[b].vel[i];
        su2double Ub[pbc::ES_NVAR] = {0.0};
        pbc::EsConservative(nDim, kGamma, Pb, Ub);
        Ub[nVar - 1] += S[b].rho * S[b].k;
        for (int cIdx = 0; cIdx < nVar; ++cIdx) {
          const su2double h = 1e-6 * std::max(su2double(1.0), su2double(fabs(Ub[cIdx])));
          su2double rp[pve::MAXNODE], rm[pve::MAXNODE];
          for (int sgn : {1, -1}) {
            su2double Ux[pbc::ES_NVAR];
            for (int v = 0; v < nVar; ++v) Ux[v] = Ub[v];
            Ux[cIdx] += sgn * h;
            Ux[nVar - 1] -= Ux[0] * S[b].k;
            const pbc::EsState Q = JacStateOf(nDim, Ux);
            pve::NodeState St[pve::MAXNODE];
            for (int a = 0; a < nNodes; ++a) St[a] = S[a];
            St[b].rho = Q.rho;
            St[b].p = Q.p;
            for (int i = 0; i < 3; ++i) St[b].vel[i] = Q.vel[i];
            energyRows(St, sgn > 0 ? rp : rm);
          }
          for (int a = 0; a < nNodes; ++a) {
            const su2double fd = addRow[a] ? (rp[a] - rm[a]) / (2.0 * h) : 0.0;
            errB = std::max(errB, su2double(fabs(fd - jac[a][b][nVar - 1][cIdx])));
            refB = std::max(refB, su2double(fabs(fd)));
          }
        }
      }
      worstB = std::max(worstB, errB / refB);
    }
    INFO("nDim " << nDim << " inlet k_bc gradient worst " << worstA << " k diffusion state Jacobian worst " << worstB);
    CHECK(worstA < 1e-6);
    CHECK(worstB < 1e-6);
  }
}

TEST_CASE("BC_ENTROPY_P1 Jacobian: outlet faces with subsonic and supersonic exits and backflow against finite differences", "[P1Flux]") {
  /*--- Interior normal Mach number 0.5 (subsonic exit), 2.2 (supersonic, ghost = state), -0.3 (subsonic backflow);
   * back pressure per face node. ---*/
  std::mt19937 gen(9303);
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  const double regimes[3] = {0.5, 2.2, -0.3};
  for (int nDim : {2, 3}) {
    for (int regime = 0; regime < 3; ++regime) {
      su2double worst = 0.0;
      for (int trial = 0; trial < 40; ++trial) {
        su2double n[3], t[3], N[3];
        JacRandomNormal(gen, nDim, n, t, N);
        const pbc::EsState base = JacStateWithMach(gen, nDim, n, t, regimes[regime] + 0.1 * u(gen), 0.3 * u(gen));
        const double eps = (trial % 2) ? 0.1 : 0.02;
        pbc::EsState states[3];
        for (int k = 0; k < nDim; ++k) states[k] = JacPerturb(gen, nDim, base, eps * sqrt(kGamma * base.p / base.rho));
        pbc::EsBoundaryData data;
        data.kind = OUTLET_FLOW;
        data.tag = "outlet";
        for (int k = 0; k < nDim; ++k) data.pExit[k] = base.p * exp(0.3 * u(gen));
        worst = std::max(worst, FaceJacobianError(nDim, &data, N, states));
      }
      INFO("nDim " << nDim << " regime " << regimes[regime] << " worst " << worst);
      CHECK(worst < 1e-6);
    }
  }
}

namespace {
/*--- Smooth Dirichlet field of the custom-marker tests: rho, u, p of order 1 with gradients of order 1. ---*/
pbc::EsState DirichletField(int nDim, const su2double* x, su2double un) {
  pbc::EsState S;
  S.rho = 1.0 + 0.2 * sin(1.3 * x[0] + 0.7 * x[1] + 0.3 * x[2]);
  S.p = 0.8 + 0.15 * cos(0.9 * x[0] - 1.1 * x[1] + 0.5 * x[2]);
  for (int i = 0; i < 3; ++i) S.vel[i] = 0.0;
  for (int i = 0; i < nDim; ++i) S.vel[i] = 0.3 * sin(x[i] + 0.4 * i) + un;
  return S;
}

/*--- Face rule of EsFaceResidual: phi[q][k] and the weight a_q. ---*/
su2double EsFaceRule(int nNodes, su2double (*phi)[3]) {
  if (nNodes == 2) {
    const su2double g = 0.5 / sqrt(3.0);
    phi[0][0] = 0.5 + g; phi[0][1] = 0.5 - g;
    phi[1][0] = 0.5 - g; phi[1][1] = 0.5 + g;
    return 0.5;
  }
  for (int q = 0; q < 3; ++q)
    for (int k = 0; k < 3; ++k) phi[q][k] = (q == k) ? 2.0 / 3.0 : 1.0 / 6.0;
  return 1.0 / 3.0;
}
}  // namespace

TEST_CASE("BC_ENTROPY_P1 custom Dirichlet: free stream, conservation and face entropy bound of the Dirichlet ghost", "[P1Flux]") {
  /*--- (a) Node states equal to a uniform Dirichlet state D: the residual is the far-field closure with inf = D and
   * sum_i b_i = F(D).A_f (free stream). (b) Random node states, x-dependent Dirichlet data: sum_i b_i = sum_q F*_q
   * (the G - Gbar terms cancel) with F*_q the Tadmor flux against g_q = u_D(x_q). (c) Face entropy bound (Lemma B7 at
   * each point plus the EC identity of the Ismail-Roe coupling):
   *   sum_i w_i.b_i >= sum_q [ sum_i phi_i(x_q) psi_i.N_q - psi(g_q).N_q + w(g_q).F*_q ],  psi = rho u. ---*/
  std::mt19937 gen(7311);
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  for (int nDim : {2, 3}) {
    const int nNodes = nDim, nVar = nDim + 2;
    for (int trial = 0; trial < 60; ++trial) {
      su2double n[3], t[3], N[3];
      JacRandomNormal(gen, nDim, n, t, N);
      const su2double un = 0.4 * u(gen);
      pbc::EsBoundaryData data;
      data.kind = CUSTOM_BOUNDARY;
      data.tag = "custom";
      for (int k = 0; k < nNodes; ++k)
        for (int i = 0; i < 3; ++i) data.coord[k][i] = (i < nDim) ? u(gen) : 0.0;

      /*--- (a) uniform data. ---*/
      const pbc::EsState D = JacRandomState(gen, nDim, 0.4, 0.6);
      data.dirichlet = [D](const su2double*, pbc::EsState& S) { S = D; };
      pbc::EsState states[3] = {D, D, D};
      su2double res[3][pbc::ES_NVAR] = {{0.0}}, resFar[3][pbc::ES_NVAR] = {{0.0}};
      pbc::EsFaceResidual(nDim, nNodes, kGamma, &data, N, states, res);
      pbc::EsBoundaryData far;
      far.kind = FAR_FIELD;
      far.tag = "farfield";
      far.inf = D;
      pbc::EsFaceResidual(nDim, nNodes, kGamma, &far, N, states, resFar);
      su2double U[pbc::ES_NVAR] = {0.0}, flux[pbc::ES_NVAR] = {0.0};
      pbc::EsConservative(nDim, kGamma, D, U);
      su2double vn = 0.0;
      for (int i = 0; i < nDim; ++i) vn += D.vel[i] * N[i];
      for (int v = 0; v < nVar; ++v) flux[v] = U[v] * vn;
      for (int i = 0; i < nDim; ++i) flux[i + 1] += D.p * N[i];
      flux[nVar - 1] += D.p * vn;
      for (int v = 0; v < nVar; ++v) {
        su2double sum = 0.0;
        for (int a = 0; a < nNodes; ++a) {
          CHECK(fabs(res[a][v] - resFar[a][v]) <= 1e-13 * (1.0 + fabs(resFar[a][v])));
          sum += res[a][v];
        }
        CHECK(fabs(sum - flux[v]) <= 1e-12 * (1.0 + fabs(flux[v])));
      }

      /*--- (b), (c) x-dependent data, node states near it. ---*/
      data.dirichlet = [nDim, un](const su2double* x, pbc::EsState& S) { S = DirichletField(nDim, x, un); };
      for (int k = 0; k < nNodes; ++k) states[k] = JacPerturb(gen, nDim, DirichletField(nDim, data.coord[k], un), 0.3);
      pbc::EsFaceResidual(nDim, nNodes, kGamma, &data, N, states, res);

      su2double phi[3][3] = {{0.0}};
      const su2double weight = EsFaceRule(nNodes, phi);
      su2double area = 0.0, unit[3] = {0.0, 0.0, 0.0}, Nq[3] = {0.0, 0.0, 0.0};
      for (int i = 0; i < nDim; ++i) area += N[i] * N[i];
      area = sqrt(area);
      for (int i = 0; i < nDim; ++i) {
        unit[i] = N[i] / area;
        Nq[i] = weight * N[i];
      }
      su2double w[3][pbc::ES_NVAR] = {{0.0}};
      for (int k = 0; k < nNodes; ++k) pbc::EsEntropyVariables(nDim, kGamma, states[k], w[k]);
      su2double sumF[pbc::ES_NVAR] = {0.0}, bound = 0.0, scale = 0.0;
      for (int q = 0; q < nNodes; ++q) {
        su2double wq[pbc::ES_NVAR] = {0.0}, xq[3] = {0.0, 0.0, 0.0};
        for (int k = 0; k < nNodes; ++k) {
          for (int v = 0; v < nVar; ++v) wq[v] += phi[q][k] * w[k][v];
          for (int i = 0; i < nDim; ++i) xq[i] += phi[q][k] * data.coord[k][i];
        }
        pbc::EsState sq, gq = DirichletField(nDim, xq, un);
        pbc::EsStateFromEntropyVariables(nDim, kGamma, wq, sq);
        su2double Fs[pbc::ES_NVAR] = {0.0}, Uq[pbc::ES_NVAR] = {0.0}, Ug[pbc::ES_NVAR] = {0.0}, wg[pbc::ES_NVAR] = {0.0};
        pbc::EsIsmailRoeFlux(nDim, kGamma, sq, gq, Nq, Fs);
        su2double vq = 0.0, vg = 0.0;
        for (int i = 0; i < nDim; ++i) {
          vq += sq.vel[i] * unit[i];
          vg += gq.vel[i] * unit[i];
        }
        const su2double lam = std::max(su2double(fabs(vq) + sqrt(kGamma * sq.p / sq.rho)),
                                       su2double(fabs(vg) + sqrt(kGamma * gq.p / gq.rho)));
        pbc::EsConservative(nDim, kGamma, sq, Uq);
        pbc::EsConservative(nDim, kGamma, gq, Ug);
        for (int v = 0; v < nVar; ++v) {
          Fs[v] -= 0.5 * lam * weight * area * (Ug[v] - Uq[v]);
          sumF[v] += Fs[v];
        }
        pbc::EsEntropyVariables(nDim, kGamma, gq, wg);
        for (int k = 0; k < nNodes; ++k)
          for (int i = 0; i < nDim; ++i) bound += phi[q][k] * states[k].rho * states[k].vel[i] * Nq[i];
        for (int i = 0; i < nDim; ++i) bound -= gq.rho * gq.vel[i] * Nq[i];
        for (int v = 0; v < nVar; ++v) {
          bound += wg[v] * Fs[v];
          scale += fabs(wg[v] * Fs[v]);
        }
      }
      su2double wb = 0.0;
      for (int v = 0; v < nVar; ++v) {
        su2double sum = 0.0;
        for (int a = 0; a < nNodes; ++a) {
          sum += res[a][v];
          wb += w[a][v] * res[a][v];
          scale += fabs(w[a][v] * res[a][v]);
        }
        INFO("nDim " << nDim << " trial " << trial << " var " << v);
        CHECK(fabs(sum - sumF[v]) <= 1e-12 * (1.0 + fabs(sumF[v])));
      }
      INFO("nDim " << nDim << " trial " << trial << " w.b " << wb << " bound " << bound);
      /*--- The truncated logarithmic mean makes the Ismail-Roe EC identity exact only to about 1e-11. ---*/
      CHECK(wb >= bound - 1e-9 * (1.0 + scale));
    }
  }
}

TEST_CASE("BC_ENTROPY_P1 Jacobian: custom Dirichlet faces against finite differences", "[P1Flux]") {
  /*--- The Dirichlet ghost does not depend on the face states (dG = 0); inflow and outflow, sub- and supersonic. ---*/
  std::mt19937 gen(7312);
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  for (int nDim : {2, 3}) {
    su2double worst = 0.0;
    for (int trial = 0; trial < 80; ++trial) {
      su2double n[3], t[3], N[3];
      JacRandomNormal(gen, nDim, n, t, N);
      const su2double un = 1.5 * u(gen);
      pbc::EsBoundaryData data;
      data.kind = CUSTOM_BOUNDARY;
      data.tag = "custom";
      for (int k = 0; k < nDim; ++k)
        for (int i = 0; i < 3; ++i) data.coord[k][i] = (i < nDim) ? u(gen) : 0.0;
      data.dirichlet = [nDim, un](const su2double* x, pbc::EsState& S) { S = DirichletField(nDim, x, un); };
      const double eps = (trial % 2) ? 0.2 : 0.02;
      pbc::EsState states[3];
      for (int k = 0; k < nDim; ++k) states[k] = JacPerturb(gen, nDim, DirichletField(nDim, data.coord[k], un), eps);
      worst = std::max(worst, FaceJacobianError(nDim, &data, N, states));
    }
    INFO("nDim " << nDim << " worst relative error " << worst);
    CHECK(worst < 1e-6);
  }
}

TEST_CASE("P1 SST matched rough wall: k_D m_a on the weak-wall closure rows, hat rows, uniform k, M5 identity", "[P1Flux]") {
  /*--- Pressure-flux closure of a weak no-slip wall face (EsFaceResidual without data) and the strong wall k k_D,a of a
   * rough wall. (a) rho and momentum rows unchanged, E - k_D m = Ehat, the k-row number is k_D,a m_a; (b) the nodal
   * masses sum to zero, so a uniform k_D adds no energy over the face; (c) free stream with a k jump: a uniform state
   * tangent to the face has m_a = 0, nothing is added for any k_D; (d) M5 at the Dirichlet nodes: sum_a theta_a (X_a -
   * k_D,a R_rho,a) = 0 exactly. ---*/
  std::mt19937 gen(7313);
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  for (int nDim : {2, 3}) {
    const int nNodes = nDim, nVar = nDim + 2;
    for (int trial = 0; trial < 50; ++trial) {
      su2double n[3], t[3], N[3];
      JacRandomNormal(gen, nDim, n, t, N);
      const pbc::EsState base = JacRandomState(gen, nDim, 0.4, 0.5);
      pbc::EsState states[3];
      for (int k = 0; k < nNodes; ++k) states[k] = JacPerturb(gen, nDim, base, 0.3);
      su2double res0[3][pbc::ES_NVAR] = {{0.0}}, res[3][pbc::ES_NVAR] = {{0.0}};
      pbc::EsFaceResidual(nDim, nNodes, kGamma, nullptr, N, states, res0);
      su2double kD[3] = {0.0, 0.0, 0.0}, kRow[3] = {0.0, 0.0, 0.0};
      for (int k = 0; k < nNodes; ++k) kD[k] = 0.01 * exp(u(gen));
      for (int a = 0; a < nNodes; ++a)
        for (int v = 0; v < nVar; ++v) res[a][v] = res0[a][v];
      P1SSTMatched::WallNodeKine(nNodes, nVar, kD, res, kRow);
      su2double massSum = 0.0, scale = 0.0, m5 = 0.0;
      for (int a = 0; a < nNodes; ++a) {
        for (int v = 0; v < nVar - 1; ++v) CHECK(res[a][v] == res0[a][v]);
        CHECK(kRow[a] == kD[a] * res0[a][0]);
        CHECK(fabs((res[a][nVar - 1] - kRow[a]) - res0[a][nVar - 1]) <= 1e-15 * (1.0 + fabs(res0[a][nVar - 1])));
        massSum += res0[a][0];
        scale += fabs(res0[a][0]);
        m5 += (states[a].rho / states[a].p) * (kRow[a] - kD[a] * res[a][0]);
      }
      CHECK(fabs(massSum) <= 1e-13 * (1.0 + scale));
      CHECK(m5 == 0.0);

      /*--- (b) uniform k_D. ---*/
      su2double kU[3] = {0.02, 0.02, 0.02}, kRowU[3] = {0.0, 0.0, 0.0};
      for (int a = 0; a < nNodes; ++a)
        for (int v = 0; v < nVar; ++v) res[a][v] = res0[a][v];
      P1SSTMatched::WallNodeKine(nNodes, nVar, kU, res, kRowU);
      CHECK(fabs(kRowU[0] + kRowU[1] + (nNodes == 3 ? kRowU[2] : 0.0)) <= 1e-13 * (1.0 + 0.02 * scale));

      /*--- (c) uniform state tangent to the face, k jump. ---*/
      pbc::EsState T = base;
      su2double vn = 0.0;
      for (int i = 0; i < nDim; ++i) vn += T.vel[i] * n[i];
      for (int i = 0; i < nDim; ++i) T.vel[i] -= vn * n[i];
      pbc::EsState flat[3] = {T, T, T};
      pbc::EsFaceResidual(nDim, nNodes, kGamma, nullptr, N, flat, res0);
      for (int a = 0; a < nNodes; ++a)
        for (int v = 0; v < nVar; ++v) res[a][v] = res0[a][v];
      P1SSTMatched::WallNodeKine(nNodes, nVar, kD, res, kRow);
      for (int a = 0; a < nNodes; ++a) {
        CHECK(fabs(kRow[a]) <= 1e-14 * (1.0 + kD[a]));
        CHECK(fabs(res[a][nVar - 1] - res0[a][nVar - 1]) <= 1e-14 * (1.0 + fabs(res0[a][nVar - 1])));
      }
    }
  }
}

TEST_CASE("P1 SST matched SPLIT boundary: hat nodal flux + k_b m, k jump, uniform k, L(k_b) Jhat T(k_i) against FD", "[P1Flux]") {
  /*--- Model of the nodal boundary flux of BC_Far_Field / BC_Inlet / BC_Outlet with SPLIT_EDGE_FLUX: a hat flux of the
   * node state against a fixed hat ghost (Ismail-Roe here), U = Uhat + rho k e_E with the node's k frozen, plus the closure
   * C k rule (BoundaryNodeKine). (a) k jump: the rho and momentum rows and E - k_b m do not depend on k_i; (b) uniform k
   * (k_i = kBc): the k-row number is k m; (c) outflow takes k_i, inflow kBc; (d) the transformed Jacobian is the derivative
   * of the matched flux with respect to the conservative U_i (central differences). ---*/
  std::mt19937 gen(7315);
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  for (int nDim : {2, 3}) {
    const int nVar = nDim + 2;
    su2double worst = 0.0;
    for (int trial = 0; trial < 60; ++trial) {
      su2double n[3], t[3], N[3];
      JacRandomNormal(gen, nDim, n, t, N);
      const pbc::EsState ghost = JacRandomState(gen, nDim, 0.3, 0.6);
      const pbc::EsState hatState = JacPerturb(gen, nDim, ghost, 0.3);
      const su2double kNode = 0.05 * exp(u(gen)), kBc = 0.05 * exp(u(gen));
      su2double Uhat[pbc::ES_NVAR] = {0.0}, U[pbc::ES_NVAR] = {0.0};
      pbc::EsConservative(nDim, kGamma, hatState, Uhat);
      for (int v = 0; v < nVar; ++v) U[v] = Uhat[v];
      U[nVar - 1] += Uhat[0] * kNode;

      /*--- Matched flux of the conservative state with the frozen k. ---*/
      auto matched = [&](const su2double* Ux, su2double k, su2double* res) {
        su2double Uh[pbc::ES_NVAR];
        for (int v = 0; v < nVar; ++v) Uh[v] = Ux[v];
        Uh[nVar - 1] -= Ux[0] * k;
        const pbc::EsState S = JacStateOf(nDim, Uh);
        pbc::EsIsmailRoeFlux(nDim, kGamma, S, ghost, N, res);
        return P1SSTMatched::BoundaryNodeKine<su2double[pbc::ES_NVAR]>(nVar, k, kBc, res, nullptr);
      };
      su2double res[pbc::ES_NVAR] = {0.0}, resHat[pbc::ES_NVAR] = {0.0};
      const su2double kRow = matched(U, kNode, res);
      pbc::EsIsmailRoeFlux(nDim, kGamma, hatState, ghost, N, resHat);
      const su2double m = resHat[0], kB = (m >= 0.0) ? kNode : kBc;
      for (int v = 0; v < nVar - 1; ++v) CHECK(fabs(res[v] - resHat[v]) <= 1e-13 * (1.0 + fabs(resHat[v])));
      CHECK(fabs(kRow - kB * m) <= 1e-13 * (1.0 + fabs(m)));
      CHECK(fabs((res[nVar - 1] - kRow) - resHat[nVar - 1]) <= 1e-13 * (1.0 + fabs(resHat[nVar - 1])));

      /*--- (a) another k with the same hat state: hat rows unchanged. ---*/
      su2double U2[pbc::ES_NVAR], res2[pbc::ES_NVAR] = {0.0};
      const su2double k2 = 3.0 * kNode;
      for (int v = 0; v < nVar; ++v) U2[v] = Uhat[v];
      U2[nVar - 1] += Uhat[0] * k2;
      const su2double kRow2 = matched(U2, k2, res2);
      for (int v = 0; v < nVar - 1; ++v) CHECK(fabs(res2[v] - res[v]) <= 1e-13 * (1.0 + fabs(res[v])));
      CHECK(fabs((res2[nVar - 1] - kRow2) - (res[nVar - 1] - kRow)) <= 1e-13 * (1.0 + fabs(res[nVar - 1])));

      /*--- (b) uniform k. ---*/
      su2double res3[pbc::ES_NVAR];
      for (int v = 0; v < nVar; ++v) res3[v] = resHat[v];
      const su2double kRow3 = P1SSTMatched::BoundaryNodeKine<su2double[pbc::ES_NVAR]>(nVar, kNode, kNode, res3, nullptr);
      CHECK(kRow3 == kNode * m);

      /*--- (d) Jacobian. ---*/
      su2double jL[pbc::ES_NVAR][pbc::ES_NVAR], jR[pbc::ES_NVAR][pbc::ES_NVAR], jac[pbc::ES_NVAR][pbc::ES_NVAR];
      pbc::EsIsmailRoeJacobian(nDim, kGamma, hatState, ghost, N, jL, jR);
      for (int a = 0; a < nVar; ++a)
        for (int b = 0; b < nVar; ++b) jac[a][b] = jL[a][b];
      su2double resJ[pbc::ES_NVAR];
      for (int v = 0; v < nVar; ++v) resJ[v] = resHat[v];
      P1SSTMatched::BoundaryNodeKine(nVar, kNode, kBc, resJ, jac);
      if (fabs(m) < 1e-3 * (1.0 + fabs(resHat[1]))) continue;  // k_b switches at m = 0
      su2double err = 0.0, ref = 0.0;
      for (int c = 0; c < nVar; ++c) {
        const su2double h = 1e-6 * std::max(su2double(1.0), su2double(fabs(U[c])));
        su2double Up[pbc::ES_NVAR], Um[pbc::ES_NVAR], rp[pbc::ES_NVAR] = {0.0}, rm[pbc::ES_NVAR] = {0.0};
        for (int v = 0; v < nVar; ++v) Up[v] = Um[v] = U[v];
        Up[c] += h;
        Um[c] -= h;
        matched(Up, kNode, rp);
        matched(Um, kNode, rm);
        for (int a = 0; a < nVar; ++a) {
          const su2double fd = (rp[a] - rm[a]) / (2.0 * h);
          err = std::max(err, su2double(fabs(fd - jac[a][c])));
          ref = std::max(ref, su2double(fabs(fd)));
        }
      }
      worst = std::max(worst, err / ref);
    }
    INFO("nDim " << nDim << " worst relative Jacobian error " << worst);
    CHECK(worst < 1e-6);
  }
}

TEST_CASE("BC_ENTROPY_P1 Jacobian: pressure flux faces and the scalar Ismail-Roe Jacobian", "[P1Flux]") {
  /*--- The wall branch of EsFaceJacobian (not used by the solver, which keeps the approximate wall Jacobian) and the
   * port of ismailRoeProjJac, which must equal the vectorized one to round-off. ---*/
  std::mt19937 gen(9404);
  su2double worstWall = 0.0, worstPort = 0.0;
  for (int nDim : {2, 3}) {
    for (int trial = 0; trial < 40; ++trial) {
      su2double n[3], t[3], N[3];
      JacRandomNormal(gen, nDim, n, t, N);
      const pbc::EsState base = JacRandomState(gen, nDim, 0.5, 0.8);
      pbc::EsState states[3];
      for (int k = 0; k < nDim; ++k) states[k] = JacPerturb(gen, nDim, base, (trial % 2) ? 0.2 : 0.02);
      pbc::EsBoundaryData data;
      data.kind = EULER_WALL;
      worstWall = std::max(worstWall, FaceJacobianError(nDim, &data, N, states));
      worstWall = std::max(worstWall, FaceJacobianError(nDim, nullptr, N, states));
    }
  }
  for (int trial = 0; trial < 100; ++trial) {
    worstPort = std::max(worstPort, IsmailRoeScalarVersusVectorized<2>(gen));
    worstPort = std::max(worstPort, IsmailRoeScalarVersusVectorized<3>(gen));
  }
  INFO("wall " << worstWall << " port " << worstPort);
  CHECK(worstWall < 1e-6);
  CHECK(worstPort < 1e-12);
}

namespace {

/*--- JST_WEAK_WALL_DISSIPATION (P1_WALL_DISSIPATION.md). Copy of the dissipation of CJSTScheme::finalizeFlux (the
 * kernel needs a solver to run): coefficients, the factor of jstWallFactor when the option is on, flux increment
 * (eps2 (Ut_i - Ut_j) - eps4 (L_i - L_j)) lambda and the Jacobian factors fixFactor (eps2 + eps4 (n+1)) lambda. ---*/
template <size_t nVar>
void JSTEdgeDissipation(su2double kappa2, su2double kappa4, su2double fixFactor, const Double& ni, const Double& nj,
                        const Double& si, const Double& sj, const Double& lambda, bool weakWall, su2double wall_i,
                        su2double wall_j, const VectorDbl<nVar>& diffU, const VectorDbl<nVar>& lapl_i,
                        const VectorDbl<nVar>& lapl_j, VectorDbl<nVar>& flux, Double& dissip_i, Double& dissip_j) {
  const Double sc2 = 3 * (ni+nj) / (ni*nj);
  const Double sc4 = 0.25*pow(sc2, 2);
  Double eps2 = kappa2 * 0.5*(si+sj) * sc2;
  Double eps4 = fmax(0.0, kappa4-eps2) * sc4;
  const Double eps2Jac = eps2;
  if (weakWall) {
    const Double keep = jstWallFactor(Double(wall_i), Double(wall_j));
    eps4 *= keep;
    eps2 *= keep;
  }
  for (size_t iVar = 0; iVar < nVar; ++iVar) {
    flux(iVar) += (eps2*diffU(iVar) - eps4*(lapl_i(iVar)-lapl_j(iVar))) * lambda;
  }
  dissip_i = fixFactor * (eps2Jac + eps4*(ni+1)) * lambda;
  dissip_j = -fixFactor * (eps2Jac + eps4*(nj+1)) * lambda;
}

/*--- A small graph: wall nodes 0..3 on a line, two interior layers (4..7, 8..11) of triangles. Undivided Laplacian as
 * in CEulerSolver::SetUndivided_Laplacian: a wall node uses only its wall neighbours. ---*/
struct JSTGraph {
  static constexpr int nNode = 12;
  std::vector<std::array<int, 2>> edges;
  std::vector<std::vector<int>> nbr;
  bool wall[nNode];

  JSTGraph() : nbr(nNode) {
    for (int i = 0; i < nNode; ++i) wall[i] = (i < 4);
    for (int layer = 0; layer < 3; ++layer) {
      for (int i = 0; i < 3; ++i) Add(4 * layer + i, 4 * layer + i + 1);
      if (layer == 2) break;
      for (int i = 0; i < 4; ++i) Add(4 * layer + i, 4 * (layer + 1) + i);
      for (int i = 0; i < 3; ++i) Add(4 * layer + i, 4 * (layer + 1) + i + 1);
    }
  }
  void Add(int a, int b) {
    edges.push_back({a, b});
    nbr[a].push_back(b);
    nbr[b].push_back(a);
  }
  /*--- L of one variable for the nodal values u. ---*/
  void Laplacian(const su2double* u, su2double* L) const {
    for (int i = 0; i < nNode; ++i) {
      L[i] = 0.0;
      for (int k : nbr[i]) {
        if (wall[i] && !wall[k]) continue;
        L[i] += u[k] - u[i];
      }
    }
  }
  /*--- Flux increment of edge e for one variable (kappa2 0.5, fixed sensors and lambda, so eps2, eps4 and lambda are
   * frozen as in the SU2 Jacobian); the Jacobian factors of both ends in dissip. ---*/
  su2double EdgeTerm(int e, const su2double* u, su2double kappa4, bool weakWall, su2double* dissip) const {
    const int i = edges[e][0], j = edges[e][1];
    su2double L[nNode];
    Laplacian(u, L);
    VectorDbl<1> diffU, lapl_i, lapl_j, flux;
    diffU(0) = u[i] - u[j];
    lapl_i(0) = L[i];
    lapl_j(0) = L[j];
    flux(0) = 0.0;
    Double di, dj;
    const Double ni = su2double(nbr[i].size()), nj = su2double(nbr[j].size());
    JSTEdgeDissipation<1>(0.5, kappa4, 1.0, ni, nj, Double(0.004), Double(0.006), Double(1.7), weakWall,
                          wall[i] ? 1.0 : 0.0, wall[j] ? 1.0 : 0.0, diffU, lapl_i, lapl_j, flux, di, dj);
    dissip[0] = di[0];
    dissip[1] = dj[0];
    return flux(0)[0];
  }
};

}  // namespace

TEST_CASE("JST weak wall: the eps4 factor is zero only on edges with one end on the wall", "[P1Flux]") {
  const su2double flag[2] = {0.0, 1.0};
  for (int a = 0; a < 2; ++a) {
    for (int b = 0; b < 2; ++b) {
      const Double f = jstWallFactor(Double(flag[a]), Double(flag[b]));
      INFO("flags " << a << " " << b);
      CHECK(f[0] == ((a == b) ? 1.0 : 0.0));
    }
  }
}

TEST_CASE("JST weak wall: option off and edges off the wall are unchanged and wall-crossing edges carry no JST dissipation", "[P1Flux]") {
  /*--- The option multiplies eps2 and eps4 by exactly 1 on edges with both ends off the wall or both on it, so their
   * flux and Jacobian factors are those of the option off, bit for bit. With one end on the wall both terms are gone
   * from the flux, and the Jacobian factors keep only the eps2 shift +-fixFactor eps2 lambda. ---*/
  constexpr size_t nVar = 5;
  std::mt19937 gen(4242);
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  const su2double kappa2 = 0.5, kappa4 = 0.02, fixFactor = 4.0;
  int mismatches = 0, compared = 0;
  for (int trial = 0; trial < 200; ++trial) {
    const Double ni = su2double(3 + trial % 6), nj = su2double(3 + (trial / 6) % 7);
    const Double si = su2double(0.002 * (1.0 + u(gen))), sj = su2double(0.002 * (1.0 + u(gen)));
    const Double lambda = su2double(exp(u(gen)));
    VectorDbl<nVar> diffU, lapl_i, lapl_j, start;
    for (size_t v = 0; v < nVar; ++v) {
      diffU(v) = su2double(u(gen));
      lapl_i(v) = su2double(u(gen));
      lapl_j(v) = su2double(u(gen));
      start(v) = su2double(0.1 * v);
    }
    const su2double sc2 = 3 * (ni[0] + nj[0]) / (ni[0] * nj[0]);
    const su2double eps2 = kappa2 * 0.5 * (si[0] + sj[0]) * sc2;
    REQUIRE(kappa4 > eps2);

    VectorDbl<nVar> fluxOff = start, fluxOne = start, fluxZero = start;
    Double offI, offJ, oneI, oneJ, zeroI, zeroJ;
    const su2double same = trial % 2;
    JSTEdgeDissipation<nVar>(kappa2, kappa4, fixFactor, ni, nj, si, sj, lambda, false, 1.0, 0.0, diffU, lapl_i, lapl_j,
                             fluxOff, offI, offJ);
    JSTEdgeDissipation<nVar>(kappa2, kappa4, fixFactor, ni, nj, si, sj, lambda, true, same, same, diffU, lapl_i,
                             lapl_j, fluxOne, oneI, oneJ);
    JSTEdgeDissipation<nVar>(kappa2, kappa4, fixFactor, ni, nj, si, sj, lambda, true, same, 1.0 - same, diffU, lapl_i,
                             lapl_j, fluxZero, zeroI, zeroJ);
    mismatches += (oneI[0] != offI[0]) + (oneJ[0] != offJ[0]);
    const su2double eps2Lam = eps2 * lambda[0];
    mismatches += (fabs(zeroI[0] - fixFactor * eps2Lam) > 1e-15 * fixFactor * eps2Lam);
    mismatches += (fabs(zeroJ[0] + fixFactor * eps2Lam) > 1e-15 * fixFactor * eps2Lam);
    for (size_t v = 0; v < nVar; ++v) {
      mismatches += (fluxOne(v)[0] != fluxOff(v)[0]);
      mismatches += (fluxZero(v)[0] != start(v)[0]);
      ++compared;
    }
    /*--- The option off really carries the fourth-order term on that edge. ---*/
    mismatches += (offI[0] <= fixFactor * eps2Lam * (1.0 + 1e-12));
  }
  INFO("compared " << compared << " flux components");
  CHECK(mismatches == 0);
}

TEST_CASE("JST weak wall: Jacobian factors equal finite differences of the edge flux on the changed edges", "[P1Flux]") {
  /*--- SU2's JST Jacobian is fixFactor (eps2 + eps4 (n+1)) lambda dUt/dU with eps2, eps4 and lambda frozen. In Ut it
   * is exact when both Laplacians contain all neighbours of the node (interior edges). With one end on the wall the
   * wall Laplacian lacks the interior neighbours, so its eps4 part is approximate; with the option the crossing flux has
   * no JST term (zero central differences) and the factor keeps only the eps2 shift. Central differences in Ut of each
   * end, fixFactor 1. ---*/
  const JSTGraph graph;
  std::mt19937 gen(777);
  std::uniform_real_distribution<double> rnd(-1.0, 1.0);
  su2double u[JSTGraph::nNode];
  for (auto& x : u) x = rnd(gen);
  const su2double kappa4 = 0.02, h = 1e-6;

  int crossing = 0, interior = 0;
  su2double errCrossingOn = 0.0, errInterior = 0.0, errCrossingOffMin = 1e30, scale = 0.0;
  for (int weak = 0; weak < 2; ++weak) {
    for (int e = 0; e < static_cast<int>(graph.edges.size()); ++e) {
      const int i = graph.edges[e][0], j = graph.edges[e][1];
      const bool isCrossing = (graph.wall[i] != graph.wall[j]);
      const bool isInterior = !graph.wall[i] && !graph.wall[j];
      if (!isCrossing && !isInterior) continue;
      su2double dissip[2], dummy[2];
      graph.EdgeTerm(e, u, kappa4, weak, dissip);
      for (int side = 0; side < 2; ++side) {
        su2double up[JSTGraph::nNode], um[JSTGraph::nNode];
        for (int k = 0; k < JSTGraph::nNode; ++k) up[k] = um[k] = u[k];
        up[graph.edges[e][side]] += h;
        um[graph.edges[e][side]] -= h;
        const su2double fd = (graph.EdgeTerm(e, up, kappa4, weak, dummy) - graph.EdgeTerm(e, um, kappa4, weak, dummy)) / (2 * h);
        const su2double err = fabs(fd - dissip[side]);
        scale = std::max(scale, fabs(dissip[side]));
        if (isInterior) errInterior = std::max(errInterior, err);
        /*--- With the option the crossing flux has no JST term (fd = 0) and the factor is the eps2 shift. ---*/
        if (isCrossing && weak) errCrossingOn = std::max(errCrossingOn, fabs(fd));
        if (isCrossing && !weak) errCrossingOffMin = std::min(errCrossingOffMin, err);
      }
      crossing += isCrossing;
      interior += isInterior;
    }
  }
  INFO("crossing edges " << crossing / 2 << " interior edges " << interior / 2 << " scale " << scale
       << " smallest off-mode error on crossing edges " << errCrossingOffMin);
  CHECK(crossing > 0);
  CHECK(interior > 0);
  CHECK(errInterior < 1e-8 * scale);
  CHECK(errCrossingOn < 1e-8 * scale);
  /*--- Without the option the factor of a crossing edge is off by eps4 lambda times a neighbour count (>= 1). ---*/
  CHECK(errCrossingOffMin > 1e-3 * scale);
}

TEST_CASE("JST weak wall: node flags are exact on owned and halo nodes and select the edges that leave the wall", "[P1Flux]") {
  /*--- The bottom marker plays the weak no-slip wall (x_d = 0). After WallNodeFlags every point of every rank, halo
   * points included, carries the flag of its position; the factor is 0 exactly on the edges with one end at x_d = 0. ---*/
  for (int nDim : {2, 3}) {
    const std::string mesh = "p1_jst_weak_wall_" + std::to_string(nDim) + "d.su2";
    WriteSimplexMesh(mesh, nDim, (nDim == 2) ? 8 : 3, 0.4, 0.5);
    MeshCase test(mesh);
    auto& geometry = *test.geometry;
    geometry.PreprocessP2PComms(test.geometry.get(), test.config.get());
    CHECK_FALSE(test.config->GetJST_Weak_Wall_Diss());

    const auto& config = *test.config;
    auto isBottom = [&](unsigned short iMarker) { return config.GetMarker_All_TagBound(iMarker) == "bottom"; };
    su2activevector flags;
    JSTWeakWall::WallNodeFlags(geometry, config, isBottom, flags);
    REQUIRE(flags.size() == geometry.GetnPoint());

    auto onWall = [&](unsigned long iPoint) { return geometry.nodes->GetCoord(iPoint, nDim - 1) == 0.0; };
    su2double wrongFlags = 0.0, wrongEdges = 0.0, crossing = 0.0, along = 0.0;
    for (auto iPoint = 0ul; iPoint < geometry.GetnPoint(); ++iPoint) {
      wrongFlags += (flags(iPoint) != (onWall(iPoint) ? 1.0 : 0.0));
    }
    for (auto iEdge = 0ul; iEdge < geometry.GetnEdge(); ++iEdge) {
      const auto iPoint = geometry.edges->GetNode(iEdge, 0);
      const auto jPoint = geometry.edges->GetNode(iEdge, 1);
      if (!geometry.nodes->GetDomain(iPoint) && !geometry.nodes->GetDomain(jPoint)) continue;
      const su2double factor = jstWallFactor(Double(flags(iPoint)), Double(flags(jPoint)))[0];
      const bool leaves = (onWall(iPoint) != onWall(jPoint));
      wrongEdges += (factor != (leaves ? 0.0 : 1.0));
      /*--- Count each edge once, on the rank that owns its first node. ---*/
      if (geometry.nodes->GetDomain(iPoint)) {
        crossing += leaves;
        along += (onWall(iPoint) && onWall(jPoint));
      }
    }
    INFO("nDim " << nDim);
    CHECK(GlobalSum(wrongFlags) == 0.0);
    CHECK(GlobalSum(wrongEdges) == 0.0);
    CHECK(GlobalSum(crossing) > 0.0);
    CHECK(GlobalSum(along) > 0.0);
  }
}

/*--- SST with the P1 options: the matched k form (P1_SST_COUPLED.md, sections 5, 7, D1-D4, E1). ---*/
namespace {
/*--- Euler flux of the conservative state Uc (rho, m, rhoE) along n, with p = (g-1)(rhoE - |m|^2/(2 rho) - rho k). ---*/
void EulerFluxK(int nDim, const su2double* Uc, su2double k, const su2double* n, su2double* F) {
  su2double m2 = 0.0, mn = 0.0;
  for (int i = 0; i < nDim; ++i) { m2 += Uc[i + 1] * Uc[i + 1]; mn += Uc[i + 1] * n[i]; }
  const su2double p = (kGamma - 1.0) * (Uc[nDim + 1] - 0.5 * m2 / Uc[0] - Uc[0] * k);
  F[0] = mn;
  for (int i = 0; i < nDim; ++i) F[i + 1] = Uc[i + 1] * mn / Uc[0] + p * n[i];
  F[nDim + 1] = (Uc[nDim + 1] + p) * mn / Uc[0];
}
/*--- A hat edge flux: central Euler flux of the hat states plus a scalar dissipation on the hat jump. ---*/
void HatEdgeFlux(int nDim, const su2double* Ui, const su2double* Uj, const su2double* n, su2double lam, su2double* F) {
  su2double Fi[5], Fj[5];
  EulerFluxK(nDim, Ui, 0.0, n, Fi);
  EulerFluxK(nDim, Uj, 0.0, n, Fj);
  for (int v = 0; v < nDim + 2; ++v) F[v] = 0.5 * (Fi[v] + Fj[v]) - lam * (Uj[v] - Ui[v]);
}
/*--- The matched flux of U_i, U_j with node k: Fhat(T(k_i) U_i, T(k_j) U_j) + k_up Fhat_rho e_E. ---*/
void MatchedEdgeFlux(int nDim, const su2double* Ui, const su2double* Uj, su2double ki, su2double kj, const su2double* n,
                     su2double lam, su2double* F) {
  su2double Hi[5], Hj[5];
  for (int v = 0; v < nDim + 2; ++v) { Hi[v] = Ui[v]; Hj[v] = Uj[v]; }
  Hi[nDim + 1] -= Ui[0] * ki;
  Hj[nDim + 1] -= Uj[0] * kj;
  HatEdgeFlux(nDim, Hi, Hj, n, lam, F);
  F[nDim + 1] += P1SSTMatched::KUpwind(F[0], ki, kj) * F[0];
}
}  // namespace

TEST_CASE("P1 SST matched: L(k_up) Jhat T(k) is the Jacobian of the matched edge flux (D1)", "[P1Flux]") {
  std::mt19937 gen(9137);
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  for (int nDim : {2, 3}) {
    const int nVar = nDim + 2;
    for (int trial = 0; trial < 50; ++trial) {
      su2double U[2][5], k[2], n[3] = {u(gen), u(gen), u(gen)};
      for (int s = 0; s < 2; ++s) {
        const su2double rho = exp(u(gen)), p = exp(u(gen));
        k[s] = 0.3 * exp(u(gen));
        su2double vel2 = 0.0;
        U[s][0] = rho;
        for (int i = 0; i < nDim; ++i) { const su2double v = u(gen); U[s][i + 1] = rho * v; vel2 += v * v; }
        U[s][nDim + 1] = p / (kGamma - 1.0) + 0.5 * rho * vel2 + rho * k[s];
      }
      const su2double lam = 0.7;
      su2double F0[5];
      MatchedEdgeFlux(nDim, U[0], U[1], k[0], k[1], n, lam, F0);
      const su2double kUp = P1SSTMatched::KUpwind(F0[0], k[0], k[1]);
      if (fabs(F0[0]) < 1e-3) continue;  // k_up is piecewise smooth at mdot = 0
      for (int side = 0; side < 2; ++side) {
        /*--- Jhat by central differences of the hat flux w.r.t. the hat state of this side. ---*/
        su2double H[2][5];
        for (int s = 0; s < 2; ++s) {
          for (int v = 0; v < nVar; ++v) H[s][v] = U[s][v];
          H[s][nVar - 1] -= U[s][0] * k[s];
        }
        su2activematrix J(nVar, nVar), Jfd(nVar, nVar);
        const su2double h = 1e-6;
        for (int jv = 0; jv < nVar; ++jv) {
          su2double Hp[2][5], Hm[2][5], Fp[5], Fm[5];
          for (int s = 0; s < 2; ++s) for (int v = 0; v < nVar; ++v) { Hp[s][v] = H[s][v]; Hm[s][v] = H[s][v]; }
          Hp[side][jv] += h; Hm[side][jv] -= h;
          HatEdgeFlux(nDim, Hp[0], Hp[1], n, lam, Fp);
          HatEdgeFlux(nDim, Hm[0], Hm[1], n, lam, Fm);
          for (int iv = 0; iv < nVar; ++iv) J(iv, jv) = (Fp[iv] - Fm[iv]) / (2 * h);
          /*--- The full matched flux w.r.t. U of this side (k frozen). ---*/
          su2double Up[2][5], Um[2][5];
          for (int s = 0; s < 2; ++s) for (int v = 0; v < nVar; ++v) { Up[s][v] = U[s][v]; Um[s][v] = U[s][v]; }
          Up[side][jv] += h; Um[side][jv] -= h;
          MatchedEdgeFlux(nDim, Up[0], Up[1], k[0], k[1], n, lam, Fp);
          MatchedEdgeFlux(nDim, Um[0], Um[1], k[0], k[1], n, lam, Fm);
          for (int iv = 0; iv < nVar; ++iv) Jfd(iv, jv) = (Fp[iv] - Fm[iv]) / (2 * h);
        }
        P1SSTMatched::TransformJacobian(nVar, J, kUp, k[side]);
        for (int iv = 0; iv < nVar; ++iv)
          for (int jv = 0; jv < nVar; ++jv) CHECK(J(iv, jv) == Approx(Jfd(iv, jv)).margin(1e-6 * (1.0 + fabs(Jfd(iv, jv)))));
      }
    }
  }
}

TEST_CASE("P1 SST matched: uniform (rho, u, p) with a k jump keeps the hat rows, a uniform k gives X = k R_rho", "[P1Flux]") {
  std::mt19937 gen(4242);
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  for (int nDim : {2, 3}) {
    const int nVar = nDim + 2;
    for (int trial = 0; trial < 50; ++trial) {
      const su2double rho = exp(u(gen)), p = exp(u(gen));
      su2double vel[3] = {u(gen), u(gen), u(gen)}, vel2 = 0.0, n[3] = {u(gen), u(gen), u(gen)};
      for (int i = 0; i < nDim; ++i) vel2 += vel[i] * vel[i];
      su2double U[2][5], k[2] = {0.2 * exp(u(gen)), 0.2 * exp(u(gen))};
      for (int s = 0; s < 2; ++s) {
        U[s][0] = rho;
        for (int i = 0; i < nDim; ++i) U[s][i + 1] = rho * vel[i];
        U[s][nDim + 1] = p / (kGamma - 1.0) + 0.5 * rho * vel2 + rho * k[s];
      }
      su2double F[5], Fhat[5], Hs[5];
      MatchedEdgeFlux(nDim, U[0], U[1], k[0], k[1], n, 0.7, F);
      for (int v = 0; v < nVar; ++v) Hs[v] = U[0][v];
      Hs[nVar - 1] -= rho * k[0];
      HatEdgeFlux(nDim, Hs, Hs, n, 0.7, Fhat);
      /*--- The k jump changes only the energy row, by the k flux of the k row, mdot k_up. ---*/
      for (int v = 0; v < nVar - 1; ++v) CHECK(F[v] == Approx(Fhat[v]).margin(1e-12 * (1.0 + fabs(Fhat[v]))));
      const su2double kFlux = P1SSTMatched::KUpwind(F[0], k[0], k[1]) * F[0];
      CHECK(F[nVar - 1] - kFlux == Approx(Fhat[nVar - 1]).margin(1e-12 * (1.0 + fabs(Fhat[nVar - 1]))));
      /*--- Closure C rule: k_b m = k_a max(m,0) + min(m,0) k_bc for both signs of m; uniform k = k_bc gives k m. ---*/
      for (su2double m : {0.37 * F[0], -0.37 * F[0]}) {
        su2double massPlus = 0.0, inflow = 0.0;
        const su2double kb = P1SSTMatched::FaceKine(m, k[0], k[1], massPlus, inflow);
        CHECK(kb * m == Approx(k[0] * massPlus + inflow).margin(1e-15));
        su2double mp = 0.0, in = 0.0;
        CHECK(P1SSTMatched::FaceKine(m, k[0], k[0], mp, in) * m == Approx(k[0] * m).margin(1e-15));
      }
    }
  }
}

TEST_CASE("P1 SST matched: EntropyVariableJacobian with k against finite differences (D3)", "[P1Flux]") {
  std::mt19937 gen(5511);
  for (int nDim : {2, 3}) {
    const int nVar = nDim + 2;
    for (int trial = 0; trial < 50; ++trial) {
      su2double x[4][3];
      pve::NodeState state[4];
      RandomElement(gen, nDim, 1.0, x, state);
      auto S = state[0];
      S.k = 0.4 * S.p / S.rho;
      su2double vel2 = 0.0;
      for (int i = 0; i < nDim; ++i) vel2 += S.vel[i] * S.vel[i];
      su2double U[5];
      U[0] = S.rho;
      for (int i = 0; i < nDim; ++i) U[i + 1] = S.rho * S.vel[i];
      U[nDim + 1] = S.p / (kGamma - 1.0) + 0.5 * S.rho * vel2 + S.rho * S.k;
      /*--- w = (0, m/p, -rho/p) of the viscous operator, p with the fixed k. ---*/
      auto wOf = [&](const su2double* Uc, su2double* w) {
        su2double m2 = 0.0;
        for (int i = 0; i < nDim; ++i) m2 += Uc[i + 1] * Uc[i + 1];
        const su2double p = (kGamma - 1.0) * (Uc[nDim + 1] - 0.5 * m2 / Uc[0] - Uc[0] * S.k);
        w[0] = 0.0;
        for (int i = 0; i < nDim; ++i) w[i + 1] = Uc[i + 1] / p;
        w[nDim + 1] = -Uc[0] / p;
      };
      su2double W[pve::MAXNVAR][pve::MAXNVAR];
      pve::EntropyVariableJacobian(nDim, kGamma, S, W);
      const su2double h = 1e-6;
      for (int jv = 0; jv < nVar; ++jv) {
        su2double Up[5], Um[5], wp[5], wm[5];
        for (int v = 0; v < nVar; ++v) { Up[v] = U[v]; Um[v] = U[v]; }
        Up[jv] += h; Um[jv] -= h;
        wOf(Up, wp);
        wOf(Um, wm);
        for (int iv = 0; iv < nVar; ++iv) {
          const su2double fd = (wp[iv] - wm[iv]) / (2 * h);
          CHECK(W[iv][jv] == Approx(fd).margin(1e-6 * (1.0 + fabs(fd))));
        }
      }
    }
  }
}

TEST_CASE("P1 SST matched: element k diffusion conserves, is exact for a linear k, and its Jacobian; D_t,T (M3, E1)", "[P1Flux]") {
  std::mt19937 gen(7788);
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  for (int nDim : {2, 3}) {
    const int nNodes = nDim + 1;
    for (int trial = 0; trial < 100; ++trial) {
      su2double x[4][3];
      pve::NodeState state[4];
      RandomElement(gen, nDim, (trial % 2) ? 1.0 : 1e-3, x, state);
      su2double g[4][3];
      const su2double size = fabs(P1Simplex::ShapeGradients(nDim, x, g));
      su2double k[4], mu[4], muT[4], F1[4], res[4];
      for (int a = 0; a < nNodes; ++a) {
        k[a] = exp(u(gen));
        mu[a] = exp(u(gen));
        muT[a] = exp(u(gen));
        F1[a] = 0.5 * (1.0 + u(gen));
      }
      const su2double D = P1SSTMatched::ElementKineDiffusivity(nNodes, mu, muT, F1);
      CHECK(D > 0.0);
      P1SSTMatched::KineDiffusion(nDim, size, g, k, D, res);
      su2double sum = 0.0, scale = 0.0;
      for (int a = 0; a < nNodes; ++a) { sum += res[a]; scale += fabs(res[a]); }
      CHECK(fabs(sum) <= 1e-12 * scale);
      /*--- Linear k = c + b.x: R_a = |T| D b.g_a, and the Jacobian is exact for this linear operator. ---*/
      su2double b[3] = {u(gen), u(gen), u(gen)}, kl[4];
      for (int a = 0; a < nNodes; ++a) {
        kl[a] = 0.3;
        for (int i = 0; i < nDim; ++i) kl[a] += b[i] * x[a][i];
      }
      P1SSTMatched::KineDiffusion(nDim, size, g, kl, D, res);
      for (int a = 0; a < nNodes; ++a) {
        su2double bg = 0.0, jsum = 0.0;
        for (int i = 0; i < nDim; ++i) bg += b[i] * g[a][i];
        for (int c = 0; c < nNodes; ++c) jsum += P1SSTMatched::KineDiffusionJacobian(nDim, size, g, D, a, c) * kl[c];
        CHECK(res[a] == Approx(size * D * bg).margin(1e-12 * (1.0 + fabs(res[a]))));
        CHECK(jsum == Approx(res[a]).margin(1e-12 * (1.0 + fabs(res[a]))));
      }
      /*--- E1: with mu = mu_t and kappa = 0, the Galerkin dissipation is D_t,T = |T| theta* mubar_t Q >= 0. ---*/
      for (int a = 0; a < nNodes; ++a) {
        state[a].mut = state[a].mu;
        state[a].kappa = 0.0;
      }
      pve::Element E;
      REQUIRE(E.Set(nDim, x, state, kGas) > 0.0);
      const su2double Dt = E.TurbulentDissipation(), Dgal = E.GalerkinDissipation();
      CHECK(Dt >= -1e-14 * fabs(Dgal));
      CHECK(Dt == Approx(Dgal).epsilon(1e-10));
    }
  }
}

/*--- JST_ENTROPY_BUDGET (P1_JST_ENTROPY.md, Lemma J1, Theorem J4). ---*/
namespace {
void RandomJSTState(std::mt19937& gen, int nDim, double spread, JSTEntropy::NodeState& S) {
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  const su2double rho = exp(spread * u(gen)), p = exp(spread * u(gen));
  su2double vel[3] = {0.0, 0.0, 0.0}, vel2 = 0.0;
  for (int i = 0; i < nDim; ++i) { vel[i] = 1.5 * u(gen); vel2 += vel[i] * vel[i]; }
  const su2double h = kGamma / (kGamma - 1.0) * p / rho + 0.5 * vel2;
  JSTEntropy::SetState(nDim, kGamma, rho, vel, p, h, S);
}
}  // namespace

TEST_CASE("JST entropy: dw.dUt is between lam_g and Lam_g times dw.dU (Lemma J1)", "[P1Flux]") {
  std::mt19937 gen(1203);
  const su2double lamG = 0.5 * (kGamma + 1.0 - sqrt(kGamma * (kGamma - 1.0)));
  const su2double LamG = 0.5 * (kGamma + 1.0 + sqrt(kGamma * (kGamma - 1.0)));
  for (int nDim : {2, 3}) {
    for (int trial = 0; trial < 2000; ++trial) {
      JSTEntropy::NodeState A, B;
      RandomJSTState(gen, nDim, 2.0, A);
      RandomJSTState(gen, nDim, (trial % 2) ? 0.01 : 2.0, B);
      su2double dwU = 0.0, dwUt = 0.0;
      for (int v = 0; v < nDim + 2; ++v) {
        dwU += (B.w[v] - A.w[v]) * (B.U[v] - A.U[v]);
        dwUt += (B.w[v] - A.w[v]) * (B.Ut[v] - A.Ut[v]);
      }
      CHECK(dwU >= 0.0);
      CHECK(dwUt >= lamG * dwU * (1.0 - 1e-10));
      CHECK(dwUt <= LamG * dwU * (1.0 + 1e-10));
    }
  }
}

namespace {
/*--- Triangulated nx x ny grid (horizontal, vertical, one diagonal per cell) with the JST boundary rule N*. ---*/
struct JSTRingGrid {
  int nx, ny, nPoint;
  std::vector<std::array<int, 2>> edges;
  std::vector<int> nNeigh;
  JSTRingGrid(int nx_, int ny_) : nx(nx_), ny(ny_), nPoint(nx_ * ny_), nNeigh(nx_ * ny_, 0) {
    for (int j = 0; j < ny; ++j)
      for (int i = 0; i < nx; ++i) {
        const int p = i + nx * j;
        if (i + 1 < nx) edges.push_back({p, p + 1});
        if (j + 1 < ny) edges.push_back({p, p + nx});
        if ((i + 1 < nx) && (j + 1 < ny)) edges.push_back({p, p + nx + 1});
      }
    for (const auto& e : edges) { nNeigh[e[0]]++; nNeigh[e[1]]++; }
  }
  bool boundary(int p) const { const int i = p % nx, j = p / nx; return (i == 0) || (j == 0) || (i == nx - 1) || (j == ny - 1); }
  bool chi(int p, int q) const { return !boundary(p) || boundary(q); }  // q in N*(p)
};

/*--- Budget of all nodes: L (kernel, unweighted over N*), Lin (Lh matched, or L for J4), cb, theta, B. ---*/
struct JSTRingBudget {
  std::vector<std::array<su2double, 6>> L, Lin;
  std::vector<su2double> cb, theta, B, Pi4;
};
JSTRingBudget ComputeJSTRingBudget(const JSTRingGrid& g, const std::vector<JSTEntropy::NodeState>& S,
                                   const std::vector<su2double>& c2, const std::vector<su2double>& c4, bool matched, int nVar) {
  namespace je = JSTEntropy;
  JSTRingBudget r;
  r.L.resize(g.nPoint); r.Lin.resize(g.nPoint); r.cb.assign(g.nPoint, 0.0); r.theta.assign(g.nPoint, 0.0);
  r.B.assign(g.nPoint, 0.0); r.Pi4.assign(g.nPoint, 0.0);
  for (int p = 0; p < g.nPoint; ++p) {
    r.L[p].fill(0.0);
    su2double sum[5] = {0.0}, cSum = 0.0;
    unsigned short nRing = 0;
    for (size_t e = 0; e < g.edges.size(); ++e) {
      int q = -1;
      if (g.edges[e][0] == p) q = g.edges[e][1];
      if (g.edges[e][1] == p) q = g.edges[e][0];
      if ((q < 0) || !g.chi(p, q)) continue;
      for (int v = 0; v < nVar; ++v) { r.L[p][v] += S[q].Ut[v] - S[p].Ut[v]; sum[v] += c4[e] * (S[q].Ut[v] - S[p].Ut[v]); }
      cSum += c4[e]; ++nRing;
    }
    if (matched) je::MatchedLaplacian(nVar, sum, cSum, nRing, r.Lin[p].data(), r.cb[p]);
    else r.Lin[p] = r.L[p];
  }
  for (int p = 0; p < g.nPoint; ++p) {
    su2double P2 = 0.0, D = 0.0, Lam[5] = {0.0};
    for (size_t e = 0; e < g.edges.size(); ++e) {
      int q = -1;
      if (g.edges[e][0] == p) q = g.edges[e][1];
      if (g.edges[e][1] == p) q = g.edges[e][0];
      if (q < 0) continue;
      je::AddEdge(nVar, S[p], S[q], c2[e], c4[e], 0.5 * c4[e] * (g.nNeigh[p] + g.nNeigh[q] + 2.0), matched ? g.chi(p, q) : true, P2,
                  Lam, D);
    }
    for (int v = 0; v < nVar; ++v) r.Pi4[p] -= r.Lin[p][v] * Lam[v];
    r.B[p] = P2 + r.Pi4[p];
    r.theta[p] = je::NodeTheta(nVar, P2, r.Lin[p].data(), Lam, D);
  }
  return r;
}
}  // namespace

TEST_CASE("JST entropy: matched term (J7) and J4, theta in [0,1] and JST production sum_i min(B_i,0) on random rings", "[P1Flux]") {
  namespace je = JSTEntropy;
  std::mt19937 gen(2718);
  std::uniform_real_distribution<double> u(-1.0, 1.0), u01(0.0, 1.0);
  const JSTRingGrid g(7, 6);
  const int nDim = 2, nVar = 4;
  int active = 0;
  for (int trial = 0; trial < 120; ++trial) {
    const bool matched = (trial % 2 == 0);
    std::vector<je::NodeState> S(g.nPoint);
    const bool shock = (trial % 12 == 11);
    for (int p = 0; p < g.nPoint; ++p) RandomJSTState(gen, nDim, (trial % 3 == 0) ? 2.0 : 0.2, S[p]);
    if (trial % 4 == 1) for (int p = 0; p < g.nPoint; ++p) if (p % g.nx > g.nx / 2) RandomJSTState(gen, nDim, 0.0, S[p]);  // a jump
    std::vector<su2double> c2(g.edges.size()), c4(g.edges.size());
    for (size_t e = 0; e < g.edges.size(); ++e) {
      c2[e] = exp(2.0 * u(gen)) * ((trial % 4 < 2) ? 1.0 : 0.0);
      c4[e] = shock ? 0.0 : exp(2.0 * u(gen));
      if ((trial % 5 == 2) && (u01(gen) < 0.3)) c4[e] = 0.0;  // mixed zero / nonzero edges (shock edges, weak walls)
    }
    const auto r = ComputeJSTRingBudget(g, S, c2, c4, matched, nVar);
    for (int p = 0; p < g.nPoint; ++p) {
      CHECK(r.theta[p] >= 0.0);
      CHECK(r.theta[p] <= 1.0);
      if (shock) CHECK(r.theta[p] == 0.0);
      active += (r.theta[p] > 0.0);
    }
    /*--- Production of the kernel fluxes plus the corrections, from the fluxes. ---*/
    su2double production = 0.0, scale = 0.0, budget = 0.0;
    for (size_t e = 0; e < g.edges.size(); ++e) {
      const int a = g.edges[e][0], b = g.edges[e][1];
      const bool chi_ab = matched ? g.chi(a, b) : true, chi_ba = matched ? g.chi(b, a) : true;
      su2double corr[5];
      je::CorrectionFlux(nVar, c4[e], 0.5 * c4[e] * (g.nNeigh[a] + g.nNeigh[b] + 2.0), r.theta[a], r.theta[b], chi_ab, chi_ba,
                         r.Lin[a].data(), r.Lin[b].data(), r.L[a].data(), r.L[b].data(), S[a].U, S[b].U, corr);
      for (int v = 0; v < nVar; ++v) {
        const su2double d = c2[e] * (S[a].Ut[v] - S[b].Ut[v]) - c4[e] * (r.L[a][v] - r.L[b][v]) + corr[v];
        production -= (S[a].w[v] - S[b].w[v]) * d;
        scale += fabs((S[a].w[v] - S[b].w[v]) * d);
      }
    }
    for (int p = 0; p < g.nPoint; ++p) budget += fmin(r.B[p], 0.0);
    CHECK(production <= 1e-12 * scale);
    CHECK(production == Approx(budget).margin(1e-10 * (scale + 1.0)));
  }
  CHECK(active > 0);  // the budget acts on rough states
}

TEST_CASE("JST entropy: matched term, leading-order sign Pi4_i <= 0 for any coefficients (Lemma J3')", "[P1Flux]") {
  namespace je = JSTEntropy;
  std::mt19937 gen(31415);
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  const JSTRingGrid g(7, 6);
  const int nDim = 2, nVar = 4;
  for (int trial = 0; trial < 40; ++trial) {
    /*--- Small perturbations of one admissible state, edge coefficients over e^-2..e^2, no eps2. ---*/
    je::NodeState base;
    RandomJSTState(gen, nDim, 1.0, base);
    const su2double rho = base.U[0], vx = base.U[1] / rho, vy = base.U[2] / rho, p = (kGamma - 1.0) * (base.U[3] - 0.5 * rho * (vx * vx + vy * vy));
    std::vector<je::NodeState> S(g.nPoint);
    for (int q = 0; q < g.nPoint; ++q) {
      const su2double eps = 1e-6, r_ = rho * (1 + eps * u(gen)), p_ = p * (1 + eps * u(gen));
      const su2double vel[2] = {vx + eps * u(gen), vy + eps * u(gen)};
      je::SetState(nDim, kGamma, r_, vel, p_, kGamma / (kGamma - 1.0) * p_ / r_ + 0.5 * (vel[0] * vel[0] + vel[1] * vel[1]), S[q]);
    }
    std::vector<su2double> c2(g.edges.size(), 0.0), c4(g.edges.size());
    for (auto& c : c4) c = exp(2.0 * u(gen));
    const auto r = ComputeJSTRingBudget(g, S, c2, c4, true, nVar);
    for (int q = 0; q < g.nPoint; ++q) {
      su2double nl = 0.0;
      for (int v = 0; v < nVar; ++v) nl += r.Lin[q][v] * r.Lin[q][v];
      /*--- -Pi4 is a quadratic form of size |Lam|^2/cb ~ cb |Lh|^2; the state variation enters at relative order eps. ---*/
      CHECK(r.Pi4[q] <= 1e-4 * r.cb[q] * nl + 1e-300);
      CHECK(r.theta[q] == 0.0);
    }
  }
}

TEST_CASE("JST entropy: matched term reduces to JST for constant positive c on interior rings, correction 0 at theta = 0", "[P1Flux]") {
  namespace je = JSTEntropy;
  std::mt19937 gen(4242);
  const JSTRingGrid g(6, 6);
  const int nDim = 2, nVar = 4;
  std::vector<je::NodeState> S(g.nPoint);
  for (auto& st : S) RandomJSTState(gen, nDim, 0.5, st);
  std::vector<su2double> c2(g.edges.size(), 0.0), c4(g.edges.size(), 2.5);
  const auto r = ComputeJSTRingBudget(g, S, c2, c4, true, nVar);
  const auto rJ = ComputeJSTRingBudget(g, S, c2, c4, false, nVar);
  for (int p = 0; p < g.nPoint; ++p) {
    CHECK(r.cb[p] == Approx(2.5));
    for (int v = 0; v < nVar; ++v) CHECK(r.Lin[p][v] == Approx(rJ.L[p][v]).margin(1e-12));
  }
  for (size_t e = 0; e < g.edges.size(); ++e) {
    const int a = g.edges[e][0], b = g.edges[e][1];
    if (g.boundary(a) || g.boundary(b)) continue;  // chi = 1 at both ends
    su2double corr[5];
    je::CorrectionFlux(nVar, c4[e], 1.0, 0.0, 0.0, true, true, r.Lin[a].data(), r.Lin[b].data(), rJ.L[a].data(), rJ.L[b].data(),
                       S[a].U, S[b].U, corr);
    for (int v = 0; v < nVar; ++v) CHECK(corr[v] == Approx(0.0).margin(1e-11));
    /*--- Jacobian factors: 0 at theta = 0, chi = 1, rho = 1; kernel surrogate + factor = surrogate of the matched term >= 0. ---*/
    su2double fii, fik;
    je::CorrectionJacobianFactors(c4[e], 0.0, 0.0, true, true, g.nNeigh[a], g.nNeigh[b], 2.5, 2.5, fii, fik);
    CHECK(fii == Approx(0.0).margin(1e-14));
    CHECK(fik == Approx(0.0).margin(1e-14));
    je::CorrectionJacobianFactors(c4[e], 0.3, 0.7, false, true, g.nNeigh[a], g.nNeigh[b], 1.0, 5.0, fii, fik);
    CHECK(c4[e] * (g.nNeigh[a] + 1.0) + fii == Approx(c4[e] * 0.3 * c4[e] / 5.0));
    CHECK(-c4[e] * (g.nNeigh[b] + 1.0) + fik == Approx(-c4[e] * 0.3 * g.nNeigh[b]));
  }
  /*--- Zero ring (all c4 = 0): cb = 0 and Lh = 0. ---*/
  su2double sum[5] = {1.0, 2.0, 3.0, 4.0, 5.0}, Lh[5], cb = 1.0;
  je::MatchedLaplacian(nVar, sum, 0.0, 6, Lh, cb);
  CHECK(cb == 0.0);
  for (int v = 0; v < nVar; ++v) CHECK(Lh[v] == 0.0);
}

/*--- SST matched k on the pairs that are not edges (SSTPATHS_PLAN.md 1, 4): the diagonals of EDGE_FORM_P1_MIXED and the
 * pairs and 3D boundary nodal terms of SPLIT_EDGE_FLUX. Model hat scheme on the pair data of a real geometry: the mean of
 * the nodal hat Euler fluxes along n, the E part (F_k - F_i).E, a scalar dissipation on the hat jump of the true edges
 * only (split pairs and diagonals have none, as CEulerSolver::SplitEdgeResidual), and the matched k of
 * P1SSTMatched::PairKine in the energy row and in the k row. ---*/
namespace {
using State5 = std::array<su2double, 5>;

struct PairResidual {
  std::vector<State5> R;     /*!< \brief Flow rows (rho, m, rhoE). */
  std::vector<su2double> Rk; /*!< \brief SST k row. */
  std::vector<su2double> Dk; /*!< \brief Diagonal of the k-row Jacobian (P1KNodalJac of the pairs). */
};

su2double NormOf(int nDim, const su2double* n) {
  su2double a = 0.0;
  for (int d = 0; d < nDim; ++d) a += n[d] * n[d];
  return sqrt(a);
}

/*--- U: conservative states with k in the energy; with k = 0 and U = Uhat this is the hat scheme. ---*/
PairResidual MatchedPairResidual(const CGeometry& g, bool mixed, const std::vector<State5>& U,
                                 const std::vector<su2double>& k) {
  const int nDim = g.GetnDim(), nVar = nDim + 2;
  PairResidual out;
  out.R.assign(g.GetnPoint(), State5{});
  out.Rk.assign(g.GetnPoint(), 0.0);
  out.Dk.assign(g.GetnPoint(), 0.0);
  auto hat = [&](unsigned long i) {
    State5 H = U[i];
    H[nDim + 1] -= U[i][0] * k[i];
    return H;
  };
  auto add = [&](unsigned long i, unsigned long j, const su2double* n, const su2double* E, su2double lam) {
    const State5 Hi = hat(i), Hj = hat(j);
    su2double Fi[5], Fj[5], flux[5];
    EulerFluxK(nDim, Hi.data(), 0.0, n, Fi);
    EulerFluxK(nDim, Hj.data(), 0.0, n, Fj);
    for (int v = 0; v < nVar; ++v) flux[v] = 0.5 * (Fi[v] + Fj[v]) - lam * (Hj[v] - Hi[v]);
    if (E != nullptr) {
      EulerFluxK(nDim, Hi.data(), 0.0, E, Fi);
      EulerFluxK(nDim, Hj.data(), 0.0, E, Fj);
      for (int v = 0; v < nVar; ++v) flux[v] += Fj[v] - Fi[v];
    }
    const su2double kFlux = P1SSTMatched::PairKine(flux[0], k[i], k[j]);
    su2double di = 0.0, dj = 0.0;
    P1SSTMatched::PairKineDiag(flux[0], U[i][0], U[j][0], di, dj);
    flux[nVar - 1] += kFlux;
    for (int v = 0; v < nVar; ++v) {
      out.R[i][v] += flux[v];
      out.R[j][v] -= flux[v];
    }
    out.Rk[i] += kFlux;
    out.Rk[j] -= kFlux;
    out.Dk[i] += di;
    out.Dk[j] += dj;
  };
  for (auto iEdge = 0ul; iEdge < g.GetnEdge(); ++iEdge) {
    const su2double* n = g.edges->GetFormNormal()[iEdge];
    add(g.edges->GetNode(iEdge, 0), g.edges->GetNode(iEdge, 1), n, g.edges->GetFormE()[iEdge], 0.3 * NormOf(nDim, n));
  }
  const auto& pairs = g.edges->GetSplitNodes();
  for (auto iSplit = 0ul; iSplit < pairs.size(); ++iSplit) {
    add(pairs[iSplit][0], pairs[iSplit][1], g.edges->GetSplitNormal()[iSplit],
        mixed ? nullptr : g.edges->GetSplitE()[iSplit], 0.0);
  }
  /*--- SPLIT_EDGE_FLUX in 3D: the nodal boundary term F_i . n_b, its k part k_i m_b (the node's own k). ---*/
  if (!mixed && (nDim == 3)) {
    for (auto i = 0ul; i < g.GetnPointDomain(); ++i) {
      const State5 Hi = hat(i);
      su2double F[5];
      EulerFluxK(nDim, Hi.data(), 0.0, g.edges->GetSplitBoundNormal()[i], F);
      const su2double kFlux = k[i] * F[0];
      F[nVar - 1] += kFlux;
      for (int v = 0; v < nVar; ++v) out.R[i][v] += F[v];
      out.Rk[i] += kFlux;
      out.Dk[i] += fmax(F[0], 0.0) / U[i][0];
    }
  }
  return out;
}

/*--- Entropy variables w of the hat state: the coupled v has v_K = -v_E, so v.(R, R_k) = w.(R_rho, R_m, R_E - R_k). ---*/
void HatEntropyVariables(int nDim, const State5& U, su2double k, su2double* w) {
  su2double m2 = 0.0;
  for (int i = 0; i < nDim; ++i) m2 += U[i + 1] * U[i + 1];
  const su2double rho = U[0];
  const su2double p = (kGamma - 1.0) * (U[nDim + 1] - 0.5 * m2 / rho - rho * k);
  const su2double s = log(p) - kGamma * log(rho), beta = 0.5 * rho / p;
  w[0] = (kGamma - s) / (kGamma - 1.0) - beta * m2 / (rho * rho);
  for (int i = 0; i < nDim; ++i) w[i + 1] = 2.0 * beta * U[i + 1] / rho;
  w[nDim + 1] = -2.0 * beta;
}

/*--- Random or uniform (rho, u, p) and k, conservative states with k in the energy. ---*/
void FillMatchedStates(std::mt19937& gen, int nDim, const CGeometry& g, bool uniformFlow, bool uniformK,
                       std::vector<State5>& U, std::vector<su2double>& k) {
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  const auto nPoint = g.GetnPoint();
  U.assign(nPoint, State5{});
  k.assign(nPoint, 0.0);
  const su2double vel0[3] = {0.6, -0.2, 0.3};
  for (auto i = 0ul; i < nPoint; ++i) {
    /*--- From the coordinates, so that halo copies on other ranks hold the same state. ---*/
    const su2double* x = g.nodes->GetCoord(i);
    su2double phase = 0.0;
    for (int d = 0; d < nDim; ++d) phase += (d + 1.3) * x[d];
    const su2double rho = uniformFlow ? 1.1 : 1.1 * exp(0.3 * sin(3.1 * phase));
    const su2double p = uniformFlow ? 0.9 : 0.9 * exp(0.3 * cos(2.3 * phase));
    k[i] = uniformK ? 0.05 : 0.05 * exp(sin(5.7 * phase + 0.4));
    su2double vel2 = 0.0;
    U[i][0] = rho;
    for (int d = 0; d < nDim; ++d) {
      const su2double v = uniformFlow ? vel0[d] : vel0[d] + 0.3 * sin(1.7 * phase + d);
      U[i][d + 1] = rho * v;
      vel2 += v * v;
    }
    U[i][nDim + 1] = p / (kGamma - 1.0) + 0.5 * rho * vel2 + rho * k[i];
  }
  (void)gen;
}

std::vector<State5> HatStates(int nDim, const std::vector<State5>& U, const std::vector<su2double>& k) {
  auto H = U;
  for (auto i = 0ul; i < U.size(); ++i) H[i][nDim + 1] -= U[i][0] * k[i];
  return H;
}

/*--- (a) free stream with a k jump, (b) uniform k, (c) conservation, (d) entropy identity, (e) k-row diagonal by FD. ---*/
void CheckMatchedPairs(const CGeometry& g, bool mixed) {
  const int nDim = g.GetnDim(), nVar = nDim + 2;
  const auto nPoint = g.GetnPoint();
  std::mt19937 gen(2468 + nDim);
  std::vector<State5> U;
  std::vector<su2double> k;
  const std::vector<su2double> zero(nPoint, 0.0);
  su2double worst[6] = {0.0};

  /*--- (a) uniform (rho, u, p) with a k jump: the k changes only the energy row, by the k row; interior hat rows = 0. ---*/
  FillMatchedStates(gen, nDim, g, true, false, U, k);
  const auto res = MatchedPairResidual(g, mixed, U, k);
  const auto hatRes = MatchedPairResidual(g, mixed, HatStates(nDim, U, k), zero);
  for (auto i = 0ul; i < g.GetnPointDomain(); ++i) {
    for (int v = 0; v < nVar - 1; ++v) worst[0] = fmax(worst[0], fabs(res.R[i][v] - hatRes.R[i][v]));
    worst[0] = fmax(worst[0], fabs(res.R[i][nVar - 1] - res.Rk[i] - hatRes.R[i][nVar - 1]));
    if (!g.nodes->GetPhysicalBoundary(i)) {
      for (int v = 0; v < nVar; ++v) worst[1] = fmax(worst[1], fabs(hatRes.R[i][v]));
      worst[1] = fmax(worst[1], fabs(res.R[i][nVar - 1] - res.Rk[i]));
    }
  }

  /*--- (b) uniform k with a random flow: R_k = k R_rho at every node (every mass term carries k). ---*/
  FillMatchedStates(gen, nDim, g, false, true, U, k);
  const auto resU = MatchedPairResidual(g, mixed, U, k);
  for (auto i = 0ul; i < g.GetnPointDomain(); ++i)
    worst[2] = fmax(worst[2], fabs(resU.Rk[i] - k[i] * resU.R[i][0]) / (1.0 + fabs(resU.R[i][0])));

  /*--- (c) conservation: the pairs give sum_i R_k,i = 0, the 3D split nodal terms add their own k_i m_b. ---*/
  FillMatchedStates(gen, nDim, g, false, false, U, k);
  const auto resC = MatchedPairResidual(g, mixed, U, k);
  su2double sumK = 0.0, nodal = 0.0, scale = 0.0;
  for (auto i = 0ul; i < g.GetnPointDomain(); ++i) {
    sumK += resC.Rk[i];
    scale += fabs(resC.Rk[i]);
    if (!mixed && (nDim == 3)) {
      State5 Hi = U[i];
      Hi[nVar - 1] -= U[i][0] * k[i];
      su2double F[5];
      EulerFluxK(nDim, Hi.data(), 0.0, g.edges->GetSplitBoundNormal()[i], F);
      nodal += k[i] * F[0];
    }
  }
  worst[3] = fabs(GlobalSum(sumK) - GlobalSum(nodal)) / (1.0 + GlobalSum(scale));

  /*--- (d) -sum v.(R, R_k) of the coupled scheme = -sum w.Rhat of the hat scheme (Theorem C1 with M1). ---*/
  const auto resH = MatchedPairResidual(g, mixed, HatStates(nDim, U, k), zero);
  su2double coupled = 0.0, hatRate = 0.0, mag = 0.0;
  for (auto i = 0ul; i < g.GetnPointDomain(); ++i) {
    su2double w[5];
    HatEntropyVariables(nDim, U[i], k[i], w);
    for (int v = 0; v < nVar - 1; ++v) {
      coupled -= w[v] * resC.R[i][v];
      hatRate -= w[v] * resH.R[i][v];
    }
    coupled -= w[nVar - 1] * (resC.R[i][nVar - 1] - resC.Rk[i]);
    hatRate -= w[nVar - 1] * resH.R[i][nVar - 1];
    for (int v = 0; v < nVar; ++v) mag += fabs(w[v] * resH.R[i][v]);
  }
  worst[4] = fabs(GlobalSum(coupled) - GlobalSum(hatRate)) / (1.0 + GlobalSum(mag));

  /*--- (e) the k-row diagonal of a pair against central differences in rho k at a frozen mass flux. ---*/
  const auto& pairs = g.edges->GetSplitNodes();
  for (auto iSplit = 0ul; iSplit < pairs.size(); ++iSplit) {
    const auto i = pairs[iSplit][0], j = pairs[iSplit][1];
    for (const su2double m : {0.37, -0.53}) {
      su2double di = 0.0, dj = 0.0;
      P1SSTMatched::PairKineDiag(m, U[i][0], U[j][0], di, dj);
      const su2double h = 1e-7;
      const su2double fdi = (P1SSTMatched::PairKine(m, k[i] + h / U[i][0], k[j]) -
                             P1SSTMatched::PairKine(m, k[i] - h / U[i][0], k[j])) / (2 * h);
      const su2double fdj = -(P1SSTMatched::PairKine(m, k[i], k[j] + h / U[j][0]) -
                              P1SSTMatched::PairKine(m, k[i], k[j] - h / U[j][0])) / (2 * h);
      worst[5] = fmax(worst[5], fabs(di - fdi) + fabs(dj - fdj));
    }
  }

  INFO("nDim " << nDim << (mixed ? " mixed diagonals" : " split pairs"));
  CHECK(GlobalMax(worst[0]) < 1e-12);
  CHECK(GlobalMax(worst[1]) < 1e-11);
  CHECK(GlobalMax(worst[2]) < 1e-13);
  CHECK(worst[3] < 1e-13);
  CHECK(worst[4] < 1e-13);
  CHECK(GlobalMax(worst[5]) < 1e-7);
}
}  // namespace

TEST_CASE("P1 SST matched: mixed-cell diagonals, free stream with a k jump, uniform k, conservation, entropy identity", "[P1Flux]") {
  for (int nDim : {2, 3}) {
    const std::string mesh = "p1_sst_mixed_" + std::to_string(nDim) + "d.su2";
    WriteMixedMesh(mesh, nDim, (nDim == 2) ? 6 : 3, 0.2, 1.0);
    MeshCase test(mesh, MixedOptions);
    auto& geometry = *test.geometry;
    geometry.SetEdgeFormP1(true);
    REQUIRE(GlobalSum(su2double(geometry.edges->GetSplitNodes().size())) > 0.0);
    CheckMatchedPairs(geometry, true);
  }
}

TEST_CASE("P1 SST matched: SPLIT_EDGE_FLUX pairs and boundary nodal terms, the same identities", "[P1Flux]") {
  struct Kind { int nDim, kind; unsigned long n; };
  for (const auto& m : {Kind{2, 0, 6}, Kind{3, 0, 3}, Kind{3, 1, 3}}) {
    const std::string name = "p1_sst_split_" + std::to_string(m.nDim) + "_" + std::to_string(m.kind) + ".su2";
    WriteGeneralMesh(name, m.nDim, m.kind, m.n, 0.2, 1.0);
    MeshCase test(name, "CONV_NUM_METHOD_FLOW= JST\nSPLIT_EDGE_FLUX= YES\n");
    auto& geometry = *test.geometry;
    geometry.SetSplitEdges();
    INFO("kind " << m.kind);
    CheckMatchedPairs(geometry, false);
  }
}

TEST_CASE("P1 SST matched: weighted sub-simplex k diffusion of a mixed cell, conservation, linear k, D_t shares (M3, M4)", "[P1Flux]") {
  for (int nDim : {2, 3}) {
    const std::string mesh = "p1_sst_mixedk_" + std::to_string(nDim) + "d.su2";
    WriteMixedMesh(mesh, nDim, (nDim == 2) ? 6 : 3, 0.2, 1.0);
    MeshCase test(mesh, MixedOptions);
    auto& geometry = static_cast<CPhysicalGeometry&>(*test.geometry);
    geometry.SetEdgeFormP1(true);
    const unsigned short nNodes = nDim + 1;
    std::vector<su2double> R(geometry.GetnPoint(), 0.0);
    su2double cellWorst = 0.0, shareWorst = 0.0;
    const su2double D = 0.7;
    for (auto iElem = 0ul; iElem < geometry.GetnElem(); ++iElem) {
      const auto* element = geometry.elem[iElem];
      /*--- A simplex is its own list with weight 1, as in CTurbSSTSolver::P1KineDiffusion. ---*/
      std::vector<P1Mixed::Simplex> own(1);
      for (unsigned short a = 0; a < nNodes; ++a) own[0].nodes[a] = a;
      own[0].weight = 1.0;
      const auto& list = (element->GetnNodes() == nNodes) ? own : geometry.GetMixedSimplices(element->GetVTK_Type());
      su2double cellSum = 0.0;
      for (const auto& entry : list) {
        const su2double* coord[4];
        unsigned long point[4];
        su2double kNode[4], res[4], grad[4][3];
        for (unsigned short a = 0; a < nNodes; ++a) {
          point[a] = element->GetNode(entry.nodes[a]);
          coord[a] = geometry.nodes->GetCoord(point[a]);
          kNode[a] = 0.3 + 0.7 * coord[a][0] - 0.2 * coord[a][1] + ((nDim == 3) ? 0.4 * coord[a][2] : 0.0);
        }
        const su2double size = fabs(P1Simplex::ShapeGradients(nDim, coord, grad));
        P1SSTMatched::KineDiffusion(nDim, size, grad, kNode, D, res);
        for (unsigned short a = 0; a < nNodes; ++a) {
          R[point[a]] += entry.weight * res[a];
          cellSum += entry.weight * res[a];
        }
        /*--- Shares Dt/((d+1) theta_a) summed with theta_a give Dt for any Dt and theta (E1). ---*/
        const su2double Dt = 0.37 * size, theta[4] = {1.3, 0.8, 1.1, 0.9};
        su2double back = 0.0;
        for (unsigned short a = 0; a < nNodes; ++a) back += theta[a] * Dt / (nNodes * theta[a]);
        shareWorst = fmax(shareWorst, fabs(back - Dt));
      }
      cellWorst = fmax(cellWorst, fabs(cellSum));
    }
    /*--- Interior nodes: a linear k with constant D gives zero (the split averages are consistent with the simplices
     * and with each other across the quad faces, P1_MIXED.md X2). ---*/
    su2double interiorWorst = 0.0;
    for (auto i = 0ul; i < geometry.GetnPointDomain(); ++i) {
      if (geometry.nodes->GetPhysicalBoundary(i)) continue;
      interiorWorst = fmax(interiorWorst, fabs(R[i]));
    }
    INFO("nDim " << nDim);
    CHECK(GlobalMax(cellWorst) < 1e-13);
    CHECK(GlobalMax(shareWorst) < 1e-15);
    CHECK(GlobalMax(interiorWorst) < 1e-12);
  }
}

/*--- SST matched k with MUSCL_TURB (SSTPATHS_PLAN.md 2): the face k is the U-MUSCL value of the k row (reconstruct with
 * EDGE_SCALAR as CScalarFlux_SST) on the upwind side, with the bounds and strong-node fallbacks of MusclFaceKine. ---*/
namespace {
/*--- The matched flux with a given face k: Fhat(T(k_i) U_i, T(k_j) U_j) + kFace Fhat_rho e_E. ---*/
void MatchedEdgeFluxFace(int nDim, const su2double* Ui, const su2double* Uj, su2double ki, su2double kj, const su2double* n,
                         su2double lam, su2double kFace, su2double* F) {
  su2double Hi[5], Hj[5];
  for (int v = 0; v < nDim + 2; ++v) { Hi[v] = Ui[v]; Hj[v] = Uj[v]; }
  Hi[nDim + 1] -= Ui[0] * ki;
  Hj[nDim + 1] -= Uj[0] * kj;
  HatEdgeFlux(nDim, Hi, Hj, n, lam, F);
  F[nDim + 1] += kFace * F[0];
}

/*--- One reconstructed scalar with the all(iVar) accessor of the MUSCL helpers. ---*/
struct KineValue {
  static constexpr size_t nVar = 1;
  Vector<su2double, 1> all;
};

/*--- U-MUSCL face values of k on the edge from 0 to 1 (vector x_1 - x_0), as the k row reconstructs them. ---*/
void MusclKine(int nDim, const su2double* x01, const su2double (*gradK)[3], su2double k0, su2double k1, LIMITER limiter,
               const su2double* pointLim, su2double kappa, su2double& kL, su2double& kR) {
  C3DDoubleMatrix gradient(2, 1, nDim);
  su2activematrix lim(2, 1);
  Vector<su2double, 3> edge3;
  for (int s = 0; s < 2; ++s) {
    for (int d = 0; d < nDim; ++d) gradient(s, 0, d) = gradK[s][d];
    lim(s, 0) = pointLim[s];
  }
  CPair<KineValue> face;
  face.i.all(0) = k0;
  face.j.all(0) = k1;
  if (nDim == 2) {
    Vector<su2double, 2> edge;
    for (int d = 0; d < 2; ++d) edge(d) = x01[d];
    reconstruct<1, MusclLimiter::EDGE_SCALAR>(0ul, 1ul, edge, gradient, lim, limiter, 0, face, kappa, su2double(1.0));
  } else {
    for (int d = 0; d < 3; ++d) edge3(d) = x01[d];
    reconstruct<1, MusclLimiter::EDGE_SCALAR>(0ul, 1ul, edge3, gradient, lim, limiter, 0, face, kappa, su2double(1.0));
  }
  kL = face.i.all(0);
  kR = face.j.all(0);
}
}  // namespace

TEST_CASE("P1 SST matched MUSCL_TURB: face k, free stream with a k jump, uniform k, fallbacks, entropy identity, L Jhat T", "[P1Flux]") {
  std::mt19937 gen(31337);
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  const su2double lo = 1e-10, hi = 1e10;
  for (int nDim : {2, 3}) {
    const int nVar = nDim + 2;
    for (const LIMITER limiter : {LIMITER::NONE, LIMITER::VAN_ALBADA_EDGE, LIMITER::VENKATAKRISHNAN}) {
      for (int trial = 0; trial < 40; ++trial) {
        su2double x01[3] = {u(gen), u(gen), u(gen)}, n[3] = {u(gen), u(gen), u(gen)}, gradK[2][3], pointLim[2];
        for (int s = 0; s < 2; ++s) {
          for (int d = 0; d < 3; ++d) gradK[s][d] = 0.2 * u(gen);
          pointLim[s] = 0.5 * (1.0 + u(gen));
        }
        su2double k[2] = {0.2 * exp(u(gen)), 0.2 * exp(u(gen))};
        const su2double kappa = 0.3 * u(gen);
        su2double kL, kR;
        MusclKine(nDim, x01, gradK, k[0], k[1], limiter, pointLim, kappa, kL, kR);

        /*--- (b) uniform k with zero gradient: the face value is k exactly. ---*/
        {
          const su2double zeroGrad[2][3] = {{0.0, 0.0, 0.0}, {0.0, 0.0, 0.0}};
          su2double uL, uR;
          MusclKine(nDim, x01, zeroGrad, k[0], k[0], limiter, pointLim, kappa, uL, uR);
          for (const su2double m : {0.4, -0.6}) {
            CHECK(P1SSTMatched::MusclFaceKine(m, uL, uR, k[0], k[0], su2double(0.0), su2double(0.0), lo, hi) == k[0]);
          }
        }
        /*--- (c) fallbacks: a strong-k upwind node and a face value out of the bounds give the node value. ---*/
        CHECK(P1SSTMatched::MusclFaceKine(su2double(0.5), kL, kR, k[0], k[1], su2double(1.0), su2double(0.0), lo, hi) == k[0]);
        CHECK(P1SSTMatched::MusclFaceKine(su2double(-0.5), kL, kR, k[0], k[1], su2double(0.0), su2double(1.0), lo, hi) == k[1]);
        CHECK(P1SSTMatched::MusclFaceKine(su2double(0.5), su2double(-1.0), kR, k[0], k[1], su2double(0.0), su2double(0.0), lo, hi) == k[0]);
        CHECK(P1SSTMatched::MusclFaceKine(su2double(0.5), kL, kR, k[0], k[1], su2double(0.0), su2double(0.0), lo, hi) ==
              ((kL < lo) ? k[0] : kL));

        /*--- (a) uniform (rho, u, p) with a k jump: the hat rows do not see k, the energy row differs by kFace m. ---*/
        const su2double rho = exp(u(gen)), p = exp(u(gen));
        su2double vel[3] = {u(gen), u(gen), u(gen)}, vel2 = 0.0;
        for (int d = 0; d < nDim; ++d) vel2 += vel[d] * vel[d];
        su2double U[2][5];
        for (int s = 0; s < 2; ++s) {
          U[s][0] = rho;
          for (int d = 0; d < nDim; ++d) U[s][d + 1] = rho * vel[d];
          U[s][nDim + 1] = p / (kGamma - 1.0) + 0.5 * rho * vel2 + rho * k[s];
        }
        su2double F[5], Fhat[5], H[5];
        for (int v = 0; v < nVar; ++v) H[v] = U[0][v];
        H[nVar - 1] -= rho * k[0];
        HatEdgeFlux(nDim, H, H, n, 0.7, Fhat);
        const su2double kFace0 = P1SSTMatched::MusclFaceKine(Fhat[0], kL, kR, k[0], k[1], su2double(0.0), su2double(0.0), lo, hi);
        MatchedEdgeFluxFace(nDim, U[0], U[1], k[0], k[1], n, 0.7, kFace0, F);
        for (int v = 0; v < nVar - 1; ++v) CHECK(F[v] == Approx(Fhat[v]).margin(1e-12 * (1.0 + fabs(Fhat[v]))));
        CHECK(F[nVar - 1] - kFace0 * F[0] == Approx(Fhat[nVar - 1]).margin(1e-12 * (1.0 + fabs(Fhat[nVar - 1]))));

        /*--- Random states for (d) and (e). ---*/
        for (int s = 0; s < 2; ++s) {
          const su2double r = exp(u(gen)), q = exp(u(gen));
          su2double w2 = 0.0;
          U[s][0] = r;
          for (int d = 0; d < nDim; ++d) { const su2double v = u(gen); U[s][d + 1] = r * v; w2 += v * v; }
          U[s][nDim + 1] = q / (kGamma - 1.0) + 0.5 * r * w2 + r * k[s];
        }
        MatchedEdgeFluxFace(nDim, U[0], U[1], k[0], k[1], n, 0.7, 0.0, F);
        const su2double m = F[0];
        if (fabs(m) < 1e-3) continue;
        const su2double kFace = P1SSTMatched::MusclFaceKine(m, kL, kR, k[0], k[1], su2double(0.0), su2double(0.0), lo, hi);
        MatchedEdgeFluxFace(nDim, U[0], U[1], k[0], k[1], n, 0.7, kFace, F);

        /*--- (d) the edge production of the coupled scheme (v_K = -v_E) is that of the hat flux. ---*/
        {
          su2double w0[5], w1[5], Hh[2][5];
          State5 S0, S1;
          for (int v = 0; v < nVar; ++v) { S0[v] = U[0][v]; S1[v] = U[1][v]; }
          HatEntropyVariables(nDim, S0, k[0], w0);
          HatEntropyVariables(nDim, S1, k[1], w1);
          for (int s = 0; s < 2; ++s) {
            for (int v = 0; v < nVar; ++v) Hh[s][v] = U[s][v];
            Hh[s][nVar - 1] -= U[s][0] * k[s];
          }
          HatEdgeFlux(nDim, Hh[0], Hh[1], n, 0.7, Fhat);
          su2double coupled = 0.0, hat = 0.0;
          const su2double Fk = kFace * F[0];
          for (int v = 0; v < nVar - 1; ++v) { coupled += (w1[v] - w0[v]) * F[v]; hat += (w1[v] - w0[v]) * Fhat[v]; }
          coupled += (w1[nVar - 1] - w0[nVar - 1]) * (F[nVar - 1] - Fk);
          hat += (w1[nVar - 1] - w0[nVar - 1]) * Fhat[nVar - 1];
          CHECK(coupled == Approx(hat).margin(1e-12 * (1.0 + fabs(hat))));
        }

        /*--- (e) FD of the matched flux with the face k frozen = L(kFace) Jhat T(k_side) (D1). ---*/
        for (int side = 0; side < 2; ++side) {
          su2double Hs[2][5];
          for (int s = 0; s < 2; ++s) {
            for (int v = 0; v < nVar; ++v) Hs[s][v] = U[s][v];
            Hs[s][nVar - 1] -= U[s][0] * k[s];
          }
          su2activematrix J(nVar, nVar), Jfd(nVar, nVar);
          const su2double h = 1e-6;
          for (int jv = 0; jv < nVar; ++jv) {
            su2double Hp[2][5], Hm[2][5], Up[2][5], Um[2][5], Fp[5], Fm[5];
            for (int s = 0; s < 2; ++s)
              for (int v = 0; v < nVar; ++v) { Hp[s][v] = Hm[s][v] = Hs[s][v]; Up[s][v] = Um[s][v] = U[s][v]; }
            Hp[side][jv] += h; Hm[side][jv] -= h;
            HatEdgeFlux(nDim, Hp[0], Hp[1], n, 0.7, Fp);
            HatEdgeFlux(nDim, Hm[0], Hm[1], n, 0.7, Fm);
            for (int iv = 0; iv < nVar; ++iv) J(iv, jv) = (Fp[iv] - Fm[iv]) / (2 * h);
            Up[side][jv] += h; Um[side][jv] -= h;
            MatchedEdgeFluxFace(nDim, Up[0], Up[1], k[0], k[1], n, 0.7, kFace, Fp);
            MatchedEdgeFluxFace(nDim, Um[0], Um[1], k[0], k[1], n, 0.7, kFace, Fm);
            for (int iv = 0; iv < nVar; ++iv) Jfd(iv, jv) = (Fp[iv] - Fm[iv]) / (2 * h);
          }
          P1SSTMatched::TransformJacobian(nVar, J, kFace, k[side]);
          for (int iv = 0; iv < nVar; ++iv)
            for (int jv = 0; jv < nVar; ++jv) CHECK(J(iv, jv) == Approx(Jfd(iv, jv)).margin(1e-6 * (1.0 + fabs(Jfd(iv, jv)))));
        }
      }
    }
  }
}

/*--- SST matched k on moving meshes (SSTPATHS_PLAN.md 3): GRID_ENTROPY_P1 + GCL_EXACT_P1 on the moved dual, the
 * entropy conservative central part with its grid term -ubar g_ik (AleFlux) on the hat states, closure C-ALE at every
 * boundary face, the dual-time GCL terms U G_i (flow) and (rho k) G_i (k row, the P1 grid flux), at a common state. ---*/
namespace {
ale::State StateFromConservative(int nDim, const su2double* U) {
  ale::State S;
  S.rho = U[0];
  su2double m2 = 0.0;
  for (int d = 0; d < nDim; ++d) { S.vel[d] = U[d + 1] / U[0]; m2 += U[d + 1] * U[d + 1]; }
  S.p = (kGamma - 1.0) * (U[nDim + 1] - 0.5 * m2 / U[0]);
  return S;
}
}  // namespace

TEST_CASE("P1 SST matched on moving meshes: free stream with a k jump, uniform k, entropy balance, ALE Jacobian", "[P1Flux]") {
  const std::string options = "TIME_DOMAIN= YES\nTIME_MARCHING= DUAL_TIME_STEPPING-2ND_ORDER\nTIME_STEP= 0.01\n"
                              "GRID_MOVEMENT= RIGID_MOTION\nCONV_NUM_METHOD_FLOW= JST\nEDGE_FORM_EC_FLUX= YES\n"
                              "BC_WEAK_P1= YES\nGCL_EXACT_P1= YES\nGRID_ENTROPY_P1= YES\nMOD_CENTROIDS= YES\nEDGE_FORM_P1= YES\n";
  for (int nDim : {2, 3}) {
    const std::string name = "p1_sst_moving_" + std::to_string(nDim) + "d.su2";
    WriteSimplexMesh(name, nDim, (nDim == 2) ? 8 : 3, 0.4, 1.0);
    MeshCase test(name, options);
    auto& geometry = *test.geometry;
    const auto config = test.config.get();
    const unsigned short nVar = nDim + 2;
    const su2double dt = 0.01, step = 0.1;
    config->SetDelta_UnstTimeND(dt);
    const auto X0 = StoreCoord(geometry);
    auto move = [&](su2double s) {
      const su2double c = cos(0.8 * s), sn = sin(0.8 * s);
      for (auto iPoint = 0ul; iPoint < geometry.GetnPoint(); ++iPoint) {
        su2double d[3] = {}, y[3] = {};
        Displacement(nDim, X0[iPoint].data(), d);
        for (int iDim = 0; iDim < nDim; ++iDim) y[iDim] = X0[iPoint][iDim] + s * d[iDim] - 0.5;
        geometry.nodes->SetCoord(iPoint, 0, 0.5 + c * y[0] - sn * y[1]);
        geometry.nodes->SetCoord(iPoint, 1, 0.5 + sn * y[0] + c * y[1]);
        if (nDim == 3) geometry.nodes->SetCoord(iPoint, 2, 0.5 + y[2]);
      }
      geometry.SetControlVolume(config, UPDATE);
      geometry.SetBoundControlVolume(config, UPDATE);
      geometry.SetBoundCVCoeffs(config, UPDATE);
      geometry.UpdateP1FluxCoefficients(config);
    };
    move(0.5 - 2 * step);
    geometry.nodes->SetVolume_n();
    geometry.nodes->SetCoord_n();
    move(0.5 - step);
    geometry.nodes->SetVolume_nM1();
    geometry.nodes->SetVolume_n();
    geometry.nodes->SetCoord_n1();
    geometry.nodes->SetCoord_n();
    move(0.5);
    for (auto iPoint = 0ul; iPoint < geometry.GetnPoint(); ++iPoint) {
      const auto* x = geometry.nodes->GetCoord(iPoint);
      const auto* x_n = geometry.nodes->GetCoord_n(iPoint);
      const auto* x_n1 = geometry.nodes->GetCoord_n1(iPoint);
      for (int iDim = 0; iDim < nDim; ++iDim)
        geometry.nodes->SetGridVel(iPoint, iDim, (1.5 * x[iDim] - 2.0 * x_n[iDim] + 0.5 * x_n1[iDim]) / dt);
    }
    geometry.UpdateSweptFluxP1(config);
    su2activevector G;
    geometry.SetGridFluxP1(config, G);
    const su2double kInf = 0.04;

    /*--- Coupled residual (flow rows R incl. U G, k row Rk incl. rho k G) of states with k, and the hat residual. ---*/
    auto residual = [&](const std::vector<ale::State>& state, const std::vector<su2double>& k, bool farField,
                        std::vector<State5>& R, std::vector<su2double>& Rk, su2double& pairSum) {
      R.assign(geometry.GetnPoint(), State5{});
      Rk.assign(geometry.GetnPoint(), 0.0);
      pairSum = 0.0;
      for (auto iEdge = 0ul; iEdge < geometry.GetnEdge(); ++iEdge) {
        const auto iPoint = geometry.edges->GetNode(iEdge, 0), kPoint = geometry.edges->GetNode(iEdge, 1);
        const su2double* normal = geometry.edges->GetFormNormal()[iEdge];
        const su2double* formE = geometry.edges->GetFormE()[iEdge];
        const auto* v_i = geometry.nodes->GetGridVel(iPoint);
        const auto* v_k = geometry.nodes->GetGridVel(kPoint);
        su2double gridFlux = geometry.edges->GetFormSwept()[iEdge];
        for (int iDim = 0; iDim < nDim; ++iDim)
          gridFlux += 0.5 * (v_i[iDim] + v_k[iDim]) * normal[iDim] + (v_k[iDim] - v_i[iDim]) * formE[iDim];
        su2double flux[5];
        ale::AleFlux(nDim, kGamma, state[iPoint], state[kPoint], normal, gridFlux, flux);
        const su2double kFlux = P1SSTMatched::PairKine(flux[0], k[iPoint], k[kPoint]);
        flux[nVar - 1] += kFlux;
        for (unsigned short v = 0; v < nVar; ++v) { R[iPoint][v] += flux[v]; R[kPoint][v] -= flux[v]; }
        Rk[iPoint] += kFlux;
        Rk[kPoint] -= kFlux;
      }
      for (auto iPoint = 0ul; iPoint < geometry.GetnPointDomain(); ++iPoint) pairSum += Rk[iPoint];
      const auto inf = GlobalState(12345, nDim);
      for (auto iMarker = 0u; iMarker < geometry.GetnMarker(); ++iMarker) {
        if (config->GetMarker_All_KindBC(iMarker) == SEND_RECEIVE) continue;
        for (auto iElem = 0ul; iElem < geometry.GetnElem_Bound(iMarker); ++iElem) {
          const auto* face = geometry.bound[iMarker][iElem];
          const auto nFaceNodes = face->GetnNodes();
          unsigned long points[3] = {};
          ale::State faceState[3];
          su2double faceVel[3][3] = {}, faceRes[3][5];
          for (unsigned short a = 0; a < nFaceNodes; ++a) {
            points[a] = face->GetNode(a);
            faceState[a] = state[points[a]];
            for (int iDim = 0; iDim < nDim; ++iDim) faceVel[a][iDim] = geometry.nodes->GetGridVel(points[a])[iDim];
          }
          const su2double kappa = geometry.BoundaryFaceKappa(config, nFaceNodes, points, face->getNormal());
          ale::FaceResidual(nDim, nFaceNodes, kGamma, farField ? &inf : nullptr, face->getNormal(), faceState, faceVel,
                            kappa, faceRes);
          for (unsigned short a = 0; a < nFaceNodes; ++a) {
            /*--- Closure C rule of Weak_BC_Residual: k_b = k_a out, k_inf in (far field), k_a at walls. ---*/
            su2double mp = 0.0, in = 0.0;
            const su2double m = faceRes[a][0];
            const su2double kb = P1SSTMatched::FaceKine(m, k[points[a]], farField ? kInf : k[points[a]], mp, in);
            faceRes[a][nVar - 1] += kb * m;
            for (unsigned short v = 0; v < nVar; ++v) R[points[a]][v] += faceRes[a][v];
            Rk[points[a]] += k[points[a]] * mp + in;
          }
        }
      }
      /*--- Dual-time GCL terms at the common state: U G_i (U with k in the energy) and (rho k) G_i. ---*/
      for (auto iPoint = 0ul; iPoint < geometry.GetnPoint(); ++iPoint) {
        su2double U[5];
        ale::Conservative(nDim, kGamma, state[iPoint], U);
        U[nVar - 1] += state[iPoint].rho * k[iPoint];
        for (unsigned short v = 0; v < nVar; ++v) R[iPoint][v] += U[v] * G[iPoint];
        Rk[iPoint] += state[iPoint].rho * k[iPoint] * G[iPoint];
      }
    };

    for (int farField = 0; farField < 2; ++farField) {
      std::vector<ale::State> state(geometry.GetnPoint());
      const auto uniform = GlobalState(12345, nDim);
      std::vector<su2double> kJump(geometry.GetnPoint()), kUniform(geometry.GetnPoint(), kInf);
      for (auto iPoint = 0ul; iPoint < geometry.GetnPoint(); ++iPoint) {
        const auto gi = geometry.nodes->GetGlobalIndex(iPoint);
        state[iPoint] = farField ? uniform : GlobalState(gi, nDim);
        kJump[iPoint] = kInf * (1.0 + 0.9 * sin(1.3 * gi));
      }
      const std::vector<su2double> zero(geometry.GetnPoint(), 0.0);
      std::vector<State5> R, Rh, Ru;
      std::vector<su2double> Rk, Rkh, Rku;
      su2double pairSum = 0.0, dummy = 0.0;
      residual(state, kJump, farField, R, Rk, pairSum);
      residual(state, zero, farField, Rh, Rkh, dummy);
      residual(state, kUniform, farField, Ru, Rku, dummy);
      su2double worstHat = 0.0, worstFree = 0.0, worstUniform = 0.0, scale = 1e-30, coupled = 0.0, hatRate = 0.0, mag = 0.0;
      for (auto iPoint = 0ul; iPoint < geometry.GetnPointDomain(); ++iPoint) {
        /*--- The k changes only the energy row, by the k row: R_E - R_k = Rhat_E, the other rows equal. ---*/
        for (unsigned short v = 0; v + 1 < nVar; ++v) worstHat = std::max(worstHat, fabs(R[iPoint][v] - Rh[iPoint][v]));
        /*--- (The k = 0 run has k_b = k_inf on far-field inflow, so its own k row is taken out as well.) ---*/
        worstHat = std::max(worstHat, fabs(R[iPoint][nVar - 1] - Rk[iPoint] - (Rh[iPoint][nVar - 1] - Rkh[iPoint])));
        for (unsigned short v = 0; v < nVar; ++v) scale = std::max(scale, fabs(Rh[iPoint][v]) + fabs(R[iPoint][v]));
        scale = std::max(scale, fabs(Rk[iPoint]));
        /*--- Uniform k: R_k = k R_rho with every boundary and GCL term (far field: k = k_inf). ---*/
        worstUniform = std::max(worstUniform, fabs(Rku[iPoint] - kInf * Ru[iPoint][0]));
        /*--- Free stream: the coupled rows vanish (hat free stream, X = R_k). ---*/
        if (farField) {
          for (unsigned short v = 0; v + 1 < nVar; ++v) worstFree = std::max(worstFree, fabs(R[iPoint][v]));
          worstFree = std::max(worstFree, fabs(R[iPoint][nVar - 1] - Rk[iPoint]));
        }
        /*--- Entropy balance at walls (Theorem A10): -sum w.(R_rho, R_m, R_E - R_k) - sum rho G = 0 with the spatial
         * residuals (the GCL terms U G and rho k G taken out again). ---*/
        su2double w[5], Uh[5];
        ale::EntropyVariables(nDim, kGamma, state[iPoint], w);
        ale::Conservative(nDim, kGamma, state[iPoint], Uh);
        const su2double rk = state[iPoint].rho * kJump[iPoint];
        coupled -= state[iPoint].rho * G[iPoint];
        hatRate -= state[iPoint].rho * G[iPoint];
        for (unsigned short v = 0; v + 1 < nVar; ++v) {
          coupled -= w[v] * (R[iPoint][v] - Uh[v] * G[iPoint]);
          hatRate -= w[v] * (Rh[iPoint][v] - Uh[v] * G[iPoint]);
        }
        coupled -= w[nVar - 1] * ((R[iPoint][nVar - 1] - (Uh[nVar - 1] + rk) * G[iPoint]) - (Rk[iPoint] - rk * G[iPoint]));
        hatRate -= w[nVar - 1] * ((Rh[iPoint][nVar - 1] - Uh[nVar - 1] * G[iPoint]) - Rkh[iPoint]);
        for (unsigned short v = 0; v < nVar; ++v) mag += fabs(w[v] * Rh[iPoint][v]);
        mag += fabs(state[iPoint].rho * G[iPoint]);
      }
      INFO("nDim " << nDim << (farField ? " far field" : " walls"));
      CHECK(GlobalMax(worstHat) < 1e-11 * GlobalMax(scale));
      CHECK(GlobalMax(worstUniform) < 1e-11 * GlobalMax(scale));
      CHECK(fabs(GlobalSum(pairSum)) < 1e-11 * GlobalMax(scale));
      if (farField) {
        CHECK(GlobalMax(worstFree) < 1e-10 * GlobalMax(scale));
      } else {
        CHECK(fabs(GlobalSum(coupled) - GlobalSum(hatRate)) < 1e-11 * GlobalSum(mag));
        CHECK(fabs(GlobalSum(hatRate)) < 1e-8 * GlobalSum(mag));
      }
    }

    /*--- ALE edge Jacobian: FD of the matched AleFlux with k_up frozen = L(k_up) Jhat T(k) (D1 with the grid term). ---*/
    std::mt19937 gen(4711 + nDim);
    std::uniform_real_distribution<double> u(-1.0, 1.0);
    for (int trial = 0; trial < 20; ++trial) {
      su2double U[2][5], k[2], n[3] = {u(gen), u(gen), u(gen)};
      for (int s = 0; s < 2; ++s) {
        const su2double rho = exp(0.3 * u(gen)), p = exp(0.3 * u(gen));
        su2double vel2 = 0.0;
        k[s] = 0.1 * exp(u(gen));
        U[s][0] = rho;
        for (int d = 0; d < nDim; ++d) { const su2double v = 0.5 * u(gen); U[s][d + 1] = rho * v; vel2 += v * v; }
        U[s][nDim + 1] = p / (kGamma - 1.0) + 0.5 * rho * vel2 + rho * k[s];
      }
      const su2double g = 0.7 * u(gen);
      auto hatFlux = [&](const su2double (*H)[5], su2double* F) {
        ale::AleFlux(nDim, kGamma, StateFromConservative(nDim, H[0]), StateFromConservative(nDim, H[1]), n, g, F);
      };
      su2double H[2][5];
      for (int s = 0; s < 2; ++s) {
        for (int v = 0; v < nVar; ++v) H[s][v] = U[s][v];
        H[s][nVar - 1] -= U[s][0] * k[s];
      }
      su2double F0[5];
      hatFlux(H, F0);
      if (fabs(F0[0]) < 1e-3) continue;
      const su2double kUp = P1SSTMatched::KUpwind(F0[0], k[0], k[1]);
      for (int side = 0; side < 2; ++side) {
        su2activematrix J(nVar, nVar), Jfd(nVar, nVar);
        const su2double h = 1e-6;
        for (int jv = 0; jv < nVar; ++jv) {
          su2double Hp[2][5], Hm[2][5], Fp[5], Fm[5];
          for (int s = 0; s < 2; ++s) for (int v = 0; v < nVar; ++v) Hp[s][v] = Hm[s][v] = H[s][v];
          Hp[side][jv] += h; Hm[side][jv] -= h;
          hatFlux(Hp, Fp);
          hatFlux(Hm, Fm);
          for (int iv = 0; iv < nVar; ++iv) J(iv, jv) = (Fp[iv] - Fm[iv]) / (2 * h);
          /*--- The matched flux w.r.t. U of this side, k frozen: hat state T(k) U, plus kUp times the mass row. ---*/
          for (int s = 0; s < 2; ++s) for (int v = 0; v < nVar; ++v) Hp[s][v] = Hm[s][v] = U[s][v];
          Hp[side][jv] += h; Hm[side][jv] -= h;
          for (int s = 0; s < 2; ++s) { Hp[s][nVar - 1] -= Hp[s][0] * k[s]; Hm[s][nVar - 1] -= Hm[s][0] * k[s]; }
          hatFlux(Hp, Fp);
          hatFlux(Hm, Fm);
          Fp[nVar - 1] += kUp * Fp[0];
          Fm[nVar - 1] += kUp * Fm[0];
          for (int iv = 0; iv < nVar; ++iv) Jfd(iv, jv) = (Fp[iv] - Fm[iv]) / (2 * h);
        }
        P1SSTMatched::TransformJacobian(nVar, J, kUp, k[side]);
        for (int iv = 0; iv < nVar; ++iv)
          for (int jv = 0; jv < nVar; ++jv) CHECK(J(iv, jv) == Approx(Jfd(iv, jv)).margin(1e-6 * (1.0 + fabs(Jfd(iv, jv)))));
      }
    }
  }
}

/*--- SST matched k with AXISYMMETRIC (SSTPATHS_PLAN.md 5): the inviscid source of CSourceAxisymmetric_Flow (v/r form
 * and the axis form rho dv/dr, blended by alpha), with k in the total energy, against its hat form. ---*/
namespace {
void AxiSource(int nDim, const su2double* U, su2double k, su2double y, su2double dvdr, su2double alpha, su2double V,
               su2double* S) {
  su2double m2 = 0.0;
  for (int d = 0; d < nDim; ++d) m2 += U[d + 1] * U[d + 1];
  const su2double p = (kGamma - 1.0) * (U[nDim + 1] - 0.5 * m2 / U[0] - U[0] * k);
  const su2double H = (U[nDim + 1] + p) / U[0];
  const su2double yinv = 1.0 / y, rho = U[0], u = U[1] / U[0];
  const su2double stdS[4] = {yinv * V * U[2], yinv * V * U[1] * U[2] / U[0], yinv * V * U[2] * U[2] / U[0], yinv * V * H * U[2]};
  const su2double gradS[4] = {V * rho * dvdr, V * rho * u * dvdr, 0.0, V * rho * H * dvdr};
  for (int v = 0; v < 4; ++v) S[v] = (1.0 - alpha) * gradS[v] + alpha * stdS[v];
}
}  // namespace

TEST_CASE("P1 SST matched AXISYMMETRIC: S_E = Shat_E + k S_rho in both forms, uniform k, entropy identity, Jacobian", "[P1Flux]") {
  std::mt19937 gen(8642);
  std::uniform_real_distribution<double> u(-1.0, 1.0);
  const int nDim = 2, nVar = 4;
  for (int trial = 0; trial < 200; ++trial) {
    const su2double alpha = (trial % 3 == 0) ? 1.0 : (trial % 3 == 1) ? 0.0 : 0.5 * (1.0 + u(gen));
    const su2double y = 0.2 + exp(u(gen)), dvdr = u(gen), V = 0.3 * exp(u(gen));
    const su2double rho = exp(0.4 * u(gen)), p = exp(0.4 * u(gen)), k = 0.2 * exp(u(gen));
    const su2double vel[2] = {u(gen), u(gen)};
    su2double U[4], Uhat[4];
    U[0] = rho;
    U[1] = rho * vel[0];
    U[2] = rho * vel[1];
    U[3] = p / (kGamma - 1.0) + 0.5 * rho * (vel[0] * vel[0] + vel[1] * vel[1]) + rho * k;
    for (int v = 0; v < nVar; ++v) Uhat[v] = U[v];
    Uhat[3] -= rho * k;
    su2double S[4], Shat[4];
    AxiSource(nDim, U, k, y, dvdr, alpha, V, S);
    AxiSource(nDim, Uhat, 0.0, y, dvdr, alpha, V, Shat);

    /*--- Free stream with a k jump: (rho, u, p) fixed, the k changes only S_E, by k S_rho (the matched k row). ---*/
    for (int v = 0; v < nVar - 1; ++v) CHECK(S[v] == Approx(Shat[v]).margin(1e-14 * (1.0 + fabs(Shat[v]))));
    CHECK(S[3] - k * S[0] == Approx(Shat[3]).margin(1e-13 * (1.0 + fabs(Shat[3]))));

    /*--- Entropy identity: v.(S, k S_rho) = w.Shat; in the v/r form w.Shat = (V v / y) eta (no production). ---*/
    State5 Us;
    for (int v = 0; v < nVar; ++v) Us[v] = U[v];
    su2double w[5];
    HatEntropyVariables(nDim, Us, k, w);
    su2double coupled = 0.0, hat = 0.0;
    for (int v = 0; v < nVar - 1; ++v) { coupled += w[v] * S[v]; hat += w[v] * Shat[v]; }
    coupled += w[3] * (S[3] - k * S[0]);
    hat += w[3] * Shat[3];
    CHECK(coupled == Approx(hat).margin(1e-13 * (1.0 + fabs(hat))));
    if (alpha == 1.0) {
      const su2double eta = -rho * (log(p) - kGamma * log(rho)) / (kGamma - 1.0);
      CHECK(hat == Approx(V * vel[1] * eta / y).margin(1e-12 * (1.0 + fabs(hat))));
    }

    /*--- Jacobians with k frozen: dS/dU = L(k) dShat/dUhat T(k) (D1 with k_up = k_i), and the k-row diagonal
     * d(k S_rho)/d(rho k) = S_rho/rho at a frozen flow. ---*/
    su2activematrix J(nVar, nVar), Jfd(nVar, nVar);
    const su2double h = 1e-6;
    for (int jv = 0; jv < nVar; ++jv) {
      su2double Up[4], Um[4], Sp[4], Sm[4];
      for (int v = 0; v < nVar; ++v) { Up[v] = Uhat[v]; Um[v] = Uhat[v]; }
      Up[jv] += h; Um[jv] -= h;
      AxiSource(nDim, Up, 0.0, y, dvdr, alpha, V, Sp);
      AxiSource(nDim, Um, 0.0, y, dvdr, alpha, V, Sm);
      for (int iv = 0; iv < nVar; ++iv) J(iv, jv) = (Sp[iv] - Sm[iv]) / (2 * h);
      for (int v = 0; v < nVar; ++v) { Up[v] = U[v]; Um[v] = U[v]; }
      Up[jv] += h; Um[jv] -= h;
      AxiSource(nDim, Up, k, y, dvdr, alpha, V, Sp);
      AxiSource(nDim, Um, k, y, dvdr, alpha, V, Sm);
      for (int iv = 0; iv < nVar; ++iv) Jfd(iv, jv) = (Sp[iv] - Sm[iv]) / (2 * h);
    }
    P1SSTMatched::TransformJacobian(nVar, J, k, k);
    for (int iv = 0; iv < nVar; ++iv)
      for (int jv = 0; jv < nVar; ++jv) CHECK(J(iv, jv) == Approx(Jfd(iv, jv)).margin(1e-6 * (1.0 + fabs(Jfd(iv, jv)))));
    const su2double rk = rho * k;
    const su2double fd = ((rk + h) / rho * S[0] - (rk - h) / rho * S[0]) / (2 * h);
    CHECK(fd == Approx(S[0] / rho).margin(1e-8));
  }
}
