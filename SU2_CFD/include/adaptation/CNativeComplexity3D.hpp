/*!
 * \file CNativeComplexity3D.hpp
 * \brief Bounded original-P1 sensor integral plus actual-query geometric BL correction.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once
#include "CBoundaryLayerMetric.hpp"
#include "../../../Common/include/adaptation/CComplexityNormalization.hpp"
#include "../../../Common/include/adaptation/CNativeMesh3D.hpp"

/*! Geometry-only stencils survive bounded/graded scale trials. Source node indices and ownership
 *  must remain fixed for this object's lifetime; no tensors sampled on an adapted mesh enter it.
 *  Background reuse requires identical current tensors, independent of BL profile/core.
 *  The correction's three empirical standard errors are HEURISTIC, not a confidence/error bound. */
class CNativeComplexity3D {
 public:
  struct SourceCell {
    SU2Native3D::Cell cell;
    std::array<unsigned long,4> nodes;
  };
  struct Assessment {
    SU2Complexity::Estimate global,mandatory;
    double sensorLower=0,sensorUpper=0,sensorQuadrature=0,correction=0,standardError=0,numericalIndicator=0;
    uint64_t samples=0,inside=0,nonzero=0,directChecks=0;
  };
  struct Statistics {
    size_t retainedBytes=0;
    uint64_t samples=0,inside=0,assessments=0,backgroundEvaluations=0,correctionEvaluations=0,profileRefreshes=0,profileGeometryQueries=0;
    uint64_t backgroundBuilds=0,backgroundReuses=0;
    double preparationSeconds=0,assessmentSeconds=0,backgroundSeconds=0,correctionSeconds=0,reductionSeconds=0;
  };
  CNativeComplexity3D(CBoundaryLayerMetric& layers,std::vector<SourceCell> source,size_t sourceNodes,
                     size_t ceiling,size_t sampleCount=8192,uint64_t seed=20261017);
  Assessment Assess(const std::vector<CBoundaryLayerMetric::Tensor>& sensor,su2double core,bool auditDirect=false);
  const Statistics& GetStatistics() const {return statistics;}
 private:
  struct Query {
    std::array<unsigned long,4> nodes;
    std::array<double,4> weights;
    SU2Native3D::Point point;
    double inversePDF;
    std::vector<CBoundaryLayerMetric::PointSample> geometry;
  };
  CBoundaryLayerMetric& layers;
  std::vector<SourceCell> source;
  std::vector<long double> volumes;
  std::vector<Query> queries;
  size_t sourceNodes,ceiling;
  uint64_t profileVersion=0;
  std::vector<CBoundaryLayerMetric::Tensor> backgroundSensor;
  std::array<long double,4> background{};
  bool backgroundValid=false;
  Statistics statistics;
};
