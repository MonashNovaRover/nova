## Here lies all ~~robot~~ rover description related files.

This was generated using the [onshape-to-robot tool](https://github.com/Rhoban/onshape-to-robot).

To replicate, copy `onshape-to-robot-config.json` to a folder **OUTSIDE** of the nova repo, run `urdf-tool` then `onshape-to-robot foldername`.\
Make sure to add your `.scad` files in the folder, or use `onshape-to-robot-edit-shape` to make them for simple shape approximation for collisions.

### Structure:
* `auto_mount`\
    contains auto mount related description files
    * `arch` and `urc`\
    contains the description files for each respective mount
    * `shared`\
    contains the description files that are shared between arch and urc (livox lidar and realsense camera)
* `banksia`\
    contains banksia related description files
* `ducket` \
    contains ducket related description files
* `old_arm` \
    contains old arm related description files
* `taipan` \
    contains taipan related description files
* `waratah`\
    contains waratah related description files


\
Each folder should contain the following:
* `meshes`\
    contains the STL files
* `urdf`\
    contains the urdf files
* `scad`\
    contains the scad files (used to generate simplified collisions in urdf)\
    Keeping for reference!

### Notes:
* 