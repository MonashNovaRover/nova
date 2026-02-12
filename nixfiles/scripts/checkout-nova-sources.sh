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

SRCDIR="$(git rev-parse --show-toplevel)/../src"
mkdir -p "$SRCDIR"

# Check for the use of SSH or HTTPS
USE_SSH=false
while getopts ":s" opt; do
    case ${opt} in
        s )
            USE_SSH=true
            ;;
        \? )
            echo "Invalid option: -$OPTARG" >&2
            exit 1
            ;;
        : )
            echo "Invalid option: -$OPTARG requires an argument" >&2
            exit 1
            ;;
    esac
done
shift $((OPTIND -1))

checkout_group() {
    groupName="$1"
    shift
    repos=("$@")

    for repo in "${repos[@]}"; do
        if [ ! -d "$SRCDIR/$groupName/$repo" ]; then
            echo Cloning "$repo"...
            if $USE_SSH; then
              git clone --recurse-submodules "git@github.com:/MonashNovaRover/$repo.git" "$SRCDIR/$groupName/$repo"
            else
              git clone --recurse-submodules "https://github.com/MonashNovaRover/$repo.git" "$SRCDIR/$groupName/$repo"
            fi
        else
            echo "$repo" already exists. Pulling.
            git -C "$SRCDIR/$groupName/$repo" pull
        fi
    done
}

checkout_group "ros" "${ROS_REPOS[@]}"
checkout_group "other" "${OTHER_REPOS[@]}"
