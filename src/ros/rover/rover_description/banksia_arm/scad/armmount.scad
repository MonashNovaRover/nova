// === STL Reference ===
%scale(1000) import("armmount.stl");

// === Trapezoid Prism Parameters ===
top_length     = 250;
bottom_length  = 350;
face_height    = 250;
depth_z        = 180;

// === Construct Trapezoid Face in XY Plane ===
// Centered at origin
trapezoid_points = [
    [-bottom_length / 2, -face_height / 2],  // Bottom left
    [ bottom_length / 2, -face_height / 2],  // Bottom right
    [ top_length    / 2,  face_height / 2],  // Top right
    [-top_length    / 2,  face_height / 2]   // Top left
];

// === Rotate, Position, and Extrude the Trapezoid Prism ===
rotate([0, 0, -90])  // Face up in Z, rotate 90° around Z axis
    translate([-65, face_height / 2 + 6, 5])  // Position it nicely in space
        linear_extrude(height = depth_z)
            polygon(points = trapezoid_points);
