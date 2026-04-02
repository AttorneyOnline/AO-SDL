#!/usr/bin/env bash
# profile-idle.sh — Attach to a running aosdl process and capture CPU samples.
#
# Usage:
#   ./scripts/profile-idle.sh [duration_seconds] [output_dir]
#
# The app must already be running. This script finds it by name,
# samples it for the given duration (default: 10s), and writes
# both a raw sample file and a parsed hot-functions summary.
#
# Tools used:
#   - `sample` (built-in macOS) for call-tree sampling
#   - `xctrace` (Instruments CLI) for Time Profiler traces (optional, pass --instruments)

set -euo pipefail

DURATION="${1:-10}"
OUTPUT_DIR="${2:-/Users/saturday/Documents/AO-SDL/profiles}"
PROCESS_NAME="aosdl"
USE_INSTRUMENTS=false

# Parse flags
for arg in "$@"; do
    case "$arg" in
        --instruments) USE_INSTRUMENTS=true ;;
    esac
done

mkdir -p "$OUTPUT_DIR"

# --- Find the process ---
PID=$(pgrep -x "$PROCESS_NAME" 2>/dev/null | head -1) || true
if [[ -z "$PID" ]]; then
    echo "ERROR: No running process named '$PROCESS_NAME' found."
    echo "Launch the app first, then re-run this script."
    exit 1
fi

TIMESTAMP=$(date +%Y%m%d_%H%M%S)
echo "=== aosdl idle CPU profiler ==="
echo "PID:       $PID"
echo "Duration:  ${DURATION}s"
echo "Output:    $OUTPUT_DIR"
echo ""

# --- sample-based profiling (always runs) ---
SAMPLE_RAW="$OUTPUT_DIR/sample_${TIMESTAMP}.txt"
SAMPLE_SUMMARY="$OUTPUT_DIR/sample_${TIMESTAMP}_hotfuncs.txt"

echo "[1/2] Sampling process $PID for ${DURATION}s ..."
sample "$PID" "$DURATION" -f "$SAMPLE_RAW" 2>&1 || {
    echo "WARNING: 'sample' failed (may need 'sudo'). Trying with sudo..."
    sudo sample "$PID" "$DURATION" -f "$SAMPLE_RAW"
}

echo "      Raw sample → $SAMPLE_RAW"

# --- Parse the sample output into a hot-functions summary ---
echo "[2/2] Parsing hot functions ..."
python3 "$(dirname "$0")/parse-sample.py" "$SAMPLE_RAW" > "$SAMPLE_SUMMARY"
echo "      Summary    → $SAMPLE_SUMMARY"
echo ""
cat "$SAMPLE_SUMMARY"

# --- Optional: Instruments Time Profiler trace ---
if $USE_INSTRUMENTS; then
    TRACE_FILE="$OUTPUT_DIR/trace_${TIMESTAMP}.trace"
    echo ""
    echo "[instruments] Recording Time Profiler for ${DURATION}s ..."
    xctrace record --template "Time Profiler" \
        --attach "$PID" \
        --time-limit "${DURATION}s" \
        --output "$TRACE_FILE" 2>&1
    echo "      Trace → $TRACE_FILE"
    echo "      Open with: open '$TRACE_FILE'"

    # Export symbols from the trace
    TRACE_EXPORT="$OUTPUT_DIR/trace_${TIMESTAMP}_export.xml"
    xctrace export --input "$TRACE_FILE" --xpath '/trace-toc/run/data/table[@schema="time-profile"]' \
        > "$TRACE_EXPORT" 2>/dev/null || true
    if [[ -s "$TRACE_EXPORT" ]]; then
        echo "      Export → $TRACE_EXPORT"
    fi
fi

echo ""
echo "Done. To re-run: ./scripts/profile-idle.sh $DURATION $OUTPUT_DIR"
