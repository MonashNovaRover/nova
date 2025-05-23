// === STL Reference ===
%scale(1000) import("eebase.stl");

// === Shared Origin Offset ===
origin_x = 7.85;
origin_y = 13.0;

// === Base Link ===
base_cylinder_radius     = 35;
base_cylinder_height     = 34;
inner_cylinder_radius    = 13.4;
inner_cylinder_height    = 4;
base_cylinder_z_offset   = -5;
inner_cylinder_z_offset  = -24.2;

translate([origin_x, origin_y, base_cylinder_z_offset])
    cylinder(r=base_cylinder_radius, h=base_cylinder_height, center=true);

translate([origin_x, origin_y, inner_cylinder_z_offset])
    cylinder(r=inner_cylinder_radius, h=inner_cylinder_height, center=true);

// === End Effector Back Plate ===
back_plate_width   = 250;
back_plate_height  = 40;
back_plate_depth   = 6;
back_plate_z       = 15;

translate([origin_x, origin_y, back_plate_z])
    cube([back_plate_width, back_plate_height, back_plate_depth], center=true);

// === Lead Screw Holders (Left and Right) ===
lead_holder_width  = 5;
lead_holder_height = 80;
lead_holder_z      = 43;
lead_holder_x_edge = (back_plate_width / 2) - 4;

for (side = [-1, 1]) {
    translate([origin_x, origin_y, back_plate_z])
        translate([side * lead_holder_x_edge, 0, lead_holder_z])
            cube([lead_holder_width, back_plate_height, lead_holder_height], center=true);
}

// === Pololu Gear Motor Blocks ===
motor1_pos = [143, 13, 49];
motor1_dim = [24, 40, 56];

motor2_pos = [104, -26, 43];
motor2_dim = [102, 40, 40];

translate(motor1_pos)
    cube(motor1_dim, center=true);

translate(motor2_pos)
    cube(motor2_dim, center=true);

// === Bottom Camera Assembly ===
camera_bottom_pos = [5, 48, 29.5];
camera_bottom_dim = [55, 30, 35];

translate(camera_bottom_pos)
    cube(camera_bottom_dim, center=true);

// === Top Camera Support + Finger Railings ===
cam_support_dim = [22, 40, 85];
cam_support_z   = 60;

translate([origin_x, origin_y, cam_support_z])
    cube(cam_support_dim, center=true);

// === Finger Railings: Cylinders (Top and Bottom) ===
cyl_r1 = 3;     // Top railing
cyl_r2 = 5;     // Bottom railing
cyl_h  = 110;
cyl_offset_z_top    = cam_support_z + (cam_support_dim[2] / 2) - 15.4;
cyl_offset_z_bottom = cam_support_z - 18;

for (side = [-1, 1]) {
    x_offset = origin_x + side * (cam_support_dim[0] / 2);
    
    // Top railing
    translate([x_offset, origin_y, cyl_offset_z_top])
        rotate([0, side * 90, 0])
            cylinder(r=cyl_r1, h=cyl_h);
    
    // Bottom railing
    translate([x_offset, origin_y, cyl_offset_z_bottom])
        rotate([0, side * 90, 0])
            cylinder(r=cyl_r2, h=cyl_h);
}

// === Top Camera Mast Assembly ===
mast_pos_1 = [origin_x, origin_y - 60, 80];
mast_angle = 58;
mast_dim_1 = [25, 50, 125];

mast_pos_2 = [origin_x, mast_pos_1[1] - 51.5, mast_pos_1[2] + 78];
mast_dim_2 = [25, 30, 135];

cam_holder_pos = [mast_pos_2[0], mast_pos_2[1], mast_pos_2[2] + 52];  // 52 ≈ half cam holder height + spacing
cam_holder_dim = [50, 40, 40];

translate(mast_pos_1)
    rotate([mast_angle, 0, 0])
        cube(mast_dim_1, center=true);

translate(mast_pos_2)
    cube(mast_dim_2, center=true);

translate(cam_holder_pos)
    cube(cam_holder_dim, center=true);
