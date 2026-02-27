% scale(1000) import("D:/Coding/nova-onshape/arm-1-02/assets/gearbox_plate.stl");
% scale(1000) import("D:/Coding/nova-onshape/arm-1-02/assets/base_plate.stl");

// arm mount
translate([0,0,127])
cube([151.4, 110, 130], center=true);

translate([0,0,127+65])
rotate([0,90,0])
cylinder(h=151.4,r=55,center=true);

translate([0,0,127+65])
rotate([0,90,0])
cylinder(h=316.4,r=87/2,center=true);