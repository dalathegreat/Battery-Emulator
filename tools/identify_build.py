import os
import subprocess

Import("env")


def git(*args):
    return subprocess.check_output(["git", *args], stderr=subprocess.DEVNULL).decode().strip()


def define_str(macro, value):
    safe = value.replace("\\", "\\\\").replace('"', '\\"')
    env.Append(CPPDEFINES=[(macro, f'\\"{safe}\\"')])


# Exact tag (release build)
try:
    define_str("GIT_TAG", git("describe", "--tags", "--exact-match"))
except subprocess.CalledProcessError:
    pass

# Ancestor tag (dev build base)
try:
    define_str("GIT_ANCESTOR_TAG", git("describe", "--tags", "--abbrev=0"))
except subprocess.CalledProcessError:
    pass

# Full SHA + short form
try:
    sha = git("rev-parse", "HEAD")
    define_str("GIT_SHA", sha)
    define_str("GIT_SHORT_SHA", sha[:7])
except subprocess.CalledProcessError:
    pass

# GitHub org + repo name (split from "owner/repo")
if repo_full := os.environ.get("GITHUB_REPOSITORY", "").strip():
    if "/" in repo_full:
        org, repo = repo_full.split("/", 1)
        define_str("GITHUB_ORG", org)
        define_str("GITHUB_REPO", repo)

# PR number (passed explicitly from workflow env)
if pr := os.environ.get("GITHUB_PR", "").strip():
    define_str("GITHUB_PR", pr)

# PR head SHA (the actual PR commit, not the synthetic merge commit)
if pr_head_sha := os.environ.get("GITHUB_PR_HEAD_SHA", "").strip():
    define_str("GITHUB_PR_HEAD_SHA", pr_head_sha)
    define_str("GITHUB_PR_HEAD_SHORT_SHA", pr_head_sha[:7])

# Local branch (omit when detached HEAD)
try:
    branch = git("rev-parse", "--abbrev-ref", "HEAD")
    if branch and branch != "HEAD":
        define_str("GIT_BRANCH", branch)
except subprocess.CalledProcessError:
    pass
