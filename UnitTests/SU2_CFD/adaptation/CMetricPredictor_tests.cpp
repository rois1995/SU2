/*!
 * \file CMetricPredictor_tests.cpp
 * \brief Unit tests of the metric prediction of the time-domain adaptation loop (ADAP_UNSTEADY_METRIC= PREDICT):
 *        optical flow on the mesh, transport and reorientation of the metric, intersection and size bounds.
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

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <memory>

#include "../../../Common/include/CConfig.hpp"
#include "../../../Common/include/adaptation/CMeshGather.hpp"
#include "../../../Common/include/geometry/CPhysicalGeometry.hpp"
#include "../../../Common/include/geometry/meshreader/CMemoryMeshReaderFVM.hpp"
#include "../../../Common/include/linear_algebra/blas_structure.hpp"
#include "../../../Common/include/toolboxes/geometry_toolbox.hpp"
#include "../../../SU2_CFD/include/adaptation/CMetricPredictor.hpp"
#include "../../../SU2_CFD/include/drivers/CDriver.hpp"
#include "../../Common/adaptation/SimplexMeshTestCase.hpp"

namespace {

/*!
 * \brief Geometry of the rectangle [0,2]x[0,1] (2D) or the unit cube (3D) of SimplexMeshTestCase, built in memory.
 */
struct PredictorTest {
  std::unique_ptr<CConfig> config;
  CGeometry** geometry = nullptr;
  unsigned short nDim;

  PredictorTest(unsigned short dim, unsigned long n) : nDim(dim) {
    auto* buffer = std::cout.rdbuf(nullptr);
    const auto mesh = simplex_test::MakeSimplexMesh(nDim, n, nDim == 2 ? simplex_test::Marker2D : simplex_test::Marker3D);
    const std::string markers = nDim == 2 ? "MARKER_FAR= (lower_a, lower_b, upper, left, right)\n"
                                          : "MARKER_FAR= (z_minus_a, z_minus_b, z_plus, x_minus, x_plus, y_minus, "
                                            "y_plus)\n";
    std::stringstream ss("SOLVER= EULER\nMACH_NUMBER= 0.5\nMESH_FORMAT= SU2\nMESH_FILENAME= unused.su2\nMGLEVEL= 0\n" +
                         markers);
    config = std::make_unique<CConfig>(ss, SU2_COMPONENT::SU2_CFD, false);
    CMemoryMeshReaderFVM reader(config.get(), mesh, 0, 1);
    CDriver::BuildGeometryFVM(config.get(), new CPhysicalGeometry(config.get(), reader, 1), geometry, true);
    std::cout.rdbuf(buffer);
  }
  ~PredictorTest() {
    delete geometry[MESH_0];
    delete[] geometry;
  }
  CGeometry& Geometry() const { return *geometry[MESH_0]; }
  unsigned long nPoint() const { return Geometry().GetnPoint(); }
  const su2double* Coord(unsigned long iPoint) const { return Geometry().nodes->GetCoord(iPoint); }
  unsigned short nMet() const { return nDim * (nDim + 1) / 2; }

  /*--- Field of the points from a function of the coordinates. ---*/
  std::vector<su2double> Field(const std::function<su2double(const su2double*)>& f) const {
    std::vector<su2double> v(nPoint());
    for (auto i = 0ul; i < nPoint(); ++i) v[i] = f(Coord(i));
    return v;
  }
  /*--- Metric field (upper triangle rows) from a function filling a 3x3 matrix. ---*/
  std::vector<su2double> Metric(const std::function<void(const su2double*, su2double (&)[3][3])>& f) const {
    std::vector<su2double> v(nPoint() * nMet());
    for (auto i = 0ul; i < nPoint(); ++i) {
      su2double M[3][3] = {{0.0}};
      f(Coord(i), M);
      for (unsigned short a = 0, k = 0; a < nDim; ++a)
        for (unsigned short b = a; b < nDim; ++b, ++k) v[i * nMet() + k] = M[a][b];
    }
    return v;
  }
  void Unpack(const std::vector<su2double>& v, unsigned long i, su2double (&M)[3][3]) const {
    for (unsigned short a = 0; a < 3; ++a)
      for (unsigned short b = 0; b < 3; ++b) M[a][b] = 0.0;
    for (unsigned short a = 0, k = 0; a < nDim; ++a)
      for (unsigned short b = a; b < nDim; ++b, ++k) M[a][b] = M[b][a] = v[i * nMet() + k];
  }
};

/*--- Gaussian bump of the invariant around a centre. ---*/
su2double Bump(unsigned short nDim, const su2double* x, const su2double* c, su2double r) {
  su2double d2 = 0.0;
  for (unsigned short i = 0; i < nDim; ++i) d2 += pow(x[i] - c[i], 2);
  return 2.0 + 2.0 * exp(-d2 / (r * r));
}

/*--- Metric of sizes h1 along the direction at angle theta (in the xy plane) and h2 across (h3 along z). ---*/
void RotatedMetric(unsigned short nDim, su2double theta, su2double h1, su2double h2, su2double (&M)[3][3]) {
  const su2double c = cos(theta), s = sin(theta);
  const su2double l1 = 1.0 / (h1 * h1), l2 = 1.0 / (h2 * h2);
  M[0][0] = l1 * c * c + l2 * s * s;
  M[0][1] = M[1][0] = (l1 - l2) * c * s;
  M[1][1] = l1 * s * s + l2 * c * c;
  if (nDim == 3) M[2][2] = l2;
}

/*--- Smallest eigenvalue of A - B (>= 0 if A is at least as fine as B in every direction), relative to |B|. ---*/
su2double OrderGap(unsigned short nDim, const su2double (&A)[3][3], const su2double (&B)[3][3]) {
  su2double D[3][3] = {{0.0}}, vec[3][3], val[3], work[3], vecB[3][3], valB[3];
  for (unsigned short a = 0; a < nDim; ++a)
    for (unsigned short b = 0; b < nDim; ++b) D[a][b] = A[a][b] - B[a][b];
  CBlasStructure::EigenDecomposition(D, vec, val, nDim, work);
  CBlasStructure::EigenDecomposition(B, vecB, valB, nDim, work);
  return *std::min_element(val, val + nDim) / *std::max_element(valB, valB + nDim);
}

}  // namespace

TEST_CASE("Metric prediction: optical flow of a translated feature", "[Adaptation]") {
  for (unsigned short nDim : {2, 3}) {
    PredictorTest test(nDim, nDim == 2 ? 24 : 12);
    CMetricPredictor predictor(test.Geometry());

    /*--- The bump moves by D over 4 time steps: W = D / 4 per step. ---*/
    const su2double r = nDim == 2 ? 0.15 : 0.2;
    const su2double cj[3] = {nDim == 2 ? 0.8 : 0.4, 0.45, 0.45};
    const su2double D[3] = {0.08, 0.04, nDim == 2 ? 0.0 : -0.04};
    const su2double ck[3] = {cj[0] + D[0], cj[1] + D[1], cj[2] + D[2]};
    const auto sj = test.Field([&](const su2double* x) { return Bump(nDim, x, cj, r); });
    const auto sk = test.Field([&](const su2double* x) { return Bump(nDim, x, ck, r); });

    CMetricPredictor::MotionReport report;
    const auto W = predictor.MotionField(sj, sk, 4.0, nullptr, 0.5, report);
    CHECK(report.mismatchAfter < 0.2 * report.mismatchBefore);
    CHECK(report.featureLength > 0.5 * r);
    CHECK(report.featureLength < 2.0 * r);

    /*--- In the feature (within r of its centre) the motion is the translation within 8% of |W| per point, 2% on
     *    average. ---*/
    su2double normW = 0.0;
    for (unsigned short i = 0; i < nDim; ++i) normW += pow(D[i] / 4.0, 2);
    normW = sqrt(normW);
    su2double maxError = 0.0, mean[3] = {0.0, 0.0, 0.0}, weight = 0.0;
    for (auto iPoint = 0ul; iPoint < test.nPoint(); ++iPoint) {
      su2double d2 = 0.0, e2 = 0.0;
      for (unsigned short i = 0; i < nDim; ++i) d2 += pow(test.Coord(iPoint)[i] - ck[i], 2);
      if (d2 > r * r) continue;
      for (unsigned short i = 0; i < nDim; ++i) {
        e2 += pow(W[iPoint * nDim + i] - D[i] / 4.0, 2);
        mean[i] += predictor.GetMass()[iPoint] * W[iPoint * nDim + i];
      }
      weight += predictor.GetMass()[iPoint];
      maxError = max(maxError, sqrt(e2));
    }
    CHECK(maxError < 0.08 * normW);
    su2double meanError = 0.0;
    for (unsigned short i = 0; i < nDim; ++i) meanError += pow(mean[i] / weight - D[i] / 4.0, 2);
    CHECK(sqrt(meanError) < 0.02 * normW);

    /*--- Starting from the exact motion gives the same field (the start with the smaller mismatch is kept). ---*/
    std::vector<su2double> guess(test.nPoint() * nDim);
    for (auto iPoint = 0ul; iPoint < test.nPoint(); ++iPoint)
      for (unsigned short i = 0; i < nDim; ++i) guess[iPoint * nDim + i] = D[i] / 4.0;
    CMetricPredictor::MotionReport reportGuess;
    const auto Wg = predictor.MotionField(sj, sk, 4.0, &guess, 0.5, reportGuess);
    CHECK(reportGuess.mismatchAfter <= report.mismatchAfter * (1.0 + 1e-6));
    CHECK(reportGuess.mismatchOther >= 0.0);

    /*--- Zero motion prescribed on the side x = xmax (as at a no-slip wall): exactly zero there; the feature still
     *    moves, the smoothness term pulls its motion towards zero by an amount that falls with the distance (here
     *    1.1 / 0.5 from the feature centre in 2D / 3D: 7% / 30% of |W| measured). ---*/
    const su2double xMax = nDim == 2 ? 2.0 : 1.0;
    std::vector<bool> fixed(test.nPoint(), false);
    for (auto iPoint = 0ul; iPoint < test.nPoint(); ++iPoint) fixed[iPoint] = test.Coord(iPoint)[0] > xMax - 1e-9;
    CMetricPredictor::MotionReport reportFixed;
    const auto Wf = predictor.MotionField(sj, sk, 4.0, &guess, 0.5, reportFixed, &fixed);
    su2double meanFixed[3] = {0.0, 0.0, 0.0};
    for (auto iPoint = 0ul; iPoint < test.nPoint(); ++iPoint) {
      su2double d2 = 0.0;
      for (unsigned short i = 0; i < nDim; ++i) d2 += pow(test.Coord(iPoint)[i] - ck[i], 2);
      for (unsigned short i = 0; i < nDim; ++i) {
        if (fixed[iPoint]) CHECK(Wf[iPoint * nDim + i] == 0.0);
        if (d2 < r * r) meanFixed[i] += predictor.GetMass()[iPoint] * Wf[iPoint * nDim + i] / weight;
      }
    }
    for (unsigned short i = 0; i < nDim; ++i) CHECK(meanFixed[i] == Approx(D[i] / 4.0).margin((nDim == 2 ? 0.12 : 0.4) * normW));

    /*--- A feature that grows in place: the flow sees a dilation (brightness constancy does not hold), but no
     *    translation: the mean motion over the feature is small. ---*/
    const auto sGrown = test.Field([&](const su2double* x) { return 2.0 + 1.5 * (Bump(nDim, x, ck, r) - 2.0); });
    CMetricPredictor::MotionReport reportGrown;
    const auto Wgrown = predictor.MotionField(sk, sGrown, 4.0, &guess, 0.5, reportGrown);
    su2double meanGrown[3] = {0.0, 0.0, 0.0};
    for (auto iPoint = 0ul; iPoint < test.nPoint(); ++iPoint) {
      su2double d2 = 0.0;
      for (unsigned short i = 0; i < nDim; ++i) d2 += pow(test.Coord(iPoint)[i] - ck[i], 2);
      if (d2 > r * r) continue;
      for (unsigned short i = 0; i < nDim; ++i) meanGrown[i] += predictor.GetMass()[iPoint] * Wgrown[iPoint * nDim + i] / weight;
    }
    for (unsigned short i = 0; i < nDim; ++i) CHECK(fabs(meanGrown[i]) < 0.1 * normW);

    /*--- No feature: no motion. ---*/
    const std::vector<su2double> flat(test.nPoint(), 3.0);
    CMetricPredictor::MotionReport reportFlat;
    const auto W0 = predictor.MotionField(flat, flat, 4.0, nullptr, 0.5, reportFlat);
    for (const auto w : W0) CHECK(w == 0.0);
  }
}

TEST_CASE("Metric prediction: translated metric feature", "[Adaptation]") {
  for (unsigned short nDim : {2, 3}) {
    PredictorTest test(nDim, nDim == 2 ? 24 : 10);
    CMetricPredictor predictor(test.Geometry());

    /*--- Uniform motion; metric with components affine in x (P1-exact) and anisotropic: each instant is the metric
     *    at x - W t exactly where the trajectory stays in the domain (no reorientation: grad W = 0). ---*/
    const su2double W[3] = {0.03, -0.01, nDim == 2 ? 0.0 : 0.02};
    std::vector<su2double> motion(test.nPoint() * nDim);
    for (auto i = 0ul; i < test.nPoint(); ++i)
      for (unsigned short d = 0; d < nDim; ++d) motion[i * nDim + d] = W[d];
    auto affine = [&](const su2double* x, su2double (&M)[3][3]) {
      M[0][0] = 400.0 + 300.0 * x[0];
      M[0][1] = M[1][0] = 50.0 + 20.0 * x[1];
      M[1][1] = 30.0 + 10.0 * x[0];
      if (nDim == 3) {
        M[2][2] = 60.0 + 20.0 * x[2];
        M[0][2] = M[2][0] = 5.0;
        M[1][2] = M[2][1] = -3.0 + 2.0 * x[0];
      }
    };
    const auto metricK = test.Metric(affine);
    const std::vector<su2double> instants = {0.0, 2.0, 4.0, 6.0};
    CMetricPredictor::Options options;
    CMetricPredictor::PredictionReport report;
    std::vector<std::vector<su2double>> perInstant;
    const auto predicted = predictor.Predict(metricK, motion, instants, options, report, &perInstant);
    REQUIRE(perInstant.size() == instants.size());
    CHECK(report.nInstant == instants.size());

    unsigned long nChecked = 0;
    for (size_t k = 0; k < instants.size(); ++k) {
      for (auto i = 0ul; i < test.nPoint(); ++i) {
        su2double y[3] = {0.0, 0.0, 0.0};
        bool inside = true;
        for (unsigned short d = 0; d < nDim; ++d) {
          y[d] = test.Coord(i)[d] - W[d] * instants[k];
          const su2double upper = (nDim == 2 && d == 0) ? 2.0 : 1.0;
          inside = inside && y[d] > 1e-9 && y[d] < upper - 1e-9;
        }
        if (!inside) continue;
        su2double expected[3][3] = {{0.0}}, got[3][3];
        affine(y, expected);
        test.Unpack(perInstant[k], i, got);
        for (unsigned short a = 0; a < nDim; ++a)
          for (unsigned short b = 0; b < nDim; ++b) CHECK(got[a][b] == Approx(expected[a][b]).epsilon(1e-10));
        ++nChecked;
      }
    }
    CHECK(nChecked > test.nPoint());

    /*--- The intersection is at least as fine as every instant. ---*/
    for (auto i = 0ul; i < test.nPoint(); ++i) {
      su2double P[3][3], M[3][3];
      test.Unpack(predicted, i, P);
      for (const auto& instant : perInstant) {
        test.Unpack(instant, i, M);
        CHECK(OrderGap(nDim, P, M) > -1e-10);
      }
    }
  }

  /*--- End to end in 2D: a fine isotropic spot moves with the optical flow; its predicted position at each instant is
   *    that of the translated spot. ---*/
  PredictorTest test(2, 24);
  CMetricPredictor predictor(test.Geometry());
  const su2double r = 0.15, cj[2] = {0.7, 0.5}, step[2] = {0.02, 0.005}, ck[2] = {0.7 + 4 * step[0], 0.5 + 4 * step[1]};
  auto spot = [&](const su2double* c) {
    return [&, c](const su2double* x, su2double (&M)[3][3]) {
      const su2double h = 0.1 / (1.0 + 9.0 * exp(-(pow(x[0] - c[0], 2) + pow(x[1] - c[1], 2)) / (r * r)));
      M[0][0] = M[1][1] = 1.0 / (h * h);
      M[0][1] = M[1][0] = 0.0;
    };
  };
  const auto Mj = test.Metric(spot(cj)), Mk = test.Metric(spot(ck));
  std::vector<su2double> sj(test.nPoint()), sk(test.nPoint());
  for (auto i = 0ul; i < test.nPoint(); ++i) {
    sj[i] = CMetricPredictor::Invariant(2, &Mj[i * 3]);
    sk[i] = CMetricPredictor::Invariant(2, &Mk[i * 3]);
  }
  CMetricPredictor::MotionReport motionReport;
  const auto motion = predictor.MotionField(sj, sk, 4.0, nullptr, 0.5, motionReport);
  CMetricPredictor::Options options;
  options.hmin = 1e-3;
  options.hmax = 0.1;
  CMetricPredictor::PredictionReport report;
  std::vector<std::vector<su2double>> perInstant;
  const std::vector<su2double> instants = {0.0, 5.0, 10.0};
  predictor.Predict(Mk, motion, instants, options, report, &perInstant);
  for (size_t k = 0; k < instants.size(); ++k) {
    /*--- Centroid of the excess density sqrt(det M) - 100 at the instant. ---*/
    su2double centroid[2] = {0.0, 0.0}, total = 0.0;
    for (auto i = 0ul; i < test.nPoint(); ++i) {
      const auto* m = &perInstant[k][i * 3];
      const su2double excess = sqrt(m[0] * m[2] - m[1] * m[1]) - 100.0;
      for (unsigned short d = 0; d < 2; ++d) centroid[d] += predictor.GetMass()[i] * excess * test.Coord(i)[d];
      total += predictor.GetMass()[i] * excess;
    }
    for (unsigned short d = 0; d < 2; ++d)
      CHECK(centroid[d] / total == Approx(ck[d] + step[d] * instants[k]).margin(0.01));
  }
}

TEST_CASE("Metric prediction: rotation reorients the metric", "[Adaptation]") {
  for (unsigned short nDim : {2, 3}) {
    PredictorTest test(nDim, nDim == 2 ? 16 : 8);
    CMetricPredictor predictor(test.Geometry());

    /*--- Rigid rotation about the z axis through c, 30 degrees over the horizon of 10 time steps. Uniform metric of
     *    aspect ratio 10 along x. At points near c (trajectory inside the domain) the metric of instant t is rotated
     *    by omega t (congruence with F = exp(t grad W), an exact rotation), sizes unchanged. ---*/
    const su2double omega = (30.0 * PI_NUMBER / 180.0) / 10.0, c[3] = {nDim == 2 ? 1.0 : 0.5, 0.5, 0.5};
    std::vector<su2double> motion(test.nPoint() * nDim, 0.0);
    for (auto i = 0ul; i < test.nPoint(); ++i) {
      motion[i * nDim + 0] = -omega * (test.Coord(i)[1] - c[1]);
      motion[i * nDim + 1] = omega * (test.Coord(i)[0] - c[0]);
    }
    const auto metricK = test.Metric([&](const su2double*, su2double (&M)[3][3]) { RotatedMetric(nDim, 0.0, 0.1, 0.01, M); });
    const std::vector<su2double> instants = {0.0, 5.0, 10.0};

    for (const su2double threshold : {1.5, 20.0}) {
      CMetricPredictor::Options options;
      options.anisoThreshold = threshold;
      CMetricPredictor::PredictionReport report;
      std::vector<std::vector<su2double>> perInstant;
      predictor.Predict(metricK, motion, instants, options, report, &perInstant);
      if (threshold > 10.0) CHECK(report.nCongruence == 0);

      for (size_t k = 0; k < instants.size(); ++k) {
        const su2double angle = threshold < 10.0 ? omega * instants[k] : 0.0;
        su2double expected[3][3] = {{0.0}};
        RotatedMetric(nDim, angle, 0.1, 0.01, expected);
        for (auto i = 0ul; i < test.nPoint(); ++i) {
          if (GeometryToolbox::Distance(nDim, test.Coord(i), c) > 0.3) continue;
          su2double M[3][3];
          test.Unpack(perInstant[k], i, M);
          for (unsigned short a = 0; a < nDim; ++a)
            for (unsigned short b = 0; b < nDim; ++b) CHECK(M[a][b] == Approx(expected[a][b]).margin(1e-8 * 1e4));
        }
      }
    }
  }
}

TEST_CASE("Metric prediction: isotropic background, bounds and intersection", "[Adaptation]") {
  for (unsigned short nDim : {2, 3}) {
    PredictorTest test(nDim, nDim == 2 ? 12 : 6);
    CMetricPredictor predictor(test.Geometry());

    /*--- Any motion (rotation, strain, shear about c): a uniform isotropic metric is unchanged, also with the
     *    congruence everywhere when the motion is a rotation. ---*/
    const su2double c[3] = {nDim == 2 ? 1.0 : 0.5, 0.5, 0.5};
    const su2double A[3][3] = {{0.01, -0.03, 0.0}, {0.02, -0.005, 0.01}, {0.0, 0.004, 0.003}};
    std::vector<su2double> motion(test.nPoint() * nDim, 0.0), rotation(test.nPoint() * nDim, 0.0);
    for (auto i = 0ul; i < test.nPoint(); ++i) {
      for (unsigned short a = 0; a < nDim; ++a)
        for (unsigned short b = 0; b < nDim; ++b) motion[i * nDim + a] += A[a][b] * (test.Coord(i)[b] - c[b]);
      rotation[i * nDim + 0] = -0.02 * (test.Coord(i)[1] - c[1]);
      rotation[i * nDim + 1] = 0.02 * (test.Coord(i)[0] - c[0]);
    }
    const auto iso = test.Metric([&](const su2double*, su2double (&M)[3][3]) {
      for (unsigned short a = 0; a < nDim; ++a) M[a][a] = 400.0;
    });
    const std::vector<su2double> instants = {0.0, 3.0, 7.0, 10.0};
    CMetricPredictor::Options options;
    CMetricPredictor::PredictionReport report;
    auto predicted = predictor.Predict(iso, motion, instants, options, report);
    CHECK(report.nCongruence == 0);
    for (size_t k = 0; k < predicted.size(); ++k) CHECK(predicted[k] == Approx(iso[k]).margin(1e-10));
    options.anisoThreshold = 1.0;  // congruence wherever anisotropic: an isotropic metric is still only moved
    predicted = predictor.Predict(iso, rotation, instants, options, report);
    for (size_t k = 0; k < predicted.size(); ++k) CHECK(predicted[k] == Approx(iso[k]).margin(1e-9));

    /*--- Size bounds: eigenvalues outside [1/hmax^2, 1/hmin^2] are limited at each instant. ---*/
    options = CMetricPredictor::Options();
    options.hmin = 0.01;
    options.hmax = 0.5;
    const auto wide = test.Metric([&](const su2double*, su2double (&M)[3][3]) { RotatedMetric(nDim, 0.3, 1.0, 1e-3, M); });
    std::vector<std::vector<su2double>> perInstant;
    const std::vector<su2double> zero(test.nPoint() * nDim, 0.0);
    predicted = predictor.Predict(wide, zero, instants, options, report, &perInstant);
    su2double bounded[3][3] = {{0.0}};
    RotatedMetric(nDim, 0.3, 0.5, 0.01, bounded);
    if (nDim == 3) bounded[2][2] = 1e4;
    for (const auto& instant : perInstant) {
      for (auto i = 0ul; i < test.nPoint(); ++i) {
        su2double M[3][3];
        test.Unpack(instant, i, M);
        for (unsigned short a = 0; a < nDim; ++a)
          for (unsigned short b = 0; b < nDim; ++b) CHECK(M[a][b] == Approx(bounded[a][b]).epsilon(1e-9).margin(1e-6));
      }
    }
    /*--- Without motion all instants are equal, so is their intersection. ---*/
    for (size_t k = 0; k < predicted.size(); ++k) CHECK(predicted[k] == Approx(perInstant[0][k]).margin(1e-7));

    /*--- Intersection of two different instants: finest in each direction. Metric of size 0.1 along x and 0.01
     *    along y moved by a rotation of 90 degrees between the two instants: the intersection is 0.01 in x and y. ---*/
    if (nDim == 2) {
      const su2double omega = 0.5 * PI_NUMBER / 10.0;
      std::vector<su2double> rot(test.nPoint() * 2);
      for (auto i = 0ul; i < test.nPoint(); ++i) {
        rot[i * 2 + 0] = -omega * (test.Coord(i)[1] - c[1]);
        rot[i * 2 + 1] = omega * (test.Coord(i)[0] - c[0]);
      }
      const auto aniso = test.Metric([&](const su2double*, su2double (&M)[3][3]) { RotatedMetric(2, 0.0, 0.1, 0.01, M); });
      options = CMetricPredictor::Options();
      predicted = predictor.Predict(aniso, rot, {0.0, 10.0}, options, report);
      for (auto i = 0ul; i < test.nPoint(); ++i) {
        if (GeometryToolbox::Distance(2, test.Coord(i), c) > 0.2) continue;
        su2double M[3][3];
        test.Unpack(predicted, i, M);
        CHECK(M[0][0] == Approx(1e4).epsilon(1e-6));
        CHECK(M[1][1] == Approx(1e4).epsilon(1e-6));
        CHECK(M[0][1] == Approx(0.0).margin(1e-6 * 1e4));
      }
    }
  }
}

namespace {
/*--- Rotation by the z-x-z Euler angles (a, b, c). ---*/
void EulerRotation(su2double a, su2double b, su2double c, su2double (&Q)[3][3]) {
  const su2double ca = cos(a), sa = sin(a), cb = cos(b), sb = sin(b), cc = cos(c), sc = sin(c);
  const su2double Rz1[3][3] = {{ca, -sa, 0.0}, {sa, ca, 0.0}, {0.0, 0.0, 1.0}};
  const su2double Rx[3][3] = {{1.0, 0.0, 0.0}, {0.0, cb, -sb}, {0.0, sb, cb}};
  const su2double Rz2[3][3] = {{cc, -sc, 0.0}, {sc, cc, 0.0}, {0.0, 0.0, 1.0}};
  su2double T[3][3] = {{0.0}};
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      for (int k = 0; k < 3; ++k) T[i][j] += Rz1[i][k] * Rx[k][j];
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j) {
      Q[i][j] = 0.0;
      for (int k = 0; k < 3; ++k) Q[i][j] += T[i][k] * Rz2[k][j];
    }
}

/*--- Upper triangle of Q diag(val) Q^T (nDim x nDim block of Q). ---*/
std::vector<su2double> PackedMetric(unsigned short nDim, const su2double (&Q)[3][3], const su2double* val) {
  std::vector<su2double> m;
  for (unsigned short a = 0; a < nDim; ++a)
    for (unsigned short b = a; b < nDim; ++b) {
      su2double sum = 0.0;
      for (unsigned short k = 0; k < nDim; ++k) sum += Q[a][k] * val[k] * Q[b][k];
      m.push_back(sum);
    }
  return m;
}
}  // namespace

TEST_CASE("Metric prediction: invariant of rotated anisotropic metrics", "[Adaptation]") {
  /*--- Valid metrics (aspect ratio sqrt(lmax/lmin) up to the default ADAP_ARMAX 1e6) at any orientation: the
   *    invariant 0.5 log10(det M) to round-off of the stored tensor (the expanded 3x3 determinant cancels). ---*/

  /*--- M = I + 1e10 ones(3,3): eigenvalues (1, 1, 3e10 + 1). ---*/
  const su2double ones[6] = {1.0 + 1e10, 1e10, 1e10, 1.0 + 1e10, 1e10, 1.0 + 1e10};
  CHECK(CMetricPredictor::Invariant(3, ones) == Approx(0.5 * log10(3e10 + 1.0)).margin(1e-6));

  const su2double eig3[][3] = {{1.0, 1e4, 1e12}, {1e-4, 1.0, 1e8}, {1e-2, 1e-2, 1e10}, {1.0, 1.0, 1e12}};
  const su2double eig2[][2] = {{1.0, 1e12}, {1e-6, 1e6}, {1.0, 1e8}};
  su2double maxError3 = 0.0, maxError2 = 0.0;
  for (int k = 0; k < 500; ++k) {
    const su2double a = 0.0123 * k, b = 0.0371 * k + 0.1, c = 0.0577 * k + 0.2;
    su2double Q[3][3];
    EulerRotation(a, b, c, Q);
    for (const auto& val : eig3) {
      const auto m = PackedMetric(3, Q, val);
      maxError3 = max(maxError3, fabs(CMetricPredictor::Invariant(3, m.data()) - 0.5 * log10(val[0] * val[1] * val[2])));
    }
    const su2double R[3][3] = {{cos(a), -sin(a), 0.0}, {sin(a), cos(a), 0.0}, {0.0, 0.0, 1.0}};
    for (const auto& val : eig2) {
      const auto m = PackedMetric(2, R, val);
      maxError2 = max(maxError2, fabs(CMetricPredictor::Invariant(2, m.data()) - 0.5 * log10(val[0] * val[1])));
    }
  }
  CHECK(maxError3 < 1e-4);
  CHECK(maxError2 < 1e-4);

  /*--- Motion of a translated feature of strongly anisotropic, rotated 3D metrics (eigenvalue ratios 1 : 1e4 : 1e10,
   *    aspect ratio 1e5): the invariant is the bump of the isotropic test, so the motion must be found as there. ---*/
  PredictorTest test(3, 12);
  CMetricPredictor predictor(test.Geometry());
  const su2double r = 0.2;
  const su2double cj[3] = {0.4, 0.45, 0.45}, D[3] = {0.08, 0.04, -0.04};
  const su2double ck[3] = {cj[0] + D[0], cj[1] + D[1], cj[2] + D[2]};
  su2double Q[3][3];
  EulerRotation(0.7, 1.1, -0.4, Q);
  auto metric = [&](const su2double* c) {
    return test.Metric([&](const su2double* x, su2double (&M)[3][3]) {
      /*--- det = 1e14 l^3 = 10^(2 s): l = 10^((2 s - 14) / 3). ---*/
      const su2double l = pow(10.0, (2.0 * Bump(3, x, c, r) - 14.0) / 3.0);
      const su2double val[3] = {l, 1e4 * l, 1e10 * l};
      const auto m = PackedMetric(3, Q, val);
      M[0][0] = m[0]; M[0][1] = M[1][0] = m[1]; M[0][2] = M[2][0] = m[2];
      M[1][1] = m[3]; M[1][2] = M[2][1] = m[4]; M[2][2] = m[5];
    });
  };
  const auto Mj = metric(cj), Mk = metric(ck);
  std::vector<su2double> sj(test.nPoint()), sk(test.nPoint());
  su2double maxInvariantError = 0.0;
  for (auto i = 0ul; i < test.nPoint(); ++i) {
    sj[i] = CMetricPredictor::Invariant(3, &Mj[i * 6]);
    sk[i] = CMetricPredictor::Invariant(3, &Mk[i * 6]);
    maxInvariantError = max(maxInvariantError, fabs(sj[i] - Bump(3, test.Coord(i), cj, r)));
  }
  CHECK(maxInvariantError < 1e-4);

  CMetricPredictor::MotionReport report;
  const auto W = predictor.MotionField(sj, sk, 4.0, nullptr, 0.5, report);
  CHECK(report.mismatchAfter < 0.2 * report.mismatchBefore);
  su2double normW = 0.0;
  for (unsigned short i = 0; i < 3; ++i) normW += pow(D[i] / 4.0, 2);
  normW = sqrt(normW);
  su2double maxError = 0.0;
  for (auto iPoint = 0ul; iPoint < test.nPoint(); ++iPoint) {
    su2double d2 = 0.0, e2 = 0.0;
    for (unsigned short i = 0; i < 3; ++i) d2 += pow(test.Coord(iPoint)[i] - ck[i], 2);
    if (d2 > r * r) continue;
    for (unsigned short i = 0; i < 3; ++i) e2 += pow(W[iPoint * 3 + i] - D[i] / 4.0, 2);
    maxError = max(maxError, sqrt(e2));
  }
  CHECK(maxError < 0.08 * normW);
}

TEST_CASE("Metric prediction: filtered history and constant-motion fallback", "[Adaptation][MetricPredictionHistory]") {
  PredictorTest test(2, 24);
  CMetricPredictor predictor(test.Geometry());
  const su2double centre[3] = {1.0, 0.5, 0};
  const std::vector<su2double> times = {-3, -2, -1, 0};
  std::vector<std::vector<su2double>> snapshots;
  for (const auto t : times) {
    const su2double c[3] = {centre[0] + 0.025 * t + 0.5 * 0.006 * t * t,
                          centre[1] + 0.01 * t, 0};
    snapshots.push_back(test.Field([&](const su2double* x) { return Bump(2, x, c, 0.2); }));
  }
  CMetricPredictor::HistoryReport unfiltered, filtered;
  std::vector<su2double> a, af;
  const auto motion = predictor.MotionHistory(snapshots, times, nullptr, 0.5, 0, a, unfiltered);
  const auto smooth = predictor.MotionHistory(snapshots, times, nullptr, 0.5, 1, af, filtered);
  CHECK(unfiltered.pairs.size() == 3);
  CHECK(unfiltered.accelerationKept);
  CHECK(unfiltered.fittedMismatch < unfiltered.constantMismatch);
  CHECK(filtered.fittedMismatch <= filtered.constantMismatch);
  su2double weight = 0, velocity = 0, acceleration = 0, filteredAcceleration = 0;
  for (auto p = 0ul; p < test.nPoint(); ++p) {
    const auto* x = test.Coord(p);
    if (pow(x[0] - centre[0], 2) + pow(x[1] - centre[1], 2) > 0.2 * 0.2) continue;
    weight += predictor.GetMass()[p];
    velocity += predictor.GetMass()[p] * motion[p * 2];
    acceleration += predictor.GetMass()[p] * a[p * 2];
    filteredAcceleration += predictor.GetMass()[p] * af[p * 2];
  }
  CHECK(velocity / weight == Approx(0.025).margin(0.003));
  CHECK(acceleration / weight == Approx(0.006).margin(0.002));
  CHECK(std::abs(filteredAcceleration) < std::abs(acceleration));
  CHECK(smooth.size() == motion.size());

  CMetricPredictor::MotionReport pairReport;
  const auto pair = predictor.MotionField(snapshots[2], snapshots[3], 1, nullptr, 0.5, pairReport);
  CMetricPredictor::HistoryReport pairHistory;
  std::vector<su2double> pairAcceleration;
  const auto fromHistory = predictor.MotionHistory({snapshots[2], snapshots[3]}, {-1, 0}, nullptr, 0.5, 1,
                                                  pairAcceleration, pairHistory);
  CHECK(pair == fromHistory);
  for (const auto v : pairAcceleration) CHECK(v == 0);

  const auto stationary = test.Field([&](const su2double* x) { return Bump(2, x, centre, 0.2); });
  CMetricPredictor::HistoryReport staticReport;
  std::vector<su2double> staticAcceleration;
  const auto noMotion = predictor.MotionHistory({stationary, stationary, stationary}, {-2, -1, 0}, nullptr, 0.5,
                                                1, staticAcceleration, staticReport);
  CHECK_FALSE(staticReport.accelerationKept);
  for (const auto v : noMotion) CHECK(v == 0);
  for (const auto v : staticAcceleration) CHECK(v == 0);
}

TEST_CASE("Metric prediction: accelerating transport preserves SPD and has the correct trajectory",
          "[Adaptation][MetricPredictionHistory]") {
  for (const auto dim : {2, 3}) {
    PredictorTest test(dim, dim == 2 ? 12 : 6);
    CMetricPredictor predictor(test.Geometry());
    const auto metric = test.Metric([&](const su2double* x, su2double (&M)[3][3]) {
      M[0][0] = 10 + x[0]; M[1][1] = 2; if (dim == 3) M[2][2] = 3;
    });
    std::vector<su2double> velocity(test.nPoint() * dim, 0), acceleration(velocity);
    for (auto p = 0ul; p < test.nPoint(); ++p) {
      velocity[p * dim] = 0.01; acceleration[p * dim] = 0.004;
    }
    CMetricPredictor::Options options;
    CMetricPredictor::PredictionReport report;
    std::vector<std::vector<su2double>> instants;
    const auto prediction = predictor.Predict(metric, velocity, {0, 1, 2, 3}, options, report, &instants, &acceleration);
    CHECK(report.nInstant == 4);
    for (auto p = 0ul; p < test.nPoint(); ++p) {
      if (test.Coord(p)[0] < 0.2) continue;
      su2double M[3][3], final[3][3], eigen[3], vectors[3][3], work[3];
      test.Unpack(instants.back(), p, M);test.Unpack(prediction, p, final);
      CHECK(M[0][0] == Approx(10 + test.Coord(p)[0] - (3 * 0.01 + 0.5 * 9 * 0.004)).margin(1e-11));
      CBlasStructure::EigenDecomposition(final, vectors, eigen, dim, work);
      for (unsigned short a = 0; a < dim; ++a) CHECK(eigen[a] > 0);
      CHECK(OrderGap(dim, final, M) >= -1e-12);
    }
    std::vector<su2double> zero(velocity.size(), 0);
    CMetricPredictor::PredictionReport first, second;
    const auto original = predictor.Predict(metric, velocity, {0, 1, 2}, options, first);
    const auto zeroAcceleration = predictor.Predict(metric, velocity, {0, 1, 2}, options, second, nullptr, &zero);
    CHECK(original == zeroAcceleration);

    const auto uniform = test.Metric([&](const su2double*, su2double (&M)[3][3]) {
      M[0][0] = 100; M[1][1] = 2; if (dim == 3) M[2][2] = 3;
    });
    for (auto p = 0ul; p < test.nPoint(); ++p)
      acceleration[p * dim] = -0.02 * (test.Coord(p)[0] - (dim == 2 ? 1 : 0.5));
    std::vector<std::vector<su2double>> strainInstants;
    predictor.Predict(uniform, zero, {0, 1, 2, 3}, options, report, &strainInstants, &acceleration);
    CHECK(report.nCongruence > 0);
    for (auto p = 0ul; p < test.nPoint(); ++p) {
      if (std::abs(test.Coord(p)[0] - (dim == 2 ? 1 : 0.5)) > 0.2) continue;
      su2double M[3][3];
      test.Unpack(strainInstants.back(), p, M);
      CHECK(M[0][0] == Approx(100 * exp(0.02 * 9)).margin(1e-8));
      CHECK(M[1][1] == Approx(2).margin(1e-10));
    }
  }
}

TEST_CASE("Metric prediction: temporal filtering damps jitter in a moving feature",
          "[Adaptation][MetricPredictionHistory]") {
  PredictorTest test(2, 24);
  CMetricPredictor predictor(test.Geometry());
  const std::vector<su2double> times = {-4, -3, -2, -1, 0};
  const su2double jitter[] = {0.008, -0.004, 0.006, -0.003, 0};
  std::vector<std::vector<su2double>> snapshots;
  for (size_t j = 0; j < times.size(); ++j) {
    const su2double centre[3] = {1 + 0.025 * times[j] + jitter[j], 0.5, 0};
    snapshots.push_back(test.Field([&](const su2double* x) { return Bump(2, x, centre, 0.2); }));
  }
  CMetricPredictor::HistoryReport raw, filtered;
  std::vector<su2double> a, af;
  predictor.MotionHistory(snapshots, times, nullptr, 0.5, 0, a, raw);
  predictor.MotionHistory(snapshots, times, nullptr, 0.5, 1, af, filtered);
  su2double rawEnergy = 0, filteredEnergy = 0;
  for (auto p = 0ul; p < test.nPoint(); ++p) {
    const auto* x = test.Coord(p);
    if (pow(x[0] - 1, 2) + pow(x[1] - 0.5, 2) > 0.04) continue;
    for (unsigned short d = 0; d < 2; ++d) {
      rawEnergy += predictor.GetMass()[p] * pow(a[p * 2 + d], 2);
      filteredEnergy += predictor.GetMass()[p] * pow(af[p * 2 + d], 2);
    }
  }
  CHECK(rawEnergy > 0);
  CHECK(filteredEnergy < rawEnergy);
  CHECK(filtered.fittedMismatch <= filtered.constantMismatch);
}

TEST_CASE("Metric prediction: history gather and prediction scatter match the complete mesh",
          "[Adaptation][MetricPredictionMPI]") {
  PredictorTest test(2, 12);
  const auto& geometry = test.Geometry();
  CMeshGather gather(geometry);
  std::vector<std::string> tags;
  for (auto m = 0u; m < geometry.GetnMarker(); ++m) tags.push_back(test.config->GetMarker_All_TagBound(m));
  const auto whole = gather.GatherMesh(*test.config, tags, false);
  auto reference = simplex_test::MakeSimplexMesh(2, 12, simplex_test::Marker2D);
  std::vector<std::array<unsigned long, 3>> cells;
  for (auto e = 0ul; e < reference.GetnElem(); ++e)
    cells.push_back({reference.elem[e * 3], reference.elem[e * 3 + 1], reference.elem[e * 3 + 2]});
  std::sort(cells.begin(), cells.end(), [](auto a, auto b) {
    std::sort(a.begin(), a.end()); std::sort(b.begin(), b.end()); return a < b;
  });
  for (size_t e = 0; e < cells.size(); ++e)
    for (unsigned k = 0; k < 3; ++k) reference.elem[e * 3 + k] = cells[e][k];
  const std::vector<su2double> times = {-3, -2, -1, 0};
  auto invariant = [](const su2double* x, su2double t) {
    const su2double centre[3] = {1 + 0.025 * t + 0.003 * t * t, 0.5, 0};
    return Bump(2, x, centre, 0.2);
  };
  std::vector<std::vector<su2double>> history, referenceHistory;
  for (const auto t : times) {
    const auto local = test.Field([&](const su2double* x) { return invariant(x, t); });
    history.push_back(gather.Gather(local.data(), 1));
    referenceHistory.emplace_back(reference.GetnPoint());
    for (auto p = 0ul; p < reference.GetnPoint(); ++p) {
      su2double x[2] = {reference.coord[p * 2], reference.coord[p * 2 + 1]};
      referenceHistory.back()[p] = invariant(x, t);
    }
  }
  std::vector<su2double> metric(reference.GetnPoint() * 3, 0);
  for (auto p = 0ul; p < reference.GetnPoint(); ++p) {
    metric[p * 3] = 10 + referenceHistory.back()[p]; metric[p * 3 + 2] = 2;
  }
  CMetricPredictor serial(reference);
  CMetricPredictor::HistoryReport expectedFit;
  CMetricPredictor::PredictionReport expectedReport;
  std::vector<su2double> expectedAcceleration;
  const auto expectedMotion = serial.MotionHistory(referenceHistory, times, nullptr, 0.5, 1,
                                                    expectedAcceleration, expectedFit);
  const auto expected = serial.Predict(metric, expectedMotion, {0, 1, 2}, {}, expectedReport, nullptr,
                                        &expectedAcceleration);
  std::vector<su2double> predicted;
  if (gather.IsRoot()) {
    CHECK(whole.coord == reference.coord);
    CHECK(whole.elem == reference.elem);
    CMetricPredictor predictor(whole);
    CMetricPredictor::HistoryReport fit;
    CMetricPredictor::PredictionReport report;
    std::vector<su2double> acceleration;
    const auto motion = predictor.MotionHistory(history, times, nullptr, 0.5, 1, acceleration, fit);
    predicted = predictor.Predict(metric, motion, {0, 1, 2}, {}, report, nullptr, &acceleration);
  }
  std::vector<su2double> local(geometry.GetnPointDomain() * 3);
  gather.Scatter(predicted, 3, local.data());
  for (auto p = 0ul; p < geometry.GetnPointDomain(); ++p)
    for (unsigned m = 0; m < 3; ++m)
      CHECK(local[p * 3 + m] == Approx(expected[geometry.nodes->GetGlobalIndex(p) * 3 + m]).margin(1e-9));
}
