modifyCheckPhase() {
    # Ensure hook only runs once
    if [ -n "${_mockJcanHookDone:-}" ]; then
        return
    fi
    local mock_jcan_dir
    mock_jcan_dir=$(ls -d "$out"/lib/python*/site-packages 2>/dev/null | head -n 1)

    if [ -n "$mock_jcan_dir" ]; then
        _mockJcanHookDone=1

        # add mock jcan to python path in preCheck
        preCheck="
export PYTHONPATH=\"$mock_jcan_dir:\$PYTHONPATH\"
echo 'Injecting mock jcan package into PYTHONPATH for testing...'
${preCheck:-}"
    fi
}

# Run the hook for the target package
addEnvHooks "$targetOffset" modifyCheckPhase