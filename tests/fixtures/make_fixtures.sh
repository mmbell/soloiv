#!/usr/bin/env bash
#
# make_fixtures.sh — regenerate the committed, range-clipped test fixtures.
#
# The committed fixtures under tests/fixtures/{ground,airborne,ship}/ are small
# (range-clipped) copies of real sweeps so the test suite runs anywhere — on a
# clean checkout and on GitHub CI — without the multi-hundred-MB local
# test_data/ tree. This script documents their provenance and regenerates them
# from that local tree; commit the OUTPUTS, not the source data.
#
# Range is clipped (gates dropped) but ALL rays are kept, so azimuth coverage
# is preserved — essential for the SEAPOL surveillance-PPI regression test.
#
# Requires LROSE RadxConvert. Override its location with RADX_CONVERT=...
# Source data root defaults to ../../test_data (override with SRC=...).

set -euo pipefail

RADX_CONVERT="${RADX_CONVERT:-/Users/mmbell/lrose/bin/RadxConvert}"
HERE="$(cd "$(dirname "$0")" && pwd)"
SRC="${SRC:-$HERE/../../test_data}"

command -v "$RADX_CONVERT" >/dev/null 2>&1 || { echo "RadxConvert not found: $RADX_CONVERT" >&2; exit 1; }
[ -d "$SRC" ] || { echo "source data not found: $SRC" >&2; exit 1; }

# convert <fmt-flag> <max_range_km> <src> <dest-dir> <canonical-name>
# Runs RadxConvert into a temp dir, then moves the single output to
# <dest-dir>/<canonical-name>.
convert() {
  local fmt="$1" maxr="$2" src="$3" destdir="$4" name="$5"
  local tmp; tmp="$(mktemp -d)"
  local extra=""
  [ "$fmt" = "-dorade" ] && extra="-disag"
  "$RADX_CONVERT" $fmt $extra -max_range "$maxr" -f "$src" -outdir "$tmp" >/dev/null 2>&1
  local out; out="$(find "$tmp" -type f | head -1)"
  [ -n "$out" ] || { echo "  FAILED to convert $src" >&2; rm -rf "$tmp"; return 1; }
  mkdir -p "$destdir"
  cp "$out" "$destdir/$name"
  rm -rf "$tmp"
  printf "  %-7s %-8s %s\n" "$(du -h "$destdir/$name" | cut -f1)" "$fmt" "$name"
}

echo "Regenerating fixtures into $HERE ..."

# Each radar has per-format subdirs (dorade/cf1/cf2) so a test's cwd is a
# clean single-format directory for the startup directory scan.

# --- ground: KLTX NEXRAD, primary_axis=axis_z (surveillance PPI) ---
echo "ground/ (KLTX)"
convert -dorade 30 \
  "$SRC/arthur2015/sweeps/swp.1140703215739.KLTX.317.0.9_SUR_v775" \
  "$HERE/ground/dorade" "swp.1140703215739.KLTX.317.0.9_SUR_v775"
convert -cf2 30 \
  "$SRC/arthur2015/cfradial/cfrad2.20140703_215739.317_to_20140703_215757.406_KLTX_SUR.nc" \
  "$HERE/ground/cf2" "cfrad2.20140703_215739.317_to_20140703_215757.406_KLTX_SUR.nc"
# cf1 has no native source; derive it from the cf2 sweep.
convert -cfradial 30 \
  "$SRC/arthur2015/cfradial/cfrad2.20140703_215739.317_to_20140703_215757.406_KLTX_SUR.nc" \
  "$HERE/ground/cf1" "cfrad.20140703_215739.317_to_20140703_215757.406_KLTX_SUR.nc"

# --- airborne: N42RF-TM P3 tail, primary_axis=axis_y_prime (georef/AIR) ---
echo "airborne/ (N42RF-TM)"
# All three airborne formats are range-clipped via Radx. The DORADE fixture is
# derived from the genuine DORADE source (not the cf2) so the DORADE-vs-cf2
# parity test is not circular. (Radx-written DORADE is not readable by the
# in-tree byte parser, so the airborne test reads every format through Radx.)
convert -dorade 15 \
  "$SRC/fiona2022/dorade/swp.1220907125318.N42RF-TM.857.20.0_AIR_v2" \
  "$HERE/airborne/dorade" "swp.1220907125318.N42RF-TM.857.20.0_AIR_v2"
convert -cfradial 15 \
  "$SRC/fiona2022/cf1/cfrad.20220907_125318.857_to_20220907_125322.837_N42RF-TM_AIR.nc" \
  "$HERE/airborne/cf1" "cfrad.20220907_125318.857_to_20220907_125322.837_N42RF-TM_AIR.nc"
convert -cf2 15 \
  "$SRC/fiona2022/cf2/cfrad2.20220907_125318.857_to_20220907_125322.837_N42RF-TM_AIR.nc" \
  "$HERE/airborne/cf2" "cfrad2.20220907_125318.857_to_20220907_125322.837_N42RF-TM_AIR.nc"

# --- ship: SEAPOL, platform mislabeled aircraft but primary_axis=axis_z ---
echo "ship/ (SEAPOL)"
convert -cfradial 20 \
  "$SRC/piccolo/cf1/cfrad.20240903_120008.372_to_20240903_120037.547_SEAPOL_SUR.nc" \
  "$HERE/ship/cf1" "cfrad.20240903_120008.372_to_20240903_120037.547_SEAPOL_SUR.nc"
convert -cf2 20 \
  "$SRC/piccolo/cf2/cfrad2.20240903_120008.372_to_20240903_120037.547_SEAPOL_PICCOLO_LONG_SUR.nc" \
  "$HERE/ship/cf2" "cfrad2.20240903_120008.372_to_20240903_120037.547_SEAPOL_PICCOLO_LONG_SUR.nc"
# SEAPOL is native CfRadial; derive a DORADE sweep from the cf2 for coverage.
convert -dorade 20 \
  "$SRC/piccolo/cf2/cfrad2.20240903_120008.372_to_20240903_120037.547_SEAPOL_PICCOLO_LONG_SUR.nc" \
  "$HERE/ship/dorade" "swp.1240903120008.SEAPOL.372.0.5_SUR_v7344"

echo "Done."
