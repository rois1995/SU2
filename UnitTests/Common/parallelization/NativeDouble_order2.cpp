/*!
 * \file NativeDouble_order2.cpp
 * \brief Include order 2 (mpi_structure.hpp first): NativeMpiDouble is the MPI type of a C double.
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

#include "../../../Common/include/parallelization/mpi_structure.hpp"
#include "../../../Common/include/parallelization/CPassiveComm.hpp"

/*--- Size in bytes of the MPI type of a passive double (0 without MPI). ---*/
int NativeDoubleSizeOrder2() {
#ifdef HAVE_MPI
  int bytes = 0;
  MPI_Type_size(NativeMpiDouble(), &bytes);
  return bytes;
#else
  return 0;
#endif
}
