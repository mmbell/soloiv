# CfRadial I/O Refactor Plan (branch: `cfradial`)

**Status:** Proposed — awaiting implementation.
**Goal:** Let soloiv (perusal + editor) read *and write* CfRadial (netCDF, v1 and v2/v2.1) sweep files in addition to DORADE, by routing **all** sweep I/O through the maintained LROSE `libRadx` C++ library via a thin `extern "C"` shim. The in-memory gate representation stays **scaled int16** (identical to today's DORADE path), so the render and editor code is reused unchanged; a native-float pipeline is a deferred optional phase (§4.7, Phase 7). The existing crashy in-tree DORADE byte parser is retired in favor of Radx's robust reader.

This document is self-contained: it can be executed from a fresh session with no prior context. All file:line references were captured against `main` at the time of writing (commit `b9ed4bc`); verify line numbers before editing, code may have shifted.

---

## 1. Decisions (agreed with user)

| # | Decision | Choice | Consequence |
|---|----------|--------|-------------|
| 1 | I/O strategy | **Link `libRadx` via `extern "C"` shim, keep custom-C reader as a structured fallback** | A `radar_io` backend abstraction with a Radx implementation now; a custom-netCDF implementation can slot behind the same interface later. |
| 2 | First-phase scope | **Read + write together** | Plan delivers a CfRadial *writer* (via Radx) alongside the reader, hooked at the editor save seam. |
| 3 | Gate data representation | **Keep scaled int16 internally now; native float (`DD_32_BIT_FP`) deferred to a later phase** | Reader re-quantizes Radx floats to int16 carrying the file's own `scale_factor`/`add_offset`/`_FillValue`. Render path and the ~50 editor sites are **unchanged**. Native float becomes a clean optional future phase. |
| 4 | DORADE handling | **Route DORADE through Radx too** | Retire the in-tree `dd_absorb_*` byte parser (source of `swp.*` segfaults). Radx reads DORADE robustly. |

**Net architecture:** every supported file → `libRadx` → `RadxVol` → C shim → soloiv's `DGI/DDS` structs holding **scaled int16** fields (same representation as today's DORADE path) → render/edit/write. Both DORADE and CfRadial converge on the existing int16 pipeline; the render and editor code is reused untouched.

### 1.1 Why int16-internal is essentially lossless (rationale for decision 3)
The arthur2015 CfRadial files store every moment field as `short` with `scale_factor = 0.01`, `add_offset = 0`, `_FillValue = -32768` — **bit-for-bit the same representation soloiv already uses** (`parameter_scale = 100`, `bias = 0`, `bad_data = -32768`). The floats shown in the examine widget are unpacked from that int16 by `dd_givfld`; there is no finer truth underneath. Re-quantizing Radx's float back to int16 on the file's own grid is therefore **zero-loss** for packed-short CfRadial (what RadxConvert writes), and read→edit→write is a bit-exact round-trip.

Quantization step = `scale_factor`; worst-case error = ±½ step. For these files that is ±0.005 dBZ / ±0.005 m/s / ±0.005 dB — 6–100× finer than the radar's own measurement resolution (NEXRAD ~0.5 dBZ, ~0.5 m/s, ~0.06 dB). Invisible on display, below the analysis noise floor. Dynamic range at scale 0.01 is ±327 units, so no clipping.

The genuine motivations for native float (the deferred phase) are **correctness/cleanliness, not display precision**:
- **Float-on-disk fields.** Some CfRadial files store fields as real `float` (derived products, other software's output) with no `scale_factor`. Quantizing those to int16 needs a chosen scale — the only new field-handling logic in this plan, specified in §4.2.1 (data-driven scale by default; optional name table). A naive fixed `scale=100` would make RHOHV (0–1.05) visibly coarse near 1.0, so it gets a finer scale automatically. arthur2015 has no float fields, so this path isn't exercised by the samples — but many real datasets have float RHOHV/KDP.
- Clean NaN missing-value semantics instead of a reserved int sentinel.
- Headroom for edit chains (add/multiply) that could exceed 16-bit dynamic range.

None of these accumulate across edit-save cycles. So int16 is the pragmatic working representation; float is a quality refactor for later.

---

## 2. Resources

- **Library:** `/Users/mmbell/lrose/lib/libRadx.a` — arm64, ~17 MB, native (non-fat). Headers in `/Users/mmbell/lrose/include/Radx/`.
- **Radx source (reference):** `/Users/mmbell/Development/lrose-core-20260425.src/codebase/libs/Radx/` — esp. `src/Cf2/Cf2RadxFile_read.cc`, `Cf2RadxFile_write.cc`.
- **CfRadial reference impl (Julia):** `/Users/mmbell/Development/Daisho.jl/src/cfradial.jl`, `src/cfradial_reader.jl`. Useful as the data-model and compliance oracle if the custom-C fallback is ever built.
- **Spec & compliance notes:** `~/Development/Daisho.jl/docs/CfRadialDoc-v2.1-20190901.pdf` and `docs/LROSE_RadxConvert_CfRadial2.1_compliance_notes.md`.
- **Sample data:** `test_data/arthur2015/sweeps/swp.*` (DORADE) and `test_data/arthur2015/cfradial/cfrad2.*.nc` (CfRadial-2). The two sets are the **same 14 KLTX NEXRAD sweeps** in both formats — a built-in round-trip/parity oracle. The CfRadial files are 1-sweep-per-file (`sweep=1`), LROSE-written v2 (confirmed quirks: `status_xml`, ISO-8601 string `time_coverage_*`, `version="2.0"`).

---

## 3. Current architecture (what we are changing)

### 3.1 Data model — the contract a reader must fill
All consumers (rendering, widgets, editor) read only from `dgi->dds->...`. They are format-agnostic. Key structs (in `include/`):

- `struct dd_general_info` (DGI) — `include/dd_general_info.h:163-257`. Per-window container; `num_parms`, `source_num_rays`, `new_sweep`, `new_vol`, `time`, `radar_name`, `sweep_file_name`, pointer `dds`. Spare `void*` slots `gpptr1..7` (`:245-251`) and a `source_fmt` field (`:207`) already exist — use them for the open Radx handle and format tag.
- `struct dds_structs` (DDS) — `include/dd_general_info.h:112-159`. The payload:
  - `struct cell_d *celv` — range gate geometry (`include/CellVector.h:40-49`): `number_cells`, `dist_cells[MAXCVGATES]` (per-gate range, m; `MAXCVGATES=8192`).
  - `struct parameter_d *parm[MAX_PARMS]` — per-field metadata (`include/Parameter.h:65-128`): `parameter_name[8]`, `param_units[8]`, **`binary_format`** (short), **`parameter_scale`**, **`parameter_bias`**, **`bad_data`**, `offset_to_data`, `number_cells`, `meters_to_first_cell`, `meters_between_cells`.
  - `char *qdat_ptrs[MAX_PARMS]` — raw per-field gate buffer (`:154`). Scaled `short` (16-bit). **Stays `short`** under decision 3; a CfRadial reader fills it exactly as the DORADE path does.
  - `struct ray_i *ryib` — per-ray (`include/Ray.h:52-69`): `azimuth`, `elevation`, time fields, `sweep_num`.
  - `struct sweepinfo_d *swib` — sweep summary (`include/Sweep.h:8-21`): `num_rays`, `fixed_angle`, `start_angle`, `stop_angle`.
  - `struct radar_d *radd` — radar/scan (`include/RadarDesc.h`): `scan_mode`, `radar_type`, lat/lon/alt, `num_parameter_des`, `eff_unamb_vel`, `eff_unamb_range`.
  - `struct platform_i *asib` — per-ray georef (airborne; `include/Platform.h`).
  - `struct volume_d *vold`, `struct super_SWIB *sswb` — volume/super-sweep headers.
- Iteration cursor `struct unique_sweepfile_info_v3` (USI) — `include/input_sweepfiles.h:7-31`: `num_rays`, `ray_num`, `num_swps`, `swp_count`, `directory`, `filename`, `final_swp_list`.

### 3.2 Format-specific seams (DORADE today)
1. **Discovery / navigation** — `translate/dd_files.c`: `ddir_files_v3()` (~`:587-730`) `readdir()` loop, filename filter `strncmp(d_name,"swp.",4)` (`:658`), and `craack_ddfn()` (`:173`) which parses `swp.YYMMDDHHMMSS.RADAR.Vn.angle_scanmode` into `struct dd_file_name_v3` **without opening the file**. Sweep-list/time navigation is entirely filename-driven. `mddir_*` wrappers (`:828-928`).
2. **Header parse** — `translate/swp_file_acc.c`: `dd_absorb_header_info(dgi)` (`:84-405`) walks 4-byte DORADE block IDs into `dds`. Called by `ddswp_new_sweep_v3(usi)` (`:1140-1195`) after `open()`.
3. **Ray parse** — `dd_absorb_ray_info(dgi)` (`:407-720`) fills `ryib`/`asib`/`qdat_ptrs`. Called by `ddswp_next_ray_v3(usi)` (`:1251-1295`).
4. **Write** — `translate/dd_swp_files.c`: `dd_dump_ray(dgi)` (`:63-217`) assembles a DORADE ray from `qdat_ptrs` into `dd_buf`; `dd_flush()` (`:220-341`) finalizes (SEDS table, header rewrite, `.tmp`→final rename); `dd_insert_headers()` (`:344`). Driven from editor `se_process_data()` (`editor/se_proc_data.c:387-396`) and interactive `sxm_examine.c:329`.

### 3.3 Render path (gate → color)
- `solo_color_cells()` — `perusal/sp_dorade.c:99-179`. The **only** render site that reads raw gates. Switches on `binary_format`, casts `qdat_ptrs[]` to `unsigned char*`/`short*`, indexes `wwptr->data_color_lut` directly by the scaled-int gate value (`:142-178`). Batch-threshold sub-branch (`:123-131,150-159`) assumes int threshold.
- `data_color_lut` — built by `solo_data_color_lut()` (`perusal/solo2.c:2482-2539`): a **64K-entry** table of pixel values, index domain `[-32768,32767]` (= range of `short`), constructed by `DD_SCALE`-ing the palette's physical value bounds into integer indices. Allocated centered in `perusal/sp_basics.c:743-751`.
- `solo_cell_lut()` (`sp_dorade.c:33-96`) maps raster cell → source gate index; **value-independent, no change needed**.
- Downstream (`sii_bscan.c:425`, `sii_xyraster.c:292`) consumes resolved pixels in `wwptr->cell_colors`, not raw gates — **no change needed**.
- **Float groundwork already present:** `DD_32_BIT_FP=4` is defined (`include/dd_defines.h:203-207`); `dd_datum_size()` returns 4 for it (`translate/ddb_common.c:531-538`); `dd_givfld()` already has a working float branch (`translate/ddb_common.c:1097`) and the examine/cursor readout already goes through it. So per-gate sizing and the value readout are already float-capable; only `solo_color_cells` is not.

### 3.4 Editor (int16, in place)
~50 sites across `editor/se_*.c` cast `qdat_ptrs[fn]` to `short*`, compare/write an **integer `bad_data` sentinel** with `==`, and hoist the user threshold to int once via `DD_SCALE`. Examples: threshold `se_for_each.c:609-650`, despeckle `se_defrkl.c:854-920`, ring-zap `se_defrkl.c:104`, unfold `se_for_each.c:~459`, plus `se_add_mult.c`, `se_BB_unfold.c`, `se_flag_ops.c`, `se_histog.c`, examine apply `sxm_examine.c:257-321`. Edits are in place on the reader's buffers; the writer re-reads the same buffers. **Because the CfRadial reader fills these same int16 buffers, this entire surface is untouched** — converting it to float is the deferred Phase 7 only.

### 3.5 Build
- Top-level `CMakeLists.txt`: C11, 64-bit only; already does `find_package(netCDF QUIET)` / `pkg_check_modules(NETCDF netcdf)` and defines `NETCDF`. Subdirs: `translate`→`editor`→`perusal`.
- `translate/` → static lib `s2` (DORADE crackers + existing netCDF translators `nc_dd.c`/`dorade_ncdf.c`/`dd_ncdf.c`). `editor/` → `soloedit`. `perusal/` → `soloiv_core` + exe `soloiv`. Link chain: `soloiv` → `soloiv_core` → `soloedit` + `s2` + GTK4 + m.

---

## 4. Target design

### 4.1 The `radar_io` backend abstraction (decision 1)
Introduce a small format-agnostic dispatch layer so the GUI/editor never call DORADE- or Radx-specific functions directly. New files in `translate/` (the `s2` lib), e.g. `radar_io.h` / `radar_io.c`:

```c
/* radar_io.h — backend-neutral sweep I/O */
typedef enum { RIO_FMT_UNKNOWN, RIO_FMT_DORADE, RIO_FMT_CFRADIAL } rio_fmt_t;

/* Discovery: enumerate sweep files in a dir, fill dd_file_name_v3 list. */
int  rio_scan_dir(const char *dir, struct dd_file_name_v3 **out, int *n);
rio_fmt_t rio_sniff(const char *path);            /* magic/extension probe */

/* Open a file (a whole CfRadial volume, or one DORADE sweep) and fill DGI
 * headers for sweep `swp_index`. Returns #rays, or <0 on error. */
int  rio_open_sweep(struct dd_general_info *dgi, const char *path, int swp_index);
int  rio_read_ray  (struct dd_general_info *dgi, int ray_index);  /* fill ryib/asib/qdat_ptrs(int16) */
void rio_close     (struct dd_general_info *dgi);

/* Write: assemble a RadxVol from edited DGI/DDS and write it out. */
int  rio_write_sweep_begin(struct dd_general_info *dgi, const char *out_path, rio_fmt_t fmt);
int  rio_write_ray        (struct dd_general_info *dgi);
int  rio_write_sweep_end  (struct dd_general_info *dgi);
```

Backends:
- **Radx backend** (primary): `translate/radx_shim.cc` (compiled C++17/libc++) implements all of the above by calling `RadxFile`/`RadxVol`/`RadxSweep`/`RadxRay`/`RadxField`. Exposed `extern "C"`.
- **Custom-C fallback** (structured, not implemented now): `translate/cfradial_c.c` could implement the read side with netCDF-C. The abstraction guarantees it can slot in without touching callers. A build flag (`SOLOIV_IO_BACKEND=radx|custom`) selects.

`dgi->source_fmt` (`dd_general_info.h:207`) records the backend; the open Radx `RadxVol*`/file handle lives in a `gpptr` slot so a CfRadial volume stays resident across `rio_read_ray` calls.

### 4.2 Radx shim — read (decision 1, 4)
One C++ file, `extern "C"`. Read sequence (per the verified API):

```cpp
RadxFile f;                       // base class auto-detects DORADE/CfRadial-1/-2
f.readFromPath(path, vol);        // RadxVol vol; returns 0 ok else getErrStr()
vol.loadSweepInfoFromRays();
// per sweep: RadxSweep* sw = vol.getSweeps()[i];  fixed angle, mode, ray range
// per ray:   RadxRay* r = vol.getRays()[k];  az/el/time/nGates/startRange/gateSpacing
// per field: RadxField* fld = r->getField(name); fld->convertToFl32();
//            const Radx::fl32* d = fld->getDataFl32(); missing = fld->getMissingFl32();
```

Mapping into DGI/DDS (on **scaled int16** buffers — same as today's DORADE path):
- `RadxVol`/`RadxSweep` → `radd` (scan_mode from `Radx::SweepMode_t`, lat/lon/alt), `swib` (`fixed_angle`, `num_rays`), `vold`, `sswb`.
- `RadxRay` → `ryib` (az/el/time), `asib` if georef present; range geom (`getStartRangeKm`/`getGateSpacingKm` or per-gate) → `celv->dist_cells[]`.
- `RadxField` → `parm[pn]` with `binary_format=DD_16_BITS`, `bad_data = -32768` (reserved int16 fill), `qdat_ptrs[pn] = malloc(nGates*2)` of `short`. How `parameter_scale`/`parameter_bias` are chosen depends on the field's **native on-disk type**, which Radx exposes per field via `RadxField::getDataType()` (`Radx::SI08/SI16/SI32/FL32`), `getScale()`, `getOffset()`. See §4.2.1 for the exact selection. Radx's per-field `getMissingFl32()` maps to `bad_data`; **per-field `_FillValue`/`_Undetect` quirks are resolved inside Radx**, so we never parse them ourselves.

#### 4.2.1 Field scale/bias selection (the only field-quantization logic to implement)
Implement one helper in the read backend, e.g. `static void rio_pick_scale(RadxField *fld, double *scale, double *bias)`:

1. **Packed field (native `SI08`/`SI16`/`SI32`):** carry the file's own packing straight through — `parameter_scale = 1.0/fld->getScale()`, `parameter_bias = -fld->getOffset()*parameter_scale`. Then re-pack via `DD_SCALE` (or copy the raw shorts directly when native type is `SI16`). **Lossless** — identical to the on-disk grid. This covers the arthur2015 samples and any packed RHOHV. *No table involved.*
2. **Float field (native `FL32`, i.e. no `scale_factor` on disk):** *this is the only new logic.* Choose a scale so the int16 range isn't clipped and precision is ~16 bits:
   - **Default — data-driven (recommended, no per-field knowledge):** scan finite (non-missing) gates of the field for `dmin`/`dmax`; set `scale = round_nice(64000.0/(dmax-dmin))`, `bias = -dmin*scale`. Fills the int16 range automatically → e.g. RHOHV(0–1.05) gets a ~0.00002 step, PHIDP a ~0.006° step. Robust for any field; no maintenance.
   - **Optional override** (only to pin a fixed canonical grid for known fields, e.g. for stable cross-file comparison): a tiny static table keyed by `fld->getName()`/units, e.g. `RHOHV → step 0.001 (scale 1000)`, `PHIDP → step 0.01 (scale 100)`, `KDP → step 0.01`, default `step 0.01 (scale 100)`. If present, it overrides the data-driven value. Start with the data-driven path; add the table only if deterministic grids are wanted.

   `round_nice(x)` snaps to the nearest 1·10ⁿ/2·10ⁿ/5·10ⁿ so scales/steps stay human-readable in the examine widget.

This helper is the entire footprint of "high-resolution field handling": packed fields reuse existing scale/bias (free); float fields hit `rio_pick_scale`. Nothing in the render or editor path changes.
- **Robustness:** always call `convertToFl32()` before `getDataFl32()` (it `assert`s otherwise — a process-abort hazard). Wrap the shim body in try/catch; never let a C++ exception cross into C.

Because Radx loads the **whole volume**, `rio_open_sweep` reads once and caches `RadxVol*`; `rio_read_ray` indexes cached rays. For 1-sweep-per-file CfRadial (our samples) and per-file DORADE, this maps 1:1 onto the existing "open file → iterate rays" flow.

### 4.3 Radx shim — write (decision 2)
Build a `RadxVol` from the edited DGI/DDS and call `RadxFile::writeToDir`/`writeToPath` (or `Cf2RadxFile`). `rio_write_sweep_begin` allocates a `RadxVol`; `rio_write_ray` appends a `RadxRay` and adds each field from the int16 `qdat_ptrs` — either hand Radx a scaled `si16` field with the same `scale_factor`/`add_offset` (lossless, preserves the on-disk grid for packed fields) or `DD_UNSCALE` to fl32 and let Radx pack. `rio_write_sweep_end` sets file format (CfRadial-2 default; DORADE selectable) and writes. This guarantees soloiv-written CfRadial is byte-compatible with LROSE tools (same library). The writer hooks the existing editor save seam (`se_proc_data.c:387-396`, replacing the `dd_dump_ray`/`dd_flush` calls with `rio_write_*`).

### 4.4 Navigation for CfRadial filenames (decision 4 seam 1)
`rio_scan_dir` replaces the `swp.`-only filter:
- Accept `swp.*` (DORADE), `cfrad.*`/`cfrad2.*` and `*.nc` (CfRadial).
- For CfRadial, parse the `cfrad[2].YYYYMMDD_HHMMSS.mmm_..._SCAN.nc` convention into `dd_file_name_v3` (radar, time, fixed angle, scan mode). When the filename is insufficient, open via Radx metadata-only (`setReadMetadataOnly(true)`) to read `sweep_fixed_angle`/`sweep_mode`/time. Cache to avoid reopening.
- **Multi-sweep CfRadial volumes:** one file may contain N sweeps. Represent each as a virtual `dd_file_name_v3` entry (same path, different `swp_index`) so the existing sweep-list/time navigation works unchanged. (Our samples are 1 sweep/file, so this can be a later refinement — but design the list entry to carry `swp_index` from the start.)

### 4.5 Render path — unchanged (int16)
Because the internal representation stays scaled int16, `solo_color_cells()` (`sp_dorade.c`) and the 64K int LUT in `solo_data_color_lut()` (`solo2.c`) are used **as-is**. The CfRadial reader just sets `binary_format=DD_16_BITS` and the field's `parameter_scale`/`bias`/`bad_data`, exactly like the DORADE path. No render-path code changes.

### 4.6 Editor — unchanged (int16)
The ~50 int16 edit sites across `editor/se_*.c` + `sxm_examine.c` are **untouched**. They operate on the same scaled-`short` `qdat_ptrs` buffers regardless of whether the source was DORADE or CfRadial. Editing and the existing DORADE-style save both keep working with no conversion.

### 4.7 Deferred: native float pipeline (future phase — see Phase 7)
A later, separable quality refactor (not required for read/write parity). It would: add a `DD_32_BIT_FP` branch to `solo_color_cells` with a value→color helper paralleling `solo_data_color_lut`; introduce typed gate accessors (`rg_get`/`rg_set`/`rg_is_bad`) and a `SOLO_BAD_FLOAT` (NaN-aware) convention; and convert the ~50 editor sites to the float domain (dropping the `DD_SCALE`-to-int threshold hoisting — a net simplification). Motivation is fidelity for genuinely float-on-disk fields (esp. RHOHV) and cleaner missing-value semantics, **not** display precision. Until then, float-source CfRadial fields are quantized to int16 on read per §4.2.

---

## 5. Build system changes

1. **Find Radx + deps** in top-level `CMakeLists.txt`: locate `/Users/mmbell/lrose/{include,lib}`, `libRadx.a`, `libNcxx.a`; Homebrew `hdf5` (incl. `hdf5_cpp`), `netcdf`, `udunits`. Add option `SOLOIV_IO_BACKEND` (default `radx`).
2. **Enable C++** in the shim's target: `enable_language(CXX)`, `CMAKE_CXX_STANDARD 17`, libc++/Apple clang (ABI must match the prebuilt lib). Compile `translate/radx_shim.cc` into `s2` (or a small sibling lib `s2_io`).
3. **Link line** (static archives, order matters: Radx → Ncxx → netcdf/hdf5/udunits):
   ```
   -L/Users/mmbell/lrose/lib -lRadx -lNcxx \
   -L$(brew --prefix hdf5)/lib -lhdf5_cpp -lhdf5 \
   -L$(brew --prefix netcdf)/lib -lnetcdf \
   -L$(brew --prefix udunits)/lib -ludunits2 \
   -lz -lbz2 -lcurl -lm -lc++
   ```
   Link the final `soloiv` executable so the C++ runtime is pulled in (either add `-lc++` or set the linker language to CXX).
4. Keep the netCDF-C dependency available for the eventual custom-C fallback backend.
5. Binary grows by tens of MB (Radx pulls in HDF5/netCDF). Acceptable per decision 1.

---

## 6. Implementation phases

Each phase ends green: full build + test suite (`-DSOLOIV_BUILD_TESTS=ON`) passing, reporting pass/fail counts. Run perusal as `../build/perusal/soloiv` from inside the relevant `test_data` dir (per project run convention).

### Phase 0 — Build & link the Radx shim skeleton
- Add Radx/HDF5/netCDF/udunits discovery + `SOLOIV_IO_BACKEND` to CMake; enable CXX for the shim target.
- Add `translate/radx_shim.cc` with a trivial `extern "C"` smoke function (`rio_radx_selftest()` that opens a sample file, reads `getNSweeps`, returns it) and a `tests/` unit that links and calls it.
- **Exit:** project links against `libRadx.a`; smoke test prints sweep count for a `cfrad2.*.nc` and a `swp.*` sample.

### Phase 1 — CfRadial *read* → scaled-int16 DGI/DDS (reuse existing render)
- Implement `radar_io` interface and the Radx read backend (`rio_sniff`, `rio_open_sweep`, `rio_read_ray`, `rio_close`) producing **scaled int16** `qdat_ptrs` and `DD_16_BITS` parm metadata, carrying the field's `scale_factor`/`add_offset`/`_FillValue` per §4.2.
- Implement `rio_scan_dir` for CfRadial filenames (1 sweep/file) and wire discovery so a `cfradial/` dir lists sweeps.
- **No render or editor changes** — the existing `DD_16_BITS` path handles it.
- Branch the GUI load path (`perusal/sii_perusal.c:solo_nab_next_file` ~`:1000-1182`) and editor read entry (`editor/se_proc_data.c:217`, `sxm_examine.c:228,927`) to call `rio_*` instead of `dd_absorb_*` when the dir/file is CfRadial.
- **Exit:** the 14 `cfrad2.*.nc` samples load and render PPIs in perusal; examine/cursor readout shows correct physical values (existing `dd_givfld`). **Parity check:** each CfRadial PPI visually matches its `swp.*` twin.

### Phase 2 — Route DORADE *read* through Radx
- Implement `rio_open_sweep`/`rio_read_ray` for DORADE via the same Radx backend (Radx `DoradeRadxFile`). Map to the same int16 pipeline (per §4.2/§4.2.1).
- Switch the `swp.*` discovery + load path onto `rio_*`; keep the old `dd_absorb_*` code compiled but unreferenced behind a fallback flag for one phase, then delete.
- **Exit:** all 14 `swp.*` samples load via Radx with no segfaults (validates the crash-fix motivation); PPIs match Phase-1 CfRadial output. Old parser removed or flag-gated off by default.

### Phase 3 — Read parity regression (int16)
- Add automated regression tests comparing rendered `cell_colors` and key metadata (fixed angle, az/el, range geom, field values via `dd_givfld`) between DORADE-via-Radx and CfRadial-via-Radx for the twinned arthur2015 samples.
- Audit residual `binary_format`/`qdat_ptrs` assumptions (colorbar legend, batch threshold) — expected no changes since representation is unchanged int16.
- **Exit:** automated parity test (DORADE twin vs CfRadial twin produce matching `cell_colors`) passes in the suite.

### Phase 4 — CfRadial (and DORADE) *write* via Radx
- Implement `rio_write_*` building a `RadxVol` from edited DGI/DDS; default output CfRadial-2, with DORADE selectable.
- Replace `dd_dump_ray`/`dd_flush` calls at the editor save seam (`se_proc_data.c:387-396`, `sxm_examine.c:329`) with `rio_write_*`.
- **Exit:** edit a CfRadial sweep, save, reopen in soloiv → edits present; `RadxConvert`/`ncdump` confirms a valid v2 file. Round-trip a `swp.*`→edit→save CfRadial→reopen.

### Phase 5 — Cleanup & docs
- Delete dead in-tree DORADE parser/writer if fully superseded (`dd_absorb_*`, `dd_dump_ray`/`dd_flush`) — or keep behind `SOLOIV_IO_BACKEND=custom` as documented fallback.
- Document the build prerequisites (LROSE lib path, Homebrew deps) in `README.md`/`ROADMAP.md`.
- Update `MEMORY.md` notes (translate/ now wraps Radx, not deprecated-and-gone).
- **Exit:** clean build, full suite green, docs updated. **Read+write CfRadial+DORADE is complete at this point.**

### Phase 7 — (Deferred, optional) Native float pipeline
Separable quality refactor, not required for read/write parity. See §4.7.
- Add the `DD_32_BIT_FP` branch to `solo_color_cells` + value→color helper paralleling `solo_data_color_lut`.
- Introduce `rg_get/rg_set/rg_is_bad` accessors + `SOLO_BAD_FLOAT` (NaN-aware); convert the ~50 int16 sites across `editor/se_*.c` + `sxm_examine.c` to the float domain, removing `DD_SCALE`-to-int hoisting. Gate unconverted ops to refuse float fields until done.
- Switch the reader to emit float (`DD_32_BIT_FP`) for float-on-disk fields instead of quantizing.
- **Motivation:** fidelity for genuinely float fields (esp. RHOHV), clean missing-value semantics, edit-chain headroom — **not** display precision.
- **Exit:** float-source fields render/edit/write without quantization; parity tests still green.

---

## 7. Key risks & mitigations

| Risk | Mitigation |
|------|------------|
| **Quantizing float-on-disk CfRadial fields to int16** could coarsen fields needing fine resolution (esp. RHOHV near 1.0). | Carry the file's own `scale_factor`/`add_offset` for packed fields (lossless); per-field scale table for float-source fields (RHOHV→1000 etc.) or derive from data range. Packed-short files (our samples) are bit-exact. Native float (Phase 7) removes the issue entirely if ever needed. |
| **Editor float conversion (~50 int16 sites)** — deferred to Phase 7, not on the read/write critical path. | int16-internal keeps the editor unchanged for now; convert later via typed accessors + `SOLO_BAD_FLOAT` only when native float is pursued. |
| **C++/libc++ ABI lock-in.** Shim + app must build with the same Apple clang/libc++ that built `libRadx.a`. | Build shim with system clang C++17/libc++; CI checks. Don't mix GCC/libstdc++. |
| **Static link order / missing deps.** No pkg-config for LROSE libs. | Hard-code the verified link line (§5.3); document Homebrew deps; Phase 0 proves linkage before any feature work. |
| **`getDataFl32()` asserts → process abort** on non-fl32 access. | Shim always calls `convertToFl32()` first; wrap shim in try/catch; never leak exceptions into C. |
| **Binary size grows tens of MB** (HDF5+netCDF pulled in even for CfRadial-1). | Accepted per decision 1; note in docs. |
| **CfRadial multi-sweep volumes** vs DORADE 1-sweep files. | `dd_file_name_v3` list entries carry `swp_index`; Radx volume cached across `rio_read_ray`. Samples are 1/file so this is incremental. |
| **Navigation assumes filename metadata.** CfRadial names differ. | `rio_scan_dir` parses `cfrad[2].*`; fall back to Radx metadata-only open for angle/mode/time. |
| **LROSE v2 quirks** (`status_xml`, string times, `version="2.0"`). | Handled inside Radx — we never parse netCDF directly in the primary path. (Relevant only if the custom-C fallback is built; Daisho notes are the oracle.) |
| **Custom-C fallback drift.** | Kept behind the `radar_io` interface and `SOLOIV_IO_BACKEND=custom`; not built now, but the seam is preserved. |

---

## 8. Validation strategy

- **Parity oracle:** the twinned `arthur2015` DORADE/CfRadial sets must render identically. Automated test compares `wwptr->cell_colors` (Phase 3) and metadata (fixed angle, az/el, range geom, field values via `dd_givfld`).
- **Crash regression:** all `swp.*` samples that currently segfault must load cleanly via Radx (Phase 2).
- **Round-trip:** read CfRadial → edit → write CfRadial → reopen; verify edits and `ncdump`/`RadxConvert` validity (Phase 5).
- Run the full suite after every phase; report pass/fail counts (per global Julia/scientific workflow instruction, applied here to the C suite).

---

## 9. First actions on approval
1. Phase 0: CMake Radx discovery + `radx_shim.cc` smoke test; confirm link + sweep-count readout on both sample formats. This de-risks the entire approach (linkage) before any feature code.
2. Proceed phase-by-phase, pausing for review at each "Exit" gate.
