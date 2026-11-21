/*!
 * \file CNativeLocalSurface3D.hpp
 * \brief Surface geometry and state publication for native local tetrahedral splits.
 * Copyright 2012-2026, SU2 Contributors (cf. AUTHORS.md).
 * SU2 is free software under the GNU Lesser General Public License, version 2.1 or later.
 */
#pragma once

#include "CNativeLocalCore3D.hpp"
#include "CNativeBoundaryParameters3D.hpp"
#include <memory>
#include <optional>

namespace SU2NativeLocal3D {
using SU2Native3D::BoundaryPoint;
using SU2Native3D::BoundaryPoints;
using SU2Native3D::SurfaceFace;
using SurfaceEdge = SU2Native3D::Edge;
using SurfaceFaceKey = SU2Native3D::Face;
constexpr double SurfaceDistanceFactor = 0.7;

struct SurfaceSamples {
  std::array<double, 4> distances{};
  size_t unconverged = 0;
};

struct SurfacePlacement {
  BoundaryPoint point;
  std::optional<SurfaceEdge> parent;
  std::map<SurfaceEdge, SU2NativeGeometry::EdgeTrace> children;
  SurfaceRefusal refusal = SurfaceRefusal::None;
  std::string reason;
  bool Accepted() const { return refusal == SurfaceRefusal::None; }
};

class SurfaceGeometry {
 public:
  virtual ~SurfaceGeometry() = default;
  virtual double Tolerance() const = 0;
  double Limit() const { return SurfaceDistanceFactor * Tolerance(); }
  virtual bool Feature(const SurfaceEdge&) const = 0;
  virtual SurfaceSamples Samples(const SurfaceFace&) = 0;
  virtual std::optional<double> Chord(const std::array<Node, 2>&) = 0;
  virtual SurfacePlacement Place(const std::array<Node, 2>&, const std::array<SurfaceFace, 2>&, Id) = 0;
  virtual bool OnEntity(const SurfaceFace&, const BoundaryPoint& middle) = 0;
  virtual void Accept(SurfacePlacement&&) = 0;
};

struct SurfaceEdits {
  BoundaryPoints points;
  std::vector<SurfaceFace> faces;
  SU2NativeGeometry::BoundaryCoverage coverage;
};

struct SurfaceStateResult {
  std::shared_ptr<SU2Native3D::BoundaryParameterState> state;
  size_t splits = 0, modeled_peak = 0;
  double seconds_gather = 0, seconds_build = 0, seconds_check = 0;
};

class StateSurfaceGeometry final : public SurfaceGeometry {
 public:
  StateSurfaceGeometry(std::shared_ptr<const SU2Native3D::PlanarReference>, SU2NativeGeometry::AtlasBudget);
  ~StateSurfaceGeometry() override;

  double Tolerance() const override;
  bool Feature(const SurfaceEdge&) const override;
  SurfaceSamples Samples(const SurfaceFace&) override;
  std::optional<double> Chord(const std::array<Node, 2>&) override;
  SurfacePlacement Place(const std::array<Node, 2>&, const std::array<SurfaceFace, 2>&, Id) override;
  bool OnEntity(const SurfaceFace&, const BoundaryPoint&) override;
  void Accept(SurfacePlacement&&) override;
  const BoundaryPoint* Find(Id) const;
  const SU2NativeGeometry::EdgeTrace* FindTrace(const SurfaceEdge&) const;
  size_t ResidentBytes() const;
  void SetCallerBytes(size_t);
  size_t Splits() const;
  void Exchange(World&, MemoryBudget&, size_t caller);
  const SU2Native3D::BoundaryParameterState& State() const;
  SurfaceEdits GatherEdits(const std::vector<SurfaceFace>&) const;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

size_t SurfaceAtlasIntegerBits();
SurfaceStateResult BuildSurfaceState(const StateSurfaceGeometry&, const SurfaceEdits&, size_t ceiling, size_t caller,
                                     bool report = true);
SurfaceStateResult BuildSurfaceState(World&, const Owned&, const StateSurfaceGeometry&, MemoryBudget&, size_t caller,
                                     bool report = true);
}  // namespace SU2NativeLocal3D
