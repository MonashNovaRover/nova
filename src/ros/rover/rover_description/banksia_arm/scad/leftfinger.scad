// === STL Reference (visual only) ===
 %scale(1000) import("leftfinger.stl");

// === Finger Tip ===
finger_tip_dim = [30, 120, 30];
finger_tip_pos = [0, 0, 15];

translate(finger_tip_pos)
    cube(finger_tip_dim, center = true);

// === Finger Attachment with Bolt Hole ===
attachment_dim = [30, 78, 24];
attachment_pos = [0, -90, 19];

bolt_hole_radius = 4.2;
bolt_hole_pos    = [0, -115.5, 20];
bolt_hole_depth  = 200;

difference() {
    translate(attachment_pos)
        cube(attachment_dim, center = true);
    translate(bolt_hole_pos)
        cylinder(r = bolt_hole_radius, h = bolt_hole_depth, center = true);
}

// === Accessories (Side Cylinders + Blocks) ===
accessory_cyl_radius = 8;
accessory_cyl_depth  = 35.2;
accessory_cyl_pos_y  = 39;
accessory_cyl_pos_z  = 19.8;

accessory_block_dim  = [10, 35, 10];
accessory_block_offset_x = 20;

for (side = [-1, 1]) {
    // Side cylinder
    translate([side * 27, accessory_cyl_pos_y, accessory_cyl_pos_z])
        rotate([90, 0, 0])
            cylinder(r = accessory_cyl_radius, h = accessory_cyl_depth, center = true);

    // Side block
    translate([side * accessory_block_offset_x, accessory_cyl_pos_y, accessory_cyl_pos_z])
        cube(accessory_block_dim, center = true);
}

// === Camera Mount ===
camera_dim = [40, 13, 30];
camera_pos = [0, -49, 45];

translate(camera_pos)
    cube(camera_dim, center = true);
