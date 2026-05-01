# arm_kinematics_benchmark

Google Benchmark executables for `arm_kinematics`.

## Build

```
nix-build ~/nova/nixfiles -A pkgs.ros.nova-arm-kinematics-benchmark
```

## Run

```
result/lib/arm_kinematics_benchmark/<benchmark_name>
```

Executables are self-contained — no environment setup needed.

## Benchmarks

### `benchmark_joint_map`
Joint map builder pipeline. No ROS dependency.

### `benchmark_twistmapper_collision`
Collision stack as used by `nova_twistmapper`:
- `BM_TwistmapperConfigureCollisionStack` — one-time setup cost (FK + collision manager + trees)
- `BM_TwistmapperCheckPathCollision` — full path collision check (interpolates + collides)
- `BM_TwistmapperUpdateAndCollideSingleState` — single pose update + broadphase collide

### `benchmark_nova_twistmapper`
Per-cycle hot path of `nova_twistmapper`:
- `BM_TwistmapperPositionFk` — `Tree::position_fk` for one frame
- `BM_TwistmapperApplyTwistWithRotation` — `apply_twist` with non-zero angular component (AngleAxisd path)
- `BM_TwistmapperApplyTwistLinearOnly` — `apply_twist` translation only
- `BM_TwistmapperMakeSingleFrameTree` — `fk->make_tree` for one frame (frame-switch cost)
- `BM_TwistmapperGetPositionIk` — `BanksiaIKPlugin::get_position_ik`
- `BM_TwistmapperGetVelocityIk` — `InverseKinematicsPlugin::get_velocity_ik`

## Baseline results

Recorded 2026-04-25. Machine: 24-core Intel @ 5662 MHz, L3 32 MiB. Built with `-O3 -DNDEBUG -DEIGEN_NO_DEBUG`.

> CPU scaling was enabled during this run — treat times as approximate lower bounds.

### `benchmark_joint_map`

| Benchmark | Joints | CPU time |
|---|---|---|
| `BM_Reachability_LinearChain` | 8 | 0.27 µs |
| `BM_Reachability_LinearChain` | 32 | 0.69 µs |
| `BM_Reachability_LinearChain` | 128 | 2.3 µs |
| `BM_Reachability_AffineFan` | 8 | 0.94 µs |
| `BM_Reachability_AffineFan` | 32 | 3.4 µs |
| `BM_Reachability_AffineFan` | 128 | 13 µs |
| `BM_Reachability_Diamond` | 8 | 0.31 µs |
| `BM_Reachability_Diamond` | 32 | 0.77 µs |
| `BM_Reachability_Diamond` | 128 | 2.7 µs |
| `BM_Reachability_InputReorder` | 8 | 0.29 µs |
| `BM_Reachability_InputReorder` | 32 | 0.92 µs |
| `BM_Reachability_InputReorder` | 128 | 3.1 µs |
| `BM_PlanJointMap_LinearChain` | 8 | 1.5 µs |
| `BM_PlanJointMap_LinearChain` | 32 | 7.3 µs |
| `BM_PlanJointMap_LinearChain` | 128 | 30 µs |
| `BM_PlanJointMap_Diamond` | 8 | 2.1 µs |
| `BM_PlanJointMap_Diamond` | 32 | 8.8 µs |
| `BM_PlanJointMap_Diamond` | 128 | 35 µs |
| `BM_PlanJointMap_InputReorder` | 8 | 0.66 µs |
| `BM_PlanJointMap_InputReorder` | 32 | 2.8 µs |
| `BM_PlanJointMap_InputReorder` | 128 | 11 µs |
| `BM_Materialize_LinearChain` | 8 | 1.1 µs |
| `BM_Materialize_LinearChain` | 32 | 7.0 µs |
| `BM_Materialize_LinearChain` | 128 | 29 µs |
| `BM_Materialize_AffineFan` | 8 | 0.047 µs |
| `BM_Materialize_AffineFan` | 32 | 0.080 µs |
| `BM_Materialize_AffineFan` | 128 | 0.22 µs |
| `BM_Materialize_InputReorder` | 8 | 0.047 µs |
| `BM_Materialize_InputReorder` | 32 | 0.079 µs |
| `BM_Materialize_InputReorder` | 128 | 0.21 µs |
| `BM_FullPipeline_LinearChain` | 8 | 3.7 µs |
| `BM_FullPipeline_LinearChain` | 32 | 16 µs |
| `BM_FullPipeline_LinearChain` | 128 | 63 µs |
| `BM_FullPipeline_AffineFan` | 8 | 2.2 µs |
| `BM_FullPipeline_AffineFan` | 32 | 6.9 µs |
| `BM_FullPipeline_AffineFan` | 128 | 27 µs |
| `BM_FullPipeline_Diamond` | 8 | 4.6 µs |
| `BM_FullPipeline_Diamond` | 32 | 19 µs |
| `BM_FullPipeline_Diamond` | 128 | 70 µs |
| `BM_FullPipeline_InputReorder` | 8 | 1.6 µs |
| `BM_FullPipeline_InputReorder` | 32 | 5.6 µs |
| `BM_FullPipeline_InputReorder` | 128 | 22 µs |

### `benchmark_twistmapper_collision`

| Benchmark | Time | CPU |
|---|---|---|
| `BM_TwistmapperConfigureCollisionStack` | 2173 µs | 2166 µs |
| `BM_TwistmapperCheckPathCollision` | 5.17 µs | 5.17 µs |
| `BM_TwistmapperUpdateAndCollideSingleState` | 1.47 µs | 1.47 µs |

### `benchmark_nova_twistmapper`

| Benchmark | Time | CPU |
|---|---|---|
| `BM_TwistmapperPositionFk` | 0.154 µs | 0.154 µs |
| `BM_TwistmapperApplyTwistWithRotation` | 0.017 µs | 0.017 µs |
| `BM_TwistmapperApplyTwistLinearOnly` | 0.002 µs | 0.002 µs |
| `BM_TwistmapperMakeSingleFrameTree` | 5.66 µs | 5.65 µs |
| `BM_TwistmapperGetPositionIk` | 0.180 µs | 0.180 µs |
| `BM_TwistmapperGetVelocityIk` | 0.204 µs | 0.203 µs |
