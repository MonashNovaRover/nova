# This nix-shell is designed to run on the base station to quickly spin up the entire stack for any payload!
# Usage (Also in macros/default.nix): nix-shell ${cfg.nixfileDir}/modules/home/macros/launch -A auto.arch --argstr rover user@ip --argstr mast user@ip
{ 
    pkgs ? import <nixpkgs> {}, 
    rover ? throw ''You must provide a target for rover ssh commands\nUsage: nix-shell shell.nix --argstr rover <user@ip>\ne.g nix-shell shell.nix --argstr rover nova@10.0.0.11'',
    mast ? null,
    dir ? "/home/nova/Builds/master/bin"
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
  local-terminal = name: cmd: ''ptyxis --tab -d ${dir} --title="${name}" -x "bash -ic '${cmd}; history -s ${cmd}; exec bash'"'';
  ssh-terminal = target: name: cmd: ''ptyxis --tab -d ${dir} --title="${name}" -x "bash -c 'ssh -t ${target} \"bash -ic \\\"${cmd}; history -s ${cmd}; exec bash -l\\\"\"; exec bash'"'';
  
  # aliases for each simple command
  base-terminal = local-terminal;
  rover-terminal = ssh-terminal rover;
  mast-terminal = ssh-terminal mast;

in
{
  # access using -A flag in the nix-shell command e.g -A auto.arch
  auto = import ./auto.nix { inherit pkgs ansi dir rover mast base-terminal rover-terminal mast-terminal; };
  drive = import ./drive.nix { inherit pkgs ansi dir rover base-terminal rover-terminal; };
  # import more payloads here
}


