% scale(1000) import("diffend.stl");

// Append pure shapes (cube, cylinder and sphere), e.g:
// cube([10, 10, 10], center=true);
// cylinder(r=10, h=10, center=true);
// sphere(10);

translate([0, 0, 6])
sphere(8);

translate([0, 0, 16])
cylinder(r=6.5, h=8, center=true);

translate([0, 0, -10])
cylinder(r=7.05, h=20, center=true);

translate([0, 0, -23.5])
cylinder(r=4.1, h=9, center=true);
