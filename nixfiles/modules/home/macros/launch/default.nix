# This nix-shell is designed to run on the base station to quickly spin up the entire stack for any payload!
# Usage (Also in macros/default.nix): nix-shell ${cfg.nixfileDir}/modules/home/macros/launch -A auto.arch --argstr rover-ip user@ip --argstr mast-ip user@ip
{ 
    pkgs ? import <nixpkgs> {}, 
    rover-ip ? null,
    mast-ip ? null,
    dir ? "/home/nova/Builds/master/bin",
    route ? ""
}:

let 
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

  # check that ip is present and optionally enforce this 
  rover-check = need-rover: if rover-ip == null && need-rover
    then throw "You must provide a target for rover ssh commands.\nUsage: nix-shell shell.nix --argstr rover-ip <user@ip>\ne.g nix-shell shell.nix --argstr rover-ip nova@10.0.0.11" 
    else if rover-ip == null && !need-rover then "" else "SSHing into rover at ${ansi.light-purple}${rover-ip}${ansi.nc}...";
  mast-check = need-mast: if mast-ip == null && need-mast
    then throw "You must provide a target for mast ssh commands.\nUsage: nix-shell shell.nix --argstr mast-ip <user@ip>\ne.g nix-shell shell.nix --argstr mast-ip nova@10.0.0.11" 
    else if mast-ip == null && !need-mast then "" else "SSHing into mast at ${ansi.light-purple}${mast-ip}${ansi.nc}...";

  # default elements of a bash shell, use pre-shell for warnings and setup, post-shell for stuff after the terminals are made
  pre-shell = {payload-name, need-rover ? false, need-mast? false}: ''
    if [ -z "$SHELL_STARTED" ]; then
    export SHELL_STARTED=1
    export TMPDIR=/tmp
    echo -e "${ansi.light-red}Tip!${ansi.nc} Change your working directory (Default: ${ansi.orange}/home/nova/Builds/master/bin${ansi.nc}) by appending ${ansi.yellow}--argstr dir ${ansi.orange}YOUR/DIR/HERE${ansi.nc}
    Launching ${ansi.light-green}${payload-name}${ansi.nc}...
    ${rover-check need-rover}
    ${mast-check need-mast}
    Running in ${ansi.orange}${dir}${ansi.nc}... S"
    ${rover-ssh-check need-rover}
    ${mast-ssh-check need-mast}
  '';
  assembleTerminal = setup: "  " + (builtins.foldl' (acc: el: el.platform el.name el.cmd + "\ \\n  & " + acc ) "" setup);
  post-shell = ''
    exit 0
    fi
  '';

  # convert structure to bash shell
  #   pre = pre-shell {payload-name="name"; need-mast=true}; # you can string concat any additions to this, need-mast is optional and defaults to false
  #   terminals = {name, platform, command}[]; # An array of sets 
  #   post = post-shell + "\nAdditional command"; # make additions with a new line since it will be added to a shell hook!
  mkBashScript = {pre ? pre-shell {payload-name="";}, terminals, post ? post-shell}: pkgs.mkShell {shellHook = "${pre}\n${assembleTerminal terminals}\n${post}";};

in
{
  # access using -A flag in the nix-shell command e.g -A auto.arch
  auto = import ./auto.nix   { inherit base rover pre-shell post-shell mkBashScript mast; };
  drive = import ./drive.nix { inherit base rover pre-shell mkBashScript; };
  gui = import ./gui.nix     { inherit base pre-shell post-shell mkBashScript base-nix route;};
  # import more payloads here
}


