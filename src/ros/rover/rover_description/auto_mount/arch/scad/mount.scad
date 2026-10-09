mesh_dir = "D:\\Coding\\nova-onshape\\automount-28-01\\meshes\\";
mesh = "mount.stl";
path = str(mesh_dir, mesh);

% scale(1000) import(path);

translate([0,40,25])
cube([204,40,50],center=true);

translate([51,40,25])
cube([18,200,50],center=true);

translate([-51,40,25])
cube([18,200,50],center=true);

translate([51,40,75])
cube([18,70,100],center=true);

translate([-51,40,75])
cube([18,70,100],center=true);