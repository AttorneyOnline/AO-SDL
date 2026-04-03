#!/bin/bash
# Deploy the kagami stack to a remote server.
# Usage: ./deploy.sh [user@host] [ssh-key]
#
# Defaults to the canary server.

set -euo pipefail

HOST="${1:-ubuntu@canary.attorneyoffline.de}"
KEY="${2:-$HOME/.ssh/kagami-deploy.pem}"
SSH="ssh -i $KEY $HOST"
SCP="scp -i $KEY"
DEPLOY_DIR="/opt/kagami"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

echo "=== Syncing deploy files to $HOST:$DEPLOY_DIR ==="
$SCP -r \
    "$SCRIPT_DIR/docker-compose.yml" \
    "$SCRIPT_DIR/Caddyfile" \
    "$SCRIPT_DIR/kagami.json" \
    "$SCRIPT_DIR/prometheus" \
    "$SCRIPT_DIR/grafana" \
    "$HOST:$DEPLOY_DIR/"

echo "=== Pulling latest images ==="
$SSH "cd $DEPLOY_DIR && docker compose pull"

echo "=== Starting stack ==="
$SSH "cd $DEPLOY_DIR && docker compose up -d"

echo "=== Status ==="
$SSH "cd $DEPLOY_DIR && docker compose ps"

echo ""
echo "Done. Services:"
echo "  Game server:  https://$(echo $HOST | cut -d@ -f2)/"
echo "  Grafana:      https://$(echo $HOST | cut -d@ -f2)/grafana/"
echo "  Prometheus:   https://$(echo $HOST | cut -d@ -f2)/prometheus/"
