#!/bin/bash
set -eu
: "${OMPI_COMM_WORLD_RANK:?}"
: "${NATIVE_FIELD_BINARY:?}"
case "$OMPI_COMM_WORLD_RANK" in 0|1|2|3) ;; *) exit 2 ;; esac
exec gdb --nx --nh --batch -x capture.gdb --args "$NATIVE_FIELD_BINARY" run.cfg > "rank${OMPI_COMM_WORLD_RANK}.log" 2>&1
