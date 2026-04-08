//
// Created by Bailey Chessum on 21/03/2026.
//

// JointMapBuilder is a pure virtual interface — no out-of-line definitions are needed in the
// base class. Concrete implementations (DefaultJointMapBuilder, FK-plugin-specific builders)
// provide the build_expected body. The legacy `build()` compatibility wrapper that threw on
// JointMapBuildError has been removed; callers use build_expected directly and pattern-match on
// tl::expected.

#include "arm_kinematics/joint_map/joint_map_builder.hpp"

namespace arm_kinematics {

// (no out-of-line definitions)

}  // namespace arm_kinematics
