/*!
 * \file CBoundaryLayerMetric.hpp
 * \brief Boundary-layer metric of wall markers for mesh adaptation (wall-normal growth, tangential wall sizes).
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

#pragma once

#include <memory>
#include <functional>
#include <stdexcept>
#include <array>
#include <cstdint>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "../../../Common/include/adt/CADTElemClass.hpp"
#include "../../../Common/include/option_structure.hpp"

class CConfig;
class CGeometry;
namespace SU2NativeBoundary2D { struct ReferenceState; }
namespace SU2Native3D { struct ReferenceState; class PlanarReference; class SupportSampler3D; }

/*!
 * \class CBoundaryLayerMetric
 * \brief Metric of the boundary layer of wall markers (ADAP_BL_*), intersected with the adaptation metric.
 * \details Each wall marker has a first height h0, a growth ratio g of geometric cell rows and a thickness T. At a point
 *          at distance d from the wall (closest point of the wall faces, exact nearest-face search of an ADT):
 *          - wall-normal size hn = max(h0, 2 (h0 + (g - 1) d) / (g + 1)): the rows of a geometric layer have heights
 *            h0 g^k, the row that starts at distance d has height h0 + (g - 1) d, and a node between two rows gets the
 *            harmonic mean of the two;
 *          - normal direction: towards the closest point of the wall (radial around convex edges and corners of the
 *            wall); at a wall point, the normal of the face, of the edge (sum of the two face normals) or of the
 *            vertex (2D: bisector of the two edges; 3D: angle-weighted face normals);
 *          - tangential sizes from the wall faces: 2D the length of the closest wall edge; 3D a size tensor per face (2/3 of the sum of
 *            e e^T of its edges, i.e. the edge length for an equilateral face; quadrilaterals: 2/4 of the sum over its
 *            four edges), averaged at the vertices (logarithms, weighted by the face angles, rotated to the vertex
 *            normal) and interpolated in the face of the closest point (logarithms, barycentric weights, rotated to
 *            the normal of the point);
 *          - D1 crease-frame law (BL_REMESH_PLAN WP-A, shared 2D/3D; it replaces the former convexity-blind 2D corner
 *            limit): a feature is a convex wall edge (3D) or vertex (2D) where the wall turns by more than ADAP_ANGLE,
 *            detected on the original faces of all BL walls and the adjacent non-BL physical faces (a symmetry plane
 *            is a mirrored continuation). Across a crease segment the size is hn_f(rho) = h0_f + ln(ADAP_HGRAD) rho,
 *            rho the distance to the segment and h0_f the smallest first height of the walls meeting there; along the
 *            crease the metric keeps its own size (F = (I - t t^T) / hn_f^2); corners and 2D feature vertices are
 *            isotropic (F = I / hn_f^2). Each wall sample is intersected with its features in a fixed canonical order
 *            BEFORE the fade (full and logarithm replaced), so every consumer of a sample sees the same law;
 *          - full strength up to max(h0, 0.9 T) from the wall, then a smoothstep fade to 0 at T (never inside h0);
 *            where it fades, the eigenvalues are interpolated in logarithm (weight w) towards the isotropic metric of
 *            the largest size of the adaptation metric (its smallest eigenvalue over the whole mesh);
 *          - each wall is intersected separately with the metric (in the order of the marker names), at the points
 *            where its weight is positive, so a wall cannot weaken another one or the finer directions of the metric;
 *          - tangential floor (not in the Python version): next to the wall, up to max(h0, floorFraction x the largest
 *            tangential size of the wall metric) from it, the tangential sizes of the result are at least those of
 *            the wall metric (union of the tangential blocks in the frame of the wall normal, the normal size of the
 *            intersection kept). Where the sensor asks for finer tangential sizes than the wall faces, MMG otherwise
 *            fills the cells on the wall faces with nodes very close to the wall (fixed surface: the faces cannot be
 *            split; adapted surface: it splits the wall into near-isotropic faces of the normal size).
 *          This is a port of BoundaryLayerMetric_Fun.py (with wallMetric2D and wallMetric3D) of the MeshAdaptation
 *          scripts. The faces of the walls are gathered from all ranks, so each rank can evaluate any of its points.
 */
class CBoundaryLayerMetric {
 public:
  /*!
   * \brief Faces of one wall marker and its boundary-layer options.
   */
  struct Wall {
    CAdapBoundaryLayer layer;        /*!< \brief Marker name, first height, growth, thickness. */
    std::vector<su2double> coord;    /*!< \brief Coordinates of the wall points (nDim per point). */
    std::vector<unsigned long> conn; /*!< \brief Points of the faces (2D: 2 per line; 3D: 4 per face, a triangle
                                          repeats its last point). 3D faces must be oriented consistently. */
    std::shared_ptr<const SU2Native3D::PlanarReference> authority;
    uint64_t authority_facet=0; /*!< Complete analytic component; absent authority keeps faceted behavior. */
    double surface_tolerance=0; /*!< Frozen physical geometry ceiling in mesh units, independent of first height. */
  };

  /*!
   * \brief Metric tensor (the first nDim x nDim entries are used).
   */
  struct Tensor {
    su2double m[3][3] = {{0.0}};
  };

  /*!
   * \brief Wall metric of one wall at one point, before the intersection.
   */
  struct Sample {
    uint64_t profile_version = 0;
    su2double distance = 0.0;   /*!< \brief Distance to the wall. */
    su2double weight = 0.0;     /*!< \brief Strength in [0,1] (1 in the full band, 0 beyond the thickness). */
    su2double normal[3] = {0.0}; /*!< \brief Unit wall-normal direction. */
    su2double hn = 0.0;         /*!< \brief Wall-normal size. */
    Tensor full;                /*!< \brief Wall metric at full strength. */
    Tensor logarithm;           /*!< \brief 3D log metric formed before mixing normal/tangent scales. */
    Tensor metric;              /*!< \brief Wall metric with the fade (equal to full where weight is 1). */
  };

  /*!
   * \brief Statistics of Apply for one wall (sums and extrema over the points of this rank).
   */
  struct WallReport {
    std::string name;                 /*!< \brief Marker name. */
    unsigned long nPoint = 0;         /*!< \brief Points where the wall metric has a positive weight. */
    unsigned long nChanged = 0;       /*!< \brief Points where the intersection changed the metric (finer). */
    su2double maxAspectRatio = 0.0;   /*!< \brief Largest aspect ratio of the full-strength wall metric. */
    unsigned long nFloor = 0;         /*!< \brief Points with the tangential floor next to the wall. */
  };

  static constexpr passivedouble floorFraction = 0.05; /*!< \brief Band of the tangential floor, fraction of the
                                                            largest tangential size of the wall metric. */

  /*!
   * \brief One original physical boundary face for D1 feature detection (nDim points: 2D a segment with the fluid on
   *        its left, 3D a triangle or quadrilateral with its normal pointing out of the fluid). wall is the BL marker
   *        name, empty for a non-BL "partner" face; reflective marks a symmetry plane (mirrored continuation).
   */
  struct FeatureFace {
    std::vector<su2double> coord;
    std::string wall;
    bool reflective = false;
  };
  /*!
   * \brief D1 options. hgrad > 1 (ADAP_HGRAD) enables the law; the test constructor default (0) disables it.
   *        faces: non-BL partner faces and, with wallFacesIncluded, the complete BL wall faces (otherwise the wall faces
   *        are taken from the walls, which must then be complete and fluid-oriented).
   */
  struct FeatureOptions {
    double hgrad = 0.0;
    std::vector<FeatureFace> faces;
    bool wallFacesIncluded = false;
  };
  /*! Detected D1 feature set (identical on every rank: detection uses replicated original geometry). */
  struct FeatureSummary {
    size_t segments = 0, images = 0, corners = 0, imageCorners = 0, carrierEnds = 0, carrierCorners = 0,
           freeCorners = 0, planes = 0;
    double angle = 0, slope = 0, maximumRadius = 0;
    uint64_t identity = 0;
  };

  /*!
   * \brief Wall faces of the markers of ADAP_BL_MARKER, gathered from all ranks (collective call).
   * \param[in] geometry - Geometry (boundary faces with their volume element).
   * \param[in] config - ADAP_BL_* options (the markers must be solid walls); ADAP_ANGLE for the corners.
   */
  CBoundaryLayerMetric(const CGeometry& geometry, const CConfig& config);

  /*!
   * \brief Given wall faces (for tests, or surfaces from elsewhere).
   * \param[in] nDim - Number of dimensions.
   * \param[in] walls - Wall markers.
   * \param[in] cornerAngle - D1 feature angle (degrees, ADAP_ANGLE): convex edges/vertices turning by more than it.
   * \param[in] features - D1 options (disabled by default).
   */
  CBoundaryLayerMetric(unsigned short nDim, std::vector<Wall> walls, su2double cornerAngle,
                       FeatureOptions features);
  /*! D1 disabled (tests and harness replays). */
  CBoundaryLayerMetric(unsigned short nDim, std::vector<Wall> walls, su2double cornerAngle = 0.0);

  /*! D1 feature set and its identity (canonical order, parameters); empty when D1 is disabled. */
  const FeatureSummary& GetFeatures() const { return featureSummary; }

  ~CBoundaryLayerMetric();

  struct OuterNormalProfile {
    double protected_distance=0,maximum_growth=0,parameter=0;
    uint64_t version=0;
    bool frozen=false;
  };
  void ConfigureOuterNormal(double protectedDistance,double maximumGrowth);
  void SelectOuterNormal(double parameter);
  void FreezeOuterNormal() { outerNormal.frozen=true; }
  const OuterNormalProfile& GetOuterNormal() const { return outerNormal; }
  /*! Native normal-ray tensor transport beyond the full-strength band. Configure after outer-normal limits.
   * Support covers all points that can exceed this core; subsequent queries may use this core or a finer one.
   * Returns true when cached geometry samples/support must be rebuilt. MMG retains its legacy fade. */
  bool ConfigureOuterTransition(double coreEigenvalue, double hgrad);
  uint64_t GetSampleVersion() const { return sampleVersion; }
  uint64_t GetSupportVersion() const { return supportVersion; }
  double GetOuterTransitionCore() const { return transitionCore; }


  /*!
   * \brief Number of walls (sorted by marker name).
   */
  unsigned short GetnWall() const { return walls.size(); }
  /*! Bounds of the immutable physical wall coordinates, without metric support padding. */
  std::array<double,6> WallBounds() const;

  /*!
   * \brief Boundary-layer options of a wall.
   */
  const CAdapBoundaryLayer& GetLayer(unsigned short iWall) const { return walls[iWall].layer; }

  /*!
   * \brief Wall metric of a wall at a point.
   * \param[in] iWall - Wall (sorted by marker name).
   * \param[in] coord - Coordinates of the point.
   * \param[in] coreEigenvalue - Eigenvalue of the isotropic metric the fade goes to.
   * \return The sample; Sample::weight is 0 beyond the thickness (the metric is then not set).
   */
  Sample Evaluate(unsigned short iWall, const su2double* coord, su2double coreEigenvalue);

  /*! Nearest-facet ownership along one chord, taken from the same faceted 3D wall-distance envelope that produced
   *  its cuts (EdgeBreakpoints). Per wall: ascending piece ends with the owning wall facet; a facet owns itself, a
   *  segment or vertex maps to its lowest candidate facet. NoFacet (support cap, analytic authority, 2D) keeps the
   *  pointwise nearest-facet rule. Pinned evaluation removes rounding-level owner flips between distance-tied facets
   *  inside one envelope piece, so the composed edge integrand is piecewise smooth between cuts.
   *  Volume samples (complexity quadrature, mandatory density, SamplePoints) keep the pointwise rule (smallest
   *  rounded distance, then lowest facet index): it is a deterministic function of the position and the replicated
   *  wall. Facets tied because they share the closest edge/vertex give the same foot, normal and interpolated tangent
   *  tensor up to rounding, so the choice changes the integrand by rounding only; genuine owner switches lie on
   *  bisector surfaces of zero volume. Only a 1D chord integral can run along a tied stretch, which is what the
   *  pinning removes. */
  struct ChordOwners {
    static constexpr unsigned long NoFacet = ~0UL;
    struct Piece { long double hi; unsigned long facet; };
    std::vector<std::vector<Piece>> walls;
    /*! Owning facet of a wall at chord parameter t in [0,1] (left piece on a shared end). */
    unsigned long Owner(unsigned short wall, long double t) const;
    size_t Pieces() const;
  };

  /*!
   * \brief Intersect a metric field with the wall metric of every wall.
   * \param[in] coord - Coordinates of the points (nDim per point).
   * \param[in,out] metric - Metric of each point (symmetric positive definite).
   * \param[in] coreEigenvalue - Eigenvalue of the isotropic metric the fade goes to (the smallest eigenvalue of the
   *            metric over the whole mesh).
   * \return Statistics of each wall (local to this rank).
   */
  std::vector<WallReport> Apply(const std::vector<su2double>& coord, std::vector<Tensor>& metric,
                                su2double coreEigenvalue);

  /*! Apply geometric constraints at a query point, without interpolating their
   *  samples on a coarse donor mesh. Reports are optional for remesher queries.
   *  Disabling the legacy tangential floor retains the intersection with the
   *  sensor: its hard band switch is unsuitable for a query-time target. */
  void ApplyPoint(const su2double* coord, Tensor& metric, su2double coreEigenvalue,
                  std::vector<WallReport>* reports = nullptr, bool useTangentialFloor = true);
  /*! ApplyPoint at a point of a chord whose ownership came from EdgeBreakpoints(a,b,...,&owners); t is the
   *  chord parameter of coord. Each faceted wall evaluates the facet owning t in that envelope (the nearest feature's
   *  facet, authoritative over the pointwise double tie-break); otherwise identical to ApplyPoint. */
  void ApplyPointOnChord(const su2double* coord, Tensor& metric, su2double coreEigenvalue,
                         const ChordOwners& owners, long double t, bool useTangentialFloor = false);

  /*! Original wall geometry is complete on every rank, including remote/private queries. */
  static std::shared_ptr<CBoundaryLayerMetric> FromNativeReference(
      const SU2NativeBoundary2D::ReferenceState& reference, const CConfig& config);
  /*! featureAngle < 0: ADAP_ANGLE; otherwise the D1 feature angle used instead (FIRST_ROW base layers: an angle no
   *  wall edge exceeds leaves every other BL ingredient unchanged and detects no D1 feature). */
  static std::shared_ptr<CBoundaryLayerMetric> FromNativeReference(
      const SU2Native3D::ReferenceState& reference, const CConfig& config, double featureAngle = -1);

  /*! Shared stable sensor-core eigenvalue for complexity trials and frozen query composition. */
  static double CoreEigenvalue2D(double xx, double xy, double yy);

  struct PointSample { unsigned long point; unsigned short wall; Sample sample; };
  struct IntegrationPoint2D {
    std::array<double, 3> barycentric;
    double weight;
    std::vector<PointSample> samples;
  };
  /*! Split donor triangles into geometric distance bands before positive degree-two quadrature.
   *  Sensor values use original-cell barycentrics; wall tensors are evaluated at the actual sample.
   *  The band ratio can be tightened to audit quadrature convergence without changing the target.
   */
  std::vector<IntegrationPoint2D> IntegrationRule2D(
      const std::array<std::array<double, 2>, 3>& triangle, double bandRatio = 1.5);

  struct IntegrationPoint3D {
    std::array<double,4> barycentric;
    double weight;
    std::vector<PointSample> samples;
  };
  using IntegrationDensity3D = std::function<double(const IntegrationPoint3D&)>;
  /*! Positive quadrature after geometric distance-band clipping, using original-cell barycentrics.
   *  Visible-band and hidden-support refinement estimate error in the supplied full target density;
   *  absent a callback, it monitors the BL composed with a unit isotropic sensor.
   *  Work/depth/point bounds reject unresolved integration; no nodal BL interpolation. */
  std::vector<IntegrationPoint3D> IntegrationRule3D(
      const std::array<std::array<double,3>,4>& tetrahedron, double bandRatio = 1.5,
      IntegrationDensity3D density = {}, bool* sensorDependent = nullptr);

  struct IntegrationLimit : std::runtime_error { using std::runtime_error::runtime_error; };
  bool SupportsCorrectionSupport3D() const;
  /*! Roundoff-level coplanarity classification for integration selection only; never changes wall geometry. */
  bool PlanarWalls3D() const;

  /*! Complete faceted-wall support proposal, independent of the sensor/scale. Analytic authority retains
   * its existing integration path. includeTransition=false proposes only the requested wall band; a caller
   * must then cover any outer continuation separately. Limits bound work/storage, not metric or integration error. */
  std::shared_ptr<SU2Native3D::SupportSampler3D> PrepareCorrectionSupport3D(size_t maximumBytes,uint64_t maximumWork,
                                                                         bool includeTransition = true) const;

  struct ComplexityEstimate3D {
    double value=0, indicator=0;
    size_t evaluations=0, panels=0, preparedChords=0, reusedChords=0, cachedBytes=0;
    double seconds=0, preparationSeconds=0, densitySeconds=0;
    bool limited=false, unresolvedSupport=false;
  };
  struct ComplexityWorkspace3D;
  /*! Geometry-only workspace tied to this wall model and one original cell. The ceiling counts retained
   *  payload/tree-node estimates, excluding allocator overhead; it does not bound transient preparation/RSS. */
  std::shared_ptr<ComplexityWorkspace3D> PrepareComplexity3D(
      const std::array<std::array<double,3>,4>& tetrahedron, size_t maximumBytes);
  /*! Bounded scalar estimate of the actual-query field. Indicator is heuristic, NOT an error bound.
   *  An unsampled possible wall support has infinite uncertainty. Complete parent contributions
   *  survive an exhausted refinement budget. No rule from this interface certifies a mesh. */
  ComplexityEstimate3D EstimateComplexity3D(const std::array<std::array<double,3>,4>& tetrahedron,
      IntegrationDensity3D density, size_t maximumEvaluations=8192, double absoluteTolerance=-1,
      ComplexityWorkspace3D* workspace=nullptr);

  /*! Sorted chord parameters resolving every faceted-wall distance band in 2D/3D.
   *  Face/edge/vertex offset intersections detect a thin band even when both endpoints
   *  and the midpoint are outside it. Used with actual-point composition, never nodal BL tensors. */
  std::vector<double> EdgeBreakpoints(const std::array<double, 3>& a, const std::array<double, 3>& b,
                                      double bandRatio = 1.5, unsigned fadeBands = 16, ChordOwners* owners = nullptr);

  /*! Retained vectors/maps/tree plus first-thread tree workspace growth; excludes allocator/RSS and construction peak. */
  size_t StorageBytes() const;
  std::vector<PointSample> SamplePoints(const std::vector<su2double>& coord);
  /*! Same intersection/fade policy for cached integration samples and actual remesher queries. */
  void ApplySample(unsigned short wall, const Sample& sample, Tensor& metric, su2double coreEigenvalue,
                   std::vector<WallReport>* reports = nullptr, bool useTangentialFloor = true);

 private:
  Sample EvaluateImpl(unsigned short wall, const su2double* coord, su2double coreEigenvalue, bool distanceOnly,
                      unsigned long pinned = ChordOwners::NoFacet);
  std::vector<double> EdgeBreakpointsImpl(const std::array<double,3>& a,const std::array<double,3>& b,
      double bandRatio,unsigned fadeBands,bool featureTransitions=true,ChordOwners* owners=nullptr);
  std::vector<IntegrationPoint3D> IntegrationRule3DImpl(
      const std::array<std::array<double,3>,4>& tetrahedron, double bandRatio,
      IntegrationDensity3D density, bool* sensorDependent, ComplexityEstimate3D* estimate, size_t maximumEvaluations,
      double absoluteTolerance=-1, ComplexityWorkspace3D* workspace=nullptr);
  std::vector<IntegrationPoint2D> IntegrationRule2DImpl(
      const std::array<std::array<double, 2>, 3>& triangle, double bandRatio, unsigned depth);
  struct SupportFace3D {
    std::array<std::array<double,3>,3> vertices;
    double thickness;
    unsigned short wall;
  };
  struct Column3D {
    std::array<std::array<double,4>,3> entry{},exit{};
    std::array<std::array<double,3>,3> projection{};
    std::array<long double,3> axial{};
    long double area=0;
    bool collapsed=false;
  };
  std::vector<IntegrationPoint3D> IntegrationRule3DVisible(const std::array<std::array<double,3>,4>& tetrahedron,
      const std::vector<SupportFace3D>& support,double bandRatio,const IntegrationDensity3D& density,
      ComplexityEstimate3D* estimate=nullptr, size_t maximumEvaluations=0,
      double absoluteTolerance=-1, ComplexityWorkspace3D* workspace=nullptr);
  std::vector<IntegrationPoint3D> IntegrationColumns3D(const std::array<std::array<double,3>,4>& tetrahedron,
      const std::vector<Column3D>& columns,const std::vector<SupportFace3D>& support,double bandRatio,
      const IntegrationDensity3D& density, ComplexityEstimate3D* estimate=nullptr, size_t maximumEvaluations=0,
      double absoluteTolerance=-1, ComplexityWorkspace3D* workspace=nullptr);
  struct WallData;
  void Build();

  /*--- D1 crease-frame law (see the class note). ---*/
  struct Feature {
    std::array<double, 3> a{}, b{}, t{};  /*!< Segment end points (canonical a < b) and unit tangent; corner: a = b. */
    double length = 0, h0 = 0;
    bool corner = false, image = false;
    std::vector<unsigned short> walls;   /*!< BL walls (sorted indices) whose samples receive this feature. */
  };
  struct FeaturePlane {
    std::array<long double, 3> normal{}, origin{};
    std::array<std::array<double, 3>, 3> face{};
    long double tolerance = 0;  /*!< Distance below which a point lies on the carrier (rounding of a declared plane). */
  };
  void BuildFeatures(FeatureOptions options);
  void IndexFeatures();
  double FeatureRadius(unsigned short wall, double h0) const;
  void ApplyFeatures(unsigned short wall, const su2double* coord, Sample& sample) const;
  bool FeatureRefinement2D(const std::array<std::array<double, 2>, 3>& triangle, double bandRatio) const;
  double featureSlope = 0;                 /*!< ln(ADAP_HGRAD); 0 disables D1. */
  std::vector<Feature> baseFeatures;       /*!< Detected original segments and corners (no images). */
  std::vector<FeaturePlane> featurePlanes; /*!< Reflective carrier planes/lines touching the BL walls. */
  std::vector<Feature> features;           /*!< Canonical order: segments, image segments, corners, image corners. */
  std::vector<double> featureHmax;         /*!< Per wall: largest tangential size (pre-fade A >= featureHmax^-2 I). */
  double featureCell = 0;
  std::map<std::array<long long, 3>, std::vector<unsigned>> featureGrid;
  FeatureSummary featureSummary;

  /*--- Tangential floor of the metric M of a point next to the wall (see the class note); false outside its band. ---*/
  bool TangentialFloor(const Sample& sample, const su2double* wallEigenvalues, su2double h0, Tensor& M) const;

  OuterNormalProfile outerNormal;
  double transitionCore=0, transitionSlope=0;
  uint64_t sampleVersion=0, supportVersion=0;
  unsigned short nDim = 0;
  su2double cornerAngle = 0.0; /*!< \brief D1 feature angle (degrees): convex edges/vertices turning by more. */
  std::vector<Wall> walls;
  std::vector<std::unique_ptr<WallData>> data;
};
