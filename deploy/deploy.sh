#!/bin/bash
# Deploy kagami to a remote host.
#
# Usage:
#   ./deploy.sh user@host --config ./kagami.json --key ~/.ssh/key.pem
#   ./deploy.sh user@host --bootstrap --config ./kagami.json --key ~/.ssh/key.pem
#
# The --config file is the single source of truth. The script reads ports,
# domain, TLS mode, and image from it to generate the Caddyfile and .env.
#
# What is deployed:
#   - kagami.json, generated Caddyfile, generated .env
#   - docker-compose.yml, generate-caddyfile.sh
#   - Content config (characters, music, areas, backgrounds)
#   - Prometheus config, Grafana dashboards
#   - Firewall helper scripts
#
# What is NOT touched if it already exists:
#   - kagami.db (ban/user database)

set -euo pipefail

# ---------------------------------------------------------------------------
# Argument parsing
# ---------------------------------------------------------------------------
BOOTSTRAP=false
CONFIG=""
KEY=""
HOST=""
MIGRATE_AKASHI=""

usage() {
    cat <<'USAGE'
Usage: deploy.sh [options] user@host

Options:
  --config <path>          Path to local kagami.json (required)
  --key <path>             SSH private key
  --bootstrap              First-time host setup (installs Docker, creates dirs)
  --migrate-akashi <path>  Migrate from an akashi installation directory
  -h, --help               Show this help

Migration:
  ./deploy.sh user@host --bootstrap --migrate-akashi /path/to/akashi --key ~/.ssh/key.pem
  This converts akashi's config.ini, content files, and database to kagami format,
  then deploys to the host. Generates kagami.json from the akashi config.
USAGE
    exit 1
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --bootstrap)        BOOTSTRAP=true; shift ;;
        --config)           CONFIG="$2"; shift 2 ;;
        --key)              KEY="$2"; shift 2 ;;
        --migrate-akashi)   MIGRATE_AKASHI="$2"; shift 2 ;;
        -h|--help)          usage ;;
        -*)                 echo "Unknown option: $1" >&2; usage ;;
        *)                  HOST="$1"; shift ;;
    esac
done

[[ -z "$HOST" ]] && { echo "Error: host argument required" >&2; usage; }

# --- Akashi migration (runs before config validation) ---
if [[ -n "$MIGRATE_AKASHI" ]]; then
    echo "--- Running akashi migration ---"
    MIGRATE_OUTPUT="/tmp/kagami-migration-$$"
    "$SCRIPT_DIR/migrate-akashi.sh" "$MIGRATE_AKASHI" --output "$MIGRATE_OUTPUT"

    # Use migrated config if no --config was given
    if [[ -z "$CONFIG" ]]; then
        CONFIG="$MIGRATE_OUTPUT/kagami.json"
        echo "  Using migrated config: $CONFIG"
    fi
fi

[[ -z "$CONFIG" ]] && { echo "Error: --config or --migrate-akashi is required" >&2; usage; }
[[ -f "$CONFIG" ]] || { echo "Error: config file not found: $CONFIG" >&2; exit 1; }

# Require jq for reading kagami.json
command -v jq &>/dev/null || { echo "Error: jq is required (brew install jq)" >&2; exit 1; }

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DEPLOY_DIR="/opt/kagami"

# SSH/SCP commands
SSH_OPTS="-o StrictHostKeyChecking=no"
[[ -n "$KEY" ]] && SSH_OPTS="$SSH_OPTS -i $KEY"
SSH="ssh $SSH_OPTS"
SCP="scp $SSH_OPTS"

# ---------------------------------------------------------------------------
# Read config
# ---------------------------------------------------------------------------
cfg() { jq -r "$1 // empty" "$CONFIG"; }
cfg_default() { jq -r "($1) // \"$2\"" "$CONFIG"; }

DOMAIN=$(cfg '.domain')
HTTP_PORT=$(cfg_default '.http_port' '8080')
WS_PORT=$(cfg_default '.ws_port' '27016')
WSS_PORT=$(cfg_default '.wss_port' '27015')
TLS_MODE=$(cfg_default '.deploy.tls' 'auto')
OBSERVABILITY=$(cfg_default '.deploy.observability' 'false')
METRICS_ALLOW=$(cfg_default '.deploy.metrics_allow' '')
IMAGE=$(cfg_default '.deploy.image' 'ghcr.io/attorneyonline/kagami:latest')
GRAFANA_USER=$(cfg_default '.deploy.grafana_user' 'admin')
GRAFANA_PASSWORD=$(cfg_default '.deploy.grafana_password' 'changeme')

[[ -z "$DOMAIN" ]] && { echo "Error: 'domain' must be set in $CONFIG" >&2; exit 1; }

echo "=== Deploying to $HOST ==="
echo "  Domain:        $DOMAIN"
echo "  Ports:         HTTP=$HTTP_PORT  WS=$WS_PORT  WSS=$WSS_PORT"
echo "  TLS:           $TLS_MODE"
echo "  Observability: $OBSERVABILITY"
echo "  Image:         $IMAGE"

# ---------------------------------------------------------------------------
# Bootstrap (first-time host setup)
# ---------------------------------------------------------------------------
if [[ "$BOOTSTRAP" == "true" ]]; then
    echo ""
    echo "--- Bootstrap: installing dependencies ---"
    # Use -t for TTY so sudo can prompt for password if needed
    $SSH -t "$HOST" bash -s <<'BOOTSTRAP_SCRIPT'
set -euo pipefail

echo "Installing Docker..."
if ! command -v docker &>/dev/null; then
    curl -fsSL https://get.docker.com | sudo sh
    sudo usermod -aG docker "$USER"
    echo "Docker installed. You may need to log out/in for group to take effect."
fi

echo "Installing Docker Compose plugin..."
if ! docker compose version &>/dev/null 2>&1; then
    sudo apt-get update -qq
    sudo apt-get install -y -qq docker-compose-plugin
fi

echo "Installing nftables..."
if ! command -v nft &>/dev/null; then
    sudo apt-get update -qq
    sudo apt-get install -y -qq nftables
fi

echo "Configuring Docker for io_uring..."
DAEMON_JSON="/etc/docker/daemon.json"
if [ ! -f "$DAEMON_JSON" ] || ! grep -q seccomp "$DAEMON_JSON" 2>/dev/null; then
    echo '{ "seccomp-profile": "unconfined" }' | sudo tee "$DAEMON_JSON" > /dev/null
    sudo systemctl restart docker
fi

echo "Creating deploy directory..."
sudo mkdir -p /opt/kagami
sudo chown "$USER:$USER" /opt/kagami

echo "Disabling snap services..."
if command -v snap &>/dev/null; then
    sudo systemctl disable --now snapd.service snapd.socket snapd.seeded.service 2>/dev/null || true
fi

echo "Bootstrap complete."
BOOTSTRAP_SCRIPT
fi

# ---------------------------------------------------------------------------
# Generate Caddyfile
# ---------------------------------------------------------------------------
echo ""
echo "--- Generating Caddyfile ---"

export CADDY_TLS="$TLS_MODE"
export CADDY_OBSERVABILITY="$OBSERVABILITY"
export CADDY_METRICS_ALLOW="$METRICS_ALLOW"
export CADDY_HTTP_PORT="$HTTP_PORT"
export CADDY_WS_PORT="$WS_PORT"
export CADDY_WSS_PORT="$WSS_PORT"

"$SCRIPT_DIR/generate-caddyfile.sh" > /tmp/kagami-Caddyfile

# ---------------------------------------------------------------------------
# Generate .env for docker-compose
# ---------------------------------------------------------------------------
cat > /tmp/kagami-env <<EOF
KAGAMI_DOMAIN=$DOMAIN
KAGAMI_IMAGE=$IMAGE
GRAFANA_ADMIN_USER=$GRAFANA_USER
GRAFANA_ADMIN_PASSWORD=$GRAFANA_PASSWORD
EOF

# ---------------------------------------------------------------------------
# Sync files to host
# ---------------------------------------------------------------------------
echo "--- Syncing files ---"
$SCP -r \
    /tmp/kagami-Caddyfile \
    /tmp/kagami-env \
    "$CONFIG" \
    "$SCRIPT_DIR/docker-compose.yml" \
    "$SCRIPT_DIR/generate-caddyfile.sh" \
    "$SCRIPT_DIR/config" \
    "$SCRIPT_DIR/prometheus" \
    "$SCRIPT_DIR/grafana" \
    "$SCRIPT_DIR/kagami-firewall.sh" \
    "$SCRIPT_DIR/kagami-firewall.conf" \
    "$SCRIPT_DIR/kagami-firewall.service" \
    "$HOST:$DEPLOY_DIR/"

# Rename temp files into place
$SSH "$HOST" "mv $DEPLOY_DIR/kagami-Caddyfile $DEPLOY_DIR/Caddyfile && mv $DEPLOY_DIR/kagami-env $DEPLOY_DIR/.env"
rm -f /tmp/kagami-Caddyfile /tmp/kagami-env

# Sync migrated content and database if applicable
if [[ -n "$MIGRATE_AKASHI" && -d "$MIGRATE_OUTPUT" ]]; then
    echo "--- Syncing migrated content ---"
    $SCP -r "$MIGRATE_OUTPUT/config/"* "$HOST:$DEPLOY_DIR/config/"
    if [[ -f "$MIGRATE_OUTPUT/kagami.db" ]]; then
        $SCP "$MIGRATE_OUTPUT/kagami.db" "$HOST:$DEPLOY_DIR/kagami.db"
        echo "  Database migrated"
    fi
    rm -rf "$MIGRATE_OUTPUT"
fi

# ---------------------------------------------------------------------------
# Install firewall helper (needs sudo)
# ---------------------------------------------------------------------------
$SSH "$HOST" bash -s <<'FW_SCRIPT'
if [ -f /opt/kagami/kagami-firewall.sh ] && [ -f /opt/kagami/kagami-firewall.service ]; then
    sudo cp /opt/kagami/kagami-firewall.sh /usr/local/bin/ 2>/dev/null && \
    sudo chmod +x /usr/local/bin/kagami-firewall.sh 2>/dev/null && \
    sudo cp /opt/kagami/kagami-firewall.service /etc/systemd/system/ 2>/dev/null && \
    sudo systemctl daemon-reload 2>/dev/null && \
    sudo systemctl enable kagami-firewall 2>/dev/null || true
fi
FW_SCRIPT

# ---------------------------------------------------------------------------
# Pull image and restart
# ---------------------------------------------------------------------------
echo "--- Pulling image ---"

# Detect if image is from ECR and login if needed
if [[ "$IMAGE" == *".dkr.ecr."* || "$IMAGE" == *".dkr-ecr."* ]]; then
    echo "  ECR image detected, logging in..."
    $SSH "$HOST" "
        export AWS_USE_DUALSTACK_ENDPOINT=true
        aws ecr get-login-password --region us-west-2 | \
            docker login --username AWS --password-stdin ${IMAGE%%/*}
    " 2>&1
fi

$SSH "$HOST" "cd $DEPLOY_DIR && docker compose pull kagami"

echo "--- Restarting ---"
$SSH "$HOST" "cd $DEPLOY_DIR && docker compose up -d && docker compose restart caddy"

echo "--- Status ---"
$SSH "$HOST" "cd $DEPLOY_DIR && docker compose ps"

echo ""
echo "Done. Services:"
echo "  Game server:  https://$DOMAIN/"
echo "  WebSocket:    wss://$DOMAIN:$WSS_PORT"
echo "  WS (plain):   ws://$DOMAIN:$WS_PORT"
[[ "$OBSERVABILITY" == "true" ]] && echo "  Grafana:      https://$DOMAIN/grafana/"
