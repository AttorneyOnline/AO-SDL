#!/usr/bin/env bash
# query-moderation-grafana.sh — pull moderation metrics from the Grafana
# Prometheus proxy on staging.
#
# Grafana is fronted by Caddy at https://kagami.yuurei.network/grafana/
# and Prometheus is at https://kagami.yuurei.network/prometheus/ behind
# basic auth. This script goes through Grafana's datasource proxy so
# the caller only needs Grafana credentials (not Prometheus').
#
# Usage:
#   GRAFANA_USER=admin GRAFANA_PASS=... scripts/query-moderation-grafana.sh
#
# Requires: curl, jq.

set -euo pipefail

HOST="${HOST:-https://kagami.yuurei.network}"
GRAFANA_USER="${GRAFANA_USER:-admin}"
GRAFANA_PASS="${GRAFANA_PASS:?set GRAFANA_PASS env var}"
# Grafana's "Prometheus" datasource has a known UID on our staging stack.
# If this breaks, look it up with:
#   curl -u user:pass https://kagami.yuurei.network/grafana/api/datasources
DS_UID="${PROM_DS_UID:-prometheus}"

# Run a single instant PromQL query via the Grafana datasource proxy.
# Usage: promql 'sum by (action) (kagami_moderation_events_total)'
promql() {
    local q="$1"
    curl -fsS -u "$GRAFANA_USER:$GRAFANA_PASS" \
        --get --data-urlencode "query=$q" \
        "$HOST/grafana/api/datasources/proxy/uid/$DS_UID/api/v1/query" \
        | jq -r '.data.result'
}

# Run a range query. Usage: promql_range QUERY START END STEP
promql_range() {
    local q="$1" start="$2" end="$3" step="${4:-60}"
    curl -fsS -u "$GRAFANA_USER:$GRAFANA_PASS" \
        --get --data-urlencode "query=$q" \
        --data-urlencode "start=$start" --data-urlencode "end=$end" \
        --data-urlencode "step=$step" \
        "$HOST/grafana/api/datasources/proxy/uid/$DS_UID/api/v1/query_range" \
        | jq -r '.data.result'
}

echo "=== Content Moderation Metrics via Grafana ==="
echo "Host:   $HOST"
echo "DS UID: $DS_UID"
echo

echo "--- Moderation events by action (current) ---"
promql 'sum by (action, channel) (kagami_moderation_events_total)'

echo
echo "--- Moderation events rate (last 5 min, per action) ---"
promql 'sum by (action) (rate(kagami_moderation_events_total[5m]))'

echo
echo "--- Layer latency (mean nanoseconds per call, last 5m) ---"
promql 'sum by (layer) (rate(kagami_moderation_layer_nanoseconds_total[5m])) / sum by (layer) (rate(kagami_moderation_layer_calls_total[5m]))'

echo
echo "--- Check latency (mean nanoseconds per call, last 5m) ---"
promql 'rate(kagami_moderation_check_nanoseconds_total[5m]) / rate(kagami_moderation_checks_total[5m])'

echo
echo "--- Currently-tracked heat IPIDs ---"
promql 'kagami_moderation_heat_tracked_ipids'

echo
echo "--- Active mutes ---"
promql 'kagami_moderation_muted_ipids'

echo
echo "--- Remote classifier errors by reason (last 15m) ---"
promql 'sum by (reason) (increase(kagami_moderation_remote_errors_total[15m]))'
