// === STL Reference ===
// %scale(1000) import("j6.stl");

// === Wrist Connector ===
wrist_radius = 28;
wrist_thickness = 4.6;

cylinder(r = wrist_radius, h = wrist_thickness);

// === Angle Bracket Plate ===
plate_width   = 60;
plate_height  = 108;
plate_depth   = 3;
plate_z_pos   = wrist_thickness + 1.4;  // Slight gap above connector

translate([0, 0, plate_z_pos])
    cube([plate_width, plate_height, plate_depth], center = true);

// === Upright Arms of Angle Bracket (Left and Right) ===
bracket_arm_width   = 60;
bracket_arm_depth   = 4;
bracket_arm_height  = 203;
bracket_arm_z_pos   = 6 + 103;

cut_angle_deg       = 4;
cut_box_width       = 60;
cut_box_depth       = 5;
cut_box_height      = 210;
cut_offset_x        = 54;

for (side = [-1, 1]) {
    translate([0, side * ((plate_height / 2) - 2), bracket_arm_z_pos])
        difference() {
            cube([bracket_arm_width, bracket_arm_depth, bracket_arm_height], center = true);

            for (cut_side = [-1, 1]) {
                translate([cut_side * cut_offset_x, 0, 0])
                    rotate([0, cut_side * -cut_angle_deg, 0])
                        cube([cut_box_width, cut_box_depth, cut_box_height], center = true);
            }
        }
}

// === J6 Housing ===
housing_radius = 44;
housing_height = 27;
housing_z_pos  = wrist_thickness + plate_depth + 5;

translate([0, 0, housing_z_pos])
    cylinder(r = housing_radius, h = housing_height);

// === J6 Block Body ===
block_width  = 55;
block_depth  = 100;
block_height = 170;
block_z_pos  = 125;

translate([0, 0, block_z_pos])
    cube([block_width, block_depth, block_height], center = true);
