# This nix-shell is designed to run on the base station to quickly spin up the entire stack for any payload!
# Usage (Also in macros/default.nix): nix-shell ${cfg.nixfileDir}/modules/home/macros/launch -A auto.arch --argstr rover-ip user@ip --argstr mast-ip user@ip
# Or nix-build ${cfg.nixfileDir}/modules/home/macros/launch -A auto.arch, then ~/Builds/build-name/bin/run-auto-arch user@rover-ip user@mast-ip
{ 
    pkgs ? import <nixpkgs> {}, 
    rover-ip ? "$1",
    mast-ip ? "$2",
    dir ? "/home/nova/Builds/master/bin",
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

  # used for filtering out the entire repo and just getting the .git
  nix-filter = import ( builtins.fetchGit { url = "https://github.com/numtide/nix-filter.git"; } );

  # get git status of repo
  git-status = ''
    mkdir -p $out
    GIT_COMMIT=$(git rev-parse HEAD)
    GIT_DATE=$(git show -s --format=%ci HEAD)
    GIT_BRANCH=$(git rev-parse --abbrev-ref HEAD)
    GIT_DIRTY=$(if [ -n "$(git status --porcelain)" ]; then echo "dirty"; else echo "clean"; fi)
    echo -e "commit: $GIT_COMMIT\ndate: $GIT_DATE\nbranch: $GIT_BRANCH\ndirty: $GIT_DIRTY" > $out/git-metadata
  '';

  # these run a single command, and record it in history, you will need to make a custom line for multiple commands
  local-terminal = name: cmd: ''ptyxis --tab -d ${dir} --title="${name}" -x "bash -ic '${cmd}; history -s \"${cmd}\"; exec bash'"'';
  local-nix-terminal = shell: name: cmd: ''ptyxis --tab -d ${dir} --title="${name}" -x "bash -ic '${shell} --command \"${cmd}; history -s \\\"${cmd}\\\"; exec bash -l\"; history -s \"${shell}\"; exec bash -l'; exec bash -l"'';
  ssh-terminal = target: name: cmd: ''ptyxis --tab -d ${dir} --title="${name}" -x "bash -ic 'ssh -t ${target} \"bash -ic \\\"${cmd}; history -s ${cmd}; exec bash -l\\\"\"; history -s \"ssh -t ${target}\"; exec bash -l'"'';
  ssh-nix-terminal = target: shell: name: cmd: ''ptyxis --tab --title="${name}" -x "bash -ic 'ssh -t ${target} \"bash -ic \\\"${shell} --command \\\\\\\"${cmd}; history -s \\\\\\\\\\\\\\\"${cmd}\\\\\\\\\\\\\\\"; exec bash -l\\\\\\\"; history -s \\\\\\\"${shell}\\\\\\\"; exec bash -l\\\"\"; history -s \"ssh -t ${target}\"; exec bash -l'"'';
  
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
  usage-string-shell = platform: "You must provide a target for ${platform} ssh commands.\nUsage: nix-shell shell.nix --argstr ${platform}-ip <user@ip>\ne.g nix-shell shell.nix --argstr ${platform}-ip nova@10.0.0.11";
  usage-string-build = platform: "You must provide a target for ${platform} ssh commands.\\nUsage: $0 <nova@rover-ip> <nova@mast-ip>\\ne.g $0 nova@10.0.0.2 nova@10.0.0.3";

  # check that ip is present and optionally enforce this 
  rover-check = need-rover: if rover-ip == "$1" && need-rover && inShell
    then throw (usage-string-shell "rover")
    else if rover-ip == "$1" && !need-rover then "" else "SSHing into rover at ${ansi.light-purple}${rover-ip}${ansi.nc}...";
  mast-check = need-mast: if mast-ip == "$2" && need-mast && inShell
    then throw (usage-string-shell "mast")
    else if mast-ip == "$2" && !need-mast then "" else "SSHing into mast at ${ansi.light-purple}${mast-ip}${ansi.nc}...";

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
      echo -e "${ansi.light-red}Tip!${ansi.nc} Change your working directory (Default: ${ansi.orange}/home/nova/Builds/master/bin${ansi.nc}) by appending ${ansi.yellow}--argstr dir ${ansi.orange}YOUR/DIR/HERE${ansi.nc} \
      Launching ${ansi.light-green}${payload-name}${ansi.nc}... \
      ${rover-check need-rover} \
      ${mast-check need-mast} \
      Running in ${ansi.orange}${dir}${ansi.nc}..."
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
  mkBashScript = {pre ? pre-shell {payload-name="";}, terminals, post ? post-shell}: "${pre}\n${assembleTerminal terminals}\n${post}";

  # construct the shell and build environments, shellString is the final shell script created using mkBashScript, shellName is the final bash script name
  shellAndBuild = shellString: shellName: pkgs.stdenv.mkDerivation {
    pname = "nova-" + shellName;
    version = "1.0";
    shellHook = shellString;
    src = nix-filter { root = ../../../../../.; include = [".git"];};

    buildInputs = [ pkgs.git ];
    # put it in the bin dir
    # add checkers for $1 and $2 for rover and mast ips
    buildPhase = ''
      ${git-status}
      mkdir -p $out/bin
      cat > $out/bin/${shellName} <<'EOF'
      #!${pkgs.bash}/bin/bash
      # THIS FILE IS AUTO GENERATED BY NIX
      ${if rover-ip != "$1" then "if [ -z \"$1\" ]; then\n  echo -e \"${usage-string-build "rover"}\"\n  exit 1\nfi" else ""}
      ${if rover-ip == "$1" then "" else if mast-ip != "$2" then "if [ -z \"$2\" ]; then\n  echo -e \"${usage-string-build "mast"}\"\n  exit 1\nfi" else ""}
      ${builtins.replaceStrings [rover-ip mast-ip] ["$1" "$2"] shellString}
      EOF
      chmod +x $out/bin/${shellName}
    '';
  };

  # final single function to pass to child nix files to make defining setups easy
  bashBuilder = struct: shellName: shellAndBuild (mkBashScript struct) shellName;

  callPackage = pkgs.lib.callPackageWith {inherit base base-nix rover rover-nix mast pre-shell post-shell bashBuilder route;};
  
in
{
  # access using -A flag in the nix-shell command e.g -A auto.arch
  auto = callPackage ./auto.nix   { };
  drive = callPackage ./drive.nix { };
  gui = callPackage ./gui.nix     { };
  # import more payloads here
}


