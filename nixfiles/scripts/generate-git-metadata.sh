#!/bin/sh

if [ ! -d .git ]; then
  echo "Not a git repository."
  exit 0
fi
echo -e "Checking git status at $(basename `git rev-parse --show-toplevel`)" >&2
GIT_COMMIT=$(git rev-parse HEAD)
GIT_DATE=$(git show -s --format=%ci HEAD)
GIT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
GIT_DIRTY=$(git status --porcelain)
echo -e "commit: $GIT_COMMIT\ndate: $GIT_DATE\nbranch: $GIT_BRANCH"
echo "$GIT_DIRTY" >&2
if [ -n "$GIT_DIRTY" ]; then
  echo -e "Uncommited changes:\n"
  git diff | cat
  git diff --cached | cat
else
  echo "Git repository is clean. No diff to write."
fi
