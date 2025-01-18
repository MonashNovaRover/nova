% scale(1000) import("l1_alpha.stl");

translate([-11, 0, 0]){
    difference(){
        rotate([0,90,0])
        cylinder(r=32, h=22, center=true);
        
        rotate([0,90,0])
        cylinder(r=20, h=22, center=true);
    };

    translate([0,0,270])
    cube([25,38,480], center=true);
};

translate([-63, 0, 0]){
    difference(){
        rotate([0,90,0])
        cylinder(r=32, h=22, center=true);
        
        rotate([0,90,0])
        cylinder(r=20, h=22, center=true);
    };

    translate([0,0,270])
    cube([25,38,480], center=true);
};

translate([-40,0,500])
rotate([0,90,0])
cylinder(r=5,h=35,center=true);