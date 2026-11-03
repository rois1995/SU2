/*!
 * \file CEdge.hpp
 * \brief Declaration of the edge class <i>CEdge.cpp</i> file.
 * \author F. Palacios, T. Economon
 * \version 8.6.0 "Harrier"
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

#include "../../containers/C2DContainer.hpp"

#include <array>
#include <vector>

class CPhysicalGeometry;

/*!
 * \class CEdge
 * \brief Class for defining the edges of the dual grid.
 * \author F. Palacios
 */
class CEdge {
  static_assert(su2activematrix::IsRowMajor, "Needed to return normal as pointer.");

 private:
  using Index = unsigned long;
  using NodeArray = C2DContainer<Index, Index, StorageType::ColumnMajor, 64, DynamicSize, 2>;
  NodeArray Nodes;        /*!< \brief Vector to store the node indices of the edge. */
  su2activematrix Normal; /*!< \brief Normal (area) of the edge. */
  su2activematrix a,b,c; /*!< \brief Normal (area) of the edge. */
  su2activematrix FormNormal, FormE; /*!< \brief Coefficients n_ik and E_ik, allocated only for EDGE_FORM_P1. */
  std::vector<std::array<Index, 2>> SplitNodes; /*!< \brief Node pairs of the split edges that are not true edges. */
  su2activematrix SplitNormal; /*!< \brief Coefficient n_ik of those pairs, allocated only for SPLIT_EDGE_FLUX. */
  su2activematrix SplitE; /*!< \brief Coefficient E_ik of those pairs, allocated only for SPLIT_EDGE_FLUX. */
  su2activematrix SplitBoundNormal; /*!< \brief Per point, outward split area minus outward vertex normal. */
  su2activevector SplitVolume; /*!< \brief Nodal volume of the split, sum_t |t|/(d+1), allocated only for SPLIT_EDGE_FLUX. */
  su2activevector FormSwept; /*!< \brief Term omega_ik of the true edges, allocated only for GCL_EXACT_P1. */
  su2activevector SplitSwept; /*!< \brief Term omega_ik of the split pairs, allocated only for GCL_EXACT_P1. */
  su2activevector BoundSwept; /*!< \brief Per point, nodal boundary term, allocated only for GCL_EXACT_P1. */
  su2activematrix BlendDelta; /*!< \brief Per simplex, W_a - |T|/(d+1) of its nodes, allocated only for EDGE_FORM_ES_BLEND. */
  su2matrix<Index> BlendEdges; /*!< \brief Per simplex, the edge of each node pair a < b, only for EDGE_FORM_ES_BLEND. */
  su2activematrix BlendY; /*!< \brief Per tetrahedron, the moved-dual face tensor Y_T (row major), only for VISC_P1_ENTROPY_MOVED. */
  const Index nEdge, nEdgeSIMD;

  friend class CPhysicalGeometry;

 public:
  enum NodePosition : unsigned long { LEFT = 0, RIGHT = 1 };

  /*!
   * \brief Constructor of the class.
   * \param[in] nEdge - Number of edges
   * \param[in] nDim - Number of dimensions of the problem.
   */
  CEdge(unsigned long nEdge, unsigned long nDim);

  /*!
   * \brief No default construction.
   */
  CEdge() = delete;

  /*!
   * \brief Get left/right node index defining the edge.
   * \param[in] iEdge - Edge index.
   * \param[in] iNode - Node index 0 or 1, LEFT or RIGHT.
   * \return Index of the node that composes the edge.
   */
  inline unsigned long GetNode(unsigned long iEdge, unsigned long iNode) const { return Nodes(iEdge, iNode); }

  /*!
   * \brief SIMD version of GetNode, iNode returned for contiguous iEdges.
   */
  template <class T, size_t N>
  FORCEINLINE simd::Array<T, N> GetNode(simd::Array<T, N> iEdge, unsigned long iNode) const {
    return simd::Array<T, N>(&Nodes(iEdge[0], iNode));
  }

  /*!
   * \brief Sets the tail of "Nodes" to repeat one of the last edges.
   * \note This is needed when using the SIMD version of GetNode and
   *       the number of edges is not a multiple of the simd width.
   */
  void SetPaddingNodes() {
    for (auto iEdge = nEdge; iEdge < nEdgeSIMD; ++iEdge) {
      /*--- Pad nodes by repeating the first edge in the last SIMD group. ---*/
      const auto iEdge0 = nEdgeSIMD - simd::preferredLen<su2double>();
      Nodes(iEdge, LEFT) = Nodes(iEdge0, LEFT);
      Nodes(iEdge, RIGHT) = Nodes(iEdge0, RIGHT);
    }
  }

  /*!
   * \brief Set the node indices of an edge.
   * \param[in] iEdge - Edge index.
   * \param[in] iPoint - Index of left node.
   * \param[in] jPoint - Index of right node.
   */
  inline void SetNodes(unsigned long iEdge, unsigned long iPoint, unsigned long jPoint) {
    Nodes(iEdge, LEFT) = iPoint;
    Nodes(iEdge, RIGHT) = jPoint;
  }

  /*!
   * \brief Get the number of nodes of an edge (2).
   */
  inline unsigned long GetnNodes() const { return 2; }

  /*!
   * \brief Compute the volume associated with an edge (3D version).
   * \param[in] coord_Edge_CG - Coordinates of the centre of gravity of the edge.
   * \param[in] coord_FaceElem_CG - Coordinates of the centre of gravity of the face of an element.
   * \param[in] coord_Elem_CG - Coordinates of the centre of gravity of the element.
   * \param[in] coord_Point - Coordinates of the point that form the control volume.
   * \return Local volume associated to the edge.
   */
  static su2double GetVolume(const su2double* coord_Edge_CG, const su2double* coord_FaceElem_CG,
                             const su2double* coord_Elem_CG, const su2double* coord_Point);

  /*!
   * \brief Compute the volume associated with an edge (2D version).
   * \param[in] coord_Edge_CG - Coordinates of the centre of gravity of the edge.
   * \param[in] coord_Elem_CG - Coordinates of the centre of gravity of the element.
   * \param[in] coord_Point - Coordinates of the point that form the control volume.
   * \return Local volume associated to the edge.
   */
  static su2double GetVolume(const su2double* coord_Edge_CG, const su2double* coord_Elem_CG,
                             const su2double* coord_Point);

  /*!
   * \brief Set the face that corresponds to an edge (3D version).
   * \param[in] iEdge - Edge index.
   * \param[in] coord_Edge_CG - Coordinates of the centre of gravity of the edge.
   * \param[in] coord_FaceElem_CG - Coordinates of the centre of gravity of the face of an element.
   * \param[in] coord_Elem_CG - Coordinates of the centre of gravity of the element.
   * \param[in] config - Definition of the particular problem.
   * \return Compute the normal (dimensional) to the face that makes the control volume boundaries.
   */
  void SetNodes_Coord(unsigned long iEdge, const su2double* coord_Edge_CG, const su2double* coord_FaceElem_CG,
                      const su2double* coord_Elem_CG);

  void SetNodes_CoordCorrection(unsigned long iEdge, const su2double* coord_Edge_CG, const su2double* coord_FaceElem_CG,
                      const su2double* coord_Elem_CG, const su2double* quadraturePoint);

  /*!
   * \brief Set the face that corresponds to an edge (2D version).
   * \param[in] iEdge - Edge index.
   * \param[in] coord_Edge_CG - Coordinates of the centre of gravity of the edge.
   * \param[in] coord_Elem_CG - Coordinates of the centre of gravity of the element.
   * \param[in] config - Definition of the particular problem.
   * \return Compute the normal (dimensional) to the face that makes the contorl volume boundaries.
   */
  void SetNodes_Coord(unsigned long iEdge, const su2double* coord_Edge_CG, const su2double* coord_Elem_CG);

  void SetNodes_CoordCorrection(unsigned long iEdge, const su2double* coord_Edge_CG, const su2double* coord_Elem_CG,
                                const su2double* quadraturePoint);

  /*!
   * \brief Copy the the normal vector of a face.
   * \param[in] iEdge - Edge index.
   * \param[out] normal - Object into which the normal (dimensional) will be copied.
   */
  template <class T>
  inline void GetNormal(unsigned long iEdge, T& normal) const {
    for (auto iDim = 0ul; iDim < Normal.cols(); iDim++) normal[iDim] = Normal(iEdge, iDim);
  }

  template <class T>
  inline void GetCorrection_X(unsigned long iEdge, T& normal) const {
    for (auto iDim = 0ul; iDim < a.cols(); iDim++) normal[iDim] = a(iEdge, iDim);
  }
  template <class T>
  inline void GetCorrection_Y(unsigned long iEdge, T& normal) const {
    for (auto iDim = 0ul; iDim < b.cols(); iDim++) normal[iDim] = b(iEdge, iDim);
  }
  template <class T>
  inline void GetCorrection_Z(unsigned long iEdge, T& normal) const {
    for (auto iDim = 0ul; iDim < c.cols(); iDim++) normal[iDim] = c(iEdge, iDim);
  }

  /*!
   * \brief Get the normal to a face of the control volume asociated with an edge.
   * \param[in] iEdge - Edge index.
   * \return Dimensional normal vector, the modulus is the area of the face.
   */
  inline const su2double* GetNormal(unsigned long iEdge) const { return Normal[iEdge]; }
  inline const su2double* GetCorrection_X(unsigned long iEdge) const { return a[iEdge]; }
  inline const su2double* GetCorrection_Y(unsigned long iEdge) const { return b[iEdge]; }
  inline const su2double* GetCorrection_Z(unsigned long iEdge) const { return c[iEdge]; }

  /*!
   * \brief Get the entire matrix of edge normals.
   */
  inline const su2activematrix& GetNormal() const { return Normal; }
  /*!
   * \brief Coefficient n_ik of the P1 edge form, one row per edge, i is the LEFT node.
   */
  inline const su2activematrix& GetFormNormal() const { return FormNormal; }

  /*!
   * \brief Coefficient E_ik of the P1 edge form, one row per edge, i is the LEFT node.
   */
  inline const su2activematrix& GetFormE() const { return FormE; }

  /*!
   * \brief Node pairs of the split edges that are not true edges, n_ik goes from the first to the second.
   */
  inline const std::vector<std::array<Index, 2>>& GetSplitNodes() const { return SplitNodes; }

  /*!
   * \brief Coefficient n_ik of the split edges that are not true edges, one row per pair.
   */
  inline const su2activematrix& GetSplitNormal() const { return SplitNormal; }

  /*!
   * \brief Coefficient E_ik of the split edges that are not true edges, one row per pair.
   */
  inline const su2activematrix& GetSplitE() const { return SplitE; }

  /*!
   * \brief Per point, outward area of the fans of the boundary faces minus the outward vertex normal.
   */
  inline const su2activematrix& GetSplitBoundNormal() const { return SplitBoundNormal; }

  /*!
   * \brief Nodal volume of the split, equal to the median volume, a check of the weights.
   */
  inline const su2activevector& GetSplitVolume() const { return SplitVolume; }

  /*!
   * \brief Pair term of GCL_EXACT_P1, with the sign of the normal of the edge.
   */
  inline const su2activevector& GetFormSwept() const { return FormSwept; }

  /*!
   * \brief Pair term of GCL_EXACT_P1 of the split edges that are not true edges.
   */
  inline const su2activevector& GetSplitSwept() const { return SplitSwept; }

  /*!
   * \brief Nodal boundary term of GCL_EXACT_P1.
   */
  inline const su2activevector& GetBoundSwept() const { return BoundSwept; }

  /*!
   * \brief Per simplex (row = element index), delta_a = W_a - |T|/(d+1) in the node order of the element.
   *        With g_a the shape gradients, E^T_ab = 0.5 (delta_a g_b + delta_b g_a) and sum_T E^T_ab = E_ab.
   */
  inline const su2activematrix& GetBlendDelta() const { return BlendDelta; }

  /*!
   * \brief Per simplex, the edge of the node pairs (0,1),(0,2),(1,2) in 2D, (0,1),(0,2),(0,3),(1,2),(1,3),(2,3) in 3D.
   */
  inline const su2matrix<Index>& GetBlendEdges() const { return BlendEdges; }

  /*!
   * \brief Per tetrahedron, Y_T = (3|T|/2) sum_o xi_o (x) g_o of the moved dual, Y(iElem, 3 i + j) = Y_ij, so that the
   *        dual facets of node a in T have the inward area vector (|T| I + Y_T^T) g_a (VISC_P1_ENTROPY_MOVED).
   */
  inline const su2activematrix& GetBlendY() const { return BlendY; }

  inline const su2activematrix& GetCorrection_X() const { return a; }
  inline const su2activematrix& GetCorrection_Y() const { return b; }
  inline const su2activematrix& GetCorrection_Z() const { return c; }

  inline void NormalizeCorrection(unsigned long iEdge, const su2double& Area) {
    // PERCHè non lo fa più??
    
//    for (int iDim = 0; iDim < 3; ++iDim) {
//      a(iEdge,iDim) /= Area;
//      b(iEdge,iDim) /= Area;
//      c(iEdge,iDim) /= Area;
//    }
  }

  /*!
   * \brief Initialize normal vector to 0.
   */
  void SetZeroValues(void);

  /*!
   * \brief Set the normal vector of an edge.
   * \param[in] iEdge - Edge index.
   * \param[in] normal - Vector to initialize the normal vector.
   * \return Value of the normal vector.
   */
  template <class T>
  void SetNormal(unsigned long iEdge, const T& normal) {
    for (auto iDim = 0ul; iDim < Normal.cols(); ++iDim) Normal(iEdge, iDim) = normal[iDim];
  }

  /*!
   * \brief Add a vector to the normal vector of an edge.
   * \param[in] iEdge - Edge index.
   * \param[in] normal - Vector to add to the normal vector.
   */
  template <class T>
  void AddNormal(unsigned long iEdge, const T& normal) {
    for (auto iDim = 0ul; iDim < Normal.cols(); ++iDim) Normal(iEdge, iDim) += normal[iDim];
  }

  /*!
   * \brief Subtract a vector to the normal vector of an edge.
   * \param[in] iEdge - Edge index.
   * \param[in] normal - Vector to add to the normal vector.
   */
  template <class T>
  void SubNormal(unsigned long iEdge, const T& normal) {
    for (auto iDim = 0ul; iDim < Normal.cols(); ++iDim) Normal(iEdge, iDim) -= normal[iDim];
  }
};
