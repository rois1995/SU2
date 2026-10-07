/*!
 * \file CNativeRemesher.hpp
 * \brief Opt-in owned-triangle remesher using coupled native MPI cavities.
 * \version 8.5.0 "Harrier"
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md)
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */

#pragma once

#include "CRemesher.hpp"
#include "CNativeMesh2D.hpp"
#include <memory>

namespace SU2NativeBoundary2D {
struct ReferenceState;
}

class CNativeRemesher final : public CRemesher {
 public:
  using CompositionFactory = std::function<SU2Native2D::MetricComposition(
      const CConfig&, const CGeometry&, const su2activematrix&)>;
  explicit CNativeRemesher(std::shared_ptr<SU2NativeBoundary2D::ReferenceState> retainedReference,
                           CompositionFactory geometricConstraints = {});
  static void CheckSupport(const CConfig& config, const CGeometry& geometry);
  void PrepareReference(const CConfig& config, const CGeometry& geometry);
  CRemeshResult Remesh(const CConfig& config, const CGeometry& geometry, const su2activematrix& metric) override;

 private:
  unsigned long remeshAttempt = 0;
  CompositionFactory compositionFactory;
  std::shared_ptr<SU2NativeBoundary2D::ReferenceState> reference;
};
