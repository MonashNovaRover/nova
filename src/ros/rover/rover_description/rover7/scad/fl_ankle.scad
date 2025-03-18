% scale(1000) import("fl_ankle.stl");

// Append pure shapes (cube, cylinder and sphere), e.g:
// cube([10, 10, 10], center=true);
// cylinder(r=10, h=10, center=true);
// sphere(10);

rotate([0, 0, 8])
translate([-8, -62, -30]) 
cube([32, 80, 60]);

rotate([0, 0, -38])
translate([-15, 10, -30]) 
cube([20, 80, 60]);

rotate([0, 0, 7])
translate([47, 60, -30]) 
cube([25, 31, 60]);

rotate([0, 0, 54])
translate([-25, -135, -30]) 
cube([12, 76, 60]);

rotate([0, 0, 97])
translate([-61, -32, -30]) 
cube([12, 30, 60]);

rotate([90, 0, 8])
translate([123, 0, 102]) 
cylinder(r=38, h=26);

rotate([90, 0, 8])
translate([80, -30, 102]) 
cube([30, 60, 12]);