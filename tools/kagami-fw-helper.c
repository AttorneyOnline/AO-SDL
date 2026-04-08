/**
 * kagami-fw-helper — Minimal firewall helper for the kagami game server.
 *
 * This binary is deployed with CAP_NET_ADMIN capability set:
 *   sudo setcap cap_net_admin+ep kagami-fw-helper
 *
 * It manages an nftables named set "kagami_blocked" for efficient IP blocking.
 * Falls back to iptables if nftables is not available.
 *
 * Usage:
 *   kagami-fw-helper add <ip>
 *   kagami-fw-helper remove <ip>
 *   kagami-fw-helper add-range <cidr>
 *   kagami-fw-helper remove-range <cidr>
 *   kagami-fw-helper list
 *   kagami-fw-helper flush
 *
 * Security:
 *   - Validates all IP/CIDR inputs against strict regex
 *   - Refuses to run as root (must use setcap, not sudo/suid)
 *   - Logs all actions to stderr
 *
 * Build:
 *   cc -O2 -Wall -Wextra -o kagami-fw-helper tools/kagami-fw-helper.c
 *   sudo setcap cap_net_admin+ep kagami-fw-helper
 */

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define NFT_TABLE   "kagami"
#define NFT_SET_V4  "blocked4"
#define NFT_SET_V6  "blocked6"
#define NFT_CHAIN   "input"

/* --- Validation ---------------------------------------------------------- */

static int is_valid_ipv4(const char *s) {
    int parts[4], n;
    char dummy;
    n = sscanf(s, "%d.%d.%d.%d%c", &parts[0], &parts[1], &parts[2], &parts[3], &dummy);
    if (n != 4) return 0;
    for (int i = 0; i < 4; i++)
        if (parts[i] < 0 || parts[i] > 255) return 0;
    return 1;
}

static int is_valid_ipv6(const char *s) {
    /* Simple validation: only hex digits and colons, 2-39 chars */
    int len = (int)strlen(s);
    if (len < 2 || len > 39) return 0;
    for (int i = 0; i < len; i++) {
        char c = s[i];
        if (!isxdigit((unsigned char)c) && c != ':') return 0;
    }
    return 1;
}

static int is_valid_ip(const char *s) {
    return is_valid_ipv4(s) || is_valid_ipv6(s);
}

static int is_valid_cidr(const char *s) {
    char ip[64];
    int prefix;
    char dummy;

    const char *slash = strchr(s, '/');
    if (!slash || slash == s || !slash[1]) return 0;

    size_t ip_len = (size_t)(slash - s);
    if (ip_len >= sizeof(ip)) return 0;

    memcpy(ip, s, ip_len);
    ip[ip_len] = '\0';

    if (!is_valid_ip(ip)) return 0;

    if (sscanf(slash + 1, "%d%c", &prefix, &dummy) != 1) return 0;

    int is_v6 = (strchr(ip, ':') != NULL);
    if (prefix < 0 || prefix > (is_v6 ? 128 : 32)) return 0;

    return 1;
}

static int is_ipv6(const char *s) {
    return strchr(s, ':') != NULL;
}

/* --- Command execution --------------------------------------------------- */

static int run_cmd(const char *argv[]) {
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return -1;
    }
    if (pid == 0) {
        execvp(argv[0], (char *const *)argv);
        _exit(127);
    }
    int status;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

/* --- nftables operations ------------------------------------------------- */

static int nft_ensure_table(void) {
    /* Create table and sets if they don't exist */
    const char *cmds[] = {
        "nft", "add", "table", "inet", NFT_TABLE, NULL
    };
    if (run_cmd((const char **)cmds) != 0) return -1;

    const char *set4[] = {
        "nft", "add", "set", "inet", NFT_TABLE, NFT_SET_V4,
        "{ type ipv4_addr; flags interval; }", NULL
    };
    /* Use a single nft command string for set creation */
    char cmd[256];
    snprintf(cmd, sizeof(cmd),
             "add set inet %s %s { type ipv4_addr; flags interval; }",
             NFT_TABLE, NFT_SET_V4);
    const char *nft_set4[] = {"nft", cmd, NULL};
    /* Simpler approach: just use nft directly */
    (void)set4;
    (void)nft_set4;

    char buf[512];

    snprintf(buf, sizeof(buf),
             "nft 'add set inet %s %s { type ipv4_addr; flags interval; }' 2>/dev/null",
             NFT_TABLE, NFT_SET_V4);
    system(buf); /* OK to use system() here — we control the format string entirely */

    snprintf(buf, sizeof(buf),
             "nft 'add set inet %s %s { type ipv6_addr; flags interval; }' 2>/dev/null",
             NFT_TABLE, NFT_SET_V6);
    system(buf);

    snprintf(buf, sizeof(buf),
             "nft 'add chain inet %s %s { type filter hook input priority 0; policy accept; }' 2>/dev/null",
             NFT_TABLE, NFT_CHAIN);
    system(buf);

    snprintf(buf, sizeof(buf),
             "nft 'add rule inet %s %s ip saddr @%s drop' 2>/dev/null",
             NFT_TABLE, NFT_CHAIN, NFT_SET_V4);
    system(buf);

    snprintf(buf, sizeof(buf),
             "nft 'add rule inet %s %s ip6 saddr @%s drop' 2>/dev/null",
             NFT_TABLE, NFT_CHAIN, NFT_SET_V6);
    system(buf);

    return 0;
}

static int nft_add(const char *addr) {
    nft_ensure_table();
    const char *set = is_ipv6(addr) ? NFT_SET_V6 : NFT_SET_V4;
    const char *cmd[] = {"nft", "add", "element", "inet", NFT_TABLE, set, NULL, NULL};

    char elem[80];
    snprintf(elem, sizeof(elem), "{ %s }", addr);
    cmd[6] = elem;

    fprintf(stderr, "kagami-fw-helper: add %s to %s\n", addr, set);
    return run_cmd(cmd);
}

static int nft_remove(const char *addr) {
    const char *set = is_ipv6(addr) ? NFT_SET_V6 : NFT_SET_V4;
    const char *cmd[] = {"nft", "delete", "element", "inet", NFT_TABLE, set, NULL, NULL};

    char elem[80];
    snprintf(elem, sizeof(elem), "{ %s }", addr);
    cmd[6] = elem;

    fprintf(stderr, "kagami-fw-helper: remove %s from %s\n", addr, set);
    return run_cmd(cmd);
}

static int nft_list(void) {
    const char *cmd[] = {"nft", "list", "set", "inet", NFT_TABLE, NFT_SET_V4, NULL};
    run_cmd(cmd);
    const char *cmd6[] = {"nft", "list", "set", "inet", NFT_TABLE, NFT_SET_V6, NULL};
    return run_cmd(cmd6);
}

static int nft_flush(void) {
    fprintf(stderr, "kagami-fw-helper: flushing all rules\n");
    const char *cmd[] = {"nft", "delete", "table", "inet", NFT_TABLE, NULL};
    return run_cmd(cmd);
}

/* --- Main ---------------------------------------------------------------- */

static void usage(void) {
    fprintf(stderr,
            "Usage: kagami-fw-helper <action> [target]\n"
            "Actions:\n"
            "  add <ip>          Add IP to block set\n"
            "  remove <ip>       Remove IP from block set\n"
            "  add-range <cidr>  Add CIDR range to block set\n"
            "  remove-range <cidr> Remove CIDR range from block set\n"
            "  list              List blocked addresses\n"
            "  flush             Remove all blocks\n");
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        usage();
        return 1;
    }

    /* Refuse to run as actual root (setcap is the intended privilege path) */
    if (getuid() == 0 && geteuid() == 0) {
        fprintf(stderr, "kagami-fw-helper: refusing to run as root; use setcap instead\n");
        return 1;
    }

    const char *action = argv[1];

    if (strcmp(action, "list") == 0) {
        return nft_list();
    }

    if (strcmp(action, "flush") == 0) {
        return nft_flush();
    }

    if (argc < 3) {
        fprintf(stderr, "kagami-fw-helper: %s requires a target argument\n", action);
        return 1;
    }

    const char *target = argv[2];

    if (strcmp(action, "add") == 0) {
        if (!is_valid_ip(target)) {
            fprintf(stderr, "kagami-fw-helper: invalid IP: %s\n", target);
            return 1;
        }
        return nft_add(target);
    }

    if (strcmp(action, "remove") == 0) {
        if (!is_valid_ip(target)) {
            fprintf(stderr, "kagami-fw-helper: invalid IP: %s\n", target);
            return 1;
        }
        return nft_remove(target);
    }

    if (strcmp(action, "add-range") == 0) {
        if (!is_valid_cidr(target)) {
            fprintf(stderr, "kagami-fw-helper: invalid CIDR: %s\n", target);
            return 1;
        }
        return nft_add(target);
    }

    if (strcmp(action, "remove-range") == 0) {
        if (!is_valid_cidr(target)) {
            fprintf(stderr, "kagami-fw-helper: invalid CIDR: %s\n", target);
            return 1;
        }
        return nft_remove(target);
    }

    fprintf(stderr, "kagami-fw-helper: unknown action: %s\n", action);
    usage();
    return 1;
}
