/*!
 * \file AllocationProbe.hpp
 * \brief Allocation probe of the memory tests (test_memory): every form of the global operator new/delete is replaced
 *        and counts the requested bytes alive (live) and their largest value (peak).
 * \note The requested size is kept in a header in front of each block, so sized and unsized deletes are exact and the
 *       bookkeeping allocates nothing. Allocations of the C library (malloc, e.g. inside MPI) are not seen. The
 *       usable bytes of the allocator (with its rounding) are counted separately where available, for information.
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

#include <cstddef>

namespace alloc_probe {

/*! \brief Requested bytes alive now (all-time counter, never reset). */
size_t Live();
/*! \brief Largest Live() since the last ResetPeak(). */
size_t Peak();
/*! \brief Start a measurement window: Peak() = Live(). */
void ResetPeak();
/*! \brief Whether usable-byte reporting is available (otherwise the usable counters return zero). */
bool HasUsable();
/*! \brief The same for the usable bytes of the allocator, where available. */
size_t LiveUsable();
size_t PeakUsable();
/*! \brief Number of operator new calls (all forms) so far. */
size_t Calls();

}  // namespace alloc_probe
