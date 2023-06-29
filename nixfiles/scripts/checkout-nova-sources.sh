#! /usr/bin/env nix-shell
#! nix-shell -i bash -p git

ROS_REPOS=(
    rover
    cameras2
)

OTHER_REPOS=(
    coms_utils
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
            git clone "https://github.com/MonashNovaRover/$repo.git" "$SRCDIR/$groupName/$repo"
        else
            echo "$repo" already exists. Skipping.
        fi
    done
}

checkout_group "ros" "${ROS_REPOS[@]}"
checkout_group "other" "${OTHER_REPOS[@]}"