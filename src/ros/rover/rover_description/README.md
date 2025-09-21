## This is Nova Rover's 7th Rover named \_\_\_\_
This was generated using the [onshape-to-robot tool](https://github.com/Rhoban/onshape-to-robot)
To regenerate, copy `onshape-to-robot-config.json` to a folder **OUTSIDE** of the nova repo, run `urdf-tool` then `onshape-to-robot foldername`.\
Make sure to add your `.scad` files in the folder, or use `onshape-to-robot-edit-shape` to make them for simple shape approximation for collisions.

### Structure:
* `arm`\
    contains arm related description files
* `base`
    contains rover related description files

\
Each folder contains the following:
* `meshes`\
    contains the STL files
* `urdf`\
    contains the urdf files
* `scad`\
    contains the scad files (used to generate simplified collisions in urdf)\
    Keeping for reference!

### Notes:
* Use provided config.json
* Replace package:// with file://$(arg rover_description_dir)
* Remove "_continuous"
* Replace inertias of generated ball joints with: 
  ```
  <mass value="0.00001"/>
  <inertia ixx="0.01" iyy="0.01" izz="0.01" ixy="0.0" ixz="0.0" iyz="0.0"/>
  ```
* Double check in Gazebo