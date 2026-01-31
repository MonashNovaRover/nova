mesh_dir = "D:\\Coding\\nova-onshape\\automount-28-01\\meshes\\";
mesh = "lidar_holder.stl";
path = str(mesh_dir, mesh);

% scale(1000) import(path);

translate([0,7,115])
cube([84,136,10],center=true);