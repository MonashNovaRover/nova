// === STL Reference ===
%scale(1000) import("j5.stl");

// === Dimensions: J6 Connector Rod ===
rod_base_radius    = 10;
rod_base_height    = 12;

rod_shaft_radius   = 4;
rod_shaft_height   = 78;

rod_top_height     = 12;

total_rod_height   = rod_base_height + rod_shaft_height + rod_top_height;

// === Connector Rod Assembly ===
cylinder(r = rod_base_radius, h = rod_base_height);

translate([0, 0, rod_base_height])
    cylinder(r = rod_shaft_radius, h = rod_shaft_height);

translate([0, 0, rod_base_height + rod_shaft_height])
    cylinder(r = rod_base_radius, h = rod_top_height);

// === J5 Housing ===
housing_radius     = 60;
housing_height     = 48;
housing_z_center   = rod_base_height + (rod_shaft_height / 2);

translate([0, 0, housing_z_center])
    cylinder(r = housing_radius, h = housing_height, center = true);

// === Side Shaft Holes (Symmetric Cylinders) ===
side_shaft_radius  = 8;
side_shaft_length  = 7;

for (side = [-1, 1]) {
    translate([side * housing_radius, 0, housing_z_center])
        rotate([0, side * 90, 0])
            cylinder(r = side_shaft_radius, h = side_shaft_length);
}

// === Extra Shaft Pins ===
side_pin_r = 4;

// Right-side small pin
translate([housing_radius + side_shaft_length, 0, housing_z_center])
    rotate([0, 90, 0])
        cylinder(r = side_pin_r, h = 11);

// Left-side long pin
translate([-(housing_radius + side_shaft_length), 0, housing_z_center])
    rotate([0, -90, 0])
        cylinder(r = side_pin_r, h = 26);

// === Encoder Bolts ===
bolt_radius = 3;
bolt_height = 15.8;

bolt_x_offset = 12;
bolt_y_offset = 27;
bolt_z_pos = rod_base_height + ((rod_shaft_height - housing_height) / 2);

for (side = [-1, 1]) {
    translate([side * bolt_x_offset, bolt_y_offset, bolt_z_pos])
        rotate([0, 180, 0])
            cylinder(r = bolt_radius, h = bolt_height);
}
