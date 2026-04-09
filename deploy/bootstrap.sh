#!/bin/bash
# Bootstrap a fresh Ubuntu 24.04 instance for the kagami stack.
# Usage: ssh user@host 'bash -s' < bootstrap.sh
#
# Prerequisites: Ubuntu 24.04 (arm64 or amd64), root/sudo access.
# This script installs Docker, AWS CLI, nftables, creates the deploy
# directory, and configures seccomp for io_uring. Safe to re-run.

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

echo "=== Installing AWS CLI ==="
if ! command -v aws &>/dev/null; then
    sudo apt-get update
    sudo apt-get install -y awscli
fi

echo "=== Installing nftables ==="
if ! command -v nft &>/dev/null; then
    sudo apt-get update
    sudo apt-get install -y nftables
fi

echo "=== Configuring Docker for io_uring ==="
# io_uring requires unconfined seccomp (Docker's default profile blocks it)
DAEMON_JSON="/etc/docker/daemon.json"
if [ ! -f "$DAEMON_JSON" ] || ! grep -q seccomp "$DAEMON_JSON" 2>/dev/null; then
    echo '{ "seccomp-profile": "unconfined" }' | sudo tee "$DAEMON_JSON" > /dev/null
    sudo systemctl restart docker
    echo "Docker seccomp set to unconfined for io_uring support."
fi

echo "=== Creating deploy directory ==="
sudo mkdir -p /opt/kagami
sudo chown "$USER:$USER" /opt/kagami

echo "=== Installing kernel firewall rules ==="
if [ -f /opt/kagami/kagami-firewall.sh ]; then
    sudo cp /opt/kagami/kagami-firewall.sh /usr/local/bin/
    sudo chmod +x /usr/local/bin/kagami-firewall.sh
    if [ -f /opt/kagami/kagami-firewall.service ]; then
        sudo cp /opt/kagami/kagami-firewall.service /etc/systemd/system/
        sudo systemctl daemon-reload
        sudo systemctl enable kagami-firewall
        sudo /usr/local/bin/kagami-firewall.sh
        echo "Kernel firewall rules installed and enabled."
    fi
fi

echo "=== Disabling snap services (saves ~60MB RAM) ==="
if command -v snap &>/dev/null; then
    sudo systemctl disable --now snapd.service snapd.socket snapd.seeded.service 2>/dev/null || true
fi

echo "=== Bootstrap complete ==="
echo "Next steps:"
echo "  1. Copy .env.example to /opt/kagami/.env and set KAGAMI_DOMAIN"
echo "  2. Copy kagami.example.json to /opt/kagami/kagami.json and configure"
echo "  3. Run deploy.sh to push files, generate Caddyfile, and start services"
echo "  See deploy/README.md for the full guide."
docker --version
docker compose version
