// === Global Parameters ===
linkage_scale = 1000;

// === Base Plate Dimensions ===
base_plate_width   = 149.4;
base_plate_depth   = 110;
base_plate_height  = 18.5;

// === Side Bracket Dimensions ===
bracket_thickness     = 6;
bracket_height        = 112.5;
bracket_top_radius    = 55;
bracket_top_thickness = 6;

// === Bottom & Inner Plate Cylinders ===
bottom_plate_radius = 33.5;
bottom_plate_height = 2.5;

inner_plate_radius = 29;
inner_plate_height = 12;

// === Gearbox Cylinder ===
gearbox_radius = 43.5;
gearbox_height = 24;

// === R80 Cap Dimensions ===
r80_outer_radius = 55;
r80_cap_depth    = 55;

r80_extended_radius = 86.404 / 2;
r80_extended_depth  = 27.573;

// === Linkage Groove Cylinders ===
groove1_radius = 20;
groove1_depth  = 139;

groove2_radius = 30;
groove2_depth  = 11.967;

groove3_radius = 38.5;
groove3_depth  = 17.877 + 22;

groove4_radius = 41.5;
groove4_depth  = 4.986;

groove5_radius = 44;
groove5_depth  = 18.948;

groove6_radius = 25;
groove6_depth  = 22;

groove7_radius = 44.5;
groove7_depth  = 19;

// === Resolver Gear & Cube ===
resolver_gear_radius = 3;
resolver_gear_depth  = 19;

resolver_cube_width  = 5;
resolver_cube_height = 65;
resolver_cube_depth  = 20;

resolver_top_cube_width  = 15;
resolver_top_cube_depth  = 28;
resolver_top_cube_height = 23;

// === Connector Cubes ===
connector1_pos = [12, 52, 0];
connector1_dim = [15.845, 18.735, 24.6];

connector2_pos = [-54.52, 52, 0];
connector2_dim = [34, 33.309, 23.15];

// === STL Reference ===
%scale(linkage_scale)
    import("linkagedrive.stl");

// === Bottom Cylinders ===
rotate([180, 0, 0])
    cylinder(r=bottom_plate_radius, h=bottom_plate_height);

rotate([180, 0, 0])
    translate([0, 0, bottom_plate_height])
    cylinder(r=inner_plate_radius, h=inner_plate_height);

// === Base Plate ===
translate([0, 0, base_plate_height / 2])
    cube([base_plate_width, base_plate_depth, base_plate_height], center=true);

// === Gearbox Cylinder ===
translate([0, 0, base_plate_height])
    cylinder(r=gearbox_radius, h=gearbox_height);

// === Side Brackets ===
bracket_z        = base_plate_height + (bracket_height / 2);
bracket_x_offset = (base_plate_width / 2) - (bracket_thickness / 2);

for (side = [-1, 1]) {
    translate([side * bracket_x_offset, 0, bracket_z])
        cube([bracket_thickness, base_plate_depth, bracket_height], center=true);
}

// === Curved Bracket Tops ===
bracket_top_z = base_plate_height + bracket_height;

module curved_bracket_top() {
    difference() {
        cylinder(r=bracket_top_radius, h=bracket_top_thickness, center=true);
        cube([base_plate_depth, bracket_top_radius, bracket_top_thickness]);
    }
}

for (side = [-1, 1]) {
    translate([side * bracket_x_offset, 0, bracket_top_z])
        rotate([0, 90, 0])
            curved_bracket_top();
}

// === R80 End Caps ===
r80_offset_x = base_plate_width / 2;

for (side = [-1, 1]) {
    rotate_angle = side * 90;
    translate([side * r80_offset_x, 0, bracket_top_z])
        rotate([0, rotate_angle, 0])
            cylinder(r=r80_outer_radius, h=r80_cap_depth);
}

// === Extended R80 Cylinders ===
for (side = [-1, 1]) {
    rotate_angle = side * 90;
    translate([side * (r80_offset_x + r80_cap_depth), 0, bracket_top_z])
        rotate([0, rotate_angle, 0])
            cylinder(r=r80_extended_radius, h=r80_extended_depth);
}

// === Linkage Grooves ===
translate([0, 0, bracket_top_z])
    rotate([0, 90, 0])
    cylinder(r=groove1_radius, h=groove1_depth, center=true);

translate([0, 0, bracket_top_z])
    rotate([0, 90, 0])
    cylinder(r=groove2_radius, h=groove2_depth, center=true);

translate([26, 0, bracket_top_z])
    rotate([0, 90, 0])
    cylinder(r=groove3_radius, h=groove3_depth, center=true);

translate([48.5, 0, bracket_top_z])
    rotate([0, 90, 0])
    cylinder(r=groove4_radius, h=groove4_depth, center=true);

translate([60, 0, bracket_top_z])
    rotate([0, 90, 0])
    cylinder(r=groove5_radius, h=groove5_depth, center=true);

translate([-35, 0, bracket_top_z])
    rotate([0, 90, 0])
    cylinder(r=groove6_radius, h=groove6_depth, center=true);
    
translate([-50.5, 0, bracket_top_z])
    rotate([0, 90, 0])
    cylinder(r=41.5, h=groove4_depth);    

translate([-69.5, 0, bracket_top_z])
    rotate([0, 90, 0])
    cylinder(r=groove7_radius, h=groove7_depth);

// === Top Resolver Gears & Cubes ===
for (side = [-1, 1]) {
    translate([side * -69.7, 0, 182.0])
        rotate(side * [0, 90, 0])
        cylinder(r=resolver_gear_radius, h=resolver_gear_depth);

    translate([side * -66.7, 0, 178])
        cube([resolver_cube_width, resolver_cube_height, resolver_cube_depth], center=true);
}

// === Top Resolver Cubes ===
for (side = [-1, 1]) {
    translate([side * 112, 0, 195])
        cube([resolver_top_cube_width, resolver_top_cube_depth, resolver_top_cube_height], center=true);
}

// === Connectors ===
translate(connector1_pos)
    cube(connector1_dim);

translate(connector2_pos)
    cube(connector2_dim);
