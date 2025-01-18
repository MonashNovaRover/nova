% scale(1000) import("l2_beta.stl");

difference(){
    cylinder(r=32, h=22, center=true);
    cylinder(r=25, h=22, center=true);
};

translate([-93, 0, 0])
cube([134,38,25], center=true);