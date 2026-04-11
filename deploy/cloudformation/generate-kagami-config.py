#!/usr/bin/env python3
"""
Generate a default kagami.json from CF_* environment variables.

This script is downloaded from S3 at instance boot by the
CloudFormation UserData script and executed in the else branch of
the "ConfigS3URL is set?" check — i.e. only when the operator did
NOT supply a pre-built kagami.json and expects the CFN form fields
to drive the config. Extracted out of the CFN template so the
UserData payload stays under EC2's 25600-byte limit.

Reads user-supplied values via os.environ (raw strings from the
exec(3) environment block) so single quotes and other shell
metacharacters in operator input can never escape into Python
source. The quoted-heredoc invocation inside UserData is what
guarantees zero shell expansion before we get here.

Writes kagami.json to the current working directory (UserData cd's
to /opt/kagami before running this).
"""
import json
import os


def env(k, default=''):
    v = os.environ.get(k, default)
    return v if v is not None else default


cm_enabled = env('CF_CM_ENABLED') == 'true'
openai_key = env('CF_OPENAI_KEY')
embed_model = env('CF_EMBED_MODEL')
slur_wordlist = env('CF_SLUR_WORDLIST')
slur_exceptions = env('CF_SLUR_EXCEPTIONS')
safe_hint_url = env('CF_SAFE_HINT_URL')

cfg = {
    'domain': env('CF_DOMAIN'),
    'server_name': env('CF_SERVER_NAME'),
    'server_description': env('CF_SERVER_DESC'),
    'bind_address': '0.0.0.0',
    'http_port': 8080,
    'ws_port': 27016,
    'wss_port': 27015,
    'cors_origin': '*',
    'max_players': int(env('CF_MAX_PLAYERS', '100')),
    'motd': env('CF_MOTD'),
    'asset_url': env('CF_ASSET_URL'),
    'mod_password': env('CF_MOD_PASSWORD'),
    'session_ttl_seconds': 300,
    'log_level': 'info',
    'log_file': '/logs/kagami.log',
    'log_file_level': 'verbose',
    'loki_url': 'http://127.0.0.1:3100',
    'metrics_enabled': True,
    'metrics_path': '/metrics',
    'advertiser': {
        'enabled': env('CF_ADVERTISE') == 'true',
        'url': 'https://servers.aceattorneyonline.com/servers',
        'ws_port': 80,
        'wss_port': 443,
    },
    'deploy': {
        'tls': 'auto',
        'observability': True,
        'metrics_allow': '127.0.0.1 ::1',
        'image': 'ghcr.io/attorneyonline/kagami:latest',
        'grafana_user': 'admin',
        'grafana_password': env('CF_GRAFANA_PASSWORD'),
    },
    'cloudwatch': {
        'region': env('CF_CW_REGION'),
        'log_group': env('CF_CW_LOG_GROUP'),
        'log_stream': 'server',
        # Empty credential fields = IMDSv2 path via instance role.
        # kagami's CloudWatchSink detects the empty key and falls
        # through to ImdsCredentialProvider for temporary creds.
        'access_key_id': '',
        'secret_access_key': '',
        'flush_interval': 5,
        'log_level': 'info',
    },
    'reverse_proxy': {
        'enabled': False,
        'trusted_proxies': [],
        'header_priority': ['X-Forwarded-For', 'X-Real-IP'],
        'proxy_protocol': False,
    },
    'reputation': {
        'enabled': True,
        'cache_ttl_hours': 24,
        'cache_failure_ttl_minutes': 5,
        'ip_api_enabled': True,
        'abuseipdb_api_key': '',
        'abuseipdb_daily_budget': 1000,
        'auto_block_proxy': False,
        'auto_block_datacenter': False,
    },
    'asn_reputation': {
        'enabled': True,
        'watch_threshold': 2,
        'rate_limit_threshold': 3,
        'block_threshold': 5,
        'window_minutes': 60,
        'auto_block_duration': '24h',
        'whitelist_asns': [],
        'whitelist_multiplier': 5,
    },
    'spam_detection': {
        'enabled': True,
        'echo_threshold': 3,
        'echo_window_seconds': 60,
        'burst_threshold': 20,
        'burst_window_seconds': 30,
        'join_spam_max_seconds': 5,
        'name_pattern_threshold': 3,
        'name_pattern_min_prefix': 4,
        'name_pattern_window_seconds': 300,
        'ghost_threshold': 5,
        'hwid_reuse_threshold': 3,
    },
    'content_moderation': {
        # Opt-in subsystem. Each sub-layer has its own enabled flag
        # so operators can mix and match (e.g. unicode + urls only,
        # no remote classifier). All defaults produce zero traffic.
        'enabled': cm_enabled,
        'check_ic': True,
        'check_ooc': True,
        'message_sample_length': 200,
        'unicode': {
            'enabled': cm_enabled,
            'combining_mark_threshold': 0.3,
            'exotic_script_threshold': 0.3,
            'format_char_threshold': 0.1,
            'max_score': 1.0,
        },
        'urls': {
            'enabled': cm_enabled,
            'blocklist': [],
            'allowlist': [],
            'blocked_score': 1.0,
            'unknown_url_score': 0.0,
        },
        'slurs': {
            # Layer 1c: activates only when subsystem is on AND a
            # wordlist URL is provided. Fetch is on a background
            # thread in main.cpp with retry + persistent cache.
            'enabled': cm_enabled and len(slur_wordlist) > 0,
            'wordlist_url': slur_wordlist,
            'exceptions_url': slur_exceptions,
            'cache_dir': '/opt/kagami/moderation-cache',
            'match_score': 1.0,
        },
        'remote': {
            # OpenAI omni-moderation. Needs a non-empty API key.
            'enabled': cm_enabled and len(openai_key) > 0,
            'provider': 'openai',
            'api_key': openai_key,
            'endpoint': 'https://api.openai.com/v1/moderations',
            'model': 'omni-moderation-latest',
            'timeout_ms': 3000,
            'fail_open': True,
            # Dedup cache eliminates duplicate API calls for repeat
            # messages (spam waves, echoed chatter). Enabled by
            # default when the remote layer itself is enabled —
            # pure win if the remote layer is on, zero effect if
            # it's off. See RemoteDedupCache.h for design.
            'cache_enabled': cm_enabled and len(openai_key) > 0,
            'cache_ttl_seconds': 300,
            'cache_max_entries': 1000,
        },
        'safe_hint': {
            # Layer 2 shortcut — requires both the embeddings layer
            # (for the backend) AND a non-empty anchor URL.
            'enabled': (
                cm_enabled and len(safe_hint_url) > 0 and len(embed_model) > 0
            ),
            'anchors_url': safe_hint_url,
            'cache_dir': '/opt/kagami/moderation-cache',
            'similarity_threshold': 0.7,
        },
        'embeddings': {
            # Inert unless subsystem is on + HF model id provided.
            'enabled': cm_enabled and len(embed_model) > 0,
            'hf_model_id': embed_model,
            # Shared moderation_cache volume — GGUF persists across
            # container recreates instead of re-downloading.
            'cache_dir': '/opt/kagami/moderation-cache',
            'ring_size': 500,
            'similarity_threshold': 0.9,
            'cluster_threshold': 3,
            'window_seconds': 60,
        },
        'heat': {
            'decay_half_life_seconds': 600.0,
            'censor_threshold': 1.0,
            'drop_threshold': 3.0,
            'mute_threshold': 6.0,
            'kick_threshold': 10.0,
            'ban_threshold': 15.0,
            'perma_ban_threshold': 25.0,
            'mute_duration_seconds': 900,
            'ban_duration_seconds': 86400,
            'weight_visual_noise': 0.5,
            'weight_link_risk': 5.0,
            # 6.0 = mute_threshold → single slur match = instant mute.
            # Lower to 3.0 for a drop-then-escalate policy.
            'weight_slurs': 6.0,
            # Roleplay-friendly defaults. Per-axis floors in
            # ContentModerator.cpp filter sub-dramatic in-character
            # language before these weights apply.
            'weight_toxicity': 1.0,
            'weight_hate': 4.0,
            'weight_sexual': 1.5,
            'weight_sexual_minors': 100.0,
            'weight_violence': 1.0,
            'weight_self_harm': 1.0,
            'weight_semantic_echo': 2.0,
        },
        'trust_bank': {
            # Trust bank (Layer 2 skip): clean messages accrue
            # negative heat which probabilistically skips the remote
            # classifier call. Opt-in — operators enable it explicitly
            # once they've confirmed their traffic mix benefits.
            # See the TrustBankConfig doc in ContentModerationConfig.h
            # for the full semantics.
            'enabled': False,
            'clean_reward': 0.1,
            'max_trust': 10.0,
            'api_skip_threshold': 5.0,
            'min_sample_rate': 0.05,
        },
        'audit': {
            'stdout_enabled': False,
            'file_path': '/logs/kagami_mod_audit.jsonl',
            'loki_url': 'http://127.0.0.1:3100',
            'loki_stream_label': 'kagami_mod_audit',
            'cloudwatch_log_group': '',
            'cloudwatch_log_stream': '',
            'sqlite_enabled': True,
            'min_action': 'log',
        },
    },
    'firewall': {
        'enabled': True,
        'helper_path': '/app/kagami-fw-helper',
        'cleanup_on_shutdown': True,
    },
    'rate_limits': {
        'session_create': {'rate': 2.0, 'burst': 5.0},
        'ws_frame': {'rate': 30.0, 'burst': 60.0},
        'ws_bytes': {'rate': 32768.0, 'burst': 65536.0},
        'ao:MS': {'rate': 5.0, 'burst': 10.0},
        'ao:CT': {'rate': 3.0, 'burst': 6.0},
        'ao:CC': {'rate': 2.0, 'burst': 4.0},
        'ao:MC': {'rate': 2.0, 'burst': 5.0},
        'ao:CH': {'rate': 2.0, 'burst': 5.0},
        'ao:HP': {'rate': 2.0, 'burst': 4.0},
        'ao:BN': {'rate': 2.0, 'burst': 4.0},
        'ao:RT': {'rate': 3.0, 'burst': 6.0},
        'ao:PE': {'rate': 2.0, 'burst': 5.0},
        'ao:EE': {'rate': 2.0, 'burst': 5.0},
        'ao:DE': {'rate': 2.0, 'burst': 5.0},
        'ao:ZZ': {'rate': 1.0, 'burst': 3.0},
        'ao:CASEA': {'rate': 0.5, 'burst': 2.0},
        'ao:SETCASE': {'rate': 1.0, 'burst': 3.0},
        'nx:ooc': {'rate': 3.0, 'burst': 6.0},
        'nx:ic': {'rate': 3.0, 'burst': 6.0},
        'nx:area_join': {'rate': 2.0, 'burst': 5.0},
        'nx:char_select': {'rate': 2.0, 'burst': 4.0},
        'nx:session_renew': {'rate': 1.0, 'burst': 3.0},
        'ws_handshake_deadline_sec': 10,
        'ws_idle_timeout_sec': 120,
        'ws_partial_frame_timeout_sec': 30,
        'ws_max_frame_size': 65536,
    },
}

json.dump(cfg, open('kagami.json', 'w'), indent=4)
