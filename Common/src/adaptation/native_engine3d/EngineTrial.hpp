// Stateless Trial operations; borrow the already-active EngineContext values.
#pragma once

struct EngineContext;

struct EngineTrial {
  static bool HeightCheck(EngineContext&, const Patch&, std::string&, bool provisional = false,
                          FirstHeightReport* measured = nullptr);
  static bool GeometryTrial(EngineContext&, bool valid);
  static bool MetricTrial(EngineContext&, bool valid, MetricDiagnosis& diagnosis, MetricChange kind,
                          bool surface_geometry_progress = false);
  static void SurfaceMerit(EngineContext&, PatchMetric&, const Patch&);
  static Tensor ComposedSample(EngineContext&, Point, Tensor);
};
