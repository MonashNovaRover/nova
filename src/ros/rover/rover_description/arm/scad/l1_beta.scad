% scale(1000) import("l1_beta.stl");

// Append pure shapes (cube, cylinder and sphere), e.g:
// cube([10, 10, 10], center=true);
// cylinder(r=10, h=10, center=true);
// sphere(10);

translate([0,23,250])
cube([25,20,520], center=true);

translate([0,-23,250])
cube([25,20,520], center=true);

rotate([90,0,0])
cylinder(r=5, h=30, center=true);

translate([0,0,500])
rotate([90,0,0])
cylinder(r=5, h=30, center=true);