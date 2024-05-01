#! /usr/bin/env nix-shell
#! nix-shell -i bash -p git

ROS_REPOS=(
    rover
    cameras2
    gui
    nova-gui
)

OTHER_REPOS=(
    coms_utils
    libblcmd
    libcanmd
)

SRCDIR="$(git rev-parse --show-toplevel)/external/src"
mkdir -p "$SRCDIR"

checkout_group() {
    groupName="$1"
    shift
    repos=("$@")

    for repo in "${repos[@]}"; do
        if [ ! -d "$SRCDIR/$groupName/$repo" ]; then
            echo Cloning "$repo"...
            git clone --recurse-submodules "https://github.com/MonashNovaRover/$repo.git" "$SRCDIR/$groupName/$repo"
        else
            echo "$repo" already exists. Pulling.
            git -C "$SRCDIR/$groupName/$repo" pull
        fi
    done
}

checkout_group "ros" "${ROS_REPOS[@]}"
checkout_group "other" "${OTHER_REPOS[@]}"
