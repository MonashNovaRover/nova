% scale(1000) import("diffbar.stl");

// Append pure shapes (cube, cylinder and sphere), e.g:
// cube([10, 10, 10], center=true);
// cylinder(r=10, h=10, center=true);
// sphere(10);

rotate([0,-80,0])
translate([0,0,200])
cylinder(r=10, h=410, center = true);

rotate([0,80,0])
translate([0,0,200])
cylinder(r=10, h=410, center = true);
