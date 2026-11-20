/*!
 * \file CEPMFieldWriter.hpp
 * \brief Writer of the per-rank EPM field files and their JSON layout.
 * \author SU2 Contributors
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

#include <string>

class CConfig;
class CGeometry;
class CEPMFieldSites;

/*!
 * \brief Write the EPM field records of this rank and, on the master rank, the JSON layout.
 * \details Every rank writes sst_epm_field_<objective>_rank<r>.bin in the current directory: one row of
 *          sizeof(Record)/8 little-endian (native) float64 values per site, in the field order of
 *          CEPMFieldSites::Record, without padding, for all sites (sites with uses == 0 included).
 *          The master writes sst_epm_field_<objective>.json that describes the columns and the files.
 *          Only the per-rank record counts are communicated. Collective over SU2_MPI::GetComm().
 * \param[in] field - Rank-local sites and records.
 * \param[in] config - Configuration (reference dimensionalization).
 * \param[in] geometry - Geometry (dimension of the problem).
 * \param[in] objective - Short objective name used in the file names (DRAG or LIFT).
 * \param[in] extras_json - JSON value written under "extract_only_residuals"; empty gives null.
 */
void WriteEPMField(const CEPMFieldSites& field, const CConfig& config, const CGeometry& geometry,
                   const std::string& objective, const std::string& extras_json);
