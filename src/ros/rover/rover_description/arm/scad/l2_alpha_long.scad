% scale(1000) import("l2_alpha_long.stl");

// Append pure shapes (cube, cylinder and sphere), e.g:
// cube([10, 10, 10], center=true);
// cylinder(r=10, h=10, center=true);
// sphere(10);


translate([0,-15,0])
cube([35,60,21.8], center=true);

translate([0,-250,0])
cube([38,470,25], center=true);

translate([0,-500,0])
cube([60,30,48], center=true);

translate([0,-548.5,7.25])
cylinder(r=43.5, h=100, center=true);

translate([0,-548.5,69])
cylinder(r=12, h=24, center=true);

//translate([0,-678.5,69])
//cylinder(r=12, h=24, center=true);
