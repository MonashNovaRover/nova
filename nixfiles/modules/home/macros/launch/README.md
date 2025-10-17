# Nova-Launch
This set of nixfiles contains nix derivations and shells which will launch a series of terminals which will run our entire stack.
Essentially a terminal launcher.
It has capabilities for SSHing, nix shells built in to the framework (read default.nix to find them).

To define your own "payload" you can add an existing attribute to a .nix file e.g
```nix
  ...
  drive-setup = {
    pre = pre-shell {payload-name="Drive"; need-rover=true;};
    terminals = [
      {name="Base:Teleop"; platform=base; cmd="launch teleop_drive_joy teleop.launch.py";}
      {name = "Rover:Drive"; platform=rover; cmd="ros2 launch drive_bringup drive.launch.py";}
    ];
    post = post-shell;
  };
in
{
  drive = bashBuilder drive-setup "run-drive";
  ...
```
- Note that each setup has a pre and post which are just bash commands which run before and after the terminals, you can add on / modify the defaults through string manipulation
- Terminals is an array of sets expecting {name, platform, cmd}
- For examples see below
- When creating setups AVOID using hardcoded file paths as since the bash scripts are created in the bin directory, it essentially links the binaries to the launch allowing versioned launch scripts! Because of this you should also avoid using aliases that also hard code paths. 
  - By the way there is a dir argument if you so need to change the DIR of a build etc (might need fixing)
- When running a terminal it will open a terminal in the same folder as the script (bin folder)
- When SSHing keep in mind that aliases are defined on a per user basis (nova aliases won't work in non-nova users)


### Build and/or Run
Follow this basic structure and then you can run it through any of the following means:\

`nix-shell ~/nova/nixfiles/modules/home/macros/launch -A MODULE-NAME.ATTR-NAME --argstr rover-ip nova@localhost --argstr mast-ip nova@localhost`
- This will immediately start the terminals of the selected setup
- MODULE-NAME refers to variable name given to the imported in default.nix
- ATTR-NAME refers to the variable name given to the particular payload inside the payload.nix 
- So in this case `-A drive.drive`
` If you want a single .nix file you can all with just `-A mynixfilename` then have a single output of the file instead of a set
` There are also currently two arguments to dynamically set the rover and mast's ip for SSHing

`nix-build -o ~/Builds/drive  ~/nova/nixfiles/modules/home/macros/launch -A drive.drive`
- This will create executable bash files in your specified output destination's bin file
- the rover-ip and mast-ip arguments are then handled by the shell files and must be passed through as argument 1 and 2 respectively
- e.g `/work/drive/bin/run-drive nova@orin-ip`
- This will also create a git metadata file with any dirty changes (non committed changes) as a patch at the end.
- Find this file in the root directory of the resultant build as `nova-git-metadata`

`ws-build`
- This will work like nix-build

### Example Setups
Last updated 17/10/2025
GUI runner which has a nix shell and web browser open
```
{ 
    base,
    pre-shell,
    post-shell,
    bashBuilder,
    base-nix,
    route
}:

let 
  gui-setup = {
    pre = pre-shell {payload-name="Nova Gui";};
    terminals = [
      {name = "Base:Gui"; platform=base-nix "gui-shell"; cmd="gui-link; gui-run";}
      {name = "Base:Rosbridge"; platform=base; cmd="gui-rosbridge";}
    ];
    post = "xdg-open http://localhost:5173/${route}\n" + post-shell;
  };
in
{
  gui = bashBuilder gui-setup "run-gui";
}
```
See drive example for SSH (above)