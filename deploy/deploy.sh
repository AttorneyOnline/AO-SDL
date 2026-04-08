#!/bin/bash
# Deploy the kagami stack to a remote server.
# Usage: ./deploy.sh [user@host] [ssh-key]
#
# Defaults to the canary server. For staging:
#   ./deploy.sh ec2-user@kagami.yuurei.network
#
# Prerequisites:
#   - SSH access to the host (open SG port 22 first for yuurei.network)
#   - AWS CLI configured locally (for ECR login)
#   - Remote host bootstrapped with bootstrap.sh

set -euo pipefail

HOST="${1:-ubuntu@canary.yuurei.network}"
KEY="${2:-$HOME/.ssh/kagami-deploy.pem}"
SSH="ssh -6 -i $KEY -o StrictHostKeyChecking=no $HOST"
SCP="scp -6 -i $KEY -o StrictHostKeyChecking=no"
DEPLOY_DIR="/opt/kagami"

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

echo "=== Syncing deploy files to $HOST:$DEPLOY_DIR ==="
$SCP -r \
    "$SCRIPT_DIR/docker-compose.yml" \
    "$SCRIPT_DIR/Caddyfile" \
    "$SCRIPT_DIR/prometheus" \
    "$SCRIPT_DIR/grafana" \
    "$SCRIPT_DIR/kagami-firewall.sh" \
    "$SCRIPT_DIR/kagami-firewall.conf" \
    "$SCRIPT_DIR/kagami-firewall.service" \
    "$HOST:$DEPLOY_DIR/"

# Copy kagami.json only if it doesn't exist on remote (don't clobber secrets)
$SSH "[ -f $DEPLOY_DIR/kagami.json ] && echo 'kagami.json exists, skipping' || echo 'NEEDS_CONFIG'"  | \
    grep -q NEEDS_CONFIG && {
        echo "=== Copying kagami.example.json as kagami.json (first deploy) ==="
        $SCP "$SCRIPT_DIR/kagami.example.json" "$HOST:$DEPLOY_DIR/kagami.json"
        echo "WARNING: Edit $DEPLOY_DIR/kagami.json on the server to configure secrets and trusted_proxies."
    }

echo "=== Logging into ECR ==="
$SSH "
    export AWS_USE_DUALSTACK_ENDPOINT=true
    aws ecr get-login-password --region us-west-2 | \
        docker login --username AWS --password-stdin 257922448991.dkr-ecr.us-west-2.on.aws
" 2>&1

echo "=== Pulling latest images ==="
$SSH "cd $DEPLOY_DIR && docker compose pull"

echo "=== Starting stack ==="
$SSH "cd $DEPLOY_DIR && docker compose up -d"

echo "=== Status ==="
$SSH "cd $DEPLOY_DIR && docker compose ps"

HOSTNAME="$(echo "$HOST" | cut -d@ -f2)"
echo ""
echo "Done. Services:"
echo "  Game server:  https://$HOSTNAME/"
echo "  WebSocket:    wss://$HOSTNAME:27015"
echo "  Grafana:      https://$HOSTNAME/grafana/"
echo "  Prometheus:   https://$HOSTNAME/prometheus/"
