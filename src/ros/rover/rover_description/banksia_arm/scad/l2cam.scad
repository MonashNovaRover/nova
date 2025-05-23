// === STL Reference (visual only, not exported) ===
// %scale(1000) import("l2cam.stl");

// === Cam Bar ===
cam_bar_dim    = [12, 80, 30];
cam_bar_angle  = 37;
cam_bar_pos    = [0, 42, 532];

translate(cam_bar_pos)
    rotate([cam_bar_angle, 0, 0])
        cube(cam_bar_dim, center = true);

// === Cam Hole Assembly ===
cam_hole_pos         = [0, 77, 558.5];
cam_outer_radius     = 15;
cam_outer_depth      = 12;

inner_hole_radius    = 7.1;
inner_hole_offset    = [-1.8, 2.8, 0];
inner_hole_depth     = 13;

translate(cam_hole_pos)
    rotate([0, 90, 0])
        difference() {
            cylinder(r = cam_outer_radius, h = cam_outer_depth, center = true);
            translate(inner_hole_offset)
                cylinder(r = inner_hole_radius, h = inner_hole_depth, center = true);
        }
