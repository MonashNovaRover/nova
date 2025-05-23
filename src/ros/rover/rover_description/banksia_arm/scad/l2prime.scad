// === STL Reference (for visual comparison only) ===
%scale(1000) import("l2prime.stl");

// === Ring Parameters ===
ring_outer_radius = 38;
ring_inner_radius = 30;
ring_thickness    = 20;

// === Trapezoid Parameters ===
trap_top_width     = 18;
trap_bottom_width  = ring_outer_radius * 2;
trap_height        = 106;
trap_depth         = ring_thickness - 8;

// === Ring Geometry ===
rotate([0, 90, 0])
    difference() {
        cylinder(r = ring_outer_radius, h = ring_thickness, center = true);
        cylinder(r = ring_inner_radius, h = ring_thickness + 5, center = true);
    }

// === Trapezoid Body (2D profile extruded into 3D) ===
module trapezoid_prism() {
    linear_extrude(height = trap_depth)
        polygon([
            [-(trap_bottom_width / 2), 0],
            [ (trap_bottom_width / 2), 0],
            [ (trap_top_width / 2), trap_height],
            [-(trap_top_width / 2), trap_height]
        ]);
}

// === Trapezoid with Internal Cutouts ===
module trapezoid_with_hole() {
    difference() {
        trapezoid_prism();

        // Central ring hole subtraction (aligned with ring's inner hole)
        translate([0, 0, 8])
            rotate([0, 180, 0])
                cylinder(r = ring_inner_radius, h = trap_depth + 5, center = true);

        // Additional internal cylindrical cut (possibly bolt hole)
        translate([0, 94, 0])
            rotate([0, 180, 0])
                cylinder(r = 7.5, h = 200, center = true);
    }
}

// === Trapezoid Placement ===
translate([0, 5, -34.5])  // Offset in global coordinates
    rotate([0, -90, 0])    // Orient face outward
        translate([ring_outer_radius, 0, -trap_depth / 2])  // Push out from ring surface
            rotate([0, 0, -37.5])  // Angle upward/right
                trapezoid_with_hole();
