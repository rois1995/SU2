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
#include <string>
#include <unordered_map>
#include <vector>

#include "../../../Common/include/adt/CADTElemClass.hpp"
#include "../../../Common/include/option_structure.hpp"

class CConfig;
class CGeometry;

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
 *          - tangential sizes from the wall faces: 2D the length of the closest wall edge, limited near a vertex where
 *            the wall turns to max(h0, 2 r / cos(theta/2)), r the distance to the vertex and theta the angle between
 *            the two edges (only vertices within half the edge length where the wall turns by more than ADAP_ANGLE,
 *            the sharp corners of the remesher; the Python version uses every vertex where the wall is not straight,
 *            which makes the metric isotropic with size h0 at every vertex of a curved wall); 3D a size tensor per face (2/3 of the sum of
 *            e e^T of its edges, i.e. the edge length for an equilateral face; quadrilaterals: 2/4 of the sum over its
 *            four edges), averaged at the vertices (logarithms, weighted by the face angles, rotated to the vertex
 *            normal) and interpolated in the face of the closest point (logarithms, barycentric weights, rotated to
 *            the normal of the point);
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
    su2double distance = 0.0;   /*!< \brief Distance to the wall. */
    su2double weight = 0.0;     /*!< \brief Strength in [0,1] (1 in the full band, 0 beyond the thickness). */
    su2double normal[3] = {0.0}; /*!< \brief Unit wall-normal direction. */
    su2double hn = 0.0;         /*!< \brief Wall-normal size. */
    Tensor full;                /*!< \brief Wall metric at full strength. */
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
   * \brief Wall faces of the markers of ADAP_BL_MARKER, gathered from all ranks (collective call).
   * \param[in] geometry - Geometry (boundary faces with their volume element).
   * \param[in] config - ADAP_BL_* options (the markers must be solid walls); ADAP_ANGLE for the corners.
   */
  CBoundaryLayerMetric(const CGeometry& geometry, const CConfig& config);

  /*!
   * \brief Given wall faces (for tests, or surfaces from elsewhere).
   * \param[in] nDim - Number of dimensions.
   * \param[in] walls - Wall markers.
   * \param[in] cornerAngle - 2D: the tangential size is limited around the vertices where the wall turns by more than
   *            this angle (degrees); 0 for every vertex where the wall is not straight (the rule of the Python version).
   */
  CBoundaryLayerMetric(unsigned short nDim, std::vector<Wall> walls, su2double cornerAngle = 0.0);

  ~CBoundaryLayerMetric();

  /*!
   * \brief Number of walls (sorted by marker name).
   */
  unsigned short GetnWall() const { return walls.size(); }

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

 private:
  struct WallData;
  void Build();

  /*--- Tangential floor of the metric M of a point next to the wall (see the class note); false outside its band. ---*/
  bool TangentialFloor(const Sample& sample, const su2double* wallEigenvalues, su2double h0, Tensor& M) const;

  unsigned short nDim = 0;
  su2double cornerAngle = 0.0; /*!< \brief Turn of the wall (degrees) above which a 2D vertex is a corner. */
  std::vector<Wall> walls;
  std::vector<std::unique_ptr<WallData>> data;
};
