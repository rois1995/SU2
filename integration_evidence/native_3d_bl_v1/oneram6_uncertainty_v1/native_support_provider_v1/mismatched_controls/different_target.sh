#!/bin/sh
if [ "${OMPI_COMM_WORLD_RANK:-0}" = 0 ]; then export SU2_NATIVE_BL_NORMALIZE=17; fi
exec "${SU2_NATIVE_TEST_DRIVER:?}" "$@"
