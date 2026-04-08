/**
 * kagami-fw-helper — Minimal firewall helper for the kagami game server.
 *
 * This binary is deployed with CAP_NET_ADMIN capability set:
 *   sudo setcap cap_net_admin+ep kagami-fw-helper
 *
 * It manages an nftables named set "kagami_blocked" for efficient IP blocking.
 *
 * Usage:
 *   kagami-fw-helper init              Create table/sets/chain (call once on startup)
 *   kagami-fw-helper add <ip>          Add IP to block set
 *   kagami-fw-helper remove <ip>       Remove IP from block set
 *   kagami-fw-helper add-range <cidr>  Add CIDR range to block set
 *   kagami-fw-helper remove-range <cidr> Remove CIDR range
 *   kagami-fw-helper list              List blocked addresses
 *   kagami-fw-helper flush             Remove all blocks
 *
 * Security:
 *   - All nft operations use fork()+execvp() or nft -f - (stdin pipe)
 *   - No system() or popen() calls — no shell parsing surface
 *   - Validates all IP/CIDR inputs against strict format checks
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

#define NFT_TABLE  "kagami"
#define NFT_SET_V4 "blocked4"
#define NFT_SET_V6 "blocked6"
#define NFT_CHAIN  "input"

/* --- Validation ---------------------------------------------------------- */

static int is_valid_ipv4(const char *s) {
    int parts[4];
    char dummy;
    int n = sscanf(s, "%d.%d.%d.%d%c", &parts[0], &parts[1], &parts[2], &parts[3], &dummy);
    if (n != 4)
        return 0;
    for (int i = 0; i < 4; i++)
        if (parts[i] < 0 || parts[i] > 255)
            return 0;
    return 1;
}

static int is_valid_ipv6(const char *s) {
    int len = (int)strlen(s);
    /* RFC 5952: max 39 chars, must contain at least one colon */
    if (len < 2 || len > 39)
        return 0;

    int has_colon = 0;
    int consecutive_colons = 0;
    int double_colon_count = 0;
    int groups = 0;

    for (int i = 0; i < len; i++) {
        char c = s[i];
        if (c == ':') {
            has_colon = 1;
            consecutive_colons++;
            if (consecutive_colons == 2) {
                double_colon_count++;
                if (double_colon_count > 1)
                    return 0; /* At most one :: */
            } else if (consecutive_colons > 2) {
                return 0; /* ::: is invalid */
            }
        } else if (isxdigit((unsigned char)c)) {
            if (consecutive_colons > 0)
                groups++;
            consecutive_colons = 0;
        } else {
            return 0; /* Invalid character */
        }
    }

    if (!has_colon)
        return 0;
    /* Count last group if string doesn't end with : */
    if (len > 0 && s[len - 1] != ':')
        groups++;
    /* With ::, fewer than 8 groups is OK; without, need exactly 8 */
    if (double_colon_count == 0 && groups != 8)
        return 0;
    if (groups > 8)
        return 0;

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

    if (!slash || slash == s || !slash[1])
        return 0;

    size_t ip_len = (size_t)(slash - s);
    if (ip_len >= sizeof(ip))
        return 0;

    memcpy(ip, s, ip_len);
    ip[ip_len] = '\0';

    if (!is_valid_ip(ip))
        return 0;
    if (sscanf(slash + 1, "%d%c", &prefix, &dummy) != 1)
        return 0;

    int is_v6 = (strchr(ip, ':') != NULL);
    if (prefix < 0 || prefix > (is_v6 ? 128 : 32))
        return 0;

    return 1;
}

static int is_ipv6(const char *s) {
    return strchr(s, ':') != NULL;
}

/* --- Command execution --------------------------------------------------- */

/**
 * Run an external command via fork()+execvp(). No shell involved.
 * Returns the exit code, or -1 on fork/exec failure.
 */
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

/**
 * Run nft with commands provided via stdin (-f -).
 * This avoids shell parsing entirely while supporting complex nft syntax
 * (braces, semicolons) that can't be passed as argv.
 * Returns the exit code, or -1 on failure.
 */
static int run_nft_stdin(const char *script) {
    int pipefd[2];
    if (pipe(pipefd) < 0) {
        perror("pipe");
        return -1;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        close(pipefd[0]);
        close(pipefd[1]);
        return -1;
    }

    if (pid == 0) {
        /* Child: read nft commands from stdin */
        close(pipefd[1]); /* Close write end */
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);
        execlp("nft", "nft", "-f", "-", (char *)NULL);
        _exit(127);
    }

    /* Parent: write script to pipe, then close */
    close(pipefd[0]);
    size_t len = strlen(script);
    ssize_t written = write(pipefd[1], script, len);
    close(pipefd[1]);

    if (written < 0 || (size_t)written != len)
        fprintf(stderr, "kagami-fw-helper: warning: partial write to nft stdin\n");

    int status;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

/* --- nftables operations ------------------------------------------------- */

/**
 * Create the kagami table, sets, chain, and drop rules.
 * Idempotent — safe to call multiple times.
 * Called explicitly via 'init' action (once on server startup).
 */
static int nft_init(void) {
    fprintf(stderr, "kagami-fw-helper: initializing nftables table '%s'\n", NFT_TABLE);

    /* Build the full nft script as a single atomic operation */
    char script[2048];
    snprintf(script, sizeof(script),
        "add table inet %s\n"
        "add set inet %s %s { type ipv4_addr; flags interval; }\n"
        "add set inet %s %s { type ipv6_addr; flags interval; }\n"
        "add chain inet %s %s { type filter hook input priority 0; policy accept; }\n"
        "add rule inet %s %s ip saddr @%s drop\n"
        "add rule inet %s %s ip6 saddr @%s drop\n",
        NFT_TABLE,
        NFT_TABLE, NFT_SET_V4,
        NFT_TABLE, NFT_SET_V6,
        NFT_TABLE, NFT_CHAIN,
        NFT_TABLE, NFT_CHAIN, NFT_SET_V4,
        NFT_TABLE, NFT_CHAIN, NFT_SET_V6);

    int rc = run_nft_stdin(script);
    if (rc != 0)
        fprintf(stderr, "kagami-fw-helper: init failed (exit %d)\n", rc);
    return rc;
}

static int nft_add(const char *addr) {
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
    const char *cmd4[] = {"nft", "list", "set", "inet", NFT_TABLE, NFT_SET_V4, NULL};
    run_cmd(cmd4);
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
        "  init              Create nftables table/sets/chain (call once)\n"
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

    if (strcmp(action, "init") == 0)
        return nft_init();

    if (strcmp(action, "list") == 0)
        return nft_list();

    if (strcmp(action, "flush") == 0)
        return nft_flush();

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
