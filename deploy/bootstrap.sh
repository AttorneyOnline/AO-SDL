#!/bin/bash
# Bootstrap a fresh Ubuntu 24.04 instance for the kagami stack.
# Usage: ssh user@host 'bash -s' < bootstrap.sh
#
# Prerequisites: Ubuntu 24.04 (arm64 or amd64), root/sudo access.
# This script installs Docker + Compose, creates the deploy directory,
# and is safe to re-run.

set -euo pipefail

echo "=== Installing Docker ==="
if ! command -v docker &>/dev/null; then
    curl -fsSL https://get.docker.com | sh
    sudo usermod -aG docker "$USER"
    echo "Docker installed. You may need to log out/in for group to take effect."
fi

echo "=== Installing Docker Compose plugin ==="
if ! docker compose version &>/dev/null; then
    sudo apt-get update
    sudo apt-get install -y docker-compose-plugin
fi

echo "=== Creating deploy directory ==="
sudo mkdir -p /opt/kagami
sudo chown "$USER:$USER" /opt/kagami

echo "=== Disabling snap services (saves ~60MB RAM) ==="
if command -v snap &>/dev/null; then
    sudo systemctl disable --now snapd.service snapd.socket snapd.seeded.service 2>/dev/null || true
fi

echo "=== Bootstrap complete ==="
echo "Next steps:"
echo "  1. Copy deploy/ contents to /opt/kagami/"
echo "  2. cd /opt/kagami && docker compose up -d"
docker --version
docker compose version
