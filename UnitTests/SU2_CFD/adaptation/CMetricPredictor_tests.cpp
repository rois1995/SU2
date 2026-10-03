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

#include <array>
#include <cmath>
#include <functional>
#include <memory>

#include "../../../Common/include/CConfig.hpp"
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
