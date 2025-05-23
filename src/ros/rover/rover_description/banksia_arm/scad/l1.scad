// === STL Reference (visual only, not exported) ===
// %scale(1000) import("l1.stl");

// === Linkage Hoop Dimensions ===
hoop_left_outer_radius  = 32;
hoop_left_inner_radius  = 24.5;
hoop_right_outer_radius = 43;
hoop_right_inner_radius = 36;
hoop_thickness          = 20;

hoop_offset_x = 35;
hoop_center_z = 0;

// === Linkage Hoops ===
module linkage_hoop(x, outer_r, inner_r) {
    translate([x, 0, hoop_center_z])
        rotate([0, 90, 0])
            difference() {
                cylinder(r = outer_r, h = hoop_thickness, center = true);
                cylinder(r = inner_r, h = hoop_thickness + 2, center = true);  // slightly larger inner cut
            }
}

linkage_hoop(-hoop_offset_x, hoop_left_outer_radius,  hoop_left_inner_radius);
linkage_hoop( hoop_offset_x, hoop_right_outer_radius, hoop_right_inner_radius);

// === Linkage Bar Dimensions ===
bar_width   = 22.5;
bar_depth   = 40;
bar_height_left  = 480;
bar_height_right = 470;

bar_center_left_z  = 265;
bar_center_right_z = 273;

module linkage_bar(x, z, h) {
    translate([x, 0, z])
        cube([bar_width, bar_depth, h], center = true);
}

linkage_bar(-hoop_offset_x, bar_center_left_z,  bar_height_left);
linkage_bar( hoop_offset_x, bar_center_right_z, bar_height_right);

// === Support Struts ===
strut_width  = 100;
strut_depth  = 50;
strut_height = 30;

strut_lower_z = 117.5;
strut_upper_z = 405.5;

module support_strut(z) {
    translate([0, 0, z])
        cube([strut_width, strut_depth, strut_height], center = true);
}

support_strut(strut_lower_z);
support_strut(strut_upper_z);

// === Roller ===
roller_radius  = 11;
roller_length  = 100;
roller_center_z = 500;

translate([0, 0, roller_center_z])
    rotate([0, 90, 0])
        cylinder(r = roller_radius, h = roller_length, center = true);
