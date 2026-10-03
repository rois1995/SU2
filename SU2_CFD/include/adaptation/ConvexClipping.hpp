/*!
 * \file ConvexClipping.hpp
 * \brief Clipping of convex polygons (2D) and polyhedra (3D) by half-spaces, for the supermesh of the conservative
 *        solution transfer (mesh adaptation).
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

#include "../../../Common/include/basic_types/datatype_structure.hpp"

/*!
 * \namespace convex_clip
 * \brief Exact clipping of a convex polytope by half-spaces n.x + d >= 0, without tolerances.
 * \note A vertex is removed when its signed distance is negative, kept otherwise (also at exactly zero); a new vertex
 * is a convex combination of the two end points of the cut edge (never an extrapolation). Each clip is independent of
 * the others, so there is no topological decision shared between polytopes that could become inconsistent; degenerate
 * cases (a plane through vertices, along an edge or a face) give duplicate vertices or zero-measure faces, which add
 * nothing to the measure and moments. Round-off errors of the measure are then of the order of the machine precision
 * times the size of the polytope. Callers should use coordinates local to a nearby point. 3D: the vertex graph of r3d
 * (Powell & Abel, J. Comput. Phys. 297, 2015), every vertex has 3 neighbours in a fixed cyclic order, as in the user's
 * Python reference (PolyClip3D_Fun.py).
 */
namespace convex_clip {

/*!
 * \brief Convex polygon, vertices counter-clockwise.
 */
struct Polygon {
  enum : int { MAXV = 32 }; /*!< \brief A triangle clipped by 6 lines has at most 9 vertices. */
  int n = 0;
  passivedouble x[MAXV][2];

  /*! \brief The triangle (a, b, c), either orientation. */
  void InitTriangle(const passivedouble* a, const passivedouble* b, const passivedouble* c) {
    const passivedouble area2 = (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0]);
    const passivedouble* p[3] = {a, area2 >= 0.0 ? b : c, area2 >= 0.0 ? c : b};
    for (int i = 0; i < 3; ++i) {
      x[i][0] = p[i][0];
      x[i][1] = p[i][1];
    }
    n = 3;
  }

  /*!
   * \brief Keep the part with nx x + ny y + d >= 0 (Sutherland-Hodgman, one plane).
   * \return Number of vertices left (0: empty, -1: capacity exceeded).
   */
  int Clip(passivedouble nx, passivedouble ny, passivedouble d) {
    if (n <= 0) return n;
    passivedouble sd[MAXV];
    int nNeg = 0;
    for (int i = 0; i < n; ++i) {
      sd[i] = nx * x[i][0] + ny * x[i][1] + d;
      nNeg += (sd[i] < 0.0);
    }
    if (nNeg == 0) return n;
    if (nNeg == n) return n = 0;
    passivedouble out[MAXV][2];
    int k = 0;
    for (int i = 0; i < n; ++i) {
      const int j = (i + 1) % n;
      const bool keepI = sd[i] >= 0.0, keepJ = sd[j] >= 0.0;
      if (keepI) {
        if (k >= MAXV) return n = -1;
        out[k][0] = x[i][0];
        out[k][1] = x[i][1];
        k++;
      }
      if (keepI != keepJ) {
        if (k >= MAXV) return n = -1;
        const passivedouble f = sd[i] / (sd[i] - sd[j]);
        out[k][0] = x[i][0] + f * (x[j][0] - x[i][0]);
        out[k][1] = x[i][1] + f * (x[j][1] - x[i][1]);
        k++;
      }
    }
    for (int i = 0; i < k; ++i) {
      x[i][0] = out[i][0];
      x[i][1] = out[i][1];
    }
    return n = k;
  }

  /*!
   * \brief Area and centroid (shoelace). The centroid is 0 for an empty polygon.
   * \note The moments are computed relative to the first vertex and the centroid translated back: about a distant
   *       origin the shoelace terms cancel, with a relative error of the area of eps (L/h)^2 for a polygon of size h at
   *       distance L (a tiny overlap far from the frame origin of a coarse element came out with zero area).
   */
  void Moments(passivedouble& area, passivedouble* centroid) const {
    centroid[0] = centroid[1] = 0.0;
    area = 0.0;
    if (n <= 0) return;
    const passivedouble r[2] = {x[0][0], x[0][1]};
    passivedouble a2 = 0.0, gx = 0.0, gy = 0.0;
    for (int i = 1; i + 1 < n; ++i) {
      const passivedouble xi = x[i][0] - r[0], yi = x[i][1] - r[1];
      const passivedouble xj = x[i + 1][0] - r[0], yj = x[i + 1][1] - r[1];
      const passivedouble c = xi * yj - xj * yi;
      a2 += c;
      gx += (xi + xj) * c;
      gy += (yi + yj) * c;
    }
    area = 0.5 * a2;
    if (area != 0.0) {
      centroid[0] = r[0] + gx / (3.0 * a2);
      centroid[1] = r[1] + gy / (3.0 * a2);
    }
  }
};

/*!
 * \brief Convex polyhedron as a vertex graph (r3d): every vertex has 3 neighbours, in an order such that the faces are
 *        traversed consistently.
 */
struct Polyhedron {
  enum : int { MAXV = 64 }; /*!< \brief A tetrahedron clipped by 7 planes has at most 18 vertices (+ new ones). */
  int n = 0;
  passivedouble x[MAXV][3];
  int nbr[MAXV][3];

  /*! \brief The tetrahedron of the 4 points, either orientation. */
  void InitTetrahedron(const passivedouble* const p[4]) {
    passivedouble a[3], b[3], c[3];
    for (int i = 0; i < 3; ++i) {
      a[i] = p[1][i] - p[0][i];
      b[i] = p[2][i] - p[0][i];
      c[i] = p[3][i] - p[0][i];
    }
    const passivedouble det =
        a[0] * (b[1] * c[2] - b[2] * c[1]) - a[1] * (b[0] * c[2] - b[2] * c[0]) + a[2] * (b[0] * c[1] - b[1] * c[0]);
    const int order[4] = {0, det >= 0.0 ? 1 : 2, det >= 0.0 ? 2 : 1, 3};
    for (int i = 0; i < 4; ++i)
      for (int d = 0; d < 3; ++d) x[i][d] = p[order[i]][d];
    /*--- r3d_init_tet connectivity of a positively oriented tetrahedron. ---*/
    const int conn[4][3] = {{1, 3, 2}, {2, 3, 0}, {0, 3, 1}, {1, 2, 0}};
    for (int i = 0; i < 4; ++i)
      for (int k = 0; k < 3; ++k) nbr[i][k] = conn[i][k];
    n = 4;
  }

  /*!
   * \brief Keep the part with nx x + ny y + nz z + d >= 0.
   * \return Number of vertices left (0: empty, -1: capacity exceeded).
   */
  int Clip(passivedouble nx, passivedouble ny, passivedouble nz, passivedouble d) {
    if (n <= 0) return n;
    passivedouble sd[MAXV];
    bool gone[MAXV];
    int nGone = 0;
    for (int v = 0; v < n; ++v) {
      sd[v] = nx * x[v][0] + ny * x[v][1] + nz * x[v][2] + d;
      gone[v] = sd[v] < 0.0;
      nGone += gone[v];
    }
    if (nGone == 0) return n;
    if (nGone == n) return n = 0;

    /*--- A new vertex on every edge from a kept vertex to a removed one. ---*/
    const int nOld = n;
    for (int vk = 0; vk < nOld; ++vk) {
      if (gone[vk]) continue;
      for (int k = 0; k < 3; ++k) {
        const int vr = nbr[vk][k];
        if (!gone[vr]) continue;
        if (n >= MAXV) return n = -1;
        const passivedouble wk = -sd[vr], wr = sd[vk], s = wk + wr;
        for (int c = 0; c < 3; ++c) x[n][c] = (wk * x[vk][c] + wr * x[vr][c]) / s;
        nbr[n][0] = vk;
        nbr[n][1] = nbr[n][2] = -1;
        nbr[vk][k] = n;
        n++;
      }
    }

    /*--- Link the new vertices around the cut faces. ---*/
    for (int vs = nOld; vs < n; ++vs) {
      int vcur = vs, vnext = nbr[vcur][0];
      while (true) {
        int np = 0;
        while (nbr[vnext][np] != vcur) np++;
        vcur = vnext;
        vnext = nbr[vcur][(np + 1) % 3];
        if (vcur >= nOld) break;
      }
      nbr[vs][2] = vcur;
      nbr[vcur][1] = vs;
    }

    /*--- Compact: drop the removed vertices. ---*/
    int newId[MAXV];
    int m = 0;
    for (int v = 0; v < n; ++v) newId[v] = (v < nOld && gone[v]) ? -1 : m++;
    for (int v = 0; v < n; ++v) {
      if (newId[v] < 0) continue;
      const int w = newId[v];
      for (int c = 0; c < 3; ++c) x[w][c] = x[v][c];
      for (int k = 0; k < 3; ++k) nbr[w][k] = newId[nbr[v][k]];
    }
    return n = m;
  }

  /*!
   * \brief Volume and centroid (faces fanned from their first vertex, tetrahedra with the first vertex of the
   *        polyhedron; the centroid is translated back). See the note of Polygon::Moments: about a distant origin the
   *        relative error of the volume is eps (L/h)^3.
   */
  void Moments(passivedouble& volume, passivedouble* centroid) const {
    centroid[0] = centroid[1] = centroid[2] = 0.0;
    volume = 0.0;
    if (n <= 0) return;
    passivedouble y[MAXV][3];
    for (int v = 0; v < n; ++v)
      for (int c = 0; c < 3; ++c) y[v][c] = x[v][c] - x[0][c];
    bool mark[MAXV][3] = {};
    passivedouble six = 0.0, mx = 0.0, my = 0.0, mz = 0.0;
    for (int vs = 0; vs < n; ++vs) {
      for (int ps = 0; ps < 3; ++ps) {
        if (mark[vs][ps]) continue;
        int vcur = vs, pn = ps;
        mark[vcur][pn] = true;
        int vnext = nbr[vcur][pn];
        const passivedouble* x0 = y[vcur];
        int np = 0;
        while (nbr[vnext][np] != vcur) np++;
        vcur = vnext;
        pn = (np + 1) % 3;
        mark[vcur][pn] = true;
        vnext = nbr[vcur][pn];
        while (vnext != vs) {
          const passivedouble* x2 = y[vcur];
          const passivedouble* x1 = y[vnext];
          const passivedouble det = x0[0] * (x1[1] * x2[2] - x1[2] * x2[1]) - x0[1] * (x1[0] * x2[2] - x1[2] * x2[0]) +
                                    x0[2] * (x1[0] * x2[1] - x1[1] * x2[0]);
          six += det;
          mx += det * (x0[0] + x1[0] + x2[0]);
          my += det * (x0[1] + x1[1] + x2[1]);
          mz += det * (x0[2] + x1[2] + x2[2]);
          np = 0;
          while (nbr[vnext][np] != vcur) np++;
          vcur = vnext;
          pn = (np + 1) % 3;
          mark[vcur][pn] = true;
          vnext = nbr[vcur][pn];
        }
      }
    }
    volume = six / 6.0;
    if (six != 0.0) {
      centroid[0] = x[0][0] + mx / (4.0 * six);
      centroid[1] = x[0][1] + my / (4.0 * six);
      centroid[2] = x[0][2] + mz / (4.0 * six);
    }
  }
};

}  // namespace convex_clip
