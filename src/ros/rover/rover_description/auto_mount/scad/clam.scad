mesh_dir = "C:\\Users\\Anthony\\Downloads\\";
mesh = "clam.stl";
path = str(mesh_dir, mesh);
% scale(1000) import(path);

translate([0,-10,-10])
cube([300,20,60],center=true);

translate([140,110,-20])
cube([20,250,40],center=true);

translate([-140,110,-20])
cube([20,250,40],center=true);

translate([-74.5,-47.5,205])
rotate([0,-0.2,0])
cylinder(r=13, h=500, center=true);

translate([87.1,-47.5,205])
rotate([0,-0.2,0])
cylinder(r=13, h=500, center=true);

//translate([7,-50,425])
//cube([200,40,50],center=true);