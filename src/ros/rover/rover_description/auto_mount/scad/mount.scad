mesh_dir = "C:\\Users\\Anthony\\Downloads\\";
mesh = "mount.stl";
path = str(mesh_dir, mesh);

% scale(1000) import(path);

cube([200,40,50],center=true);