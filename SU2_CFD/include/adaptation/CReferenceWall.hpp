/*!
 * \file CReferenceWall.hpp
 * \brief Reference geometry of 2D boundary-layer walls (cycle-0 wall fitted by cubic splines), wall size rule and
 *        wall feasibility diagnostics of the two-pass boundary-layer remesh (ADAP_BL_METHOD= TWO_PASS).
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

#include <array>
#include <string>
#include <vector>

#include "../../../Common/include/adaptation/CSimplexMesh.hpp"

/*!
 * \class CReferenceWall
 * \brief Fixed reference geometry of the 2D boundary-layer walls: the wall of the first mesh, split into feature
 *        segments and fitted by interpolating cubic splines. New wall points are projected onto it, so the wall does
 *        not drift over the adaptation cycles (the substitute for a CAD model, SERIAL_BL_FIX_PLAN.md 2.2).
 * \details
 *  - Each marker's boundary lines are chained into polylines. A chain is split into segments at its open ends, at
 *    points shared with other markers, and at vertices where the wall turns by more than the corner angle (sharp
 *    corners such as a trailing edge). A closed chain without such a vertex is one periodic segment.
 *  - Each segment is interpolated by a cubic spline in the chord-length parameter (natural ends, periodic for a
 *    closed segment). With two points it is the straight line.
 *  - Curvature used by the size rule: max(|curvature of the spline|, 2 sin(turn/2) / L of the polyline at the nearest
 *    knot), so a coarse wall cannot hide a small radius from the rule.
 *  - Projection: the closest polyline interval (brute force over the marker, or over one segment of it), then Newton on
 *    the spline parameter in that interval and its two neighbours; the closest result is kept.
 *  - The fit is a model of the first mesh's wall, not of the true geometry: its accuracy is that of the spline
 *    through the first mesh's points (0.25 h0 on the NACA0012 225x65 wall, 128 points, for h0 = 4e-6).
 */
class CReferenceWall {
 public:
  /*!
   * \brief One smooth piece of a wall marker.
   */
  struct Segment {
    std::string marker;                         /*!< \brief Marker name. */
    bool closed = false;                        /*!< \brief Periodic (closed smooth loop). */
    bool sharpStart = false, sharpEnd = false;  /*!< \brief Ends at a sharp corner (turn above the corner angle). */
    std::vector<passivedouble> s;               /*!< \brief Chord-length parameter of the knots. */
    std::vector<std::array<passivedouble, 2>> x;   /*!< \brief Knots. */
    std::vector<std::array<passivedouble, 2>> m;   /*!< \brief Second derivatives of the spline at the knots. */
    std::vector<passivedouble> turnCurvature;   /*!< \brief 2 sin(turn/2) / mean adjacent edge length at the knots. */
  };

  /*!
   * \brief Closest point of the reference wall.
   */
  struct Projection {
    passivedouble x[2] = {0.0, 0.0};      /*!< \brief Closest point. */
    passivedouble distance = 0.0;         /*!< \brief Distance to it. */
    long segment = -1;                    /*!< \brief Segment index (-1: marker not found). */
    passivedouble s = 0.0;                /*!< \brief Spline parameter. */
    passivedouble tangent[2] = {0.0, 0.0};/*!< \brief Unit tangent. */
    passivedouble curvature = 0.0;        /*!< \brief Curvature used by the size rule (>= 0). */
  };

  CReferenceWall() = default;

  /*!
   * \brief Fit the wall markers of a 2D mesh.
   * \param[in] mesh - 2D mesh (only its points and boundary lines are used).
   * \param[in] markers - Names of the wall markers to fit.
   * \param[in] cornerAngle - Vertices where the wall turns by more than this angle (degrees) end a segment.
   */
  CReferenceWall(const CSimplexMesh& mesh, const std::vector<std::string>& markers, passivedouble cornerAngle);

  /*!
   * \brief Segments of the wall.
   */
  const std::vector<Segment>& GetSegments() const { return segments; }

  /*!
   * \brief Point of a segment at the parameter s, with its first and second derivatives.
   */
  void Evaluate(const Segment& seg, passivedouble s, passivedouble* x, passivedouble* dx = nullptr,
                passivedouble* ddx = nullptr) const;

  /*!
   * \brief Closest point on the segments of a marker.
   * \param[in] point - Point.
   * \param[in] marker - Marker name.
   * \param[in] onlySegment - If >= 0: only this segment (it must belong to the marker), e.g. the own feature segment of
   *            a wall point, which must not jump to another segment of the same marker.
   */
  Projection Project(const passivedouble* point, const std::string& marker, long onlySegment = -1) const;

  /*!
   * \brief Guaranteed upper bound of the largest distance between the reference arc of a segment between two
   *        parameters and the finite chord [a, b] (gate G3 of SERIAL_BL_FIX_PLAN.md 2.1.5).
   * \details On each spline piece spanned by the arc, the signed distance to the chord's line c(s) and the chord
   *          parameter u(s) of the arc are cubics. The distance to the finite chord is exactly sqrt(c^2 + e^2), with the
   *          overshoot e = |b - a| max(0, -u, u - 1); max |c| and max e are exact (sub-interval ends and the real roots
   *          of the quadratic derivatives), and the bound is sqrt(max c^2 + max e^2) per piece plus a rounding margin
   *          (64 eps x the absolute coordinates, 1e-12 x the local cubic terms; independent of a translation). Closed segments: the arc runs from s0 to s1 without wrapping its length
   *          (|s1 - s0| at most the segment length; the parameters may lie outside [0, length]).
   * \return The bound; infinity for a degenerate chord (a = b).
   */
  passivedouble ArcChordDistance(const Segment& seg, passivedouble s0, passivedouble s1, const passivedouble* a,
                                 const passivedouble* b) const;

  /*!
   * \brief Curvature used by the size rule at the parameter s of a segment (see the class note).
   */
  passivedouble Curvature(const Segment& seg, passivedouble s) const;

  /*!
   * \brief Write the reference (knots, segment flags, settings) as text with a fingerprint line.
   */
  void Write(const std::string& filename) const;

  /*!
   * \brief Read a reference written by Write (structure, finite coordinates, distinct knots and the fingerprint are
   *        checked). On failure the object is left unchanged.
   * \param[in] filename - File.
   * \param[out] error - If given: why the file was not accepted (missing, malformed, truncated, fingerprint).
   * \return False if the file does not exist or is not a valid reference file.
   */
  bool Read(const std::string& filename, std::string* error = nullptr);

  /*!
   * \brief Fingerprint: number of segments and knots, a hash of the knot coordinates, the corner angle, and the
   *        marker and feature flags (closed, sharp start, sharp end) of every segment.
   */
  std::string Fingerprint() const;

  /*!
   * \brief Largest distance of the boundary points of the given markers of a mesh from the reference (to detect a
   *        reference that does not belong to the mesh).
   */
  passivedouble MaxDistance(const CSimplexMesh& mesh, const std::vector<std::string>& markers) const;

  /*!
   * \brief Corner angle (degrees) of the segmentation.
   */
  passivedouble GetCornerAngle() const { return cornerAngle; }

 private:
  void Fit(Segment& seg) const;
  void ProjectOnInterval(const Segment& seg, unsigned long i, const passivedouble* p, Projection& best,
                         long iSeg) const;

  std::vector<Segment> segments;
  passivedouble cornerAngle = 45.0;
};

/*!
 * \brief Wall tangential size rule and wall feasibility checks of the two-pass boundary-layer remesh.
 */
namespace BLWallRule {

/*!
 * \brief Target tangential size along one reference segment, sampled at the parameters s.
 */
struct SizeSamples {
  std::vector<passivedouble> s;      /*!< \brief Parameters of the samples. */
  std::vector<passivedouble> size;   /*!< \brief Target size t_w. */
  unsigned long nConflict = 0;       /*!< \brief Samples where the minimum size exceeds the curvature cap. */
  unsigned long nCurvatureLimited = 0; /*!< \brief Samples where the curvature cap is the smallest bound. */
  passivedouble tmin = 0.0;          /*!< \brief t_min of the segment. */
};

/*!
 * \brief A sharp corner of the reference: two segment ends at the same knot, both flagged sharp (one segment twice for
 *        a closed chain with one sharp vertex, e.g. an airfoil with a sharp trailing edge), or ends of two different
 *        markers whose end tangents turn by more than the corner angle (SERIAL_BL_FIX_PLAN.md 11.7).
 */
struct Corner {
  unsigned long seg[2] = {0, 0};      /*!< \brief The two segments. */
  bool atEnd[2] = {false, false};     /*!< \brief The corner is the end (true) or the start (false) of the segment. */
  passivedouble x[2] = {0.0, 0.0};    /*!< \brief Position. */
  bool matched = false;               /*!< \brief Point of the input mesh found and its triangle fan valid (else no
                                           floor). Set by the caller, like the fields below. */
  bool convex = false;                /*!< \brief Fluid-side angle above pi. */
  passivedouble wedge = 0.0;          /*!< \brief Solid wedge angle (radians, convex only), from the two wall edges. */
  passivedouble h0 = 0.0;             /*!< \brief Smallest first height of the two sides. */
  passivedouble tmin = 0.0;           /*!< \brief t_min = max(ADAP_HMIN, 2 h0) of the corner. */
  passivedouble requested = 0.0;      /*!< \brief Requested floor m_c h0 / sin(wedge / 2) (convex only). */
  passivedouble floor = 0.0;          /*!< \brief Effective floor t_c (0: none). */
  passivedouble changed = 0.0;        /*!< \brief Largest distance from the corner of a size sample changed by the
                                           corner rule (see CornerChanges). */
};

/*!
 * \brief What the corner rule changed (SERIAL_BL_FIX_PLAN.md 11.7).
 */
struct CornerChanges {
  unsigned long nSymmetry = 0;        /*!< \brief Samples lowered to the size of the other side of a corner. */
  unsigned long nFloorRaised = 0;     /*!< \brief Samples raised by a convex-corner floor. */
  passivedouble maxFloorRatio = 1.0;  /*!< \brief Largest raise factor of the floor. */
  std::vector<passivedouble> changed; /*!< \brief Per corner: the largest distance from it of a sample the rule
                                           changed (each changed sample counts for the nearest corner of its segment). */
};

/*!
 * \brief Sharp corners of a reference (see Corner). A point where more or fewer than two segment ends meet is not one.
 */
std::vector<Corner> FindCorners(const CReferenceWall& wall);

/*!
 * \brief Effective convex-corner floor: min(requested, hmax, (gradation length / 2 + tmin) / (1 + gradation)), so the
 *        support of the floor profile, t_c + (t_c - tmin) / gradation, stays within half of the shorter adjacent
 *        segment (length); 0 if that is not above tmin.
 */
passivedouble CornerFloor(passivedouble requested, passivedouble hmax, passivedouble length, passivedouble tmin,
                          passivedouble gradation);

/*!
 * \brief Distance from a convex corner where its floor profile reaches tmin (0 without a floor).
 */
passivedouble CornerReach(const Corner& corner, passivedouble gradation);

/*!
 * \brief Gradation |dt/ds| <= gradation along the samples (two sweeps; a closed segment cyclic, twice). Only lowers.
 */
void Grade(SizeSamples& samples, passivedouble gradation, bool closed);

/*!
 * \brief Corner rule on the sampled sizes of all segments (SERIAL_BL_FIX_PLAN.md 11.7):
 *        - symmetry at every corner: within r <= max(2 max(t_A(0), t_B(0)), reach of the floor) of it (r = arc
 *          length from the corner), both sides take min(t_A(r), t_B(r)) respecting each side's t_min, then the
 *          touched segments are graded again; repeat within the initial windows until no sizes change;
 *        - floor at a convex corner with floor t_c > 0: t <- max(t, f(r)), f(r) = max(t_min, t_c - gradation
 *          max(0, r - t_c)); the maximum of two graded profiles is graded.
 */
CornerChanges ApplyCorners(const CReferenceWall& wall, const std::vector<Corner>& corners, passivedouble gradation,
                           std::vector<SizeSamples>& samples);

/*!
 * \brief Settings of the size rule t_w = max(t_min, min(t_sensor, c sqrt(2 R h0), hmax)), graded along the wall
 *        (|dt/ds| <= gradation) and toward sharp corners (t <= max(t_min, cornerGrowth x distance to the corner)).
 */
struct SizeRule {
  passivedouble h0 = 0.0;            /*!< \brief First height of the wall. */
  passivedouble curvatureFactor = 0.7; /*!< \brief c. */
  passivedouble hmin = 0.0;          /*!< \brief t_min (at least 2 h0 and ADAP_HMIN). */
  passivedouble hmax = 1e300;        /*!< \brief Largest size. */
  passivedouble gradation = 0.15;    /*!< \brief Largest |dt/ds|. */
  passivedouble cornerGrowth = 0.5;  /*!< \brief Size growth away from sharp corners. */
};

/*!
 * \brief Sample the size rule along a segment.
 * \param[in] wall - Reference wall.
 * \param[in] iSeg - Segment.
 * \param[in] rule - Settings.
 * \param[in] sensorSize - Tangential size of the sensor metric at a point of the wall (along the given tangent).
 * \param[in] samplesPerInterval - Samples per knot interval.
 */
template <class SensorSize>
SizeSamples SampleSize(const CReferenceWall& wall, unsigned long iSeg, const SizeRule& rule,
                       const SensorSize& sensorSize, unsigned short samplesPerInterval = 4);

/*!
 * \brief Size at a parameter (linear interpolation of the samples).
 */
passivedouble SizeAt(const SizeSamples& samples, passivedouble s);

/*!
 * \brief Wall feasibility per boundary line of a marker: L sin(turn/2) with L the line length and turn the larger
 *        turn of the wall at its two ends, ignoring vertices whose turn exceeds the corner angle (sharp corners).
 *        The first cell of a metric-conforming mesh on this line cannot be thinner than about this value (the floor
 *        of CMMGInterface::FloorFixedBoundaryMetric raises the normal size to it).
 * \return One value per line of the marker, in the order of its elements.
 */
std::vector<passivedouble> NormalExtent(const CSimplexMesh& mesh, const std::string& marker,
                                        passivedouble cornerAngle);

/*!
 * \brief Robust orientation of a triangle (gate G4): sign of (b - a) x (c - a), exact (a floating-point filter, then
 *        exact expansion arithmetic with fused multiply-add products when the filter cannot decide).
 * \return 1 counterclockwise, -1 clockwise, 0 collinear.
 */
int Orientation(const passivedouble* a, const passivedouble* b, const passivedouble* c);

}  // namespace BLWallRule

#include "CReferenceWall.inl"
