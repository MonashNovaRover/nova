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
- `need-rover` and `need-mast` adds the checks to ssh into the rover and mast respectively, if you don't add them SSHs may fail!
- payload-name just prints it out in console, purely *a e s t h e t i c*
- By default, terminals will open in the result directory's bin folder that this package was built to however this behaviour is not shared with SSHing as SSH may not have the same build directory. Thus you must implement this behaviour manually.
  - In the scripts you can use `$BUILD_DIR` to access the bin directory that the local terminals will point to if needed.
  - To use the binaries in the bin folder, append `./` to your commands so that it uses the binaries in that folder instead of the binaries in path
- When SSHing keep in mind that aliases are defined on a per user basis (nova aliases won't work in non-nova users)
- Optional flags are also supported where you can define the letter of the flag, the variable that gets assigned, a default value and a description. You can then call the variable of these flags to use them in the commands.
  - e.g `optional-args = [ {letter="n"; variable="NOVA_REPO_PATH"; default="/home/nova/nova"; description="Path to the nova repo";} ];` can be called with `$NOVA_REPO_PATH`
- You can make delayed terminal commands by using:
  ```bash
  command1
  (sleep 10; command2) &
  command3
  ```
  - This will make command 2 run 10 seconds after command 1 while command 3 will still run immediately after command 1
  (Good for race conditions)


### Build and/or Run
Follow this basic structure and then you can run it through any of the following means:\

`nix-shell ~/nova/nixfiles/modules/home/macros/launch -A MODULE-NAME.ATTR-NAME --argstr rover-ip nova@localhost --argstr mast-ip nova@localhost`
- This will immediately start the terminals of the selected setup
- MODULE-NAME refers to variable name given to the imported in default.nix
- ATTR-NAME refers to the variable name given to the particular payload inside the payload.nix 
- So in this case `-A drive.drive`
- If you want a single .nix file you can all with just `-A mynixfilename` then have a single output of the file instead of a set
- There are also currently two arguments to dynamically set the rover and mast's ip for SSHing

`nix-build -o ~/Builds/drive  ~/nova/nixfiles/modules/home/macros/launch -A drive.drive`
- Allows you to test specific payloads!
- This will create executable bash files in your specified output destination's launch folder
- the rover-ip and mast-ip arguments are then handled by the shell files and must be passed through as argument 1 and 2 respectively
- e.g `~/Builds/drive/launch/run-drive nova@orin-ip`
- This will also create a git metadata file with any dirty changes (non committed changes) as a patch at the end.
- Find this file in the root directory of the resultant build as `nova-git-metadata`

`nix-build -o ~/Builds/nova-launch-scripts -A nova-launch-scripts` 
- Same as above but with all payload scripts built at once

`nix-build -o ~/Builds/nova-git-metadata -A nova-git-metadata`
- Test the git metadata feature 

`ws-build`
- This will work like nix-build but build all of the payloads at once with the rest of our workspace
- You can also do 