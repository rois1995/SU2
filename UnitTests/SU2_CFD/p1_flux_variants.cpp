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
#include "../../SU2_CFD/include/numerics/flow/p1_viscous_entropy.hpp"
#include "../../SU2_CFD/include/numerics/flow/p1_ale_entropy.hpp"
#include "../../SU2_CFD/include/numerics_simd/flow/convection/common.hpp"
#include "../../SU2_CFD/include/numerics_simd/flow/variables.hpp"

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
