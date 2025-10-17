# This nix-shell is designed to run on the base station to quickly spin up the entire stack for any payload!
# Usage (Also in macros/default.nix): nix-shell ${cfg.nixfileDir}/modules/home/macros/launch -A auto.arch --argstr rover-ip user@ip --argstr mast-ip user@ip
# Or nix-build ${cfg.nixfileDir}/modules/home/macros/launch -A auto.arch, then ~/Builds/build-name/bin/run-auto-arch user@rover-ip user@mast-ip
{ 
    pkgs ? import <nixpkgs> {}, 
    rover-ip ? "$1",
    mast-ip ? "$2",
    dir ? "$( dirname \"$\{BASH_SOURCE[0]}\" )", # default to dir the shell script is in
    route ? "",
}:

let 
  # are we running nix-shell or nix-build
  inShell = builtins.getEnv "IN_NIX_SHELL" != "";

  # ansi strings for nice colours
  ansi = {
    light-red = ''\033[1;31m'';
    orange = ''\033[0;33m'';
    yellow = ''\033[1;33m'';
    light-green = ''\033[1;32m'';
    light-purple = ''\033[1;35m'';
    nc = ''\033[0m''; # no colour
  };

  # these run a single command, and record it in history, you will need to make a custom line for multiple commands
  local-terminal = name: cmd: ''${pkgs.ptyxis}/bin/ptyxis --tab -d ${dir} --title="${name}" -x "bash -ic '${cmd}; history -s \"${cmd}\"; exec bash'"'';
  local-nix-terminal = shell: name: cmd: ''${pkgs.ptyxis}/bin/ptyxis --tab -d ${dir} --title="${name}" -x "bash -ic '${shell} --command \"${cmd}; history -s \\\"${cmd}\\\"; exec bash -l\"; history -s \"${shell}\"; exec bash -l'; exec bash -l"'';
  ssh-terminal = target: name: cmd: ''${pkgs.ptyxis}/bin/ptyxis --tab --title="${name}" -x "bash -ic 'ssh -t ${target} \"bash -ic \\\"cd ${dir}; ${cmd}; history -s ${cmd}; exec bash -l\\\"\"; history -s \"ssh -t ${target}\"; exec bash -l'"'';
  ssh-nix-terminal = target: shell: name: cmd: ''${pkgs.ptyxis}/bin/ptyxis --tab --title="${name}" -x "bash -ic 'ssh -t ${target} \"bash -ic \\\"${shell} --command \\\\\\\"cd ${dir}; ${cmd}; history -s \\\\\\\\\\\\\\\"${cmd}\\\\\\\\\\\\\\\"; exec bash -l\\\\\\\"; history -s \\\\\\\"${shell}\\\\\\\"; exec bash -l\\\"\"; history -s \"ssh -t ${target}\"; exec bash -l'"'';
  
  # aliases for each simple command
  base = local-terminal;
  base-nix = local-nix-terminal;
  rover = ssh-terminal rover-ip;
  rover-nix = ssh-nix-terminal rover-ip;
  mast = ssh-terminal mast-ip;

  # commands to automatically ssh into the targeted device
  ssh-check = payload-ip: "ssh-copy-id ${payload-ip}";
  rover-ssh-check = need-rover: if need-rover then ssh-check rover-ip else "";
  mast-ssh-check = need-mast: if need-mast then ssh-check mast-ip else "";

  # usage strings for nix-shell and nix-build respectively
  usage-string-shell = platform: "You must provide a target for ${platform} ssh commands.\n  Usage: nix-shell shell.nix --argstr ${platform}-ip <user@ip>\n  e.g nix-shell shell.nix --argstr ${platform}-ip nova@10.0.0.11";
  usage-string-build = platform: "${ansi.light-red}ERROR:${ansi.nc} ${platform}-ip is required for ssh commands. \\n  Usage: $0 \[-flag \<value\>\] \<nova@rover-ip\> \[nova@mast-ip\] \\n  e.g $0 nova@10.0.0.2 nova@10.0.0.3";

  # check that ip is present and optionally enforce this 
  rover-check = need-rover: if rover-ip == "$1" && need-rover && inShell
    then throw (usage-string-shell "rover")
    else if rover-ip == "$1" && !need-rover then "" else "  echo \"SSHing into rover at ${ansi.light-purple}${rover-ip}${ansi.nc}...\"";
  mast-check = need-mast: if mast-ip == "$2" && need-mast && inShell
    then throw (usage-string-shell "mast")
    else if mast-ip == "$2" && !need-mast then "" else "  echo \"SSHing into mast at ${ansi.light-purple}${mast-ip}${ansi.nc}...\"";

  # default pre-shell for setup before terminals
  # if statement prevents infinite loop
  # create tmp dir for nix-shell inception having no access to /tmp
  # echo tips
  # check for rover or mast ip if required
  # copy ssh keys if first time target so ssh is instant
  pre-shell = {payload-name, need-rover ? false, need-mast? false}: ''
    if [ -z "$SHELL_STARTED" ]; then
      export SHELL_STARTED=1
      export TMPDIR=/tmp
      echo -e "Launching ${ansi.light-green}${payload-name}${ansi.nc}... \nRunning in ${ansi.orange}${dir}${ansi.nc}..."
      ${rover-check need-rover}
      ${mast-check need-mast}
      ${rover-ssh-check need-rover}
      ${mast-ssh-check need-mast}
  '';
  # combine all the terminals together so that they all run simultaneously
  assembleTerminal = setup: "  " + (builtins.foldl' (acc: el: el.platform el.name el.cmd + "\ \\n  & " + acc ) "" setup);
  # default post-shell for after commands
  post-shell = ''
      exit 0
    fi
  '';

  # convert structure to bash shell
  #   pre = pre-shell {payload-name="name"; need-mast=true}; # you can string concat any additions to this, need-mast is optional and defaults to false
  #   terminals = {name, platform, command}[]; # An array of sets 
  #   post = post-shell + "\nAdditional command"; # make additions with a new line since it will be added to a shell hook!
  #   buildInputs = pkgs[]
  #   optional-args = {letter, variable, default}[] # letter must be single character, variable will be the bash variable that gets set to it, default is the default value of that variable
  mkBashScript = {pre ? pre-shell {payload-name="";}, terminals, post ? post-shell, buildInputs ? [], optional-args ? []}: {
    shellString = "${pre}\n${assembleTerminal terminals}\n${post}"; 
    inherit buildInputs optional-args;
  };

  # construct the shell and build environments, shellString is the final shell script created using mkBashScript, shellName is the final bash script name
  shellAndBuild = {shellString, buildInputs, optional-args}: shellName: pkgs.stdenv.mkDerivation {
    pname = "nova-" + shellName;
    version = "1.0";
    shellHook = (builtins.concatStringsSep "\n" (map (opt: opt.variable + "=\"" + opt.default + "\"") optional-args)) + "\n" + shellString;
    inherit buildInputs;

    # put it in the bin dir
    # implement optional arguments with flags
    # add checkers for $1 and $2 for rover and mast ips
    buildPhase = ''
      mkdir -p $out/bin
      cat > $out/bin/${shellName} <<'EOF'
      #!${pkgs.bash}/bin/bash
      # THIS FILE IS AUTO GENERATED BY NIX
      
      ${builtins.concatStringsSep "\n" (map (opt: opt.variable + "=\"" + opt.default + "\"") optional-args)}
      while getopts "${builtins.concatStringsSep ":" (map (opt: opt.letter) optional-args)}:" opt; do
        case "$opt" in
          ${builtins.concatStringsSep "\n    " (map (opt: opt.letter + ") " + opt.variable + "=\"$OPTARG\" ;;") optional-args)}
          *) echo "Usage: $0 \[-flag \<value\>\] \<nova@rover-ip\> \[nova@mast-ip\]"; exit 1 ;;
        esac
      done
      shift $((OPTIND - 1))

      ${
        builtins.replaceStrings [(rover-check true) (mast-check true)] 
        ["if [ -z \"$1\" ]; then\n    echo -e \"${usage-string-build "rover"}\"\n    exit 1\n  fi" 
        "if [ -z \"$2\" ]; then\n    echo -e \"${usage-string-build "mast"}\"\n    exit 1\n  fi"] 
        shellString # replace the nix-shell arg check with bash arg check
      }
      EOF
      chmod +x $out/bin/${shellName}
    '';
  };

  # final single function to pass to child nix files to make defining setups easy
  bashBuilder = struct: shellName: shellAndBuild (mkBashScript struct) shellName;

  callPackage = pkgs.lib.callPackageWith {inherit pkgs base base-nix rover rover-nix mast pre-shell post-shell bashBuilder route;};
in
rec { 
  # import more payloads here
  auto = callPackage ./auto.nix   { };
  drive = callPackage ./drive.nix { };
  gui = callPackage ./gui.nix     { };

  # pull nested setups and flatten into attributes of one set
  all-setups = builtins.removeAttrs (builtins.foldl' pkgs.lib.mergeAttrs { } [auto drive gui]) ["override" "overrideDerivation"];

  # build all setup scripts
  all = pkgs.stdenv.mkDerivation rec {
    pname = "nova-launch-scripts";
    version = "1.0";
    src = ./.;

    # get buildInputs of all setups
    buildInputs = [ pkgs.ptyxis ] ++ builtins.concatLists (
      builtins.attrValues (builtins.mapAttrs (_: drv: drv.buildInputs) all-setups)
    );

    # combine all buildPhases and optional bash shell input checks
    buildPhase = builtins.concatStringsSep "\n" (
      builtins.attrValues (builtins.mapAttrs (_: drv: drv.buildPhase) all-setups)
    );
  };

  # git metadata derivation
  git-metadata = pkgs.stdenv.mkDerivation {
    pname = "nova-git-metadata";
    version = "1.0";
    # this is relative to where this file is but it points at the root dir of the repo for git e.g /home/nova/nova
    # it slows the build down significantly but we need to check the diff of every file
    src = ../../../../../.;
    buildInputs = [pkgs.git];
    buildPhase = ''
      mkdir -p $out
      echo -e "Checking git status at $(basename `git rev-parse --show-toplevel`)"
      GIT_COMMIT=$(git rev-parse HEAD)
      GIT_DATE=$(git show -s --format=%ci HEAD)
      GIT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
      GIT_DIRTY=$(git status --porcelain)
      echo -e "commit: $GIT_COMMIT\ndate: $GIT_DATE\nbranch: $GIT_BRANCH" > $out/nova-git-metadata
      echo "$GIT_DIRTY"
      if [ -n "$GIT_DIRTY" ]; then
        echo -e "Uncommited changes:\n" >> $out/nova-git-metadata
        git diff >> $out/nova-git-metadata
        git diff --cached >> $out/nova-git-metadata
      else
        echo "Git repository is clean. No diff to write." >> $out/nova-git-metadata
      fi
    '';
  };
}