mesh_dir = "C:\\Users\\Anthony\\Downloads\\";
mesh = "camera_holder.stl";
path = str(mesh_dir, mesh);

% scale(1000) import(path);

translate([0,35,0])
cube([143,86,50],center=true);