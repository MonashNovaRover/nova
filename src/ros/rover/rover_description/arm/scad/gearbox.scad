% scale(1000) import("gearbox.stl");

//basegear
translate([0,0,20])
cylinder(r=75, h=40, center=true);
translate([0,0,48])
cylinder(r=44, h=35, center=true);

// side plate
translate([60, 0, 60])
rotate([0, 0, 90])
cube([150, 10, 40], center=true);

translate([60, 55, 135])
rotate([0, 0, 90])
cube([40, 10, 160], center=true);

translate([60, -55, 135])
rotate([0, 0, 90])
cube([40, 10, 160], center=true);

// sideplate 2

translate([-60, 0, 60])
rotate([0, 0, 90])
cube([150, 10, 40], center=true);

translate([-60, 55, 135])
rotate([0, 0, 90])
cube([40, 10, 160], center=true);

translate([-60, -55, 135])
rotate([0, 0, 90])
cube([40, 10, 160], center=true);

// shaft
translate([0,0,165])
rotate([90, 0, 0])
cylinder(r=25, h=160, center=true);

// sidegear 1
translate([0,-75,165])
rotate([90, 0, 0]) {
    translate([0,0,20])
    cylinder(r=75, h=40, center=true);
    translate([0,0,48])
    cylinder(r=44, h=35, center=true);
}

// sidegear 2
translate([0,75,165])
rotate([-90, 0, 0]) {
    translate([0,0,20])
    cylinder(r=75, h=40, center=true);
    translate([0,0,48])
    cylinder(r=44, h=35, center=true);
}