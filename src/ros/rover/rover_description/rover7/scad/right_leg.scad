% scale(1000) import("right_leg.stl");

// Append pure shapes (cube, cylinder and sphere), e.g:
// cube([10, 10, 10], center=true);
// cylinder(r=10, h=10, center=true);
// sphere(10);

//center
cylinder(r=43, h=58, center=true);

rotate([0,0,-13.5]){
    //connector bar -x
    rotate([-7, 75, 0])
    translate([-15, 0, -130]) 
    cylinder(r=30, h=90);

    rotate([-7, 76, 0])
    translate([-13, 3, -518]) 
    cylinder(r=25, h=390);
}
    rotate([0,0,180])
    translate([391, -29, -125]) 
    rotate([0, 16, -2])
    cube([106, 65, 77]);

    rotate([0,0,180])
    translate([438, 10, -57]) 
    rotate([0, 15, 0])
    cylinder(r=45, h=52);

rotate([0,0,7]){
    //connector bar x
    rotate([4, 75, 180])
    translate([-15, 0, -130]) 
    cylinder(r=30, h=90);

    rotate([4, 76, 180])
    translate([-13, -3, -518]) 
    cylinder(r=25, h=390);
}
    translate([400, -55, -127]) 
    rotate([0, 16, -2])
    cube([107, 65, 77]);

    translate([447, -34, -60]) 
    rotate([0, 15, 0])
    cylinder(r=45, h=52);