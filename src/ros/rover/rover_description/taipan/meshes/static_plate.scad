% scale(1000) import("D:/Coding/nova-onshape/arm-1-02/assets/new_linkage_mount.stl");
% scale(1000) import("D:/Coding/nova-onshape/arm-1-02/assets/dust_cover___bottom.stl");
% scale(1000) import("D:/Coding/nova-onshape/arm-1-02/assets/dust_cover___top_front.stl");
% scale(1000) import("D:/Coding/nova-onshape/arm-1-02/assets/static_plate.stl");
% scale(1000) import("D:/Coding/nova-onshape/arm-1-02/assets/l1_l2_shaft__configuration_default.stl");

//note: the assembly is closer together with 4.503 overlap between dustcover bottom and new_linkage_mount. This is using the center axis distance between the furthest face on new_linkage_mount and the furthest circle on static_plate. Its also using the distance between the furthest faces of the L1-L2 Shaft (from L1)
translate([67.4,0,9])
cube([262.787,95.40000,92.274], center=true);