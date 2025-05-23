% scale(1000) import("gearbox.stl");

// Append pure shapes (cube, cylinder and sphere), e.g:
// cube([10, 10, 10], center=true);
// cylinder(r=10, h=10, center=true);
// sphere(10);
cylinder(r=70, h=51);
difference() {
    // Outer cylinder
    translate([0, 0, 51])
    cylinder(r=58, h=5);
    
    // Inner cylinder (slightly smaller radius, same height)
    translate([0, 0, 51])
    cylinder(r=55, h=5);  // Adjust r=40 to your desired wall thickness
};

difference() {
    // Outer cylinder
    translate([0, 0, 51])
    cylinder(r=42.5, h=11);
    
    // Inner cylinder
    translate([0, 0, 51])
    cylinder(r=34.5, h=11);
};