# Prompt: Add Macro Echoes to Scanner C View Ordering

## Goal
Port the macro echo/coarse-to-fine assignment from commit `2348620474146589741f799cee3fed361d8ae8a0` (`macro echoes`) into the scanner C code, starting with the 3D `CHEVRON` and `CROC` paths.

The relevant scanner entry point is `ks_generate_peplan_from_kcoords()` in `KSFoundation_host.c`. The relevant implementation paths are:

- `ks_generate_peplan_from_kcoords_american_3d()` for `CHEVRON` and `ECLIPSE`; only enable the macro echo behavior for `CHEVRON` initially.
- `ks_generate_peplan_from_kcoords_CROC()` and `_croc_search()` for `CROC`.

Do not add tests in this change.

## Background
The dashboard commit introduced macro echoes to reduce within-shot k-space jumps, especially for accelerated scans with large ETL. The dashboard-side reference implementation lives in `dashboard/kspace-utils.js` and adds these main pieces:

- `preserveBaseEcho()`
- `applyMacroCoarseFineAssignment()`
- `macroSegmentWidth()`
- `macroEchoFromRank()`
- `assignMacroEchoesBySortedIndices()`
- `coarseCenterEcho()`
- `assignChevronCoarseMacroEchoes()`
- `assignCrocCoarseMacroEchoes()`
- `assignMacroSegmentPaths()`
- `calculateJumpMetrics()`

The key idea is:

1. Compute the ordinary Chevron/CROC ordering enough to know the base distance/order and base echo behavior.
2. Group echoes into macro segments using a deterministic width schedule.
3. Assign each coordinate to a macro echo by rank in the coarse distance ordering.
4. Within each macro segment, sort coordinates by the radar/path order and pack them into shot lanes with fine echoes inside that macro segment.

This produces much shorter jumps within a shot while preserving center echo placement at the macro level.

## Constraints
- Keep the first implementation limited to `CHEVRON` and `CROC`.
- Do not add tests.
- Keep behavior unchanged when macro echo width is disabled or set to `1`.
- Preserve the existing public behavior and error handling as much as possible.
- Avoid large unrelated refactors in `KSFoundation_host.c`.
- Use the existing C sorting/comparator style and memory management conventions.
- Make sure every `malloc` is checked and every allocation is freed on both success and failure paths.

## Important Indexing Detail
The dashboard code uses 1-based echo numbers. The C scanner code uses 0-based `encode` values and receives `center` as `[0, etl - 1]`.

When translating helpers:

- Dashboard `macroEcho` maps to a C 0-based `macro_segment`.
- Build and use explicit 0-based segment boundaries instead of assuming constant width:
  ```c
  segment_start[segment] <= encode < segment_end[segment]
  ```
- Map `center` to the segment whose boundary range contains `center`.
- A segment width is:
  ```c
  width = segment_end[segment] - segment_start[segment];
  ```
- Map rank to segment by cumulative candidate capacity:
  ```c
  capacity = width * shot_dist.shots;
  ```

Be careful with the existing `KS_PEPLAN_SHOT_DISTRIBUTION`: the current Chevron/CROC code assigns `views[idx].encode = idx / shot_dist.shots` and allocates the final `peplan` with `etl`. Match the existing scanner semantics rather than blindly assuming every shot fills all `etl` slots.

## Suggested Implementation Shape

### 1. Add small helper functions near the peplan helpers
Add narrowly scoped static helpers for macro echo assignment. Suggested helpers:

- `ks_peplan_build_macro_segments(etl, max_macro_width, segment_start, segment_end, max_segments)`
- `ks_peplan_macro_segment_from_rank(rank, segment_start, segment_end, num_segments, shots)`
- `ks_peplan_macro_segment_from_encode(encode, segment_start, segment_end, num_segments)`
- A helper to find the coordinate nearest k-space origin.
- A helper to directly set a macro-assigned plan from final `(encode, shot)` assignments.
- A helper to calculate RMS jump for a candidate final assignment.

Prefer the simplest temporary state that fits the existing code. In most cases this can be a few local arrays indexed like `views` (`base_encode`, `macro_segment`, `final_encode`, `final_shot`, and possibly `original_rank`) rather than a new public or file-level struct. Only introduce a small private struct if the array bookkeeping becomes harder to read than the struct.

### 2. Parameter/default for macro width
Use the existing scanner parameter/configuration mechanism if one exists in the destination repository.

If there is no obvious parameter surface, implement the helper so that `macro_width == 1` is the old/no-macro baseline, positive values are the maximum width for a ramped schedule, and `macro_width == -1` enables automatic maximum-width selection. Do not make the scanner impossible to disable.

For fixed positive values:

- `macro_width == 1`: use the old path with no macro echo changes.
- `macro_width > 1`: build a linear ramp schedule from width `2` in the first macro segment to width `macro_width` in the final macro segment. A fixed positive `macro_width` is therefore a maximum ramp width, not a constant segment width.

Build the ramp schedule deterministically in encode order:

1. Start with `start_encode = 0`.
2. For each segment, compute `progress = start_encode / max(1, etl - 1)`.
3. Compute `width = roundf(2.0f + progress * (max_macro_width - 2))`.
4. Clip `width` to `[2, max_macro_width]`, then clip it again so the segment does not extend past `etl`.
5. Store `segment_start[segment] = start_encode` and `segment_end[segment] = start_encode + width`.
6. Continue from `start_encode = segment_end[segment]` until `start_encode == etl`.

This makes early segments narrower than or equal to late segments when `max_macro_width > 2`, which avoids over-perturbing the first echo segments where radius changes quickly.

When `macro_width == -1`, exhaustively evaluate candidate maximum widths `1` through `5` for the current CROC/Chevron candidate:

1. Generate the complete candidate final `(encode, shot)` assignment for each width.
2. Calculate RMS jump for each candidate from the final shot paths.
3. Select the width with the lowest RMS jump.
4. Track both the best and worst RMS jump observed during the search.
5. Report the selected max width, best RMS jump, worst max width, and worst RMS jump with `ks_dbg()`.

Candidate width `1` is important because it is the baseline behavior; including it makes the debug output quantify how much the selected macro echo schedule improves over the old ordering. Candidates `2` through `5` use ramp schedules from `2` up to that candidate maximum width.

The RMS jump calculation should match the dashboard `calculateJumpMetrics()` behavior: group final assignments by shot, sort each shot by final encode, sum squared Euclidean jumps in `(ky, kz)` between consecutive encodes in the same shot, and return `sqrt(total_squared_jump / transition_count)`. Use unshifted k-space coordinates for the jump calculation; the constant matrix offset does not affect distances.

### 3. Chevron macro echo assignment
In `ks_generate_peplan_from_kcoords_american_3d()`, after the final `axis_ratio` is known and `views[idx].encode` has been assigned by sorted Chevron radius:

1. Preserve each view's base encode and original rank.
2. Build the macro segment boundary arrays for the selected candidate max width.
3. Assign each coordinate a macro segment by sorting/ranking on the Chevron radius (`coord->user1`) using the cumulative segment capacities:
   ```c
   macro_segment = ks_peplan_macro_segment_from_rank(rank, segment_start, segment_end, num_segments, shot_dist.shots);
   ```
4. Use Chevron radar order as the macro path key. The current code already computes `coord->t` via `_pe_set__theta()`, and existing sorting uses `ks_comp_kview_e_tr`.
5. For each macro segment:
   - collect the views whose `macro_segment` matches the segment,
   - sort by `macro_order_key`, then `base_encode`, then `original_rank`,
   - assign final lanes:
     ```c
     for shot in 0..shot_dist.shots-1:
       for encode = segment_start[segment]..segment_end[segment]-1:
         final_shot = shot;
         final_encode = encode;
     ```
6. Build the `KS_PHASEENCODING_PLAN` from the final `(encode, shot)` assignments.

This should replace or bypass the final `qsort(views, ..., ks_comp_kview_e_tr)` + `ks_peplan_assign_shots()` path only when macro echoes are enabled. Keep the old path for fixed `macro_width == 1`; use the candidate-generation path for `macro_width == -1` so width `1` can still be measured as the baseline candidate.

### 4. CROC macro echo assignment
In `ks_generate_peplan_from_kcoords_CROC()`, after `_croc_search()` has found `new_center`/`yz_ratio` and after reversed encode handling if applicable:

1. Preserve each view's base encode and original rank.
2. Build the macro segment boundary arrays for the selected candidate max width.
3. Assign macro segments by CROC distance rank from the offset center, following the dashboard `assignCrocCoarseMacroEchoes()` behavior but using schedule-aware segment capacities.
4. Preserve the existing CROC angle/radar key logic:
   - The code stores the angle in `view->coord->user1`.
   - Existing sorting uses `ks_get_comp_kview_e_user1(reversed)`.
   - The special encode-0 singularity handling should either be preserved for the macro segment containing encode `0`, or explicitly folded into the macro segment sort so that the first segment does not regress.
5. Pack each macro segment into shot lanes exactly as for Chevron, using that segment's own `[start, end)` boundary.
6. Directly set the peplan from final `(encode, shot)` assignments, or prepare a view order that is exactly equivalent.

For center search, prefer matching the dashboard behavior: search for the target macro segment, not the exact fine encode, when macro echoes are enabled. The target macro segment is the segment whose `[start, end)` range contains `center`. If the existing `_croc_search()` is reused unchanged, verify that the origin still lands in the macro segment containing `center`; if not, add a macro-aware `_croc_search` variant using `ks_peplan_macro_segment_from_rank()`.

### 5. Direct peplan fill helper
Because macro packing naturally produces explicit `(final_encode, final_shot)` pairs, it may be clearer and less error-prone to allocate and fill the plan directly instead of forcing the result back through `ks_peplan_assign_shots()`.

Use the same coordinate offsets as the existing code:

```c
ks_phaseencoding_set(peplan,
                     final_encode,
                     final_shot,
                     coord->y + kacq->matrix_size[YGRAD] / 2,
                     coord->z + kacq->matrix_size[ZGRAD] / 2);
```

Leave unfilled slots as `KS_NOTSET`, matching the allocation behavior.

## Acceptance Criteria
- `CHEVRON` and `CROC` 3D paths use macro echo assignment when enabled.
- `macro_width == 1` preserves the existing behavior.
- `macro_width > 1` uses a ramped schedule whose early segments are narrower than or equal to later segments.
- `macro_width == -1` compares the baseline plus ramped max-width candidates `2..5`, selects the lowest RMS jump candidate, and logs the best and worst RMS jump with `ks_dbg()`.
- The k-space origin remains in the macro segment containing the requested `center` echo.
- The final plan has no duplicate `(encode, shot)` assignments.
- All acquired coordinates are assigned exactly once.
- CROC reversed-center behavior still works.
- Existing non-CROC, non-Chevron paths are unchanged.
- No tests are added.

## Useful Reference
Use commit `2348620474146589741f799cee3fed361d8ae8a0` as the algorithm reference:

```bash
git show 2348620474146589741f799cee3fed361d8ae8a0 -- dashboard/kspace-utils.js
```

The most relevant dashboard functions are `applyMacroCoarseFineAssignment()`, `assignChevronCoarseMacroEchoes()`, `assignCrocCoarseMacroEchoes()`, `assignMacroSegmentPaths()`, and `calculateJumpMetrics()`. The dashboard currently uses constant macro widths; the scanner implementation requested here should use the ramped schedule described above.
