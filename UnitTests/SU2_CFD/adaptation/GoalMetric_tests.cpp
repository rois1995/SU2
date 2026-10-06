/*!
 * \file GoalMetric_tests.cpp
 * \brief Goal-oriented metric (ADAP_SENSOR= GOAL, stage G1b): Euler fluxes and Jacobians, point estimate, mirror rules of
 *        the vector/tensor fields at symmetry planes, H_go on meshes (manufactured fields, mirrored meshes, slip-wall
 *        reconstruction of the normal momentum lambda), the symmetry support check, and (with any number of ranks,
 *        e.g. mpirun -n 2 test_driver "[GoalMPI]") the partition independence.
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

#include "catch.hpp"

#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <random>

#include "TransferTestCase.hpp"
#include "../../../Common/include/linear_algebra/blas_structure.hpp"
#include "../../../Common/include/parallelization/CPassiveComm.hpp"
#include "../../../SU2_CFD/include/adaptation/CGoalMetric.hpp"

using namespace transfer_test;

namespace {

constexpr passivedouble GAMMA = 1.4;

using FieldFunction = std::function<void(const passivedouble* x, su2double* values)>;

/*--- Max over all ranks. ---*/
passivedouble MaxAll(passivedouble value) { return CPassiveComm::Allreduce(value, CPassiveComm::Op::MAX); }

/*--- Config of a flow solver with a feature metric (its storage is resized to the GOAL fields by the tests). ---*/
std::unique_ptr<CConfig> GoalConfig(const string& markers, const string& extra = "") {
  stringstream options("SOLVER= EULER\nMATH_PROBLEM= DIRECT\nMACH_NUMBER= 0.5\nMESH_FORMAT= SU2\n"
                       "MESH_FILENAME= unused.su2\n" + markers + "MGLEVEL= 0\nCOMPUTE_METRIC= YES\n"
                       "ADAP_SENSOR= (MACH)\nADAP_HMIN= 1e-6\nADAP_HMAX= 1e3\nADAP_ARMAX= 1e8\n"
                       "ADAP_COMPLEXITY= 400\n" + extra);
  Mute mute;
  return std::unique_ptr<CConfig>(new CConfig(options, SU2_COMPONENT::SU2_CFD, false));
}

/*--- GOAL work columns on a flow solver built from a feature config. ---*/
void PrepareGoal(CSolver* flow, const CGeometry& geometry) {
  const auto nField = GoalMetric::FieldCount(geometry.GetnDim());
  auto* nodes = flow->GetNodes();
  nodes->GetAuxVar_Adapt().resize(geometry.GetnPoint(), nField) = su2double(0.0);
  nodes->GetGradient_Adapt().resize(geometry.GetnPoint(), nField, geometry.GetnDim(), 0.0);
}

/*--- Set all fields of the domain points (halos NaN: the computation must communicate them). ---*/
void SetFields(CSolver* flow, const CGeometry& geometry, const FieldFunction& f) {
  const auto nDim = geometry.GetnDim();
  const auto nField = GoalMetric::FieldCount(nDim);
  auto& field = flow->GetNodes()->GetAuxVar_Adapt();
  su2double values[GoalMetric::MAXFIELD];
  for (auto iPoint = 0ul; iPoint < geometry.GetnPoint(); ++iPoint) {
    passivedouble x[3] = {0.0, 0.0, 0.0};
    for (auto iDim = 0u; iDim < nDim; ++iDim) x[iDim] = SU2_TYPE::GetValue(geometry.nodes->GetCoord(iPoint, iDim));
    f(x, values);
    for (auto k = 0u; k < nField; ++k)
      field(iPoint, k) = (iPoint < geometry.GetnPointDomain()) ? values[k] : su2double(std::nan(""));
  }
}

/*--- Fields from lambda(x) and a conservative state U(x) (fluxes F(U)). ---*/
FieldFunction FromState(unsigned short nDim, std::function<void(const passivedouble*, su2double*)> lambda,
                        std::function<void(const passivedouble*, su2double*)> state) {
  return [=](const passivedouble* x, su2double* values) {
    su2double U[GoalMetric::MAXVAR];
    lambda(x, values);
    state(x, U);
    GoalMetric::FluxFields(nDim, GAMMA, U, values + GoalMetric::NumVar(nDim));
  };
}

/*--- A smooth admissible state and a smooth lambda (no symmetry). ---*/
void SmoothState(unsigned short nDim, const passivedouble* x, su2double* U) {
  const passivedouble rho = 1.0 + 0.2 * sin(1.1 * x[0] + 0.7 * x[1] + 0.3 * x[2]);
  const passivedouble v[3] = {0.5 + 0.2 * cos(x[1] - 0.4 * x[2]), 0.1 * sin(2.0 * x[0]), 0.15 * cos(x[0] + x[1])};
  const passivedouble p = (1.0 + 0.1 * cos(0.9 * x[0] - 1.3 * x[1] + 0.5 * x[2])) / GAMMA;
  passivedouble v2 = 0.0;
  U[0] = rho;
  for (auto iDim = 0u; iDim < nDim; ++iDim) {
    U[1 + iDim] = rho * v[iDim];
    v2 += v[iDim] * v[iDim];
  }
  U[nDim + 1] = p / (GAMMA - 1.0) + 0.5 * rho * v2;
}
void SmoothLambda(unsigned short nDim, const passivedouble* x, su2double* lambda) {
  for (auto j = 0u; j < GoalMetric::NumVar(nDim); ++j)
    lambda[j] = sin(0.8 * x[0] + 0.3 * j) * cos(0.6 * x[1] - 0.2 * j) + 0.1 * j * x[2];
}

/*--- Rectangle [0,2]x[0,1] or unit cube (uniform simplices) with a marker function. ---*/
CSimplexMesh Mesh(unsigned short nDim, unsigned long n, const simplex_test::MarkerFunction& markers) {
  return simplex_test::MakeSimplexMesh(nDim, n, markers);
}

/*--- Symmetric part of the 3x3 product helpers. ---*/
using Mat3 = std::array<std::array<passivedouble, 3>, 3>;

/*--- |H| of a symmetric matrix. ---*/
Mat3 AbsMatrix(unsigned short nDim, const Mat3& H) {
  su2double A[3][3] = {{0.0}}, vec[3][3], val[3], work[3], R[3][3];
  for (auto a = 0u; a < nDim; ++a)
    for (auto b = 0u; b < nDim; ++b) A[a][b] = H[a][b];
  CBlasStructure::EigenDecomposition(A, vec, val, nDim, work);
  for (auto a = 0u; a < nDim; ++a) val[a] = fabs(val[a]);
  CBlasStructure::EigenRecomposition(R, vec, val, nDim);
  Mat3 out{};
  for (auto a = 0u; a < nDim; ++a)
    for (auto b = 0u; b < nDim; ++b) out[a][b] = SU2_TYPE::GetValue(R[a][b]);
  return out;
}

}  // namespace

/*--------------------------------------------------------------------------------------------------------------------*/
/*--- U1: fluxes and Jacobians. ---*/

TEST_CASE("Goal fluxes and Jacobians", "[GoalMetric]") {
  std::mt19937 gen(7);
  std::uniform_real_distribution<passivedouble> uni(-1.0, 1.0);
  for (unsigned short nDim : {2, 3}) {
    const auto nVar = GoalMetric::NumVar(nDim);
    for (int sample = 0; sample < 20; ++sample) {
      su2double U[5];
      const passivedouble rho = 1.0 + 0.5 * uni(gen), p = 1.0 + 0.5 * uni(gen);
      passivedouble v[3] = {uni(gen), uni(gen), uni(gen)}, v2 = 0.0;
      U[0] = rho;
      for (auto i = 0u; i < nDim; ++i) {
        U[1 + i] = rho * v[i];
        v2 += v[i] * v[i];
      }
      U[nDim + 1] = p / (GAMMA - 1.0) + 0.5 * rho * v2;

      /*--- Independent formula from the primitive variables. ---*/
      su2double F[15];
      GoalMetric::FluxFields(nDim, GAMMA, U, F);
      const passivedouble H = (SU2_TYPE::GetValue(U[nDim + 1]) + p) / rho;
      for (auto d = 0u; d < nDim; ++d) {
        CHECK(SU2_TYPE::GetValue(F[d * nVar]) == Approx(rho * v[d]).epsilon(1e-14));
        for (auto i = 0u; i < nDim; ++i)
          CHECK(SU2_TYPE::GetValue(F[d * nVar + 1 + i]) == Approx(rho * v[d] * v[i] + (i == d ? p : 0.0)).margin(1e-14));
        CHECK(SU2_TYPE::GetValue(F[d * nVar + nDim + 1]) == Approx(rho * H * v[d]).margin(1e-14));
      }

      /*--- Jacobians against central differences. ---*/
      su2double A[3][5][5];
      GoalMetric::FluxJacobians(nDim, GAMMA, U, A);
      passivedouble maxErr = 0.0, scale = 0.0;
      for (auto j = 0u; j < nVar; ++j) {
        const passivedouble h = 1e-6 * std::max(1.0, fabs(SU2_TYPE::GetValue(U[j])));
        su2double Up[5], Um[5], Fp[15], Fm[15];
        for (auto k = 0u; k < nVar; ++k) Up[k] = Um[k] = U[k];
        Up[j] += h;
        Um[j] -= h;
        GoalMetric::FluxFields(nDim, GAMMA, Up, Fp);
        GoalMetric::FluxFields(nDim, GAMMA, Um, Fm);
        for (auto d = 0u; d < nDim; ++d)
          for (auto i = 0u; i < nVar; ++i) {
            const passivedouble fd = SU2_TYPE::GetValue(Fp[d * nVar + i] - Fm[d * nVar + i]) / (2 * h);
            maxErr = std::max(maxErr, fabs(fd - SU2_TYPE::GetValue(A[d][i][j])));
            scale = std::max(scale, fabs(SU2_TYPE::GetValue(A[d][i][j])));
          }
      }
      CHECK(maxErr <= 1e-7 * scale);

      /*--- Mirror invariance F(T U) = T F(U) S for a random unit normal. ---*/
      passivedouble n[3] = {uni(gen), uni(gen), nDim == 3 ? uni(gen) : 0.0}, norm = 0.0;
      for (auto i = 0u; i < nDim; ++i) norm += n[i] * n[i];
      for (auto i = 0u; i < nDim; ++i) n[i] /= sqrt(norm);
      auto mirror = [&](const su2double* vec, su2double* out) {
        passivedouble dot = 0.0;
        for (auto i = 0u; i < nDim; ++i) dot += SU2_TYPE::GetValue(vec[i]) * n[i];
        for (auto i = 0u; i < nDim; ++i) out[i] = vec[i] - 2.0 * dot * n[i];
      };
      su2double TU[5], FT[15];
      TU[0] = U[0];
      TU[nDim + 1] = U[nDim + 1];
      mirror(U + 1, TU + 1);
      GoalMetric::FluxFields(nDim, GAMMA, TU, FT);
      /*--- T Phi S: column d of the result = sum_e Phi_e S_ed, then T on the rows. ---*/
      for (auto d = 0u; d < nDim; ++d) {
        su2double col[5] = {0.0}, Tcol[5];
        for (auto e = 0u; e < nDim; ++e) {
          const passivedouble S = (d == e ? 1.0 : 0.0) - 2.0 * n[d] * n[e];
          for (auto j = 0u; j < nVar; ++j) col[j] += F[e * nVar + j] * S;
        }
        Tcol[0] = col[0];
        Tcol[nDim + 1] = col[nDim + 1];
        mirror(col + 1, Tcol + 1);
        for (auto j = 0u; j < nVar; ++j)
          CHECK(SU2_TYPE::GetValue(FT[d * nVar + j]) == Approx(SU2_TYPE::GetValue(Tcol[j])).margin(1e-13));
      }
    }
  }
}

TEST_CASE("Goal eq. (33) weights and point estimate", "[GoalMetric]") {
  /*--- G_j = sum_d sum_k A_d,kj d_d lambda_k on a hand case: only d_x lambda_rho = 1 -> G_j = A_x,0j (= 0,1,0,0). ---*/
  const unsigned short nDim = 2;
  su2double U[4] = {1.2, 0.3, -0.1, 2.5}, A[3][5][5];
  GoalMetric::FluxJacobians(nDim, GAMMA, U, A);
  su2double gradLambda[10] = {0.0}, num = 0.0, den = 0.0;
  gradLambda[0 * nDim + 0] = 1.0;
  GoalMetric::Eq33Terms(nDim, gradLambda, A, num, den);
  CHECK(SU2_TYPE::GetValue(num) == Approx(1.0));
  CHECK(SU2_TYPE::GetValue(den) == Approx(1.0));
  /*--- Only d_y lambda_E = 1 -> G_j = A_y,3j. ---*/
  gradLambda[0] = 0.0;
  gradLambda[3 * nDim + 1] = 1.0;
  GoalMetric::Eq33Terms(nDim, gradLambda, A, num, den);
  passivedouble expected = 0.0;
  for (auto j = 0u; j < 4; ++j) expected += fabs(SU2_TYPE::GetValue(A[1][3][j]));
  CHECK(SU2_TYPE::GetValue(num) == Approx(expected));
  /*--- Zero gradient: both 0. ---*/
  gradLambda[3 * nDim + 1] = 0.0;
  GoalMetric::Eq33Terms(nDim, gradLambda, A, num, den);
  CHECK(num == 0.0);
  CHECK(den == 0.0);

  /*--- |H| of an indefinite matrix and cancellation: weights +1 and -1 on the same H -> signed sum 0. ---*/
  const su2double H[4] = {1.0, 2.0, 2.0, -3.0};
  su2double Hgo[3][3] = {{0.0}}, S[3][3] = {{0.0}};
  CHECK(GoalMetric::AccumulateField(nDim, 1.0, H, Hgo, S));
  CHECK(GoalMetric::AccumulateField(nDim, -1.0, H, Hgo, S));
  Mat3 Hm{};
  Hm[0][0] = 1.0; Hm[0][1] = Hm[1][0] = 2.0; Hm[1][1] = -3.0;
  const auto absH = AbsMatrix(nDim, Hm);
  for (auto a = 0u; a < nDim; ++a)
    for (auto b = 0u; b < nDim; ++b) {
      CHECK(SU2_TYPE::GetValue(Hgo[a][b]) == Approx(2.0 * absH[a][b]).margin(1e-14));
      CHECK(fabs(SU2_TYPE::GetValue(S[a][b])) < 1e-15);
    }
  CHECK(SU2_TYPE::GetValue(GoalMetric::NuclearNorm(nDim, S)) < 1e-14);
  /*--- |H| = (eigenvalues 1 +- sqrt(1+... )): trace |H| = sum |eig| = sqrt((1+3)^2 + 16) = sqrt(32). ---*/
  CHECK(absH[0][0] + absH[1][1] == Approx(sqrt(32.0)));
  /*--- A non-finite Hessian is rejected and adds nothing. ---*/
  const su2double bad[4] = {1.0, std::nan(""), std::nan(""), 1.0};
  CHECK_FALSE(GoalMetric::AccumulateField(nDim, 1.0, bad, Hgo, S));
  CHECK(SU2_TYPE::GetValue(Hgo[0][0]) == Approx(2.0 * absH[0][0]).margin(1e-14));
}

/*--------------------------------------------------------------------------------------------------------------------*/
/*--- U5: mirror rules at a point. ---*/

TEST_CASE("Goal mirror rules at a point", "[GoalMetric]") {
  std::mt19937 gen(11);
  std::uniform_real_distribution<passivedouble> uni(-1.0, 1.0);
  for (unsigned short nDim : {2, 3}) {
    const auto nVar = GoalMetric::NumVar(nDim), nFlux = GoalMetric::NumFlux(nDim), nField = GoalMetric::FieldCount(nDim);
    for (int sample = 0; sample < 10; ++sample) {
      su2double n[3] = {uni(gen), uni(gen), nDim == 3 ? uni(gen) : 0.0};
      passivedouble norm = 0.0;
      for (auto i = 0u; i < nDim; ++i) norm += SU2_TYPE::GetValue(n[i] * n[i]);
      for (auto i = 0u; i < nDim; ++i) n[i] /= sqrt(norm);

      su2double values[20], grad[60], hess[15 * 9];
      for (auto& v : values) v = uni(gen);
      for (auto& v : grad) v = uni(gen);
      for (auto f = 0u; f < nFlux; ++f)
        for (auto a = 0u; a < nDim; ++a)
          for (auto b = a; b < nDim; ++b)
            hess[(f * nDim + a) * nDim + b] = hess[(f * nDim + b) * nDim + a] = uni(gen);

      GoalMetric::MirrorValues(nDim, n, values);
      GoalMetric::MirrorGradients(nDim, n, grad);
      GoalMetric::MirrorHessians(nDim, n, hess);

      /*--- Parity: the values have no odd part; for each derivative component, the even part has no normal
       *    gradient and the odd part no tangential gradient; Hessians: even no mixed block, odd only it. ---*/
      su2double odd[20];
      GoalMetric::OddPart(nDim, n, values, values + nVar, odd, odd + nVar);
      for (auto k = 0u; k < nField; ++k) CHECK(fabs(SU2_TYPE::GetValue(odd[k])) < 1e-14);

      su2double comp[3][20], compOdd[3][20];
      for (auto a = 0u; a < nDim; ++a) {
        for (auto k = 0u; k < nField; ++k) comp[a][k] = grad[k * nDim + a];
        GoalMetric::OddPart(nDim, n, comp[a], comp[a] + nVar, compOdd[a], compOdd[a] + nVar);
      }
      for (auto k = 0u; k < nField; ++k) {
        su2double evenN = 0.0, oddT[3] = {0.0}, oddN = 0.0;
        for (auto a = 0u; a < nDim; ++a) {
          evenN += (comp[a][k] - compOdd[a][k]) * n[a];
          oddN += compOdd[a][k] * n[a];
        }
        for (auto a = 0u; a < nDim; ++a) oddT[a] = compOdd[a][k] - oddN * n[a];
        CHECK(fabs(SU2_TYPE::GetValue(evenN)) < 1e-14);
        for (auto a = 0u; a < nDim; ++a) CHECK(fabs(SU2_TYPE::GetValue(oddT[a])) < 1e-14);
      }

      /*--- Idempotence. ---*/
      su2double values2[20], grad2[60], hess2[15 * 9];
      std::copy(values, values + nField, values2);
      std::copy(grad, grad + nField * nDim, grad2);
      std::copy(hess, hess + nFlux * nDim * nDim, hess2);
      GoalMetric::MirrorValues(nDim, n, values2);
      GoalMetric::MirrorGradients(nDim, n, grad2);
      GoalMetric::MirrorHessians(nDim, n, hess2);
      for (auto k = 0u; k < nField; ++k) CHECK(SU2_TYPE::GetValue(values2[k] - values[k]) == Approx(0.0).margin(1e-14));
      for (auto k = 0u; k < nField * nDim; ++k) CHECK(SU2_TYPE::GetValue(grad2[k] - grad[k]) == Approx(0.0).margin(1e-14));
      for (auto k = 0u; k < nFlux * nDim * nDim; ++k)
        CHECK(SU2_TYPE::GetValue(hess2[k] - hess[k]) == Approx(0.0).margin(1e-14));
    }

    /*--- Rotation equivariance: rotating the input (normal, field components, derivative directions) rotates the
     *    output. ---*/
    for (int sample = 0; sample < 5; ++sample) {
      /*--- Random rotation by Gram-Schmidt. ---*/
      passivedouble R[3][3] = {{0.0}};
      for (auto i = 0u; i < nDim; ++i) {
        for (auto j = 0u; j < nDim; ++j) R[i][j] = uni(gen);
        for (auto k = 0u; k < i; ++k) {
          passivedouble dot = 0.0;
          for (auto j = 0u; j < nDim; ++j) dot += R[i][j] * R[k][j];
          for (auto j = 0u; j < nDim; ++j) R[i][j] -= dot * R[k][j];
        }
        passivedouble norm = 0.0;
        for (auto j = 0u; j < nDim; ++j) norm += R[i][j] * R[i][j];
        for (auto j = 0u; j < nDim; ++j) R[i][j] /= sqrt(norm);
      }
      auto rotVec = [&](const su2double* v, su2double* out, size_t stride = 1) {
        su2double tmp[3] = {0.0};
        for (auto i = 0u; i < nDim; ++i)
          for (auto j = 0u; j < nDim; ++j) tmp[i] += R[i][j] * v[j * stride];
        for (auto i = 0u; i < nDim; ++i) out[i * stride] = tmp[i];
      };
      /*--- Field components: lambda_m -> R lambda_m; Phi -> T_R Phi R^T (columns d mix, momentum rows rotate). ---*/
      auto rotFields = [&](const su2double* in, su2double* out, bool withLambda) {
        const auto offset = withLambda ? nVar : 0;
        if (withLambda) {
          for (auto j = 0u; j < nVar; ++j) out[j] = in[j];
          rotVec(in + 1, out + 1);
        }
        su2double phi[15];
        for (auto d = 0u; d < nDim; ++d)
          for (auto j = 0u; j < nVar; ++j) {
            phi[d * nVar + j] = 0.0;
            for (auto e = 0u; e < nDim; ++e) phi[d * nVar + j] += R[d][e] * in[offset + e * nVar + j];
          }
        for (auto d = 0u; d < nDim; ++d) {
          out[offset + d * nVar] = phi[d * nVar];
          out[offset + d * nVar + nDim + 1] = phi[d * nVar + nDim + 1];
          rotVec(phi + d * nVar + 1, out + offset + d * nVar + 1);
        }
      };

      su2double n[3] = {uni(gen), uni(gen), nDim == 3 ? uni(gen) : 0.0}, nR[3];
      passivedouble norm = 0.0;
      for (auto i = 0u; i < nDim; ++i) norm += SU2_TYPE::GetValue(n[i] * n[i]);
      for (auto i = 0u; i < nDim; ++i) n[i] /= sqrt(norm);
      rotVec(n, nR);

      su2double grad[60], gradR[60], hess[135], hessR[135];
      for (auto& v : grad) v = uni(gen);
      for (auto f = 0u; f < nFlux; ++f)
        for (auto a = 0u; a < nDim; ++a)
          for (auto b = a; b < nDim; ++b)
            hess[(f * nDim + a) * nDim + b] = hess[(f * nDim + b) * nDim + a] = uni(gen);

      /*--- Rotated input: fields rotated per derivative component, derivatives rotated per field. ---*/
      auto rotGrad = [&](const su2double* in, su2double* out) {
        su2double tmp[60];
        for (auto a = 0u; a < nDim; ++a) {
          su2double c[20], cr[20];
          for (auto k = 0u; k < nField; ++k) c[k] = in[k * nDim + a];
          rotFields(c, cr, true);
          for (auto k = 0u; k < nField; ++k) tmp[k * nDim + a] = cr[k];
        }
        for (auto k = 0u; k < nField; ++k) rotVec(tmp + k * nDim, out + k * nDim);
      };
      auto rotHess = [&](const su2double* in, su2double* out) {
        su2double tmp[135];
        for (auto a = 0u; a < nDim; ++a)
          for (auto b = 0u; b < nDim; ++b) {
            su2double c[15], cr[15];
            for (auto f = 0u; f < nFlux; ++f) c[f] = in[(f * nDim + a) * nDim + b];
            rotFields(c, cr, false);
            for (auto f = 0u; f < nFlux; ++f) tmp[(f * nDim + a) * nDim + b] = cr[f];
          }
        for (auto f = 0u; f < nFlux; ++f) {
          for (auto a = 0u; a < nDim; ++a)
            for (auto b = 0u; b < nDim; ++b) {
              su2double value = 0.0;
              for (auto c = 0u; c < nDim; ++c)
                for (auto e = 0u; e < nDim; ++e) value += R[a][c] * tmp[(f * nDim + c) * nDim + e] * R[b][e];
              out[(f * nDim + a) * nDim + b] = value;
            }
        }
      };
      rotGrad(grad, gradR);
      rotHess(hess, hessR);
      GoalMetric::MirrorGradients(nDim, n, grad);
      GoalMetric::MirrorHessians(nDim, n, hess);
      GoalMetric::MirrorGradients(nDim, nR, gradR);
      GoalMetric::MirrorHessians(nDim, nR, hessR);
      su2double expectG[60], expectH[135];
      rotGrad(grad, expectG);
      rotHess(hess, expectH);
      for (auto k = 0u; k < nField * nDim; ++k) CHECK(SU2_TYPE::GetValue(gradR[k] - expectG[k]) == Approx(0.0).margin(1e-13));
      for (auto k = 0u; k < nFlux * nDim * nDim; ++k)
        CHECK(SU2_TYPE::GetValue(hessR[k] - expectH[k]) == Approx(0.0).margin(1e-13));
    }
  }
}

/*--------------------------------------------------------------------------------------------------------------------*/
/*--- U2, U3, U4: H_go on meshes. ---*/

namespace {

/*--- Rectangle markers: far field everywhere (names of SimplexMeshTestCase). ---*/
const string FAR2D = "MARKER_FAR= (left, right, upper, lower_a, lower_b)\n";
const string FAR3D = "MARKER_FAR= (x_minus, x_plus, y_minus, y_plus, z_plus, z_minus_a, z_minus_b)\n";

struct GoalMesh {
  std::unique_ptr<CConfig> config;
  std::unique_ptr<MeshSolution> mesh;
  CSolver* flow = nullptr;
  GoalMesh(unsigned short nDim, unsigned long n, const string& markers, const string& extra = "",
           const simplex_test::MarkerFunction& markerOf = nullptr,
           const std::function<void(passivedouble*)>& map = nullptr) {
    config = GoalConfig(markers, extra);
    auto simplex = Mesh(nDim, n, markerOf ? markerOf : (nDim == 2 ? simplex_test::Marker2D : simplex_test::Marker3D));
    if (map)
      for (auto iPoint = 0ul; iPoint < simplex.GetnPoint(); ++iPoint) map(&simplex.coord[iPoint * nDim]);
    mesh = std::unique_ptr<MeshSolution>(new MeshSolution(config.get(), simplex, 0));
    flow = mesh->solver[MESH_0][FLOW_SOL];
    PrepareGoal(flow, mesh->Fine());
  }
  CGeometry& Geo() const { return mesh->Fine(); }
  void Compute(vector<su2double>* fluxHessians = nullptr) {
    std::unique_ptr<Mute> mute(getenv("GOAL_TEST_VERBOSE") ? nullptr : new Mute);
    flow->ComputeGoalHessian(&Geo(), config.get(), nullptr, fluxHessians);
  }
};

/*--- Linear lambda and quadratic flux fields (any coefficients), and the exact H_go. ---*/
struct Manufactured {
  unsigned short nDim;
  passivedouble a[5][3], c[5], Q[15][3][3], b[15][3];
  explicit Manufactured(unsigned short dim, int rankDeficient = 0) : nDim(dim) {
    std::mt19937 gen(3);
    std::uniform_real_distribution<passivedouble> uni(-1.0, 1.0);
    for (auto j = 0u; j < 5; ++j) {
      c[j] = uni(gen);
      for (auto d = 0u; d < 3; ++d) a[j][d] = uni(gen);
    }
    for (auto f = 0u; f < 15; ++f)
      for (auto d = 0u; d < 3; ++d) {
        b[f][d] = uni(gen);
        for (auto e = d; e < 3; ++e) Q[f][d][e] = Q[f][e][d] = uni(gen);
      }
    if (rankDeficient) {  // fluxes depending on x only: Hessians of rank 1
      for (auto f = 0u; f < 15; ++f)
        for (auto d = 0u; d < 3; ++d)
          for (auto e = 0u; e < 3; ++e) Q[f][d][e] = (d == 0 && e == 0) ? Q[f][0][0] : 0.0;
    }
  }
  FieldFunction Fields() const {
    return [this](const passivedouble* x, su2double* values) {
      const auto nVar = GoalMetric::NumVar(nDim);
      for (auto j = 0u; j < nVar; ++j) {
        values[j] = c[j];
        for (auto d = 0u; d < nDim; ++d) values[j] += a[j][d] * x[d];
      }
      for (auto f = 0u; f < GoalMetric::NumFlux(nDim); ++f) {
        passivedouble v = 0.0;
        for (auto d = 0u; d < nDim; ++d) {
          v += b[f][d] * x[d];
          for (auto e = 0u; e < nDim; ++e) v += 0.5 * x[d] * Q[f][d][e] * x[e];
        }
        values[nVar + f] = v;
      }
    };
  }
  Mat3 Exact() const {
    const auto nVar = GoalMetric::NumVar(nDim);
    Mat3 H{};
    for (auto d = 0u; d < nDim; ++d)
      for (auto j = 0u; j < nVar; ++j) {
        Mat3 Qm{};
        for (auto p = 0u; p < nDim; ++p)
          for (auto q = 0u; q < nDim; ++q) Qm[p][q] = Q[d * nVar + j][p][q];
        const auto absQ = AbsMatrix(nDim, Qm);
        for (auto p = 0u; p < nDim; ++p)
          for (auto q = 0u; q < nDim; ++q) H[p][q] += fabs(a[j][d]) * absQ[p][q];
      }
    return H;
  }
};

/*--- Max over the domain points at least two layers inside of |H_go - exact| / max |exact|. ---*/
passivedouble ManufacturedError(CGeometry& geo, CSolver* flow, const Manufactured& m, unsigned long n,
                                unsigned long& nChecked) {
  const auto nDim = m.nDim;
  const auto exact = m.Exact();
  passivedouble scale = 0.0, err = 0.0;
  for (auto p = 0u; p < nDim; ++p)
    for (auto q = 0u; q < nDim; ++q) scale = std::max(scale, fabs(exact[p][q]));
  const passivedouble h = 1.0 / n, upper[3] = {nDim == 2 ? 2.0 : 1.0, 1.0, 1.0};
  nChecked = 0;
  for (auto iPoint = 0ul; iPoint < geo.GetnPointDomain(); ++iPoint) {
    bool inside = true;
    for (auto iDim = 0u; iDim < nDim; ++iDim) {
      const auto x = SU2_TYPE::GetValue(geo.nodes->GetCoord(iPoint, iDim));
      inside = inside && x > 2 * h - 1e-10 && x < upper[iDim] - 2 * h + 1e-10;
    }
    if (!inside) continue;
    ++nChecked;
    su2double H[3][3];
    flow->GetNodes()->GetHessianMat(iPoint, 0, H);
    for (auto p = 0u; p < nDim; ++p)
      for (auto q = 0u; q < nDim; ++q) err = std::max(err, fabs(SU2_TYPE::GetValue(H[p][q]) - exact[p][q]));
  }
  return err / scale;
}

}  // namespace

TEST_CASE("Goal Hessian of manufactured fields", "[GoalMetric]") {
  for (const auto* method : {"GREEN_GAUSS", "WEIGHTED_LEAST_SQUARES"}) {
    for (unsigned short nDim : {2, 3}) {
      for (int deficient : {0, 1}) {
        const unsigned long n = (nDim == 2) ? 8 : 6;
        GoalMesh test(nDim, n, nDim == 2 ? FAR2D : FAR3D, string("NUM_METHOD_HESS= ") + method + "\n");
        const Manufactured m(nDim, deficient);
        SetFields(test.flow, test.Geo(), m.Fields());
        test.Compute();
        unsigned long nChecked = 0;
        const auto err = ManufacturedError(test.Geo(), test.flow, m, n, nChecked);
        INFO(method << " nDim " << nDim << " rank-deficient " << deficient);
        CHECK(CPassiveComm::AllreduceSum(nChecked) > 0);
        CHECK(err < 1e-10);
        CHECK(test.flow->GetGoalRejected() == 0);
        CHECK(test.flow->GetGoalNonFinite() == 0);
        CHECK(test.flow->GetGoalMinRatio() >= -1e-12);
      }
    }
  }
}

TEST_CASE("Goal Hessian of constant fields is zero, the metric is SPD", "[GoalMetric]") {
  for (unsigned short nDim : {2, 3}) {
    /*--- Constant lambda and fluxes: H_go is round-off. Zero lambda: H_go and the ratios exactly 0. ---*/
    GoalMesh test(nDim, 4, nDim == 2 ? FAR2D : FAR3D, "NUM_METHOD_HESS= GREEN_GAUSS\n");
    for (const passivedouble lambda : {0.7, 0.0}) {
      SetFields(test.flow, test.Geo(), [lambda](const passivedouble*, su2double* values) {
        for (auto k = 0u; k < 20; ++k) values[k] = (k < 5) ? lambda : 0.3 + 0.1 * k;
      });
      test.Compute();
      passivedouble maxH = 0.0, maxRatio = 0.0;
      for (auto iPoint = 0ul; iPoint < test.Geo().GetnPointDomain(); ++iPoint) {
        for (auto iMet = 0u; iMet < 3u * (nDim - 1); ++iMet)
          maxH = std::max(maxH, fabs(SU2_TYPE::GetValue(test.flow->GetNodes()->GetHessian(iPoint, 0, iMet))));
        maxRatio = std::max({maxRatio, test.flow->GetGoalDiagnostic(iPoint, 0), test.flow->GetGoalDiagnostic(iPoint, 1)});
      }
      CHECK(maxH < 1e-12);
      if (lambda == 0.0) {
        CHECK(maxH == 0.0);
        CHECK(maxRatio == 0.0);
      }
    }
    {
      Mute mute;
      test.flow->ComputeMetric(&test.Geo(), test.config.get());
    }
    bool spd = true;
    for (auto iPoint = 0ul; iPoint < test.Geo().GetnPointDomain(); ++iPoint) {
      su2double M[3][3], vec[3][3], val[3], work[3];
      test.flow->GetNodes()->GetMetricMat(iPoint, M);
      CBlasStructure::EigenDecomposition(M, vec, val, nDim, work);
      for (auto i = 0u; i < nDim; ++i) spd = spd && std::isfinite(SU2_TYPE::GetValue(val[i])) && val[i] > 0.0;
    }
    CHECK(spd);
  }
}

TEST_CASE("Goal Hessian of smooth fields is PSD, the metric SPD at the complexity", "[GoalMetric]") {
  for (const auto* method : {"GREEN_GAUSS", "WEIGHTED_LEAST_SQUARES"}) {
    for (unsigned short nDim : {2, 3}) {
      GoalMesh test(nDim, nDim == 2 ? 10 : 5, nDim == 2 ? FAR2D : FAR3D, string("NUM_METHOD_HESS= ") + method + "\n");
      SetFields(test.flow, test.Geo(), FromState(nDim, [nDim](const passivedouble* x, su2double* l) { SmoothLambda(nDim, x, l); },
                                                 [nDim](const passivedouble* x, su2double* U) { SmoothState(nDim, x, U); }));
      const auto nVar = GoalMetric::NumVar(nDim);
      vector<su2double> state(test.Geo().GetnPointDomain() * nVar);
      for (auto iPoint = 0ul; iPoint < test.Geo().GetnPointDomain(); ++iPoint) {
        passivedouble x[3] = {0.0};
        for (auto a = 0u; a < nDim; ++a) x[a] = SU2_TYPE::GetValue(test.Geo().nodes->GetCoord(iPoint, a));
        SmoothState(nDim, x, &state[iPoint * nVar]);
      }
      {
        Mute mute;
        test.flow->ComputeGoalHessian(&test.Geo(), test.config.get(), &state);
      }
      CHECK(test.flow->GetGoalRejected() == 0);
      CHECK(test.flow->GetGoalNonFinite() == 0);
      CHECK(test.flow->GetGoalMinRatio() >= -1e-12);
      passivedouble maxEq33 = 0.0;
      for (auto iPoint = 0ul; iPoint < test.Geo().GetnPointDomain(); ++iPoint) {
        for (unsigned short k = 0; k < 2; ++k) {
          const auto ratio = test.flow->GetGoalDiagnostic(iPoint, k);
          CHECK(std::isfinite(ratio));
          CHECK(ratio >= 0.0);
          CHECK(ratio <= 1.0);
        }
        maxEq33 = std::max(maxEq33, test.flow->GetGoalDiagnostic(iPoint, 1));
      }
      CHECK(MaxAll(maxEq33) > 0.0);
      {
        Mute mute;
        test.flow->ComputeMetric(&test.Geo(), test.config.get());
      }
      passivedouble complexity = 0.0;
      bool spd = true;
      for (auto iPoint = 0ul; iPoint < test.Geo().GetnPointDomain(); ++iPoint) {
        su2double M[3][3], vec[3][3], val[3], work[3];
        test.flow->GetNodes()->GetMetricMat(iPoint, M);
        CBlasStructure::EigenDecomposition(M, vec, val, nDim, work);
        passivedouble det = 1.0;
        for (auto i = 0u; i < nDim; ++i) {
          spd = spd && std::isfinite(SU2_TYPE::GetValue(val[i])) && val[i] > 0.0;
          det *= SU2_TYPE::GetValue(val[i]);
        }
        complexity += sqrt(det) * SU2_TYPE::GetValue(test.Geo().nodes->GetVolume(iPoint));
      }
      complexity = CPassiveComm::Allreduce(complexity, CPassiveComm::Op::SUM);
      CHECK(spd);
      CHECK(complexity == Approx(400.0).epsilon(1e-6));
    }
  }
}

/*--------------------------------------------------------------------------------------------------------------------*/

TEST_CASE("Goal diagnostics keep zero-trace ratios zero and count non-finite eq. (33) terms", "[GoalMetric]") {
  GoalMesh test(2, 4, FAR2D, "NUM_METHOD_HESS= GREEN_GAUSS\n");
  auto& geo = test.Geo();
  SetFields(test.flow, geo, [](const passivedouble* x, su2double* values) {
    SmoothLambda(2, x, values);
    for (auto k = 4u; k < GoalMetric::FieldCount(2); ++k) values[k] = 0.0;
  });
  const auto nVar = GoalMetric::NumVar(2);
  vector<su2double> state(geo.GetnPointDomain() * nVar, 1.0);
  /*--- Finite state, nonzero lambda gradients, but H_go = 0: both ratios remain 0. ---*/
  {
    Mute mute;
    test.flow->ComputeGoalHessian(&geo, test.config.get(), &state);
  }
  CHECK(test.flow->GetGoalNonFinite() == 0);
  for (auto iPoint = 0ul; iPoint < geo.GetnPointDomain(); ++iPoint)
    for (unsigned short k = 0; k < 2; ++k) CHECK(test.flow->GetGoalDiagnostic(iPoint, k) == 0.0);
  /*--- A non-finite denominator must be counted even when the ratio would otherwise be replaced by 0. ---*/
  for (const passivedouble rho : {std::nan(""), std::numeric_limits<passivedouble>::infinity()}) {
    for (auto iPoint = 0ul; iPoint < geo.GetnPointDomain(); ++iPoint) state[iPoint * nVar] = rho;
    {
      Mute mute;
      test.flow->ComputeGoalHessian(&geo, test.config.get(), &state);
    }
    CHECK(test.flow->GetGoalRejected() == 0);
    CHECK(test.flow->GetGoalNonFinite() == geo.GetGlobal_nPointDomain());
    for (auto iPoint = 0ul; iPoint < geo.GetnPointDomain(); ++iPoint)
      for (unsigned short k = 0; k < 2; ++k) CHECK(test.flow->GetGoalDiagnostic(iPoint, k) == 0.0);
  }
}

/*--------------------------------------------------------------------------------------------------------------------*/
/*--- U6: symmetry planes on meshes (half box + rule = full mirrored box, Green-Gauss). ---*/

namespace {

/*--- Hexahedral box (BOX reader) with given element counts, lengths and offsets, rotated about the y axis. ---*/
struct GoalBox {
  std::unique_ptr<CConfig> config;
  std::unique_ptr<CGeometry> geometry;
  CSolver** solver = nullptr;
  CSolver* flow = nullptr;

  GoalBox(const string& markers, const string& method, const string& size, const string& length, const string& offset,
          passivedouble angle, const string& extra = "", const std::function<void(su2double*)>& map = nullptr) {
    const string aoa = (extra.find("AOA=") == string::npos) ? "AOA= " + std::to_string(angle * 180.0 / PI_NUMBER) + "\n" : "";
    const string options = "SOLVER= EULER\nMATH_PROBLEM= DIRECT\nMESH_FORMAT= BOX\nINIT_OPTION= TD_CONDITIONS\n" +
                           markers + "MESH_BOX_SIZE= " + size + "\nMESH_BOX_LENGTH= " + length +
                           "\nMESH_BOX_OFFSET= " + offset + "\nCOMPUTE_METRIC= YES\nADAP_SENSOR= (MACH)\n"
                           "NUM_METHOD_HESS= " + method + "\n" + aoa + extra;
    Mute mute;
    stringstream ss(options);
    config = std::unique_ptr<CConfig>(new CConfig(ss, SU2_COMPONENT::SU2_CFD, false));
    {
      auto aux = std::unique_ptr<CGeometry>(new CPhysicalGeometry(config.get(), 0, 1));
      aux->SetColorGrid_Parallel(config.get());
      geometry = std::unique_ptr<CGeometry>(new CPhysicalGeometry(aux.get(), config.get()));
    }
    for (unsigned long point = 0; point < geometry->GetnPoint(); ++point) {
      const auto x = geometry->nodes->GetCoord(point, 0), z = geometry->nodes->GetCoord(point, 2);
      geometry->nodes->SetCoord(point, 0, cos(angle) * x - sin(angle) * z);
      geometry->nodes->SetCoord(point, 2, sin(angle) * x + cos(angle) * z);
      if (map) {
        su2double c[3] = {geometry->nodes->GetCoord(point, 0), geometry->nodes->GetCoord(point, 1),
                          geometry->nodes->GetCoord(point, 2)};
        map(c);
        for (unsigned short iDim = 0; iDim < 3; ++iDim) geometry->nodes->SetCoord(point, iDim, c[iDim]);
      }
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
    geometry->FindNormal_Neighbor(config.get());
    geometry->SetGlobal_to_Local_Point();
    geometry->PreprocessP2PComms(geometry.get(), config.get());
    solver = CSolverFactory::CreateSolverContainer(config->GetKind_Solver(), config.get(), geometry.get(), 0);
    flow = solver[FLOW_SOL];
    PrepareGoal(flow, *geometry);
  }
  ~GoalBox() {
    for (unsigned short iSol = 0; iSol < MAX_SOLS; ++iSol) {
      CSolverFactory::ClearSolverMeta(solver[iSol]);
      delete solver[iSol];
    }
    delete[] solver;
  }
};

/*--- Compare independently partitioned half/full boxes through owned passive field records. ---*/
template<class T>
std::array<long long, 3> GoalPointKey(const T* x) {
  return {std::llround(SU2_TYPE::GetValue(x[0]) * 1e9), std::llround(SU2_TYPE::GetValue(x[1]) * 1e9),
          std::llround(SU2_TYPE::GetValue(x[2]) * 1e9)};
}

using GoalFieldRecord = std::array<passivedouble, 201>; // 20 gradients, 15 flux Hessians, goal Hessian.
std::map<std::array<long long, 3>, GoalFieldRecord> GlobalBoxFields(const GoalBox& box,
                                                                 const vector<su2double>& fluxHessians) {
  vector<std::array<long long, 3>> keys;
  vector<GoalFieldRecord> records;
  const auto& gradients = box.flow->GetNodes()->GetGradient_Adapt();
  for (auto p = 0ul; p < box.geometry->GetnPointDomain(); ++p) {
    keys.push_back(GoalPointKey(box.geometry->nodes->GetCoord(p)));
    GoalFieldRecord row{};
    for (unsigned short k = 0; k < 20; ++k)
      for (unsigned short d = 0; d < 3; ++d) row[3*k+d] = SU2_TYPE::GetValue(gradients(p,k,d));
    for (unsigned short k = 0; k < 135; ++k) row[60+k] = SU2_TYPE::GetValue(fluxHessians[135*p+k]);
    for (unsigned short k = 0; k < 6; ++k) row[195+k] = SU2_TYPE::GetValue(box.flow->GetNodes()->GetHessian(p,0,k));
    records.push_back(row);
  }
  keys = CPassiveComm::Allgatherv(keys, nullptr);
  records = CPassiveComm::Allgatherv(records, nullptr);
  REQUIRE(keys.size() == records.size());
  std::map<std::array<long long, 3>, GoalFieldRecord> result;
  for (size_t k = 0; k < keys.size(); ++k) REQUIRE(result.emplace(keys[k],records[k]).second);
  return result;
}

/*--- State and lambda with exact mirror parity about the plane through 0 with normal n (frame t1, t2, n). ---*/
FieldFunction MirrorFields(const passivedouble* t1, const passivedouble* t2, const passivedouble* n) {
  return [=](const passivedouble* x, su2double* values) {
    passivedouble s = 0.0, a = 0.0, b = 0.0;
    for (auto i = 0u; i < 3; ++i) {
      s += x[i] * n[i];
      a += x[i] * t1[i];
      b += x[i] * t2[i];
    }
    const passivedouble rho = 1.0 + 0.1 * sin(1.3 * a + 0.7 * b) + 0.2 * s * s;
    const passivedouble vt1 = 0.4 + 0.1 * cos(b + s * s), vt2 = 0.1 * sin(a), vn = s * (0.3 + 0.1 * a);
    const passivedouble p = (1.0 + 0.1 * cos(a) + 0.1 * s * s) / GAMMA;
    su2double U[5];
    U[0] = rho;
    for (auto i = 0u; i < 3; ++i) U[1 + i] = rho * (vt1 * t1[i] + vt2 * t2[i] + vn * n[i]);
    U[4] = p / (GAMMA - 1.0) + 0.5 * rho * (vt1 * vt1 + vt2 * vt2 + vn * vn);
    const passivedouble lt1 = 0.2 * b + s * s, lt2 = sin(a), ln = s * (1.0 + b);
    values[0] = 0.5 + cos(a + s * s);
    for (auto i = 0u; i < 3; ++i) values[1 + i] = lt1 * t1[i] + lt2 * t2[i] + ln * n[i];
    values[4] = 0.3 * a * b + s * s;
    GoalMetric::FluxFields(3, GAMMA, U, values + 5);
  };
}

}  // namespace

TEST_CASE("Goal Hessian of manufactured fields on hexahedra", "[GoalMetric]") {
  /*--- U2 on the BOX mesh (7 nodes per direction, h = 1/6): exact on the nodes two layers inside. ---*/
  for (const auto* method : {"GREEN_GAUSS", "WEIGHTED_LEAST_SQUARES"}) {
    for (int deficient : {0, 1}) {
      GoalBox box("MARKER_FAR= (x_minus, x_plus, y_minus, y_plus, z_minus, z_plus)\n", method, "7,7,7", "1,1,1",
                  "0,0,0", 0.0);
      const Manufactured m(3, deficient);
      SetFields(box.flow, *box.geometry, m.Fields());
      {
        Mute mute;
        box.flow->ComputeGoalHessian(box.geometry.get(), box.config.get());
      }
      unsigned long nChecked = 0;
      const auto err = ManufacturedError(*box.geometry, box.flow, m, 6, nChecked);
      INFO(method << " rank-deficient " << deficient);
      CHECK(CPassiveComm::AllreduceSum(nChecked) > 0);
      CHECK(err < 1e-10);
      CHECK(box.flow->GetGoalRejected() == 0);
      CHECK(box.flow->GetGoalNonFinite() == 0);
      CHECK(box.flow->GetGoalMinRatio() >= -1e-12);
    }
  }
}

TEST_CASE("Goal symmetry plane: half mesh with the mirror rule equals the full mirrored mesh", "[GoalMetric]") {
  for (const passivedouble angle : {0.0, PI_NUMBER / 4}) {
    const passivedouble t1[3] = {cos(angle), 0.0, sin(angle)}, t2[3] = {0.0, 1.0, 0.0}, n[3] = {-sin(angle), 0.0, cos(angle)};
    GoalBox half("MARKER_SYM= (z_minus)\nMARKER_FAR= (x_minus, x_plus, y_minus, y_plus, z_plus)\n", "GREEN_GAUSS",
                 "5,5,5", "1,1,1", "0,0,0", angle);
    GoalBox full("MARKER_FAR= (x_minus, x_plus, y_minus, y_plus, z_minus, z_plus)\n", "GREEN_GAUSS", "5,5,9", "1,1,2",
                 "0,0,-1", angle);
    vector<su2double> hessHalf, hessFull;
    for (auto* test : {&half, &full}) {
      SetFields(test->flow, *test->geometry, MirrorFields(t1, t2, n));
      Mute mute;
      test->flow->ComputeGoalHessian(test->geometry.get(), test->config.get(), nullptr,
                                     test == &half ? &hessHalf : &hessFull);
    }

    /*--- Points of the half box in the full box, by coordinates. ---*/
    const auto fullFields = GlobalBoxFields(full, hessFull);
    const unsigned short nField = 20, nFlux = 15;
    passivedouble scaleG[20] = {0.0}, scaleH[15] = {0.0}, scaleGo = 0.0;
    passivedouble errG[20] = {0.0}, errH[15] = {0.0}, errGo = 0.0;
    unsigned long nPlane = 0, nMatched = 0;
    const auto& gHalf = half.flow->GetNodes()->GetGradient_Adapt();
    for (auto iPoint = 0ul; iPoint < half.geometry->GetnPointDomain(); ++iPoint) {
      const auto it = fullFields.find(GoalPointKey(half.geometry->nodes->GetCoord(iPoint)));
      REQUIRE(it != fullFields.end());
      const auto& fullRow = it->second;
      ++nMatched;
      passivedouble s = 0.0;
      for (auto i = 0u; i < 3; ++i) s += SU2_TYPE::GetValue(half.geometry->nodes->GetCoord(iPoint, i)) * n[i];
      if (fabs(s) < 1e-10) ++nPlane;
      for (auto k = 0u; k < nField; ++k)
        for (auto a = 0u; a < 3; ++a) {
          scaleG[k] = std::max(scaleG[k], fabs(fullRow[3*k+a]));
          errG[k] = std::max(errG[k], fabs((SU2_TYPE::GetValue(gHalf(iPoint, k, a)) - fullRow[3*k+a])));
        }
      for (auto f = 0u; f < nFlux; ++f)
        for (auto ab = 0u; ab < 9; ++ab) {
          const auto a = SU2_TYPE::GetValue(hessHalf[(iPoint * nFlux + f) * 9 + ab]);
          const auto b = fullRow[60+9*f+ab];
          scaleH[f] = std::max(scaleH[f], fabs(b));
          errH[f] = std::max(errH[f], fabs(a - b));
        }
      for (auto iMet = 0u; iMet < 6; ++iMet) {
        const auto a = SU2_TYPE::GetValue(half.flow->GetNodes()->GetHessian(iPoint, 0, iMet));
        const auto b = fullRow[195+iMet];
        scaleGo = std::max(scaleGo, fabs(b));
        errGo = std::max(errGo, fabs(a - b));
      }
    }
    INFO("angle " << angle);
    CHECK(CPassiveComm::AllreduceSum(nPlane) > 0);
    CHECK(nMatched == half.geometry->GetnPointDomain());
    for (auto k = 0u; k < nField; ++k) CHECK(errG[k] <= 1e-12 * scaleG[k]);
    for (auto f = 0u; f < nFlux; ++f) CHECK(errH[f] <= 1e-12 * scaleH[f]);
    CHECK(scaleGo > 0.0);
    CHECK(errGo <= 1e-12 * scaleGo);
    /*--- F_{n,m_n} (direction n, normal momentum) explicitly: with angle 0, field 5 + 2 * 5 + 3. ---*/
    if (angle == 0.0) CHECK(scaleH[2 * 5 + 3] > 0.0);
  }
}

TEST_CASE("Goal symmetry plane: two orthogonal planes", "[GoalMetric]") {
  const passivedouble t1[3] = {1.0, 0.0, 0.0}, t2[3] = {0.0, 1.0, 0.0}, n[3] = {0.0, 0.0, 1.0};
  /*--- The field is mirror-symmetric about z = 0 only; about y = 0 too if it is even in y: use y -> y^2 terms via a
   *    state built in the frame (t1, n2 = y, n): reuse MirrorFields with t2 as the second normal is not symmetric, so
   *    the test compares quarter vs full on a field symmetric about both planes. ---*/
  auto both = [&](const passivedouble* x, su2double* values) {
    const passivedouble xs[3] = {x[0], x[1] * x[1], x[2]};  // even in y
    MirrorFields(t1, t2, n)(xs, values);
    /*--- Odd components about y: lambda_my and the flux components with one y index (direction or momentum). ---*/
    values[2] *= x[1];
    for (unsigned short d = 0; d < 3; ++d)
      for (unsigned short j = 0; j < 5; ++j) {
        const bool oddY = (d == 1) != (j == 2);
        if (oddY) values[5 + d * 5 + j] *= x[1];
      }
  };
  GoalBox quarter("MARKER_SYM= (z_minus, y_minus)\nMARKER_FAR= (x_minus, x_plus, y_plus, z_plus)\n", "GREEN_GAUSS",
                  "5,5,5", "1,1,1", "0,0,0", 0.0);
  GoalBox full("MARKER_FAR= (x_minus, x_plus, y_minus, y_plus, z_minus, z_plus)\n", "GREEN_GAUSS", "5,9,9", "1,2,2",
               "0,-1,-1", 0.0);
  vector<su2double> hessQ, hessF;
  for (auto* test : {&quarter, &full}) {
    SetFields(test->flow, *test->geometry, both);
    Mute mute;
    test->flow->ComputeGoalHessian(test->geometry.get(), test->config.get(), nullptr, test == &quarter ? &hessQ : &hessF);
  }
  const auto fullFields = GlobalBoxFields(full, hessF);
  passivedouble scale = 0.0, err = 0.0;
  unsigned long nEdge = 0;
  for (auto iPoint = 0ul; iPoint < quarter.geometry->GetnPointDomain(); ++iPoint) {
    const auto* x = quarter.geometry->nodes->GetCoord(iPoint);
    const auto& fullRow = fullFields.at(GoalPointKey(x));
    if (fabs(SU2_TYPE::GetValue(x[1])) < 1e-10 && fabs(SU2_TYPE::GetValue(x[2])) < 1e-10) ++nEdge;
    for (auto iMet = 0u; iMet < 6; ++iMet) {
      const auto a = SU2_TYPE::GetValue(quarter.flow->GetNodes()->GetHessian(iPoint, 0, iMet));
      const auto b = fullRow[195+iMet];
      scale = std::max(scale, fabs(b));
      err = std::max(err, fabs(a - b));
    }
  }
  CHECK(CPassiveComm::AllreduceSum(nEdge) > 0);
  CHECK(scale > 0.0);
  CHECK(err <= 1e-12 * scale);
}

TEST_CASE("Goal symmetry plane with WLS: exact for symmetric quadratic fields", "[GoalMetric]") {
  /*--- Plane z = 0; even components: no odd powers of z; odd components: z (c + d.x_t); lambda linear with parity. ---*/
  GoalBox box("MARKER_SYM= (z_minus)\nMARKER_FAR= (x_minus, x_plus, y_minus, y_plus, z_plus)\n",
              "WEIGHTED_LEAST_SQUARES", "7,7,7", "1,1,1", "0,0,0", 0.0);
  const passivedouble n[3] = {0.0, 0.0, 1.0};
  /*--- Parity of the components: lambda_mz odd; flux (d, j) odd iff (d == z) != (j == m_z). ---*/
  auto oddField = [](unsigned short k) {
    if (k < 5) return k == 3;
    const unsigned short d = (k - 5) / 5, j = (k - 5) % 5;
    return (d == 2) != (j == 3);
  };
  std::mt19937 gen(5);
  std::uniform_real_distribution<passivedouble> uni(-1.0, 1.0);
  passivedouble coef[20][7];
  for (auto& row : coef)
    for (auto& v : row) v = uni(gen);
  auto fields = [&](const passivedouble* x, su2double* values) {
    for (unsigned short k = 0; k < 20; ++k) {
      const auto* c = coef[k];
      if (k < 5) values[k] = oddField(k) ? c[0] * x[2] : c[0] + c[1] * x[0] + c[2] * x[1];
      else if (oddField(k)) values[k] = x[2] * (c[0] + c[1] * x[0] + c[2] * x[1]);
      else values[k] = c[0] + c[1] * x[0] + c[2] * x[1] + c[3] * x[0] * x[0] + c[4] * x[0] * x[1] + c[5] * x[1] * x[1] +
                       c[6] * x[2] * x[2];
    }
  };
  SetFields(box.flow, *box.geometry, fields);
  {
    Mute mute;
    box.flow->ComputeGoalHessian(box.geometry.get(), box.config.get());
  }
  /*--- Exact H_go. ---*/
  Mat3 exact{};
  for (unsigned short d = 0; d < 3; ++d)
    for (unsigned short j = 0; j < 5; ++j) {
      const unsigned short k = 5 + d * 5 + j;
      const auto* c = coef[k];
      Mat3 Hk{};
      if (oddField(k)) {
        Hk[0][2] = Hk[2][0] = c[1];
        Hk[1][2] = Hk[2][1] = c[2];
      } else {
        Hk[0][0] = 2 * c[3]; Hk[0][1] = Hk[1][0] = c[4]; Hk[1][1] = 2 * c[5]; Hk[2][2] = 2 * c[6];
      }
      /*--- d_d lambda_j: even lambda components have gradient (c1, c2, 0), odd (lambda_mz) (0, 0, c0). ---*/
      const auto* cl = coef[j];
      const passivedouble g = oddField(j) ? (d == 2 ? cl[0] : 0.0) : (d == 0 ? cl[1] : d == 1 ? cl[2] : 0.0);
      const auto absH = AbsMatrix(3, Hk);
      for (auto a = 0u; a < 3; ++a)
        for (auto b = 0u; b < 3; ++b) exact[a][b] += fabs(g) * absH[a][b];
    }
  passivedouble scale = 0.0, err = 0.0;
  unsigned long nPlane = 0;
  for (auto a = 0u; a < 3; ++a)
    for (auto b = 0u; b < 3; ++b) scale = std::max(scale, fabs(exact[a][b]));
  const passivedouble h = 1.0 / 6;
  for (auto iPoint = 0ul; iPoint < box.geometry->GetnPointDomain(); ++iPoint) {
    const auto* x = box.geometry->nodes->GetCoord(iPoint);
    bool inside = true;
    for (auto i = 0u; i < 2; ++i) inside = inside && x[i] > 2 * h - 1e-10 && x[i] < 1 - 2 * h + 1e-10;
    inside = inside && x[2] < 1 - 2 * h + 1e-10;
    if (!inside) continue;
    if (fabs(SU2_TYPE::GetValue(x[2])) < 1e-10) ++nPlane;
    su2double H[3][3];
    box.flow->GetNodes()->GetHessianMat(iPoint, 0, H);
    for (auto a = 0u; a < 3; ++a)
      for (auto b = 0u; b < 3; ++b) err = std::max(err, fabs(SU2_TYPE::GetValue(H[a][b]) - exact[a][b]));
    /*--- Parity zeros of the gradients on the plane: odd components have no tangential gradient. ---*/
    if (fabs(SU2_TYPE::GetValue(x[2])) < 1e-10) {
      for (unsigned short k = 0; k < 20; ++k) {
        const auto& g = box.flow->GetNodes()->GetGradient_Adapt();
        if (oddField(k)) {
          CHECK(g(iPoint, k, 0) == 0.0);
          CHECK(g(iPoint, k, 1) == 0.0);
        } else {
          CHECK(g(iPoint, k, 2) == 0.0);
        }
      }
    }
  }
  CHECK(CPassiveComm::AllreduceSum(nPlane) > 0);
  CHECK(err <= 1e-9 * scale);
  (void)n;
}

/*--------------------------------------------------------------------------------------------------------------------*/
/*--- Symmetry support check. ---*/

TEST_CASE("Goal symmetry support check", "[GoalMetric]") {
  auto check = [](const string& markers, const string& extra, const std::function<void(su2double*)>& map = nullptr) {
    GoalBox box(markers, "GREEN_GAUSS", "4,4,4", "1,1,1", "0,0,0", 0.0, extra, map);
    vector<std::array<su2double, 3>> normals;
    return GoalMetric::CheckSymmetry(*box.geometry, *box.config, normals);
  };
  const string ySym = "MARKER_SYM= (y_minus)\nMARKER_FAR= (x_minus, x_plus, y_plus, z_minus, z_plus)\n";
  CHECK(check(ySym, "OBJECTIVE_FUNCTION= DRAG\n").empty());
  CHECK(check(ySym, "OBJECTIVE_FUNCTION= LIFT\nAOA= 3.0\n").empty());
  CHECK(check(ySym, "OBJECTIVE_FUNCTION= MOMENT_Y\n").empty());
  CHECK(check(ySym, "OBJECTIVE_FUNCTION= FORCE_Z\n").empty());
  /*--- Planarity is independent of translation and scale. ---*/
  CHECK(check(ySym, "", [](su2double* x) {
    for (unsigned short a = 0; a < 3; ++a) x[a] += 1e8;
  }).empty());
  CHECK(check(ySym, "", [](su2double* x) {
    for (unsigned short a = 0; a < 3; ++a) x[a] *= 1e-3;
  }).empty());
  /*--- Parallel patches at different offsets: the small step keeps normal errors below 1e-8.
   *    Translation along the plane and scaling must not loosen the offset tolerance. ---*/
  for (const passivedouble scale : {1.0, 1e-3}) {
    for (const passivedouble translation : {0.0, 1e8}) {
      const auto patches = check(ySym, "", [=](su2double* x) {
        if (x[0] > 0.5) x[1] += 1e-9;
        for (unsigned short a = 0; a < 3; ++a) x[a] *= scale;
        x[0] += translation;
      });
      CHECK(patches.find("planar") != string::npos);
    }
  }
  /*--- Opposite 1e-6 tilts preserve the area-weighted plane normal and all vertex offsets. ---*/
  {
    GoalBox box(ySym, "GREEN_GAUSS", "4,4,4", "1,1,1", "0,0,0", 0.0);
    for (auto iMarker = 0u; iMarker < box.geometry->GetnMarker(); ++iMarker) {
      if (box.config->GetMarker_All_KindBC(iMarker) != SYMMETRY_PLANE) continue;
      for (auto iVertex = 0ul; iVertex < box.geometry->GetnVertex(iMarker); ++iVertex) {
        const auto iPoint = box.geometry->vertex[iMarker][iVertex]->GetNode();
        auto* normal = box.geometry->vertex[iMarker][iVertex]->GetNormal();
        const su2double tilt = box.geometry->nodes->GetCoord(iPoint, 0) < 0.5 ? su2double(1e-6) : su2double(-1e-6);
        normal[2] = tilt * fabs(normal[1]);
      }
    }
    vector<std::array<su2double, 3>> normals;
    CHECK(GoalMetric::CheckSymmetry(*box.geometry, *box.config, normals).find("planar") != string::npos);
  }
  CHECK_FALSE(check(ySym, "OBJECTIVE_FUNCTION= SIDEFORCE\n").empty());
  CHECK_FALSE(check(ySym, "OBJECTIVE_FUNCTION= MOMENT_X\n").empty());
  CHECK_FALSE(check(ySym, "OBJECTIVE_FUNCTION= FORCE_Y\n").empty());
  CHECK_FALSE(check(ySym, "OBJECTIVE_FUNCTION= DRAG\nSIDESLIP_ANGLE= 5.0\n").empty());
  const string ySymWall = "MARKER_SYM= (y_minus)\nMARKER_EULER= (x_plus)\nMARKER_FAR= (x_minus, y_plus, z_minus, z_plus)\n"
                         "MARKER_MONITORING= (x_plus)\nREF_ORIGIN_MOMENT_X= 0.0\nREF_ORIGIN_MOMENT_Z= 0.0\n";
  CHECK(check(ySymWall, "OBJECTIVE_FUNCTION= MOMENT_Y\nREF_ORIGIN_MOMENT_Y= 0.0\n").empty());
  CHECK_FALSE(check(ySymWall, "OBJECTIVE_FUNCTION= MOMENT_Y\nREF_ORIGIN_MOMENT_Y= 0.5\n").empty());
  CHECK_FALSE(check(ySym, "OBJECTIVE_FUNCTION= EFFICIENCY\n").empty());
  /*--- Orthogonal symmetry planes and an orthogonal symmetry/Euler-wall junction are accepted. ---*/
  CHECK(check("MARKER_SYM= (y_minus, z_minus)\nMARKER_FAR= (x_minus, x_plus, y_plus, z_plus)\n", "").empty());
  CHECK(check("MARKER_SYM= (y_minus)\nMARKER_EULER= (z_minus)\nMARKER_FAR= (x_minus, x_plus, y_plus, z_plus)\n", "").empty());
  /*--- Non-planar marker (y_minus bent); planes and a wall meeting at 60 degrees (z_minus sheared). ---*/
  auto bend = [](su2double* x) { x[1] += 0.1 * x[0] * x[0]; };
  auto shear = [](su2double* x) { x[2] += x[1] / sqrt(3.0); };
  const auto bent = check(ySym, "", bend);
  CHECK(bent.find("planar") != string::npos);
  const auto planes = check("MARKER_SYM= (y_minus, z_minus)\nMARKER_FAR= (x_minus, x_plus, y_plus, z_plus)\n", "", shear);
  CHECK(planes.find("orthogonal") != string::npos);
  const auto wall = check("MARKER_SYM= (y_minus)\nMARKER_EULER= (z_minus)\nMARKER_FAR= (x_minus, x_plus, y_plus, z_plus)\n",
                          "", shear);
  CHECK(wall.find("right angles") != string::npos);
}

/*--------------------------------------------------------------------------------------------------------------------*/
/*--- U7: slip-wall reconstruction of the normal momentum lambda. ---*/

namespace {

/*--- Affine lambda; at the wall points the normal momentum component is set to 0 (as the capture does). ---*/
passivedouble WallReconstructionError(const string& markers, const simplex_test::MarkerFunction& markerOf,
                                      const std::function<void(passivedouble*)>& map, passivedouble scale,
                                      unsigned long& nWall, const string& method = "GREEN_GAUSS") {
  GoalMesh test(2, 8, markers, "NUM_METHOD_HESS= " + method + "\n", markerOf, [&](passivedouble* x) {
    if (map) map(x);
    x[0] *= scale;
    x[1] *= scale;
  });
  auto& geo = test.Geo();
  const passivedouble L[4][2] = {{0.3, -0.2}, {1.1, 0.4}, {-0.7, 0.9}, {0.2, 0.5}}, c[4] = {0.1, 0.3, -0.2, 0.7};
  auto lambda = [&](const su2double* x, unsigned short j) {
    return c[j] + (L[j][0] * SU2_TYPE::GetValue(x[0]) + L[j][1] * SU2_TYPE::GetValue(x[1])) / scale;
  };
  SetFields(test.flow, geo, [](const passivedouble*, su2double* values) {
    for (auto k = 0u; k < 12; ++k) values[k] = 0.0;
  });
  auto& field = test.flow->GetNodes()->GetAuxVar_Adapt();
  for (auto iPoint = 0ul; iPoint < geo.GetnPointDomain(); ++iPoint)
    for (unsigned short j = 0; j < 4; ++j) field(iPoint, j) = lambda(geo.nodes->GetCoord(iPoint), j);
  /*--- Zero normal component at the Euler-wall vertices (each marker normal, as the capture projects). ---*/
  for (auto iMarker = 0u; iMarker < geo.GetnMarker(); ++iMarker) {
    if (test.config->GetMarker_All_KindBC(iMarker) != EULER_WALL) continue;
    for (auto iVertex = 0ul; iVertex < geo.GetnVertex(iMarker); ++iVertex) {
      const auto iPoint = geo.vertex[iMarker][iVertex]->GetNode();
      if (iPoint >= geo.GetnPointDomain()) continue;
      const auto* normal = geo.vertex[iMarker][iVertex]->GetNormal();
      const su2double area = sqrt(normal[0] * normal[0] + normal[1] * normal[1]);
      const su2double proj = (field(iPoint, 1) * normal[0] + field(iPoint, 2) * normal[1]) / area;
      field(iPoint, 1) -= proj * normal[0] / area;
      field(iPoint, 2) -= proj * normal[1] / area;
    }
  }
  test.Compute();
  passivedouble err = 0.0, lamScale = 0.0;
  nWall = 0;
  for (auto iPoint = 0ul; iPoint < geo.GetnPointDomain(); ++iPoint) {
    bool wall = false;
    for (auto iMarker = 0u; iMarker < geo.GetnMarker(); ++iMarker)
      wall = wall || (test.config->GetMarker_All_KindBC(iMarker) == EULER_WALL && geo.nodes->GetVertex(iPoint, iMarker) >= 0);
    if (!wall) continue;
    ++nWall;
    for (unsigned short c2 = 0; c2 < 2; ++c2) {
      const auto exact = lambda(geo.nodes->GetCoord(iPoint), 1 + c2);
      lamScale = std::max(lamScale, fabs(exact));
      err = std::max(err, fabs(test.flow->GetGoalDiagnostic(iPoint, 2 + c2) - exact));
    }
  }
  nWall = CPassiveComm::AllreduceSum(nWall);
  lamScale = MaxAll(lamScale);
  REQUIRE(lamScale > 0);
  return MaxAll(err) / lamScale;
}

/*--- Rectangle mapped to an annular sector (curved lower wall r = 1). ---*/
void Annulus(passivedouble* x) {
  const passivedouble r = 1.0 + 0.5 * x[1], theta = 0.4 * x[0];
  x[0] = r * cos(theta);
  x[1] = r * sin(theta);
}

}  // namespace

TEST_CASE("Goal slip-wall reconstruction of the normal momentum lambda", "[GoalMetric]") {
  const string flat = "MARKER_EULER= (lower_a, lower_b)\nMARKER_FAR= (left, right, upper)\n";
  const string corner = "MARKER_EULER= (lower_a, lower_b, left)\nMARKER_FAR= (right, upper)\n";
  unsigned long nWall = 0;
  for (const auto* method : {"GREEN_GAUSS", "WEIGHTED_LEAST_SQUARES"}) {
    CHECK(WallReconstructionError(flat, simplex_test::Marker2D, nullptr, 1.0, nWall, method) < 1e-12);
    CHECK(nWall > 0);
  }
  CHECK(WallReconstructionError(flat, simplex_test::Marker2D, nullptr, 1e-3, nWall) < 1e-12);
  CHECK(WallReconstructionError(flat, simplex_test::Marker2D, Annulus, 1.0, nWall) < 1e-12);
  CHECK(WallReconstructionError(corner, simplex_test::Marker2D, nullptr, 1.0, nWall) < 1e-12);
}

TEST_CASE("Goal slip-wall reconstruction: deficient stencils are counted", "[GoalMetric]") {
  /*--- Rectangle of 4 x 2 cells, Euler walls all around: the three interior points have collinear non-wall
   *    neighbours (rank-deficient donors), the two corners without a diagonal have no non-wall neighbour. ---*/
  GoalMesh test(2, 2, "MARKER_EULER= (lower_a, lower_b, left, right, upper)\n", "NUM_METHOD_HESS= GREEN_GAUSS\n");
  SetFields(test.flow, test.Geo(), FromState(2, [](const passivedouble* x, su2double* l) { SmoothLambda(2, x, l); },
                                             [](const passivedouble* x, su2double* U) { SmoothState(2, x, U); }));
  test.Compute();
  CHECK(test.flow->GetGoalWallCount(0) == 12);
  CHECK(test.flow->GetGoalWallCount(1) == 3);
  CHECK(test.flow->GetGoalWallCount(2) == 2);
  CHECK(test.flow->GetGoalNonFinite() == 0);
  CHECK(test.flow->GetGoalRejected() == 0);
}

/*--------------------------------------------------------------------------------------------------------------------*/
/*--- U8: partition independence (run with any number of ranks). ---*/

namespace {
template <class F>
void SerialRun(F f) {
#ifdef HAVE_MPI
  const auto world = SU2_MPI::GetComm();
  SU2_MPI::SetComm(MPI_COMM_SELF);
  f();
  SU2_MPI::SetComm(world);
#else
  f();
#endif
}
}  // namespace

TEST_CASE("Goal Hessian and metric independent of the partition", "[GoalMetric][GoalMPI]") {
  for (const auto* method : {"GREEN_GAUSS", "WEIGHTED_LEAST_SQUARES"}) {
    for (unsigned short nDim : {2, 3}) {
      const string markers = (nDim == 2) ? "MARKER_EULER= (lower_a, lower_b, right)\nMARKER_SYM= (upper)\nMARKER_FAR= (left)\n"
                                         : "MARKER_EULER= (z_minus_a, z_minus_b)\nMARKER_SYM= (y_minus)\n"
                                           "MARKER_FAR= (x_minus, x_plus, y_plus, z_plus)\n";
      const unsigned long n = (nDim == 2) ? 10 : 5;
      const auto nField = GoalMetric::FieldCount(nDim);
      const unsigned short nMet = 3 * (nDim - 1);
      const unsigned short nValue = nMet * 2 + nField * nDim + nDim;
      std::vector<passivedouble> reference;
      auto run = [&](bool parallel) {
        GoalMesh test(nDim, n, markers, string("NUM_METHOD_HESS= ") + method + "\n");
        auto& geo = test.Geo();
        SetFields(test.flow, geo, FromState(nDim, [nDim](const passivedouble* x, su2double* l) { SmoothLambda(nDim, x, l); },
                                            [nDim](const passivedouble* x, su2double* U) { SmoothState(nDim, x, U); }));
        test.Compute();
        {
          Mute mute;
          test.flow->ComputeMetric(&geo, test.config.get());
        }
        CHECK(test.flow->GetGoalRejected() == 0);
        CHECK(test.flow->GetGoalNonFinite() == 0);
        const auto* nodes = test.flow->GetNodes();
        const auto& grad = nodes->GetGradient_Adapt();
        if (!parallel) reference.assign(geo.GetGlobal_nPointDomain() * nValue, 0.0);
        std::vector<passivedouble> scale(nValue, 0.0), err(nValue, 0.0);
        unsigned long nCutWall = 0, nCutJunction = 0;
        for (auto iPoint = 0ul; iPoint < geo.GetnPointDomain(); ++iPoint) {
          const auto global = geo.nodes->GetGlobalIndex(iPoint);
          passivedouble values[2 * 6 + 20 * 3 + 3];
          unsigned short k = 0;
          for (unsigned short iMet = 0; iMet < nMet; ++iMet) values[k++] = SU2_TYPE::GetValue(nodes->GetHessian(iPoint, 0, iMet));
          for (unsigned short iMet = 0; iMet < nMet; ++iMet) values[k++] = SU2_TYPE::GetValue(nodes->GetMetric(iPoint, iMet));
          for (unsigned short f = 0; f < nField; ++f)
            for (unsigned short a = 0; a < nDim; ++a) values[k++] = SU2_TYPE::GetValue(grad(iPoint, f, a));
          for (unsigned short a = 0; a < nDim; ++a) values[k++] = test.flow->GetGoalDiagnostic(iPoint, 2 + a);
          for (unsigned short i = 0; i < nValue; ++i) {
            CHECK(std::isfinite(values[i]));
            if (!parallel) reference[global * nValue + i] = values[i];
            else {
              CHECK(std::isfinite(reference[global * nValue + i]));
              scale[i] = std::max(scale[i], fabs(reference[global * nValue + i]));
              err[i] = std::max(err[i], fabs(values[i] - reference[global * nValue + i]));
            }
          }
          bool wall = false, sym = false, haloNeighbour = false;
          for (auto iMarker = 0u; iMarker < geo.GetnMarker(); ++iMarker) {
            const bool on = geo.nodes->GetVertex(iPoint, iMarker) >= 0;
            wall = wall || (test.config->GetMarker_All_KindBC(iMarker) == EULER_WALL && on);
            sym = sym || (test.config->GetMarker_All_KindBC(iMarker) == SYMMETRY_PLANE && on);
          }
          for (const auto jPoint : geo.nodes->GetPoints(iPoint)) haloNeighbour = haloNeighbour || jPoint >= geo.GetnPointDomain();
          if (wall && haloNeighbour) ++nCutWall;
          if (wall && sym && haloNeighbour) ++nCutJunction;
        }
        if (parallel) {
          passivedouble worst = 0.0;
          for (unsigned short i = 0; i < nValue; ++i) {
            const auto s = MaxAll(scale[i]), e = MaxAll(err[i]);
            worst = std::max(worst, s > 0.0 ? e / s : e);
            CHECK(e <= 1e-12 * s);
          }
          const auto nCut = CPassiveComm::AllreduceSum(nCutWall);
          const auto nCutJ = CPassiveComm::AllreduceSum(nCutJunction);
          if (SU2_MPI::GetRank() == MASTER_NODE)
            cout << "Goal MPI " << method << " " << nDim << "D: max relative difference " << worst
                 << ", wall points next to the cut " << nCut << ", symmetry/wall junction points next to the cut "
                 << nCutJ << endl;
          if (SU2_MPI::GetSize() > 1) CHECK(nCut > 0);
          if (SU2_MPI::GetSize() > 1 && nDim == 3) CHECK(nCutJ > 0);
        }
      };
      SerialRun([&]() { run(false); });
      run(true);
    }
  }
}
