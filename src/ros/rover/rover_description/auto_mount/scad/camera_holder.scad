mesh_dir = "D:\\Coding\\nova-onshape\\automount-28-01\\meshes\\";
mesh = "camera_holder.stl";
path = str(mesh_dir, mesh);

% scale(1000) import(path);

translate([0,-32,25])
cube([143,86,50],center=true);