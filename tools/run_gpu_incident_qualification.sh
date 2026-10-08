#!/bin/sh
# Python owns journal parsing, signal handling, and the workload process group.
set -eu
exec python3 "$(dirname -- "$0")/gpu_incident_qualification.py" "$@"
