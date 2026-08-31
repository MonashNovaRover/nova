---
name: fix-errors
description: 'Get one single build error from @ws-build, then explain, fix, and @git-commit it. Then repeat until there are no more errors.'
allowed-tools: Bash
---
Repeat this loop until @ws-build completes successfully with no build errors:

1. MANDATORY FIRST ACTION: Invoke @ws-build.
   Do not inspect, diagnose, modify files, run another build command, or take any
   other action before invoking @ws-build.

2. If @ws-build succeeds with no build errors, stop.

3. From the @ws-build output, select exactly ONE build error.

4. Explain the cause of that error.

5. Fix only that error.

6. Invoke @git-commit to commit the fix.

7. Start a new iteration. The first action of every new iteration MUST be a fresh
   invocation of @ws-build.

Rules:
- Never skip @ws-build.
- Never skip @git-commit.
- Every iteration begins with a fresh @ws-build invocation.
- Do not fix multiple unrelated errors in one iteration.
- Do not stop after a fix or commit; rerun @ws-build.
- Only stop when @ws-build itself reports no build errors.