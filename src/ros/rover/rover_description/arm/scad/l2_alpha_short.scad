% scale(1000) import("l2_alpha_short.stl");

// Append pure shapes (cube, cylinder and sphere), e.g:
// cube([10, 10, 10], center=true);
// cylinder(r=10, h=10, center=true);
// sphere(10);

translate([-75,0,0])
cube([120,38,25],center = true);

translate([-12,0,0])
cube([20,22,21.8], center=true);

cylinder(r=10, h=21.8,center = true);