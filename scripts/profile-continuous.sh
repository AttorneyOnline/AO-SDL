#!/usr/bin/env bash
# profile-continuous.sh — Repeatedly sample aosdl to track CPU usage over time.
#
# Usage:
#   ./scripts/profile-continuous.sh [sample_duration] [interval] [count]
#
# Defaults: 5s samples, every 30s, 10 iterations.
# Produces a timeline CSV and per-sample summaries.

set -euo pipefail

SAMPLE_DUR="${1:-5}"
INTERVAL="${2:-30}"
COUNT="${3:-10}"
PROCESS_NAME="aosdl"
OUTPUT_DIR="/Users/saturday/Documents/AO-SDL/profiles/continuous_$(date +%Y%m%d_%H%M%S)"

mkdir -p "$OUTPUT_DIR"

PID=$(pgrep -x "$PROCESS_NAME" 2>/dev/null | head -1) || true
if [[ -z "$PID" ]]; then
    echo "ERROR: No running process named '$PROCESS_NAME'. Launch it first."
    exit 1
fi

TIMELINE="$OUTPUT_DIR/timeline.csv"
echo "timestamp,cpu_percent,rss_mb,threads,top_function,top_pct" > "$TIMELINE"

echo "=== Continuous profiler ==="
echo "PID: $PID | Sample: ${SAMPLE_DUR}s | Interval: ${INTERVAL}s | Count: $COUNT"
echo "Output: $OUTPUT_DIR"
echo ""

for i in $(seq 1 "$COUNT"); do
    TS=$(date +%Y%m%d_%H%M%S)
    echo "[$i/$COUNT] Sampling at $TS ..."

    # Grab current CPU% and RSS from ps
    PS_INFO=$(ps -p "$PID" -o %cpu=,rss= 2>/dev/null || echo "0 0")
    CPU_PCT=$(echo "$PS_INFO" | awk '{print $1}')
    RSS_KB=$(echo "$PS_INFO" | awk '{print $2}')
    RSS_MB=$(echo "scale=1; $RSS_KB / 1024" | bc 2>/dev/null || echo "?")
    THREADS=$(ps -M -p "$PID" 2>/dev/null | tail -n +2 | wc -l | tr -d ' ')

    # Run sample
    SAMPLE_FILE="$OUTPUT_DIR/sample_${TS}.txt"
    sample "$PID" "$SAMPLE_DUR" -f "$SAMPLE_FILE" 2>/dev/null || {
        echo "  WARNING: sample failed, trying sudo"
        sudo sample "$PID" "$SAMPLE_DUR" -f "$SAMPLE_FILE" 2>/dev/null || continue
    }

    # Quick-parse top function
    TOP_FUNC=$(python3 "$(dirname "$0")/parse-sample.py" "$SAMPLE_FILE" --top 1 2>/dev/null \
        | grep -A1 "^  Function" | tail -1 | awk '{print $1}' || echo "unknown")
    TOP_PCT=$(python3 "$(dirname "$0")/parse-sample.py" "$SAMPLE_FILE" --top 1 2>/dev/null \
        | grep -A1 "^  Function" | tail -1 | awk '{print $(NF-1)}' || echo "0")

    echo "$TS,$CPU_PCT,$RSS_MB,$THREADS,$TOP_FUNC,$TOP_PCT" >> "$TIMELINE"
    echo "  CPU: ${CPU_PCT}% | RSS: ${RSS_MB}MB | Threads: $THREADS | Top: $TOP_FUNC (${TOP_PCT}%)"

    if [[ $i -lt $COUNT ]]; then
        WAIT=$((INTERVAL - SAMPLE_DUR))
        if [[ $WAIT -gt 0 ]]; then
            sleep "$WAIT"
        fi
    fi
done

echo ""
echo "=== Timeline ==="
column -t -s, "$TIMELINE"
echo ""
echo "Full timeline: $TIMELINE"
echo "Individual samples in: $OUTPUT_DIR/"
