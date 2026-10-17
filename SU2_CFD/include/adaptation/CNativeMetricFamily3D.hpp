/*!
 * \file CNativeMetricFamily3D.hpp
 * \brief Original, unclipped sensor frames for explicit frozen normalization stages.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */
#pragma once
#include "CBoundaryLayerMetric.hpp"
#include "../../../Common/include/adaptation/CComplexityNormalization.hpp"
#include "../../../Common/include/adaptation/CNativeMesh3D.hpp"
#include "../../../Common/include/adaptation/CTransferMemory.hpp"
#include <algorithm>
#include <stdexcept>
#include <functional>
#include <memory>

class CGeometry;
class CConfig;
/*! This snapshot belongs to one metric event on an unchanged original CFD mesh. */
struct CNativeMetricFamily3D {
  struct Node { SU2Native3D::Id id=0;SU2Native3D::Point point;size_t local=0; };
  const CGeometry* geometry=nullptr;
  std::vector<Node> nodes; // Sorted global IDs; local indices include halos.
  std::vector<CBoundaryLayerMetric::Tensor> frames,selected;
  std::vector<su2double> eigenvalues;
  std::shared_ptr<CBoundaryLayerMetric> layers;
  std::shared_ptr<const SU2Native3D::PlanarReference> authority;
  double surface_tolerance=0;
  bool Compatible(const CConfig& config)const;
  std::vector<CAdapBoundaryLayer> layer_controls;
  bool boundary_layer=false;
  std::array<double,8> controls{}; // hmin, hmax, ARmax, hgrad, budget, BL policy enabled/protected distance/outer growth.
  SU2Complexity::Normalization normalization;
  SU2Complexity::ProfileSelection budget_selection;
  double lower=0,upper=0,core=0;
  size_t owned=0;
  uint64_t node_span=0;
  size_t ResidentBytes()const {
    return transfer_memory::Add(sizeof(*this),transfer_memory::Bytes(nodes),transfer_memory::Bytes(frames),
        transfer_memory::Bytes(selected),transfer_memory::Bytes(eigenvalues),transfer_memory::Bytes(layer_controls),layers?layers->StorageBytes():0);
  }
  size_t LocalNode(SU2Native3D::Id sourceNode,uint64_t sourceSpan=0)const {
    if(!node_span)throw std::runtime_error("Original metric family has no node identity range.");
    // Generated collars use the physical-source span, which can exclude unused CFD points.
    const auto id=sourceNode%(sourceSpan?sourceSpan:node_span);
    const auto it=std::lower_bound(nodes.begin(),nodes.end(),id,[](const Node& n,auto key){return n.id<key;});
    if(it==nodes.end() || it->id!=id)throw std::runtime_error("Original metric family lacks a source value node.");
    return it->local;
  }
};
/*! Geometry/stencils in assess are fixed; only CURRENT original-node sensor values/core change per trial. */
struct CNativeMetricEvaluation3D {
  std::shared_ptr<const CNativeMetricFamily3D> family;
  std::function<SU2Complexity::Estimate(const std::vector<CBoundaryLayerMetric::Tensor>&,su2double&)> assess;
  std::vector<CBoundaryLayerMetric::Tensor> normalized;
  SU2Complexity::Normalization normalization;
  double core=0,seconds=0;
};
