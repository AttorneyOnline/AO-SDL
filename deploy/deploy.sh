#!/bin/bash
# Deploy kagami to the staging server (kagami.yuurei.network).
#
# What gets deployed:
#   - docker-compose.yml, Caddyfile, prometheus config, grafana dashboards
#   - Content config (characters, music, areas, backgrounds)
#   - Docker image (pulled from ECR)
#
# What is NOT touched (server-specific, never overwritten):
#   - kagami.json (server settings, mod password, secrets)
#   - .env (Grafana admin password, domain)
#   - docker-compose.override.yml (per-host service overrides)
#   - kagami.db (ban/user database)
#
# Prerequisites:
#   - SSH access (open SG port 22 first)
#   - AWS CLI configured locally (for ECR login)

set -euo pipefail

HOST="${1:-ec2-user@kagami.yuurei.network}"
KEY="${2:-$HOME/.ssh/kagami-deploy.pem}"
SSH="ssh -6 -i $KEY -o StrictHostKeyChecking=no $HOST"
SCP="scp -6 -i $KEY -o StrictHostKeyChecking=no"
DEPLOY_DIR="/opt/kagami"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

echo "=== Deploying to $HOST ==="

# Sync infrastructure files (safe to overwrite — not server-specific)
echo "--- Syncing deploy files ---"
$SCP -r \
    "$SCRIPT_DIR/docker-compose.yml" \
    "$SCRIPT_DIR/Caddyfile.staging" \
    "$SCRIPT_DIR/config" \
    "$SCRIPT_DIR/prometheus" \
    "$SCRIPT_DIR/grafana" \
    "$SCRIPT_DIR/kagami-firewall.sh" \
    "$SCRIPT_DIR/kagami-firewall.conf" \
    "$SCRIPT_DIR/kagami-firewall.service" \
    "$HOST:$DEPLOY_DIR/"

# Rename Caddyfile.staging → Caddyfile on the host
$SSH "mv $DEPLOY_DIR/Caddyfile.staging $DEPLOY_DIR/Caddyfile"

# First-deploy bootstrap: copy example config if kagami.json doesn't exist
$SSH "[ -f $DEPLOY_DIR/kagami.json ] && echo 'kagami.json exists, skipping' || echo 'NEEDS_CONFIG'" | \
    grep -q NEEDS_CONFIG && {
        echo "--- First deploy: seeding kagami.json from example ---"
        $SCP "$SCRIPT_DIR/kagami.example.json" "$HOST:$DEPLOY_DIR/kagami.json"
        echo "WARNING: Edit $DEPLOY_DIR/kagami.json on the server to configure mod_password and trusted_proxies."
    }

# Pull and restart
echo "--- Logging into ECR ---"
$SSH "
    export AWS_USE_DUALSTACK_ENDPOINT=true
    aws ecr get-login-password --region us-west-2 | \
        docker login --username AWS --password-stdin 257922448991.dkr-ecr.us-west-2.on.aws
" 2>&1

echo "--- Pulling latest images ---"
$SSH "cd $DEPLOY_DIR && docker compose pull kagami"

echo "--- Restarting ---"
$SSH "cd $DEPLOY_DIR && docker compose up -d && docker compose restart caddy"

echo "--- Status ---"
$SSH "cd $DEPLOY_DIR && docker compose ps"

HOSTNAME="$(echo "$HOST" | cut -d@ -f2)"
echo ""
echo "Done. Services:"
echo "  Game server:  https://$HOSTNAME/"
echo "  WebSocket:    wss://$HOSTNAME:27015"
echo "  WS (plain):   ws://$HOSTNAME:27016"
echo "  Grafana:      https://$HOSTNAME/grafana/"
