#!/bin/bash
# Explicit standalone research build; at most two compiler jobs.
set -e
SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
exec python3 "$SCRIPT_DIR/build_e0.py" "${1:-$SCRIPT_DIR/../build-integrated}" "${2:-$SCRIPT_DIR/bin}"
