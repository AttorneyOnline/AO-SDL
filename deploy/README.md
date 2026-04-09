# Deploying Kagami

## Quick Start

### One-click AWS deploy

[![Launch Stack](https://s3.amazonaws.com/cloudformation-examples/cloudformation-launch-stack.png)](https://console.aws.amazon.com/cloudformation/home#/stacks/new?stackName=kagami&templateURL=https://kagami-deploy.s3.us-west-2.amazonaws.com/cloudformation/kagami.yaml)

Fill in the form (server name, domain, password) and launch. Creates a fully
configured EC2 instance with TLS, observability, and CloudWatch logging.
Estimated cost: **~$17/month** (t4g.small) or **~$8/month** (t4g.nano).

### Manual deploy

```bash
# 1. Copy and edit the config
cp kagami.example.json kagami.json
# Set "domain" at minimum. Set "mod_password" for moderation.

# 2. Deploy (first time — installs Docker, nftables, creates dirs)
./deploy.sh user@host --bootstrap --config ./kagami.json --key ~/.ssh/key.pem

# 3. Subsequent deploys (just pushes config + pulls latest image)
./deploy.sh user@host --config ./kagami.json --key ~/.ssh/key.pem
```

## Architecture

```
Client :443  (HTTPS + WSS) --> Caddy --> kagami :8080  (REST API)
                                     --> kagami :27016 (AO2 WebSocket)
Client :80   (HTTP + WS)   --> Caddy --> kagami :27016 (WS upgrade)
                                     --> HTTPS redirect (everything else)
```

Everything runs on ports 80 and 443. Caddy detects WebSocket upgrade requests
and routes them to the game server, while all other traffic goes to the REST API.
All services run in Docker with host networking.

## SSH Access

```bash
# CloudFormation deploy — find IP from stack outputs
aws cloudformation describe-stacks --stack-name kagami \
  --query 'Stacks[0].Outputs[?OutputKey==`IPv4Address`].OutputValue' --output text

# SSH in (Ubuntu)
ssh -i ~/.ssh/your-key.pem ubuntu@<ip>
```

## Managing the Server

The game server runs as a Docker container managed by `docker compose`.
All commands should be run from `/opt/kagami/`.

```bash
cd /opt/kagami

# View running containers
docker compose ps

# View kagami logs (live)
docker compose logs kagami -f

# Restart kagami
docker compose restart kagami

# Pull latest image and restart
docker compose pull kagami && docker compose up -d kagami
```

### Editing kagami.json

**Important:** Kagami writes `mod_password` and `server_description` back to
`kagami.json` on shutdown. This means if you edit the file while the server is
running and then restart, your changes to those fields will be overwritten by
the runtime values.

To safely edit config:

```bash
cd /opt/kagami

# 1. Stop kagami first (waits for graceful shutdown + config save)
docker compose stop kagami

# 2. Edit the config
nano kagami.json   # or vim, etc.

# 3. Start kagami with the new config
docker compose start kagami
```

If you edit other fields (ports, rate limits, reputation, etc.) you can
restart without stopping first — those fields are not written back on shutdown.

### Updating the Docker image

```bash
cd /opt/kagami

# Pull the latest image
docker compose pull kagami

# Restart with the new image
docker compose up -d kagami
```

To use a specific image (e.g. from ECR for testing):

```bash
# Edit .env to change the image
sed -i 's|^KAGAMI_IMAGE=.*|KAGAMI_IMAGE=your-registry/kagami:tag|' .env

# Pull and restart (source .env so compose picks up the new image)
. ./.env && docker pull $KAGAMI_IMAGE && docker compose up -d --force-recreate kagami
```

## Configuration

Everything is in **one file**: `kagami.json`. See `kagami.example.json` for all
options with defaults.

### Key settings

| Key | Default | Description |
|-----|---------|-------------|
| `domain` | *(required)* | Public domain for TLS + advertiser |
| `server_name` | `"Kagami Server"` | Displayed in server list |
| `mod_password` | `""` | Moderator password (empty = disabled) |
| `max_players` | `100` | Player limit shown in server browser |
| `asset_url` | `""` | Content URL for WebAO clients |
| `ws_port` | `27016` | Internal WebSocket port (kagami listens) |
| `http_port` | `8080` | Internal HTTP port (kagami listens) |

### Advertiser settings

| Key | Default | Description |
|-----|---------|-------------|
| `advertiser.enabled` | `false` | List on the master server |
| `advertiser.url` | `"https://servers.aceattorneyonline.com/servers"` | Master server URL |
| `advertiser.ws_port` | `80` | WS port advertised to clients |
| `advertiser.wss_port` | `443` | WSS port advertised to clients |

### Deploy settings (`deploy` section)

| Key | Default | Description |
|-----|---------|-------------|
| `tls` | `"auto"` | `"auto"` (HTTP-01) or `"dns"` (Route53 DNS-01) |
| `observability` | `false` | Enable Grafana + Prometheus routes |
| `metrics_allow` | `""` | IPs allowed to reach `/metrics` (space-separated) |
| `image` | `ghcr.io/.../kagami:latest` | Docker image to pull |
| `grafana_user` | `"admin"` | Grafana login (when observability enabled) |
| `grafana_password` | `"changeme"` | Grafana password |

### TLS modes

- **`auto`** (default): Caddy obtains certificates via HTTP-01 ACME challenges.
  Requires port 80 reachable from the internet. Works with vanilla `caddy:latest`.

- **`dns`**: Uses Route53 DNS-01 challenges. For IPv6-only or firewalled instances.
  Requires the `caddy-dns/route53` image and AWS credentials on the host.

## deploy.sh Reference

```
Usage: deploy.sh [options] user@host

Options:
  --config <path>          Path to local kagami.json (required)
  --key <path>             SSH private key
  --bootstrap              First-time host setup (installs Docker, creates dirs)
  --migrate-akashi <path>  Migrate from an akashi installation
```

The deploy script:
1. Reads `kagami.json` for ports, domain, and deploy settings
2. Generates a Caddyfile and `.env` from those values
3. Uploads everything to the host
4. Pulls the Docker image and restarts services

## Migrating from Akashi

```bash
# One command — migrates config, content, and database
./deploy.sh user@host --bootstrap --migrate-akashi /path/to/akashi --key ~/.ssh/key.pem
```

This automatically:
- Converts `config.ini` settings to `kagami.json` (server name, ports, MOTD, etc.)
- Copies content files (characters, music, areas, backgrounds)
- Copies the SQLite database (bans and users — schema is compatible)
- Deploys everything to the host

You can also run the migration separately:

```bash
./migrate-akashi.sh /path/to/akashi --output ./migrated
# Review and edit ./migrated/kagami.json, then deploy:
./deploy.sh user@host --config ./migrated/kagami.json --key ~/.ssh/key.pem
```

After migrating, set `"domain"` in `kagami.json` — akashi doesn't have an
equivalent field so it's left empty. Auth mode is auto-detected: if the
database has users, kagami switches to ADVANCED auth automatically.

## Content Config

Game content is loaded from the `config/` directory:

| File | Purpose |
|------|---------|
| `characters.txt` | Character list (one per line) |
| `music.json` | Music tracks with metadata |
| `areas.ini` | Area definitions and settings |
| `backgrounds.txt` | Available backgrounds |

If no `config/` directory exists, kagami falls back to a minimal built-in set
(5 characters, 3 tracks, 3 areas).

## Files

| File | Tracked | Purpose |
|------|---------|---------|
| `deploy.sh` | Yes | Deploy script (includes bootstrap) |
| `generate-caddyfile.sh` | Yes | Caddyfile generator |
| `migrate-akashi.sh` | Yes | Akashi migration tool |
| `docker-compose.yml` | Yes | Service definitions |
| `kagami.example.json` | Yes | Config template |
| `cloudformation/kagami.yaml` | Yes | AWS CloudFormation template |
| `kagami.json` | No | Your server config |
| `.env` | No | Generated by deploy.sh |
| `Caddyfile` | No | Generated by deploy.sh |
