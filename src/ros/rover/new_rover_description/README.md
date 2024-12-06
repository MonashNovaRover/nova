## This is Nova Rover's 7th Rover named \_\_\_\_
This was generated using the (onshape-to-robot tool)[https://github.com/Rhoban/onshape-to-robot]
Still need meshlab stl simplification, but collisions are now done.

### structure:
* `meshes`\
    contains the STL files
* `urdf`\
    contains the urdf files
* `scad`\
    contains the scad files (used to generate simplified collisions in urdf)\
    Keeping for reference!

### Notes:
* Chassis, wheels and ankles have simplified collision (using scad), the rest have no collision.