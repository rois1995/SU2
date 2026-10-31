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
