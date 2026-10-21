#!/usr/bin/env bash
# Pack a Rack 2 patch into a .vcv file (zstd-compressed tar), the format Rack 2 saves.
#
# Usage: mkpatch.sh <patch.json | patch-dir> <out.vcv>
#   patch.json : a single patch JSON file (stored in the archive as ./patch.json)
#   patch-dir  : a directory containing patch.json and optionally modules/<id>/... data
#
# Note: Rack 2 also loads a plain (uncompressed) patch.json passed directly on the
# command line (anything not starting with the zstd magic is treated as a legacy
# JSON patch), so packing is only needed if you want a real .vcv file.
#
# Unpack: zstd -dc in.vcv | tar -xf - -C outdir
set -euo pipefail

if [ $# -ne 2 ]; then
	sed -n '2,13p' "$0" | sed 's/^# \{0,1\}//'
	exit 2
fi
in=$1
out=$2

stage=$(mktemp -d "${TMPDIR:-/tmp}/mkpatch.XXXXXX")
trap 'rm -rf "$stage"' EXIT

if [ -d "$in" ]; then
	[ -f "$in/patch.json" ] || { echo "mkpatch: $in has no patch.json" >&2; exit 1; }
	cp -a "$in/." "$stage/"
else
	[ -f "$in" ] || { echo "mkpatch: $in not found" >&2; exit 1; }
	cp "$in" "$stage/patch.json"
fi
jq empty "$stage/patch.json" || { echo "mkpatch: patch.json is not valid JSON" >&2; exit 1; }

# Same layout as Rack's own archives: entries "./", "./patch.json", "./modules/..."
tar --format=pax --owner=0 --group=0 --numeric-owner -C "$stage" -cf - . \
	| zstd -q -19 -f -o "$out"
