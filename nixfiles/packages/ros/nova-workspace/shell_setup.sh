#! @runtimeShell@

# This setup script is designed to run in Bash and Zsh at a minimum.
# It is not in charge of setting up any search paths; that should be done with Nix tooling.

# Set up autocompletion.
eval "$(@argcomplete@/bin/register-python-argcomplete ros2)"
eval "$(@argcomplete@/bin/register-python-argcomplete colcon)"