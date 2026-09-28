modifyCheckPhase() {
    # Ensure hook only runs once
    if [ -n "${_pytestFrameworkHookDone:-}" ]; then
        return
    fi

    _pytestFrameworkHookDone=1

    # Change log dir to allow ros to run during checkPhase
    preCheck="
export ROS_LOG_DIR="$TMPDIR/.ros/log"
${preCheck:-}"
}

# Run the hook for the target package
addEnvHooks "$targetOffset" modifyCheckPhase