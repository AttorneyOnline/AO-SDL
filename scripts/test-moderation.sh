#!/usr/bin/env bash
# test-moderation.sh — exercise the content moderation stack end-to-end.
#
# Connects to a kagami server (default: staging), sends a battery of
# test messages across the moderation axes, then queries the Prometheus
# metrics endpoint to verify the counters moved.
#
# Usage:
#   scripts/test-moderation.sh [HOST]
#   HOST defaults to https://kagami.yuurei.network.
#
# Requires: curl, jq, bash 4+.

set -euo pipefail

HOST="${1:-https://kagami.yuurei.network}"
# Strip any trailing slash.
HOST="${HOST%/}"

echo "=== Content Moderation Smoke Test ==="
echo "Target: $HOST"
echo

# -- Helpers ------------------------------------------------------------------

# Count a Prometheus metric value by label. Returns 0 if not present.
# Example: metric_value 'kagami_moderation_events_total{action="none"}'
metric_value() {
    local pattern="$1"
    curl -fsS "$HOST/metrics" 2>/dev/null | awk -v pat="$pattern" '
        $0 ~ pat { print $NF; exit }
    '
}

# Compact display of all moderation metrics currently exposed.
dump_metrics() {
    echo "--- kagami_moderation_* ---"
    curl -fsS "$HOST/metrics" 2>/dev/null | grep -E '^kagami_moderation_' | grep -v '^#' || echo "(none)"
    echo
}

# Open a short-lived AONX session so we have a token to post with.
# Returns the session token on stdout.
create_session() {
    local resp
    resp=$(curl -fsS -X POST "$HOST/aonx/v1/session" \
        -H 'Content-Type: application/json' \
        -d '{"client":"mod-smoke-test","version":"1.0","hardware_id":"smoketest"}')
    echo "$resp" | jq -r '.token'
}

# Post an OOC message via the NX REST endpoint.
post_ooc() {
    local token="$1"
    local area="${2:-Lobby}"
    local message="$3"
    curl -fsS -X POST "$HOST/aonx/v1/areas/$area/ooc" \
        -H "Authorization: Bearer $token" \
        -H 'Content-Type: application/json' \
        -d "$(jq -n --arg m "$message" '{name:"mod-smoke", message:$m}')" \
        -o /dev/null -w "%{http_code}"
    echo
}

# -- Pre-test state -----------------------------------------------------------

echo "Baseline metrics:"
dump_metrics

# -- Test messages ------------------------------------------------------------

# Each category below exercises a distinct moderation axis. The script
# is deliberately gentle — we're verifying the pipeline emits events,
# not trying to trip the ban threshold. A hostile stress test would
# want to push harder and validate rate-limiting, which is out of scope
# for this smoke test.

TOKEN="$(create_session)"
if [ -z "$TOKEN" ] || [ "$TOKEN" = "null" ]; then
    echo "ERROR: failed to create NX session (server unreachable?)"
    exit 1
fi
echo "Got session token: ${TOKEN:0:8}..."
echo

# --- 1. Clean message baseline ---
echo "[1/6] Clean message (should score zero)"
post_ooc "$TOKEN" "Lobby" "hello everyone, nice weather today"

# --- 2. Visual-noise test (zalgo) ---
# Combining marks on every base letter.
# Layer: unicode_classifier
echo "[2/6] Zalgo visual-noise test"
ZALGO="h$'\xcc\x80\xcc\x81\xcc\x82\xcc\x83'ello$'\xcc\x80\xcc\x81\xcc\x82\xcc\x83' everyone"
post_ooc "$TOKEN" "Lobby" "$ZALGO"

# --- 3. URL blocklist (requires operator-configured blocklist) ---
echo "[3/6] URL extraction (benign link, scored only if blocklist matches)"
post_ooc "$TOKEN" "Lobby" "check out aceattorneyonline.com for more"

# --- 4. Harassment proxy (remote classifier required for real signal) ---
# This is mild enough not to score high without the remote layer but
# exercises the code path. A real test of hate/slurs requires the
# remote classifier to be enabled and an API key configured.
echo "[4/6] Mild insult (needs remote classifier for real signal)"
post_ooc "$TOKEN" "Lobby" "you're all a bunch of clowns, honestly"

# --- 5. Exotic unicode (cuneiform blast) ---
echo "[5/6] Exotic unicode (cuneiform)"
CUNEIFORM=$'\xf0\x92\x80\x80\xf0\x92\x80\x80\xf0\x92\x80\x80\xf0\x92\x80\x80\xf0\x92\x80\x80\xf0\x92\x80\x80'
post_ooc "$TOKEN" "Lobby" "$CUNEIFORM"

# --- 6. Normal chatter (should stay zero) ---
echo "[6/6] Normal chat again"
post_ooc "$TOKEN" "Lobby" "what's everyone up to today?"

echo
echo "=== Post-test metrics ==="
dump_metrics

# -- Post-test diff -----------------------------------------------------------

echo "Moderation events should show nonzero counts if the subsystem is"
echo "enabled and at least the unicode layer is on. If metrics are empty,"
echo "check kagami.json: content_moderation.enabled and content_moderation.unicode.enabled."
