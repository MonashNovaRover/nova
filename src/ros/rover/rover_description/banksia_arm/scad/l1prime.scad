// === STL Reference (visual only, not exported) ===
// %scale(1000) import("l1prime.stl");

// === Bar Dimensions ===
bar_width   = 20;
bar_depth   = 19;
bar_height  = 500;
bar_center_y = 80;
bar_center_z = 310;

translate([0, bar_center_y, bar_center_z])
    cube([bar_width, bar_depth, bar_height], center = true);

// === Rollers ===
roller1_radius = 6;
roller2_radius = 7.5;
roller_depth   = 21;

roller1_z = 58;
roller2_z = 560;
roller_y  = bar_center_y;

module roller(z, r) {
    translate([0, roller_y, z])
        rotate([0, 90, 0])
            cylinder(r = r, h = roller_depth, center = true);
}

roller(roller1_z, roller1_radius);
roller(roller2_z, roller2_radius);

// === Wire Clips ===
clip_width  = 30;
clip_depth  = 40;
clip_height = 20;

clip_y = 85;
clip_bottom_z = 170;
clip_top_z    = 450;

module wire_clip(z) {
    translate([0, clip_y, z])
        cube([clip_width, clip_depth, clip_height], center = true);
}

wire_clip(clip_bottom_z);
wire_clip(clip_top_z);
