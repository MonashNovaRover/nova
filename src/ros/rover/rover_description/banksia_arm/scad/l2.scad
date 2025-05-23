// === STL Reference (visual only, not exported) ===
// %scale(1000) import("l2.stl");

// === Common Rotation Angle for L2 Axis ===
l2_angle = 127;

// === Bar ===
bar_pos      = [-12, 0, 474];
bar_dim      = [25, 40, 230];

translate(bar_pos)
    rotate([l2_angle, 0, 0])
        cube(bar_dim);

// === L2 Fixed Joint Hole ===
joint_pos = [0, 0, 500];
joint_outer_dim = [25, 35, 32];
joint_hole_radius = 10;
joint_hole_depth = 35;

translate(joint_pos)
    difference() {
        rotate([l2_angle, 0, 0])
            cube(joint_outer_dim, center = true);
        rotate([0, 90, 0])
            cylinder(r = joint_hole_radius, h = joint_hole_depth, center = true);
    }

// === Wire Clips ===
clip_dim = [30, 60, 20];

clip_top_pos = [0, -58, 463];
clip_bot_pos = [0, -137, 402];

translate(clip_top_pos)
    rotate([l2_angle, 0, 0])
        cube(clip_dim, center = true);

translate(clip_bot_pos)
    rotate([l2_angle, 0, 0])
        cube(clip_dim, center = true);

// === J4 Housing Cylinder ===
j4_housing_radius = 45;
j4_housing_depth  = 100;
j4_housing_pos    = [0, -220, 332];

translate(j4_housing_pos)
    rotate([0, 90, 0])
        cylinder(r = j4_housing_radius, h = j4_housing_depth, center = true);

// === J4–J5 Connector Block with Hole ===
conn_block_pos   = [100 / 2, -185, 334];
conn_block_dim   = [35, 40, 190];
conn_hole_radius = 3;
conn_hole_length = 450;
conn_hole_pos    = [100 / 2, -333, 248];

difference() {
    translate(conn_block_pos)
        rotate([l2_angle, 0, 0])
            cube(conn_block_dim);
    translate(conn_hole_pos)
        rotate([0, 90, 0])
            cylinder(r = conn_hole_radius, h = conn_hole_length, center = true);
}

// === Side Cube Connector (Rotated Block) ===
side_cube_pos = [100 / 2, -277, 310];
side_cube_dim = [120, 30, 30];
side_cube_angle = 143;

translate(side_cube_pos)
    rotate([side_cube_angle, 0, 180])
        cube(side_cube_dim);

// === Left Clip Bracket with Hole ===
clip_bracket_pos = [-70, -258, 287];
clip_bracket_dim = [10, 30, 100];

difference() {
    translate(clip_bracket_pos)
        rotate([l2_angle, 0, 0])
            cube(clip_bracket_dim);
    translate(conn_hole_pos)
        rotate([0, 90, 0])
            cylinder(r = conn_hole_radius, h = conn_hole_length, center = true);
}
