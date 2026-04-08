#!/usr/bin/env bash
# Layer 0: Kernel-level rate limiting via ip6tables.
#
# Protects against SYN floods and per-IP connection storms before
# any userspace processing. Applied as a systemd service on boot.
#
# Install:
#   sudo cp kagami-firewall.sh /usr/local/bin/
#   sudo cp kagami-firewall.service /etc/systemd/system/
#   sudo systemctl enable kagami-firewall
#
# Configure via /etc/kagami/firewall.conf or environment variables.

set -euo pipefail

# Defaults (override via /etc/kagami/firewall.conf or env vars)
CONF_FILE="/etc/kagami/firewall.conf"
[ -f "$CONF_FILE" ] && source "$CONF_FILE"

HTTPS_PORT="${KAGAMI_HTTPS_PORT:-443}"
WS_PORT="${KAGAMI_WS_PORT:-27015}"
HTTP_PORT="${KAGAMI_HTTP_PORT:-80}"

# SYN flood protection: new connections per source IP
SYN_RATE="${KAGAMI_SYN_RATE:-50/sec}"
SYN_BURST="${KAGAMI_SYN_BURST:-100}"

# Max concurrent connections from a single IP
CONN_LIMIT="${KAGAMI_CONN_LIMIT:-100}"

# Per-port new connection rate (catches targeted floods)
HTTPS_CONN_RATE="${KAGAMI_HTTPS_CONN_RATE:-200/sec}"
HTTPS_CONN_BURST="${KAGAMI_HTTPS_CONN_BURST:-500}"
WS_CONN_RATE="${KAGAMI_WS_CONN_RATE:-20/sec}"
WS_CONN_BURST="${KAGAMI_WS_CONN_BURST:-40}"

# ICMP flood protection
ICMP_RATE="${KAGAMI_ICMP_RATE:-10/sec}"
ICMP_BURST="${KAGAMI_ICMP_BURST:-20}"

apply_rules() {
    local cmd=$1

    # Flush existing kagami chain
    $cmd -D INPUT -j KAGAMI 2>/dev/null || true
    $cmd -F KAGAMI 2>/dev/null || true
    $cmd -X KAGAMI 2>/dev/null || true
    $cmd -N KAGAMI

    # --- SYN flood protection (per source IP) ---
    $cmd -A KAGAMI -p tcp --syn -m hashlimit \
        --hashlimit-name kagami-syn \
        --hashlimit-above "$SYN_RATE" \
        --hashlimit-burst "$SYN_BURST" \
        --hashlimit-mode srcip \
        --hashlimit-htable-expire 30000 \
        -j DROP

    # --- Per-IP concurrent connection cap ---
    for port in $HTTPS_PORT $WS_PORT $HTTP_PORT; do
        $cmd -A KAGAMI -p tcp --dport "$port" -m connlimit \
            --connlimit-above "$CONN_LIMIT" \
            --connlimit-mask 128 \
            -j REJECT --reject-with tcp-reset
    done

    # --- Per-port new connection rate ---
    $cmd -A KAGAMI -p tcp --dport "$HTTPS_PORT" --syn -m hashlimit \
        --hashlimit-name kagami-https \
        --hashlimit-above "$HTTPS_CONN_RATE" \
        --hashlimit-burst "$HTTPS_CONN_BURST" \
        --hashlimit-mode srcip \
        --hashlimit-htable-expire 30000 \
        -j DROP

    $cmd -A KAGAMI -p tcp --dport "$WS_PORT" --syn -m hashlimit \
        --hashlimit-name kagami-ws \
        --hashlimit-above "$WS_CONN_RATE" \
        --hashlimit-burst "$WS_CONN_BURST" \
        --hashlimit-mode srcip \
        --hashlimit-htable-expire 30000 \
        -j DROP

    # --- ICMPv6 flood protection ---
    local icmp_proto="icmp"
    [ "$cmd" = "ip6tables" ] && icmp_proto="icmpv6"
    $cmd -A KAGAMI -p "$icmp_proto" -m hashlimit \
        --hashlimit-name kagami-icmp \
        --hashlimit-above "$ICMP_RATE" \
        --hashlimit-burst "$ICMP_BURST" \
        --hashlimit-mode srcip \
        --hashlimit-htable-expire 30000 \
        -j DROP

    # Insert into INPUT chain
    $cmd -I INPUT -j KAGAMI

    echo "[$cmd] Applied: SYN rate=$SYN_RATE conn_limit=$CONN_LIMIT"
}

# Apply to ip6tables (primary — IPv6-only instances)
if command -v ip6tables &>/dev/null; then
    apply_rules ip6tables
fi

# Apply to iptables too (for dual-stack or during transition)
if command -v iptables &>/dev/null; then
    apply_rules iptables
fi

echo "Kagami firewall rules applied"
