#!/bin/bash
# Migrate an akashi server installation to kagami.
#
# Reads akashi's config.ini, content files, and database, then produces
# a kagami.json and config/ directory ready for deployment.
#
# Usage:
#   ./migrate-akashi.sh /path/to/akashi [--output ./output-dir]
#
# The akashi path should contain:
#   config/config.ini       Server settings
#   config/characters.txt   Character list
#   config/music.json       Music list (JSON format)
#   config/areas.ini        Area definitions
#   config/backgrounds.txt  Background list
#   akashi.db               SQLite database (optional)
#
# Output (written to --output dir, default: ./kagami-migrated/):
#   kagami.json             Converted server config
#   config/                 Content files (copied as-is where compatible)
#   kagami.db               Database (copied as-is — schema is compatible)

set -euo pipefail

# ---------------------------------------------------------------------------
# Arguments
# ---------------------------------------------------------------------------
AKASHI_DIR=""
OUTPUT_DIR="./kagami-migrated"

usage() {
    cat <<'USAGE'
Usage: migrate-akashi.sh [options] /path/to/akashi

Options:
  --output <dir>   Output directory (default: ./kagami-migrated)
  -h, --help       Show this help

The akashi path should be the root of the akashi installation
(containing config/ and optionally akashi.db).
USAGE
    exit 1
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --output)  OUTPUT_DIR="$2"; shift 2 ;;
        -h|--help) usage ;;
        -*)        echo "Unknown option: $1" >&2; usage ;;
        *)         AKASHI_DIR="$1"; shift ;;
    esac
done

[[ -z "$AKASHI_DIR" ]] && { echo "Error: akashi directory required" >&2; usage; }
[[ -d "$AKASHI_DIR" ]] || { echo "Error: not a directory: $AKASHI_DIR" >&2; exit 1; }

# Locate config directory (akashi uses config/ under its root)
AKASHI_CONFIG="$AKASHI_DIR/config"
if [[ ! -d "$AKASHI_CONFIG" ]]; then
    # Maybe they pointed directly at the config dir
    if [[ -f "$AKASHI_DIR/config.ini" ]]; then
        AKASHI_CONFIG="$AKASHI_DIR"
        AKASHI_DIR="$(dirname "$AKASHI_DIR")"
    else
        echo "Error: could not find config/ or config.ini in $AKASHI_DIR" >&2
        exit 1
    fi
fi

CONFIG_INI="$AKASHI_CONFIG/config.ini"
[[ -f "$CONFIG_INI" ]] || { echo "Error: config.ini not found at $CONFIG_INI" >&2; exit 1; }

echo "=== Migrating akashi → kagami ==="
echo "  Source:  $AKASHI_DIR"
echo "  Output:  $OUTPUT_DIR"

mkdir -p "$OUTPUT_DIR/config"

# ---------------------------------------------------------------------------
# Parse config.ini
# ---------------------------------------------------------------------------
# Simple INI parser: reads key=value pairs, strips quotes and comments.
# Usage: ini_get <section> <key> <default>
ini_get() {
    local section="$1" key="$2" default="${3:-}"
    local in_section=false
    while IFS= read -r line; do
        # Strip comments and leading/trailing whitespace
        line="${line%%[;#]*}"
        line="${line#"${line%%[![:space:]]*}"}"
        line="${line%"${line##*[![:space:]]}"}"
        [[ -z "$line" ]] && continue

        # Section header
        if [[ "$line" =~ ^\[([^]]+)\]$ ]]; then
            if [[ "${BASH_REMATCH[1]}" == "$section" ]]; then
                in_section=true
            else
                $in_section && break  # past our section
                in_section=false
            fi
            continue
        fi

        # Key=value in the right section
        if $in_section && [[ "$line" =~ ^([^=]+)=(.*)$ ]]; then
            local k="${BASH_REMATCH[1]}"
            local v="${BASH_REMATCH[2]}"
            # Trim whitespace
            k="${k#"${k%%[![:space:]]*}"}"
            k="${k%"${k##*[![:space:]]}"}"
            v="${v#"${v%%[![:space:]]*}"}"
            v="${v%"${v##*[![:space:]]}"}"
            # Strip surrounding quotes
            if [[ "$v" =~ ^\"(.*)\"$ ]]; then
                v="${BASH_REMATCH[1]}"
            fi
            if [[ "$k" == "$key" ]]; then
                echo "$v"
                return
            fi
        fi
    done < "$CONFIG_INI"
    echo "$default"
}

echo ""
echo "--- Reading config.ini ---"

# [Options]
SERVER_NAME=$(ini_get Options server_name "An Unnamed Server")
SERVER_DESC=$(ini_get Options server_description "")
MAX_PLAYERS=$(ini_get Options max_players 100)
MOTD=$(ini_get Options motd "")
MOD_PASS=$(ini_get Options modpass "")
ASSET_URL=$(ini_get Options asset_url "")
WS_PORT=$(ini_get Options port 27016)

# [Advertiser]
ADVERTISE=$(ini_get Advertiser advertise "true")
MS_URL=$(ini_get Advertiser ms_ip "https://servers.aceattorneyonline.com/servers")
HOSTNAME=$(ini_get Advertiser hostname "")

echo "  Server:     $SERVER_NAME"
echo "  Port:       $WS_PORT"
echo "  Advertise:  $ADVERTISE"
echo "  Asset URL:  ${ASSET_URL:-(none)}"

# Convert advertise boolean
ADVERTISE_BOOL="false"
[[ "$ADVERTISE" == "true" || "$ADVERTISE" == "1" || "$ADVERTISE" == "yes" ]] && ADVERTISE_BOOL="true"

# ---------------------------------------------------------------------------
# Generate kagami.json
# ---------------------------------------------------------------------------
echo ""
echo "--- Generating kagami.json ---"

# Use jq if available for clean JSON, fall back to cat
if command -v jq &>/dev/null; then
    jq -n \
        --arg domain "$HOSTNAME" \
        --arg server_name "$SERVER_NAME" \
        --arg server_description "$SERVER_DESC" \
        --argjson max_players "$MAX_PLAYERS" \
        --arg motd "$MOTD" \
        --arg asset_url "$ASSET_URL" \
        --arg mod_password "$MOD_PASS" \
        --argjson ws_port "$WS_PORT" \
        --argjson advertise "$ADVERTISE_BOOL" \
        --arg ms_url "$MS_URL" \
        '{
            domain: $domain,
            server_name: $server_name,
            server_description: $server_description,
            bind_address: "0.0.0.0",
            http_port: 8080,
            ws_port: $ws_port,
            wss_port: ($ws_port - 1),
            cors_origin: "*",
            max_players: $max_players,
            motd: $motd,
            asset_url: $asset_url,
            mod_password: $mod_password,
            session_ttl_seconds: 300,
            log_level: "info",
            log_file: "/logs/kagami.log",
            log_file_level: "info",
            loki_url: "",
            metrics_enabled: true,
            metrics_path: "/metrics",
            advertiser: {
                enabled: $advertise,
                url: $ms_url
            },
            deploy: {
                tls: "auto",
                observability: false,
                metrics_allow: "",
                image: "ghcr.io/attorneyonline/kagami:latest",
                grafana_user: "admin",
                grafana_password: "changeme"
            },
            cloudwatch: {
                region: "",
                log_group: "",
                log_stream: "",
                access_key_id: "",
                secret_access_key: "",
                flush_interval: 5,
                log_level: "info"
            },
            reverse_proxy: {
                enabled: false,
                trusted_proxies: [],
                header_priority: ["X-Forwarded-For", "X-Real-IP"],
                proxy_protocol: false
            },
            reputation: {
                enabled: true,
                cache_ttl_hours: 24,
                cache_failure_ttl_minutes: 5,
                ip_api_enabled: true,
                abuseipdb_api_key: "",
                abuseipdb_daily_budget: 1000,
                auto_block_proxy: false,
                auto_block_datacenter: false
            },
            asn_reputation: {
                enabled: true,
                watch_threshold: 2,
                rate_limit_threshold: 3,
                block_threshold: 5,
                window_minutes: 60,
                auto_block_duration: "24h",
                whitelist_asns: [],
                whitelist_multiplier: 5
            },
            spam_detection: {
                enabled: true,
                echo_threshold: 3,
                echo_window_seconds: 60,
                burst_threshold: 20,
                burst_window_seconds: 30,
                join_spam_max_seconds: 5,
                name_pattern_threshold: 3,
                name_pattern_min_prefix: 4,
                name_pattern_window_seconds: 300,
                ghost_threshold: 5,
                hwid_reuse_threshold: 3
            },
            firewall: {
                enabled: false,
                helper_path: "",
                cleanup_on_shutdown: true
            },
            rate_limits: {
                session_create: { rate: 2.0, burst: 5.0 },
                ws_frame: { rate: 30.0, burst: 60.0 },
                ws_bytes: { rate: 32768.0, burst: 65536.0 },
                "ao:MS": { rate: 5.0, burst: 10.0 },
                "ao:CT": { rate: 3.0, burst: 6.0 },
                "ao:CC": { rate: 2.0, burst: 4.0 },
                "ao:MC": { rate: 2.0, burst: 5.0 },
                "ao:CH": { rate: 2.0, burst: 5.0 },
                "ao:HP": { rate: 2.0, burst: 4.0 },
                "ao:BN": { rate: 2.0, burst: 4.0 },
                "ao:RT": { rate: 3.0, burst: 6.0 },
                "ao:PE": { rate: 2.0, burst: 5.0 },
                "ao:EE": { rate: 2.0, burst: 5.0 },
                "ao:DE": { rate: 2.0, burst: 5.0 },
                "ao:ZZ": { rate: 1.0, burst: 3.0 },
                "ao:CASEA": { rate: 0.5, burst: 2.0 },
                "ao:SETCASE": { rate: 1.0, burst: 3.0 },
                "nx:ooc": { rate: 3.0, burst: 6.0 },
                "nx:area_join": { rate: 2.0, burst: 5.0 },
                "nx:char_select": { rate: 2.0, burst: 4.0 },
                "nx:session_renew": { rate: 1.0, burst: 3.0 },
                ws_handshake_deadline_sec: 10,
                ws_idle_timeout_sec: 120,
                ws_partial_frame_timeout_sec: 30,
                ws_max_frame_size: 65536
            }
        }' > "$OUTPUT_DIR/kagami.json"
else
    echo "Warning: jq not found, writing minimal kagami.json" >&2
    cat > "$OUTPUT_DIR/kagami.json" <<EOF
{
    "domain": "$HOSTNAME",
    "server_name": "$SERVER_NAME",
    "server_description": "$SERVER_DESC",
    "ws_port": $WS_PORT,
    "wss_port": $((WS_PORT - 1)),
    "max_players": $MAX_PLAYERS,
    "motd": "$MOTD",
    "asset_url": "$ASSET_URL",
    "mod_password": "$MOD_PASS",
    "advertiser": { "enabled": $ADVERTISE_BOOL, "url": "$MS_URL" }
}
EOF
fi

echo "  Written to $OUTPUT_DIR/kagami.json"

# ---------------------------------------------------------------------------
# Copy content files
# ---------------------------------------------------------------------------
echo ""
echo "--- Copying content files ---"

copy_if_exists() {
    local src="$1" dst="$2" name="$3"
    if [[ -f "$src" ]]; then
        cp "$src" "$dst"
        echo "  $name: copied"
    else
        echo "  $name: not found (skipping)"
    fi
}

copy_if_exists "$AKASHI_CONFIG/characters.txt"  "$OUTPUT_DIR/config/characters.txt"  "characters.txt"
copy_if_exists "$AKASHI_CONFIG/music.json"       "$OUTPUT_DIR/config/music.json"       "music.json"
copy_if_exists "$AKASHI_CONFIG/areas.ini"        "$OUTPUT_DIR/config/areas.ini"        "areas.ini"
copy_if_exists "$AKASHI_CONFIG/backgrounds.txt"  "$OUTPUT_DIR/config/backgrounds.txt"  "backgrounds.txt"

# Also check for music.txt (older akashi format — plain text, one per line).
# Kagami uses music.json, so we'd need to convert. For now, warn.
if [[ -f "$AKASHI_CONFIG/music.txt" && ! -f "$AKASHI_CONFIG/music.json" ]]; then
    echo ""
    echo "  WARNING: Found music.txt but no music.json."
    echo "  Kagami uses music.json format. Converting..."

    # Convert plain-text music list to JSON.
    # music.txt is one entry per line — category headers start with "=="
    if command -v python3 &>/dev/null; then
        python3 -c "
import json, sys

entries = []
current_category = None
current_songs = []

for line in open('$AKASHI_CONFIG/music.txt'):
    line = line.strip()
    if not line:
        continue
    if line.startswith('==') or line.startswith('~'):
        # Flush previous category
        if current_category is not None:
            entries.append({'category': current_category, 'songs': current_songs})
        current_category = line
        current_songs = []
    else:
        current_songs.append({'name': line, 'length': -1})

# Flush last category
if current_category is not None:
    entries.append({'category': current_category, 'songs': current_songs})
elif current_songs:
    entries.append({'category': '== Music ==', 'songs': current_songs})

json.dump(entries, open('$OUTPUT_DIR/config/music.json', 'w'), indent=2)
print('  Converted music.txt → music.json (' + str(sum(len(e[\"songs\"]) for e in entries)) + ' tracks)')
" || echo "  Failed to convert music.txt — please convert manually"
    else
        echo "  python3 not available — please convert music.txt to music.json manually"
    fi
fi

# ---------------------------------------------------------------------------
# Copy database
# ---------------------------------------------------------------------------
echo ""
echo "--- Database ---"

DB_FILE=""
for candidate in "$AKASHI_DIR/akashi.db" "$AKASHI_DIR/config/akashi.db"; do
    if [[ -f "$candidate" ]]; then
        DB_FILE="$candidate"
        break
    fi
done

if [[ -n "$DB_FILE" ]]; then
    cp "$DB_FILE" "$OUTPUT_DIR/kagami.db"
    # Count bans and users for the summary
    if command -v sqlite3 &>/dev/null; then
        BANS=$(sqlite3 "$OUTPUT_DIR/kagami.db" "SELECT COUNT(*) FROM bans;" 2>/dev/null || echo "?")
        USERS=$(sqlite3 "$OUTPUT_DIR/kagami.db" "SELECT COUNT(*) FROM users;" 2>/dev/null || echo "?")
        echo "  Copied akashi.db → kagami.db ($BANS bans, $USERS users)"
    else
        echo "  Copied akashi.db → kagami.db"
    fi
else
    echo "  No akashi.db found (skipping — starting fresh)"
fi

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
echo ""
echo "=== Migration complete ==="
echo ""
echo "Output files:"
ls -la "$OUTPUT_DIR/" "$OUTPUT_DIR/config/" 2>/dev/null | grep -v "^total\|^d"
echo ""
echo "Next steps:"
echo "  1. Review $OUTPUT_DIR/kagami.json — set 'domain' to your server's hostname"
echo "  2. Deploy: ./deploy.sh user@host --config $OUTPUT_DIR/kagami.json --key ~/.ssh/key.pem"
if [[ -n "$DB_FILE" ]]; then
    echo "  3. Copy database to host: scp $OUTPUT_DIR/kagami.db user@host:/opt/kagami/"
fi
