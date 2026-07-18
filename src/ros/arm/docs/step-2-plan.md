# Step 2: Fix Collision Parity in `arm_kinematics`

## Context

The three arm controllers (`nova_arm_controller`, `nova_twistmapper`, `nova_path_planner`)
all use MoveIt's `PlanningScene::checkSelfCollision()` for self-collision checking. They share
an identical pattern: set the robot state to default values (zero pose), run a collision check,
and mark all contact pairs as allowed in the ACM. This is how the controllers avoid flagging
links that are always in contact at rest.

`arm_kinematics` already provides `CollisionManager`, `DiscreteCollisionPlugin`,
`ColliderDefinitions`, and `AllowedCollisionMatrix` — but the ACM is only populated with
same-link collider pairs at construction time. There is no zero-pose probing, no explicit
allowed-pair configuration, and the pair-collection codepath has a bug. These gaps must be
closed before any controller can migrate off MoveIt collision.

## Work Items

### 2a. Fix `collide(pairs)` bug

**File:** `arm_kinematics/arm_kinematics/src/plugins/collision/fcl/fcl_collision_plugin.cpp:128`

**Problem:** The `collide(colliding_pairs, max_colliding_pairs)` method passes `collide_with_acm`
(the boolean-only callback) instead of `collide_with_acm_and_pairs` (which also populates the
pairs vector). The result: collision is detected correctly but the pairs vector is always empty.

**Fix:** Change line 128 from:
```cpp
manager_.collide(&query, collide_with_acm);
```
to:
```cpp
manager_.collide(&query, collide_with_acm_and_pairs);
```

One-line fix.

### 2b. Add test coverage for `collide(pairs)`

**File:** `arm_kinematics/arm_kinematics/test/collision/fcl/test_fcl_collision_plugin.cpp`

Add a test case to the existing `SimpleUrdfCollisionTests` fixture that:

1. Puts the robot in a colliding configuration (`{-2, -2}`)
2. Calls `collide(pairs)` via a public accessor on CollisionManager (see 2f)
3. Asserts `pairs` is non-empty
4. Asserts the returned pair contains the expected collider indices

This test would have caught the bug in 2a. It should be written first so it fails, then
fixed by 2a.

### 2c. Zero-pose ACM probing

**What the controllers do today (identically in all three):**

```cpp
// nova_twistmapper.cpp:760-791, nova_path_planner.cpp:737-768,
// self_collision_limiter.cpp:134-160
state.setToDefaultValues();
state.update();
planning_scene_->checkSelfCollision(req, res, state);
for (auto & [pair, contacts] : res.contacts)
  acm.setEntry(pair.first, pair.second, true);
```

MoveIt's ACM uses link names. arm_kinematics' ACM uses collider indices. But
`collide(pairs)` already returns `vector<pair<size_t, size_t>>` — collider-index pairs — so
the arm_kinematics version is simpler: no name-to-index mapping needed for zero-pose probing.

**New free function** in `arm_kinematics/collision/`:

```cpp
/// Probe for collisions at the given joint configuration and add all
/// colliding pairs to the plugin's AllowedCollisionMatrix.
///
/// Intended for setup-time ACM population: run at zero/default pose to
/// discover which collider pairs are always in contact, then allow them.
///
/// \param plugin   Collision plugin (already initialized)
/// \param tree     FK tree to compute collider poses
/// \param joint_values  Joint configuration to probe (typically zero pose)
void probe_and_allow_collisions(
    DiscreteCollisionPlugin & plugin,
    ForwardKinematicsPlugin::Tree & tree,
    span<const double> joint_values);
```

Implementation:
1. Compute FK: `tree.position_fk(joint_values, poses)`
2. Update plugin poses: `plugin.update_poses(0, poses)`
3. Collect pairs: `plugin.collide(pairs)` — note: this relies on the 2a fix
4. Add each pair to ACM: `plugin.get_allowed_collision_matrix().set(a, b, true)`

This is a non-member function because it operates on the plugin and tree before they are
wrapped in a `CollisionManager`. It lives near `CollisionManager` as a peer utility.

### 2d. Explicit allowed-pair configuration by link name

The controllers also need to allow specific pairs via ROS parameters (e.g. links that are
always in contact due to mechanical design but not at the zero pose).

**Problem:** Explicit pairs are specified by link name, but the ACM uses collider indices. A
single link may have multiple colliders. We need a link-name → collider-indices mapping.

**The mapping already exists:** `ColliderDefinitions::frames.parent_link_names[i]` gives the
parent link name for collider index `i`. This parallel vector is constructed alongside the
colliders in `collider_definitions.cpp:27-67`.

**New free function** in `arm_kinematics/collision/`:

```cpp
/// Add allowed collision pairs to the ACM by link name.
///
/// For each (link_a, link_b) pair, all colliders belonging to link_a are
/// allowed to collide with all colliders belonging to link_b.
///
/// \param acm  The ACM to mutate
/// \param parent_link_names  Parallel vector mapping collider index → link name
///        (from ColliderDefinitions::frames.parent_link_names)
/// \param allowed_pairs  Pairs of link names to allow
void allow_collision_pairs_by_link(
    AllowedCollisionMatrix & acm,
    span<const std::string> parent_link_names,
    span<const std::pair<std::string, std::string>> allowed_pairs);
```

Implementation: for each link pair, find all collider indices with matching parent link names,
then `acm.set(i, j, true)` for each combination. This is O(pairs × colliders²) but runs once
at setup time.

**Diagnostics:** If a link name in `allowed_pairs` does not match any entry in
`parent_link_names`, log a warning and skip the pair (do not fail). Discovered pairs from
`probe_and_allow_collisions` are logged at DEBUG level.

### 2e. `CollisionConfig` struct + parameter reading

A struct capturing the ACM configuration, read from ROS parameters during `on_configure`:

```cpp
namespace arm_kinematics {

struct CollisionConfig {
  /// Whether to probe at a default pose and allow all pairs found in collision.
  bool generate_from_default_pose = true;

  /// Per-joint default pose overrides. Joints not listed default to 0.0.
  /// Keyed by joint name.
  std::unordered_map<std::string, double> default_pose_overrides{};

  /// Explicit allowed collision pairs, specified by link name.
  std::vector<std::pair<std::string, std::string>> allowed_pairs{};
};

}
```

**Parameter schema** (read via `ParamReader` or the controller's `generate_parameter_library`
params struct):

```yaml
collision:
  generate_from_default_pose: true       # bool
  default_pose:                          # map<string, double>
    shoulder_yaw: 0.1
    elbow: -0.5
  allowed_pairs:                         # list of "link_a/link_b" strings
    - "base_link/shoulder_link"
```

The parameter reading is controller-side (each controller reads its own params). The
`CollisionConfig` struct is in arm_kinematics so it can be passed to the factory.

### 2f. `make_collision_manager` overload with `CollisionConfig`

A new overload that performs the full ACM setup:

```cpp
tl::expected<CollisionManager, MakeTreeError> make_collision_manager(
    PluginLoader & loader,
    const ForwardKinematicsPlugin::SharedPtr & fk,
    const std::vector<std::string> & joint_names,
    const CollisionConfig & config);
```

Implementation:
1. `loader.make_collision(joint_names, fk)` → `{tree, plugin}`
   - Currently this calls `unwrap_make_tree_result` which throws. After step 1 (error type
     redesign), this returns a typed error. Until then, wrap in try/catch and convert.
2. If `config.generate_from_default_pose`:
   - Build joint-value vector from `joint_names`: 0.0 for each, overridden by
     `config.default_pose_overrides[name]` if present
   - Call `probe_and_allow_collisions(plugin, tree, joint_values)`
3. If `config.allowed_pairs` is non-empty:
   - Retrieve `parent_link_names` — this requires `ColliderDefinitions::frames` to be
     available. Currently `make_collision` consumes the frames (moves them into `make_tree`).
     **This is a design issue** (see Open Issue below).
   - Call `allow_collision_pairs_by_link(acm, parent_link_names, config.allowed_pairs)`
4. Return `CollisionManager{tree, plugin}`

**Open issue — frame data availability:** `PluginLoader::make_collision(joint_names, fk)`
currently moves `ColliderDefinitions::frames` into `fk->make_tree(...)` and the
`parent_link_names` are consumed. The explicit-pairs utility needs those names. Options:

- **(a)** Clone `parent_link_names` before moving frames into `make_tree`. Cheap — it's a
  `vector<string>` of ~10 elements, used once at setup time.
- **(b)** Have `make_collision` return the parent link names alongside the tree and plugin
  (extend `MakeCollisionResult`).
- **(c)** Re-derive from URDF after construction. Wasteful but safe.

Recommend **(b)**: extend `MakeCollisionResult` to include `parent_link_names`. It's a natural
part of the collision construction result and avoids the clone.

### 2g. Expose `collide(pairs)` through `CollisionManager`

`CollisionManager` currently only exposes `bool collide()`. The zero-pose probing utility
(2c) operates on the raw plugin + tree before wrapping, so it doesn't need this. But for
future use and for testing (2b), `CollisionManager` should also expose the pair-collecting
overload:

```cpp
struct CollisionManager {
  // existing:
  [[nodiscard]] bool collide() const;
  void update_poses(const std::vector<double> & joint_states);

  // new:
  [[nodiscard]] bool collide(
      std::vector<std::pair<size_t, size_t>> & colliding_pairs,
      size_t max_colliding_pairs = std::numeric_limits<size_t>::max()) const;
};
```

This forwards to `plugin_->collide(colliding_pairs, max_colliding_pairs)`.

## File Manifest

| File | Action | Purpose |
|------|--------|---------|
| `src/plugins/collision/fcl/fcl_collision_plugin.cpp` | Edit | Fix callback on line 128 |
| `test/collision/fcl/test_fcl_collision_plugin.cpp` | Edit | Add `collide(pairs)` test |
| `include/arm_kinematics/collision/collision_manager.hpp` | Edit | Add `collide(pairs)` overload |
| `src/collision/collision_manager.cpp` | Edit | Implement `collide(pairs)` forwarding |
| `include/arm_kinematics/collision/collision_config.hpp` | New | `CollisionConfig` struct |
| `include/arm_kinematics/collision/collision_utilities.hpp` | New | `probe_and_allow_collisions`, `allow_collision_pairs_by_link` |
| `src/collision/collision_utilities.cpp` | New | Implementations |
| `src/plugin_loader.cpp` | Edit | New `make_collision_manager` overload; extend `MakeCollisionResult` |
| `include/arm_kinematics/plugin_loader.hpp` | Edit | `MakeCollisionResult` gains `parent_link_names` |
| `test/collision/` | New file | Tests for probing + explicit pair utilities |

All paths are relative to `arm_kinematics/arm_kinematics/`.

## Dependency on Step 1

Step 1 (error type redesign) changes `MakeTreeError` to a `std::variant`-based type and
removes the `unwrap_make_tree_result` throw. The new `make_collision_manager` overload (2f)
should return `tl::expected<CollisionManager, MakeTreeError>` and propagate the typed error.

If step 1 is not yet complete when step 2 begins:
- The 2a bug fix, 2b test, 2c probing utility, 2d explicit pairs, 2e config struct, and 2g
  accessor are all independent of the error type shape
- Only 2f (the factory overload) directly depends on step 1. It can be written with the
  current `const char *` error type and updated when step 1 lands.

## Order of Implementation

1. **2a** — fix `collide(pairs)` bug (one-line change)
2. **2g** — add `collide(pairs)` to `CollisionManager`
3. **2b** — add test for pair collection (verifies 2a + 2g)
4. **2c** — `probe_and_allow_collisions` utility
5. **2d** — `allow_collision_pairs_by_link` utility
6. **2e** — `CollisionConfig` struct
7. **2f** — `make_collision_manager` overload with config + extend `MakeCollisionResult`
8. Tests for 2c, 2d, 2f

## Verification

- `colcon test --packages-select arm_kinematics` passes (existing + new tests)
- New test for `collide(pairs)` demonstrates pairs are populated (catches 2a regression)
- New test for `probe_and_allow_collisions` demonstrates that after probing at a colliding
  pose, those pairs are marked allowed and subsequent `collide()` returns false
- New test for `allow_collision_pairs_by_link` demonstrates link-name-based allowance maps
  correctly to collider indices
