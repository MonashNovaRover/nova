# Joint Map Pipeline — Performance Analysis

**Date:** 2026-04-12  
**Profiled by:** Bailey Chessum  
**Binary:** `benchmark_joint_map` (built with `-O3 -fno-omit-frame-pointer -g`)  
**Profiling tools:** Google Benchmark, `perf stat`, `perf record --call-graph fp`, `eu-unstrip`

---

## Methodology

### Step 1 — Build with profiling flags

Nix strips debug info and omits frame pointers by default. To get usable symbols and call graphs,
`default.nix` was updated to add two flags to `NIX_CFLAGS_COMPILE`:

```nix
NIX_CFLAGS_COMPILE = [
  "-O3"
  "-DNDEBUG"
  "-DEIGEN_NO_DEBUG"
  "-fno-omit-frame-pointer"   # required for --call-graph fp
  "-g"                         # emit DWARF debug info (split into separate .debug files by nix)
];
```

Then rebuilt:

```bash
nix-build /home/nova/nova/nixfiles -A pkgs.ros.nova-arm-kinematics
```

The resulting benchmark binary is at:

```
./result/lib/arm_kinematics/benchmark_joint_map
```

### Step 2 — Benchmark binary

`benchmark/joint_map_builder_benchmark.cpp` exercises each pipeline stage in isolation and as a full
pipeline across three synthetic topology families:

| Family | Description |
|---|---|
| `LinearChain(N)` | N joints connected by N-1 serial single-input/single-output transmissions |
| `AffineFan(N)` | Joint 0 as source; joints 1…N-1 mimic it via affine transmissions |
| `Diamond(N)` | 1 source → N intermediates (fan-out) → 1 sink (fan-in) |

Each is parameterized at N ∈ {8, 32, 128}. Topology construction is outside the measured loop;
only the target function call is timed per iteration. Run with:

```bash
./result/lib/arm_kinematics/benchmark_joint_map
```

### Step 3 — Hardware counters with `perf stat`

`perf` is not in the nix package closure, so it was accessed via a temporary shell:

```bash
nix-shell -p linuxPackages.perf
```

Then:

```bash
perf stat -e cycles,instructions,cache-misses,cache-references,branch-misses,branches \
  ./result/lib/arm_kinematics/benchmark_joint_map --benchmark_filter=128
```

This measures IPC (instructions/cycle), cache miss rate, and branch miss rate for the N=128
benchmarks. These counters characterise *what kind* of bottleneck the code has — compute-bound,
memory-bound, branch-misprediction-bound — before looking at individual functions.

### Step 4 — Merge split debug symbols

Nix builds split DWARF: the binary itself has no embedded debug info; symbols live in a
separate `.debug` file inside the nix store. `perf report` can't resolve function names without
merging them back. `eu-unstrip` (from `elfutils`) does this:

```bash
nix-shell -p binutils  # provides eu-unstrip

BENCH=./result/lib/arm_kinematics/benchmark_joint_map
DEBUGF=$(find /nix/store -name "benchmark_joint_map.debug" 2>/dev/null | head -1)

eu-unstrip "$BENCH" "$DEBUGF" -o /tmp/bench_with_syms
```

The output `/tmp/bench_with_syms` is a fully self-contained binary with all symbols inlined.

### Step 5 — Call-graph profile with `perf record`

```bash
perf record --call-graph fp \
  -o /tmp/perf.data \
  -- /tmp/bench_with_syms \
       --benchmark_filter=FullPipeline \
       --benchmark_repetitions=5
```

`--call-graph fp` uses the frame pointer (enabled by `-fno-omit-frame-pointer` above) to unwind
call stacks. This is faster and lower-overhead than DWARF unwinding, and sufficient here since
the hot functions are not deeply inlined leaf-only code.

`--benchmark_filter=FullPipeline` focuses sampling on the end-to-end pipeline benchmarks, which
exercise all pipeline stages and produce the most representative profile.

### Step 6 — Inspect the report

```bash
perf report -i /tmp/perf.data
```

The interactive TUI shows self-time percentages per symbol. Expand a symbol with `a` to see
annotated source/disassembly. The top-level flat profile (not call-tree) was used to produce
the table in the next section.

---

## Benchmark results (N=128)

| Benchmark | Time |
|---|---|
| `BM_Reachability_LinearChain/128` | 13.6 µs |
| `BM_Reachability_AffineFan/128` | 21.3 µs |
| `BM_Reachability_Diamond/128` | 12.8 µs |
| `BM_PlanJointMap_LinearChain/128` | 36.0 µs |
| `BM_PlanJointMap_Diamond/128` | 42.7 µs |
| `BM_Materialize_LinearChain/128` | 29.7 µs |
| `BM_Materialize_AffineFan/128` | 1.0 µs |
| `BM_FullPipeline_LinearChain/128` | 87.6 µs |
| `BM_FullPipeline_AffineFan/128` | 40.6 µs |
| `BM_FullPipeline_Diamond/128` | 97.4 µs |

`plan_joint_map` (36–43 µs) and `materialize_joint_map` (30 µs) dominate for the transmission-heavy
`LinearChain` and `Diamond` topologies. The `AffineFan` pipeline is fast (~40 µs total) because
materialize is near-free (1 µs) — affine transmissions require no `ComputeTransmission` allocation.

---

## Hardware counter summary

| Counter | Value | Interpretation |
|---|---|---|
| IPC | **4.06** | Compute-bound — CPU is fully occupied |
| L1/L2 cache miss rate | **0.10%** | Working set fits in cache — NOT memory-bound |
| Branch miss rate | **0.28%** | Branches are well-predicted |

The process is **compute-bound, not memory-bound**. All hot data fits comfortably in L1/L2. Latency
improvements must come from reducing work (fewer allocations, simpler data structures), not from
cache-friendlier memory layouts.

---

## Perf report — top functions (FullPipeline/128, annotated)

| % CPU | Symbol | Location |
|---|---|---|
| 13.07% | `_int_malloc` | glibc allocator |
| 12.81% | `_int_free` | glibc allocator |
| 7.26% | `malloc` | glibc allocator |
| 7.14% | `materialize_pure_affine` + `__memcmp_evex_movbe` | materialize_joint_map.cpp |
| 5.87% | `malloc_consolidate` | glibc allocator |
| 4.45% | `_M_find_before_node` | unordered_map bucket traversal |
| 4.32% | `run_fixed_point` inner lambda | transmission_reachability.cpp |
| 3.77% | `plan_joint_map` | joint_map_blueprint.cpp |
| 3.21% | `cfree` | glibc allocator |
| 2.87% | `unlink_chunk` | glibc allocator |
| 1.91% | `process_affine_hypernode` | transmission_reachability.cpp |
| 1.73% | `__memmove_avx512` | vector element shifting |
| 1.69% | `_M_insert_unique_node` | SID→Producer map insertion |
| 1.26% | `operator new` | C++ operator new |
| 1.20% | `_Prime_rehash_policy::_M_need_rehash` | unordered_map rehashing |
| 0.83% | `vector<variant>::_M_realloc_append` | variant vector reallocation |

**~46% of all CPU time is inside the glibc heap allocator.** This is the dominant bottleneck.

---

## Issues identified

### Issue 1 — Allocator dominance (~46% CPU) ⚠️ HIGH

**Found by:** perf report  
**Symptoms:** `_int_malloc` (13%), `_int_free` (13%), `malloc` (7%), `malloc_consolidate` (6%),
`cfree` (3%), `unlink_chunk` (3%), `operator new` (1%) total ~46%.

This is the single largest bottleneck. Each call to `build_expected()` triggers dozens of
`unordered_map`, `vector`, and `unique_ptr` heap allocations across every pipeline stage. The
allocator is thrashing because many small objects are allocated and freed within a single pipeline
run.

**Root cause locations:**
- `plan_joint_map`: `unordered_set<TransmissionInstanceId>`, `unordered_set<StateInterfaceDefinition>`,
  `unordered_map<TID, vector<TID>>`, `unordered_map<TID, size_t>`, `vector<ScratchFillRow>` per-stage
  (`joint_map_blueprint.cpp:68-69, 193-217`)
- `materialize_joint_map`: `unordered_map<StateInterfaceDefinition, size_t>` for scratch buffer
  indexing, `unique_ptr<ComputeTransmission>` per transmission
- `TransmissionReachability::analyze()`: producer map insertions for each SID

**Fix direction:** Pre-reserve all containers whose size is bounded by analysis counts. Replace
`unordered_map<TransmissionInstanceId, T>` (dense integer key) with `vector<T>` indexed directly.

---

### Issue 2 — `unordered_map` with dense integer keys should be `vector` ⚠️ HIGH

**Found by:** code analysis + `_M_find_before_node` in perf (4.45% CPU)  
**Location:** `src/joint_map/joint_map_blueprint.cpp:68-69`

```cpp
std::unordered_map<TransmissionInstanceId, std::vector<TransmissionInstanceId>> dependents;
std::unordered_map<TransmissionInstanceId, std::size_t> in_degree;
```

`TransmissionInstanceId` is a dense integer index (`size_t`) starting from 0. Using `unordered_map`
means every lookup involves a hash, a bucket scan, and a heap-allocated node chain. Since
`topo_order.size()` is known before these maps are built, both can be replaced with plain `vector`s
indexed by `TransmissionInstanceId` directly.

**Same pattern in reachability:** `producer_of()` lookup map in `TransmissionReachability` uses
`unordered_map<StateInterfaceId, Producer>` where `StateInterfaceId` is also a dense integer.

---

### Issue 3 — `StateInterfaceDefinition` equality via `memcmp` on string data (7% CPU)

**Found by:** perf report — `__memcmp_evex_movbe` co-appearing with `materialize_pure_affine`
and `_M_find_before_node`  
**Location:** `include/arm_kinematics/joint_map/state_interface_definition.hpp:42-48`,
`src/joint_map/materialize_joint_map.cpp`

`StateInterfaceDefinition::operator==` delegates to `InterfaceId::operator==`, which compares
by hash first but then by name string (to guard against hash collisions). The `unordered_map`
in `materialize_joint_map` uses `StateInterfaceDefinition` as the key, so every lookup involves
`memcmp` on interface name strings.

Internally, once a `StateInterfaceId` is assigned, all comparisons and lookups should use
the dense integer rather than the struct. The `materialize_joint_map` scratch-index map should
be keyed by `StateInterfaceId`, not `StateInterfaceDefinition`. The `StateInterfaceDefinition`
→ `StateInterfaceId` translation happens once at `ensure_state_interface_id()`.

---

### Issue 4 — O(N) `ready.erase(begin())` in Kahn's topological sort

**Found by:** code analysis + `__memmove_avx512` in perf (1.73% CPU confirms vector shifting)  
**Location:** `src/joint_map/joint_map_blueprint.cpp:110`

```cpp
const TransmissionInstanceId tid = ready.front();
ready.erase(ready.begin());   // O(N) shift of remaining elements
```

The `ready` vector is maintained in sorted order. Popping the front shifts all remaining elements
left via `memmove`. The perf profile shows `__memmove_avx512` at 1.73%, confirming this occurs in
hot code.

**Fix:** Use `std::priority_queue` (min-heap) or a sorted `std::set` for O(log N) dequeue, or
use an index cursor (`size_t front = 0; tid = ready[front++];`) since the sorted order is
maintained on insertion anyway.

---

### Issue 5 — `unordered_map` rehashing from missing `.reserve()` (1.2% CPU)

**Found by:** perf report — `_Prime_rehash_policy::_M_need_rehash` at 1.20%  
**Location:** `src/joint_map/joint_map_blueprint.cpp:204-205`, `src/joint_map/transmission_reachability.cpp`

```cpp
std::unordered_map<TransmissionInstanceId, std::size_t> stage_index_of;
stage_index_of.reserve(topo_order.size());  // ← this one IS reserved
// But dependents and in_degree above are not reserved before the insertion loop
```

`dependents` and `in_degree` at lines 68-69 are never `.reserve()`-d before the loop that fills
them. Each rehash allocates a new bucket array, copies all entries, and frees the old array —
visible as `_Prime_rehash_policy` in the profile.

**Fix:** Call `.reserve(sorted_required.size())` immediately after construction for both maps.
(Or replace with vectors as in Issue 2 — that eliminates the issue entirely.)

---

### Issue 6 — Temporary vector allocation inside fixed-point inner loop

**Found by:** code analysis + `vector<variant>::_M_realloc_append` in perf (0.83%)  
**Location:** `src/joint_map/transmission_reachability.cpp`

`process_affine_hypernode` rebuilds a local `affine_candidates` vector on each invocation. The
fixed-point loop calls `process_affine_hypernode` repeatedly per iteration until convergence,
making this a hot allocation path.

**Fix:** Hoist the candidates buffer to the outer scope and `clear()` it at the top of each call
instead of reconstructing it.

---

### Issue 7 — O(K×S) output scan in `emit_affine_batch_for_stage`

**Found by:** code analysis  
**Location:** `src/joint_map/joint_map_blueprint.cpp:289-291`

```cpp
for (std::size_t i = 0; i < output_count; ++i) {
  if (affine_batch_stage[i] != stage) continue;   // scans ALL outputs per stage call
  ...
}
```

`emit_affine_batch_for_stage` is called once per transmission stage. For K transmissions and
S outputs this is O(K×S). With N=128 and a diamond topology this is ~16K comparisons.

**Fix:** Pre-bucket outputs by stage into a `vector<vector<size_t>> outputs_by_stage` before
the emit loop. Each stage then iterates only its own outputs — O(S) total across all stages.

---

## Prioritized fix list

| Priority | Issue | Expected gain | Effort |
|---|---|---|---|
| 1 | Replace `unordered_map<TID, T>` with `vector<T>` (Issues 2 + 5) | ~8–12% CPU | Low |
| 2 | Reserve all unordered containers before fill loops (Issue 5) | ~1.5% CPU | Very low |
| 3 | Fix Kahn's erase(begin()) → cursor or priority_queue (Issue 4) | ~2% CPU at N=128, worse at N=512 | Low |
| 4 | Key scratch map by `StateInterfaceId` not `StateInterfaceDefinition` (Issue 3) | ~3–5% CPU | Medium |
| 5 | Hoist `affine_candidates` buffer out of inner loop (Issue 6) | ~1% CPU | Very low |
| 6 | Pre-bucket outputs by stage (Issue 7) | < 1% CPU at N=128, linear improvement | Low |

Issues 1–2 together address the `unordered_map` node-allocation and traversal overhead, which
together account for ~6% of measured CPU time directly and contribute significantly to the ~46%
allocator total.

---

## Notes

- All timings are from an isolated benchmark binary; the real robot arm has N≤12 joints, so
  absolute times are well under 10 µs for realistic inputs. The N=128 case stress-tests the
  algorithm's asymptotic behavior.
- The pipeline is initialization-only (called once at startup), so correctness and robustness
  outweigh micro-optimization. Issues 1–3 are worth fixing because they're clean data-structure
  improvements, not micro-optimizations.
- Build flags for profiling (`-fno-omit-frame-pointer -g`) are currently committed to `default.nix`.
  These are benign at `-O3` (frame pointer costs ~1 register; debug info is stripped at install
  time by default). They can be removed if binary size becomes a concern.
