---
name: git-commit
description: 'Execute git commit with conventional commit message analysis, intelligent staging, and message generation. Use when user asks to commit changes, create a git commit, or mentions "/commit". Supports: (1) Auto-detecting type and scope from changes, (2) Generating conventional commit messages from diff, (3) Interactive commit with optional type/scope/description overrides, (4) Intelligent file staging for logical grouping'
license: MIT
allowed-tools: Bash
source: https://github.com/github/awesome-copilot/blob/main/skills/git-commit/SKILL.md?plain=1
---

# Git Commit with Conventional Commits

## Overview

Create and push standardized, semantic git commits using the Conventional Commits specification. Analyze the actual diff to determine appropriate type, scope, and message. Do NOT skip `git push`.

## Conventional Commit Format

```
<type>[optional scope]: <description>

[optional body]

[optional footer(s)]
```

## Commit Types

| Type       | Purpose                        |
| ---------- | ------------------------------ |
| `feat`     | New feature                    |
| `fix`      | Bug fix                        |
| `test`     | Add/update tests               |
| `misc`     | misc                           |

## Workflow

### 1. Analyze Diff

```bash
# If files are staged, use staged diff
git diff --staged

# If nothing staged, use working tree diff
git diff

# Also check status
git status --porcelain
```

### 2. Stage Files and Checkout Submodule (if needed)

If nothing is staged:

```bash
# Stage all files
git add ~/nova/

# If files are in a submodule, stash them, checkout to a new branch in that submodule, and pop the files from the stash
export GIT_BRANCH=$(git branch --show-current)
git stash && git checkout -b $GIT_BRANCH && git push --set-upstream origin $GIT_BRANCH && git stash pop
```

**Never commit secrets** (.env, credentials.json, private keys).

### 3. Generate Commit Message

Analyze the diff to determine:

- **Type**: What kind of change is this?
- **Scope**: What area/module is affected?
- **Description**: One-line summary of what changed (present tense, imperative mood, <72 chars)

### 4. Execute and Push Commit

```bash
# Single line
git commit -m "<type>[scope]: <description>" && git push

# Multi-line with body/footer
git commit -m "$(cat <<'EOF'
<type>[scope]: <description>

<optional body>

<optional footer>
EOF
)" && git push
```

## Best Practices

- One logical change per commit
- Present tense: "add" not "added"
- Imperative mood: "fix bug" not "fixes bug"
- Keep description under 72 characters

## Git Safety Protocol

- NEVER update git config
- NEVER run destructive commands (--force, hard reset) without explicit request
- NEVER skip hooks (--no-verify) unless user asks
- NEVER force push to main/master
- NEVER git commit in a submodule unless checked out to a new branch
- If commit fails due to hooks, fix and create NEW commit (don't amend)