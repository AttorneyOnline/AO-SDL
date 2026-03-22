#!/usr/bin/env python3
"""Parse CI failures from GitHub Actions for the current repo.

Auth: uses GITHUB_TOKEN env var, or reads from ~/.config/aosdl/github_token.
If no token found, opens browser to create a PAT.

Usage:
  python3 scripts/ci-failures.py          # latest run
  python3 scripts/ci-failures.py 12345    # specific run ID
"""

import json
import os
import sys
import urllib.request
import urllib.error

TOKEN_PATH = os.path.expanduser("~/.config/aosdl/github_token")
PAT_URL = "https://github.com/settings/tokens/new?scopes=repo&description=aosdl-ci-viewer"
API = "https://api.github.com"


def load_token():
    t = os.environ.get("GITHUB_TOKEN")
    if t:
        return t
    if os.path.exists(TOKEN_PATH):
        with open(TOKEN_PATH) as f:
            return f.read().strip()
    return None


def save_token(token):
    os.makedirs(os.path.dirname(TOKEN_PATH), exist_ok=True)
    with open(TOKEN_PATH, "w") as f:
        f.write(token)
    os.chmod(TOKEN_PATH, 0o600)


def prompt_for_token():
    print("\n  No GitHub token found. Create a Personal Access Token:\n")
    print(f"    {PAT_URL}\n")
    try:
        import webbrowser
        webbrowser.open(PAT_URL)
    except Exception:
        pass
    token = input("  Paste token here: ").strip()
    if not token:
        print("  No token provided.")
        sys.exit(1)
    save_token(token)
    print("  Token saved.\n")
    return token


def api_get(path, token):
    url = f"{API}{path}" if path.startswith("/") else path
    req = urllib.request.Request(url, headers={
        "Authorization": f"token {token}",
        "Accept": "application/vnd.github.v3+json",
    })
    try:
        return json.loads(urllib.request.urlopen(req).read())
    except urllib.error.HTTPError as e:
        if e.code == 401:
            if os.path.exists(TOKEN_PATH):
                os.remove(TOKEN_PATH)
            print("Token expired or invalid. Re-run to re-authenticate.")
            sys.exit(1)
        raise


def api_get_text(path, token):
    """GET that follows redirects and returns raw text (for log downloads)."""
    url = f"{API}{path}" if path.startswith("/") else path
    req = urllib.request.Request(url, headers={
        "Authorization": f"token {token}",
    })
    try:
        resp = urllib.request.urlopen(req)
        return resp.read().decode("utf-8", errors="replace")
    except urllib.error.HTTPError:
        return None


def get_repo():
    import subprocess
    out = subprocess.check_output(
        ["git", "remote", "get-url", "origin"],
        cwd=os.path.dirname(os.path.abspath(__file__)) or ".",
        text=True,
    ).strip()
    if out.startswith("git@"):
        out = out.split(":", 1)[1]
    else:
        out = "/".join(out.split("/")[-2:])
    return out.removesuffix(".git")


def get_failed_step_logs(token, repo, job_id, failed_step_names):
    """Get logs for a job and extract sections for failed steps."""
    log = api_get_text(f"/repos/{repo}/actions/jobs/{job_id}/logs", token)
    if not log:
        return None

    # GitHub logs have step headers like "##[group]Run step name" or
    # timestamps with step markers. Parse sections by step.
    # Each step section starts with a line containing the step name.
    sections = {}
    current_step = None
    current_lines = []

    for line in log.split("\n"):
        # Detect step boundaries (GitHub format: "YYYY-MM-DDTHH:MM:SS.xxxZ ##[group]Step Name")
        if "##[group]" in line:
            if current_step:
                sections[current_step] = current_lines
            current_step = line.split("##[group]")[-1].strip()
            current_lines = []
        elif current_step:
            current_lines.append(line)

    if current_step:
        sections[current_step] = current_lines

    # Try to find logs for failed steps
    results = {}
    for step_name in failed_step_names:
        # Fuzzy match - step names in logs may differ slightly
        for key, lines in sections.items():
            if step_name.lower() in key.lower() or key.lower() in step_name.lower():
                results[step_name] = lines
                break

    # If no fuzzy match found, extract error lines from the full log
    if not results:
        results["(full log)"] = log.split("\n")

    return results


def extract_errors(lines, context=3, max_lines=60):
    """Extract lines around errors from a list of log lines."""
    error_indices = set()
    keywords = ["error:", "Error:", "ERROR", "FAILED", "fatal:", "Fatal:",
                 "error[", "error C", "undefined reference", "No such file",
                 "cannot find", "not found", "CMake Error"]

    for i, line in enumerate(lines):
        if any(kw in line for kw in keywords):
            for j in range(max(0, i - context), min(len(lines), i + context + 1)):
                error_indices.add(j)

    if not error_indices:
        # No keyword matches — return last N lines
        start = max(0, len(lines) - 30)
        return [lines[i] for i in range(start, len(lines))]

    # Group contiguous indices into chunks, cap total output
    sorted_idx = sorted(error_indices)
    result = []
    prev = -10

    for idx in sorted_idx:
        if idx > prev + 1 and result:
            result.append("  ...")
        result.append(lines[idx])
        prev = idx
        if len(result) >= max_lines:
            result.append("  ... (truncated)")
            break

    return result


def main():
    token = load_token()
    if not token:
        token = prompt_for_token()

    # Verify
    try:
        api_get("/user", token)
    except Exception:
        if os.path.exists(TOKEN_PATH):
            os.remove(TOKEN_PATH)
        print("Token invalid. Re-run to re-authenticate.")
        sys.exit(1)

    repo = get_repo()
    print(f"Repository: {repo}")

    # Get run
    run_id_arg = sys.argv[1] if len(sys.argv) > 1 else None
    if run_id_arg:
        run = api_get(f"/repos/{repo}/actions/runs/{run_id_arg}", token)
    else:
        runs = api_get(f"/repos/{repo}/actions/runs?per_page=1", token)
        if not runs.get("workflow_runs"):
            print("No workflow runs found.")
            return
        run = runs["workflow_runs"][0]

    run_id = run["id"]
    conclusion = run.get("conclusion", "in_progress")
    commit_msg = run["head_commit"]["message"].split("\n")[0][:60]

    print(f"Run #{run['run_number']}: {commit_msg}")
    print(f"Branch: {run['head_branch']} | Status: {run['status']} | Conclusion: {conclusion}")
    print(f"{run['html_url']}\n")

    # Get jobs
    all_jobs = api_get(f"/repos/{repo}/actions/runs/{run_id}/jobs?per_page=100", token)
    jobs = all_jobs.get("jobs", [])

    # Summary table
    print(f"  {'Job':<35} {'Conclusion':<12}")
    print(f"  {'-' * 47}")
    for j in jobs:
        icon = {"success": "✓", "failure": "✗", "cancelled": "—", "skipped": "○"}.get(
            j.get("conclusion", ""), "…")
        print(f"  {icon} {j['name']:<33} {j.get('conclusion', j['status']):<12}")
    print()

    if conclusion == "success":
        print("All jobs passed!")
        return

    # Check annotations for quick error context
    annotations = api_get(
        f"/repos/{repo}/check-runs/{jobs[0]['id'] if jobs else 0}/annotations",
        token,
    ) if jobs else []

    # Detail failures
    failed = [j for j in jobs if j.get("conclusion") == "failure"]
    if not failed:
        print("No failures (run may still be in progress).")
        return

    for job in failed:
        failed_steps = [s for s in job.get("steps", []) if s.get("conclusion") == "failure"]
        step_names = [s["name"] for s in failed_steps]

        print(f"{'━' * 70}")
        print(f"✗ FAILED: {job['name']}")
        for s in failed_steps:
            print(f"  └ Step: {s['name']}")
        print(f"{'━' * 70}")

        # Try annotations first (structured errors)
        job_annotations = api_get(
            f"/repos/{repo}/check-runs/{job['id']}/annotations", token
        )
        if job_annotations:
            for ann in job_annotations:
                level = ann.get("annotation_level", "")
                msg = ann.get("message", "")
                path = ann.get("path", "")
                line = ann.get("start_line", "")
                if msg:
                    prefix = f"  {path}:{line}: " if path else "  "
                    print(f"{prefix}{msg}")
            print()

        # Then get full logs for failed steps
        step_logs = get_failed_step_logs(token, repo, job["id"], step_names)
        if step_logs:
            for step_name, lines in step_logs.items():
                errors = extract_errors(lines)
                if errors:
                    for line in errors:
                        # Strip timestamp prefixes (2024-01-01T00:00:00.000Z)
                        clean = line
                        if len(clean) > 28 and clean[4] == '-' and 'T' in clean[:20]:
                            clean = clean[28:] if len(clean) > 28 else clean
                        print(f"  {clean.rstrip()}")
            print()
        else:
            print("  (Could not retrieve logs)\n")


if __name__ == "__main__":
    main()
