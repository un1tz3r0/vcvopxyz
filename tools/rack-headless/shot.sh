#!/usr/bin/env bash
# Screenshot a VCV Rack 2 patch headlessly (Xvfb + Mesa software GL).
#
# Usage: shot.sh <plugin-dir[:plugin-dir...] | -> <patch.vcv|patch.json> <out.png> [seconds=4] [WxH=1600x1000]
#
#   plugin-dir  Folder containing plugin.json + plugin.so (+ res/). Several may be given
#               separated by ':'. Use '-' for none (Core modules only). If the patch uses
#               "Fundamental" modules and none was given, ../Fundamental next to this
#               script is added automatically.
#   patch       .vcv (zstd tar) or plain patch.json. Not modified.
#   seconds     How long the engine runs after the window appears, before the capture.
#   WxH         Screen and window size.
#
# Environment:
#   ZOOM=fit      (default) press F4 "zoom to fit modules" before the capture
#   ZOOM=patch    keep the patch's own "zoom"/"gridOffset"
#   ZOOM=<float>  e.g. ZOOM=1.5: that zoom, scrolled to the top-left module
#   FULLSCREEN=1  also press F11 (hides the menu bar)
#   DARK=1        prefer dark panels (settings "preferDarkPanels")
#   CABLES=0.5    cable opacity (0 hides them)
#   ACTIONS=f    source script f after zooming (xdotool clicks; "snap name.png" captures)
#   KEEP=1        keep the temporary work dir (user folder, log.txt) and print its path
#   THREADS=n     engine threads (default 1), to test modules across cores
#   PLUGIN_SETTINGS='{"vcvopxyz": {...}}'  Rack's per-plugin settings (pluginSettings in settings.json)
#   RACK_DIR=...  Rack build to use (default: ./Rack next to this script)
set -euo pipefail

HERE=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
RACK_DIR=${RACK_DIR:-$HERE/Rack}

usage() { sed -n '2,26p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; exit 2; }

# ---------------------------------------------------------------------------
# Inner part: runs inside xvfb-run (DISPLAY is set).
# ---------------------------------------------------------------------------
if [ "${1:-}" = "--inner" ]; then
	WORK=$2 PATCH=$3 OUT=$4 SECS=$5 W=$6 H=$7
	U=$WORK/user
	export LIBGL_ALWAYS_SOFTWARE=1 JACK_NO_START_SERVER=1
	# Rack shows dialogs through zenity; a stub makes every dialog return "cancel"
	# immediately and logs its text instead of blocking.
	export PATH="$WORK/bin:$PATH"

	cd "$RACK_DIR"
	./Rack -s "$RACK_DIR" -u "$U" "$PATCH" >"$WORK/stdout.txt" 2>&1 &
	pid=$!

	# Wait for the Rack window (up to 30 s)
	wid=
	for _ in $(seq 1 300); do
		if ! kill -0 "$pid" 2>/dev/null; then
			echo "shot.sh: Rack exited before its window appeared" >&2
			exit 1
		fi
		wid=$(xdotool search --onlyvisible --name '^VCV Rack' 2>/dev/null | head -n1 || true)
		[ -n "$wid" ] && break
		sleep 0.1
	done
	if [ -z "$wid" ]; then
		echo "shot.sh: timed out waiting for the Rack window" >&2
		kill -KILL "$pid" 2>/dev/null || true
		exit 1
	fi

	# Park the pointer on the empty right end of the menu bar (no hover effects there)
	# and give the window keyboard focus.
	xdotool mousemove $((W - 3)) 3
	xdotool windowfocus "$wid" 2>/dev/null || true
	sleep 0.5
	if [ "${FULLSCREEN:-0}" = 1 ]; then
		xdotool key F11
		sleep 0.5
		xdotool mousemove $((W - 3)) $((H - 3))
	fi
	if [ "${ZOOM:-fit}" = fit ]; then
		xdotool key F4
	fi

	# ACTIONS=<script> runs here, after the zoom, with xdotool and snap <name.png> (saved next to out.png).
	snap() { import -window root "$(dirname "$OUT")/$1"; }
	if [ -n "${ACTIONS:-}" ]; then sleep 1; . "$ACTIONS"; fi

	sleep "$SECS"
	import -window root "$OUT"

	# Quit: Ctrl+Q (clean shutdown), then TERM, then KILL.
	xdotool key ctrl+q 2>/dev/null || true
	for _ in $(seq 1 50); do kill -0 "$pid" 2>/dev/null || break; sleep 0.1; done
	if kill -0 "$pid" 2>/dev/null; then
		kill -TERM "$pid" 2>/dev/null || true
		sleep 1
		kill -KILL "$pid" 2>/dev/null || true
	fi
	wait "$pid" 2>/dev/null || true
	exit 0
fi

# ---------------------------------------------------------------------------
# Outer part
# ---------------------------------------------------------------------------
[ $# -ge 3 ] && [ $# -le 5 ] || usage
PLUGINS=$1 PATCH_IN=$2 OUT=$3 SECS=${4:-4} SIZE=${5:-1600x1000}
[[ $SIZE =~ ^([0-9]+)x([0-9]+)$ ]] || { echo "shot.sh: bad size '$SIZE' (want WxH)" >&2; exit 2; }
W=${BASH_REMATCH[1]} H=${BASH_REMATCH[2]}
[ -f "$PATCH_IN" ] || { echo "shot.sh: patch '$PATCH_IN' not found" >&2; exit 2; }
[ -x "$RACK_DIR/Rack" ] || { echo "shot.sh: no Rack binary in $RACK_DIR" >&2; exit 2; }
OUT=$(realpath -m "$OUT")
mkdir -p "$(dirname "$OUT")"

t0=$(date +%s.%N)
WORK=$(mktemp -d "${TMPDIR:-/tmp}/rackshot.XXXXXX")
cleanup() {
	if [ "${KEEP:-0}" = 1 ]; then echo "shot.sh: work dir kept: $WORK" >&2; else rm -rf "$WORK"; fi
}
trap cleanup EXIT
U=$WORK/user
PLUGDIR=$U/plugins-lin-x64
mkdir -p "$PLUGDIR" "$WORK/patch" "$WORK/bin"

cat >"$WORK/bin/zenity" <<'EOF'
#!/bin/sh
echo "shot.sh: [Rack dialog] $*" >&2
case "$*" in *--file-selection*) [ -n "$ZENITY_ANSWER" ] && { echo "$ZENITY_ANSWER"; exit 0; } ;; esac
exit 1
EOF
chmod +x "$WORK/bin/zenity"

# --- Patch: unpack (if .vcv) into $WORK/patch, adjust view, repack --------
if [ "$(head -c4 "$PATCH_IN" | od -An -tx1 | tr -d ' \n')" = "28b52ffd" ]; then
	zstd -q -dc "$PATCH_IN" | tar -xf - -C "$WORK/patch"
else
	cp "$PATCH_IN" "$WORK/patch/patch.json"
fi
PJ=$WORK/patch/patch.json
[ -f "$PJ" ] || { echo "shot.sh: no patch.json in $PATCH_IN" >&2; exit 1; }
jq empty "$PJ" || { echo "shot.sh: patch.json is not valid JSON" >&2; exit 1; }

case "${ZOOM:-fit}" in
	fit | patch) ;;
	*)
		[[ ${ZOOM} =~ ^[0-9]*\.?[0-9]+$ ]] || { echo "shot.sh: bad ZOOM '$ZOOM'" >&2; exit 2; }
		# Scroll so the top-left module sits at the top-left corner (gridOffset is in HP / rack rows)
		jq --argjson z "$ZOOM" '
			.zoom = $z
			| .gridOffset = (if ((.modules // []) | length) > 0
				then [([.modules[].pos[0]] | min) - 0.5, ([.modules[].pos[1]] | min) - 0.02]
				else [0, 0] end)' "$PJ" >"$PJ.tmp" && mv "$PJ.tmp" "$PJ"
		;;
esac
PATCH=$WORK/patch.vcv
"$HERE/mkpatch.sh" "$WORK/patch" "$PATCH"

# --- Plugins ---------------------------------------------------------------
declare -A have=()
add_plugin() {
	local dir slug
	dir=$(realpath "$1")
	[ -f "$dir/plugin.json" ] || { echo "shot.sh: $dir has no plugin.json" >&2; exit 1; }
	[ -f "$dir/plugin.so" ] || { echo "shot.sh: $dir has no plugin.so (build it first)" >&2; exit 1; }
	slug=$(jq -r .slug "$dir/plugin.json")
	ln -sfn "$dir" "$PLUGDIR/$slug"
	have[$slug]=1
}
if [ "$PLUGINS" != - ] && [ -n "$PLUGINS" ]; then
	IFS=: read -r -a plist <<<"$PLUGINS"
	for p in "${plist[@]}"; do [ -n "$p" ] && add_plugin "$p"; done
fi
for slug in $(jq -r '[.modules[]?.plugin] | unique | .[]' "$PJ"); do
	[ "$slug" = Core ] && continue
	[ -n "${have[$slug]:-}" ] && continue
	if [ "$slug" = Fundamental ] && [ -f "$HERE/Fundamental/plugin.so" ]; then
		add_plugin "$HERE/Fundamental"
	else
		echo "shot.sh: warning: patch uses plugin '$slug' which is not installed" >&2
	fi
done

# --- Settings: no tips, no update checks, no tooltips, fixed window ------
PLUGIN_SETTINGS_J=${PLUGIN_SETTINGS:-'{}'}
cat >"$U/settings.json" <<EOF
{
  "language": "en",
  "token": "",
  "windowMaximized": false,
  "windowSize": [$W, $H],
  "windowPos": [0, 0],
  "pixelRatio": 0.0,
  "tooltips": false,
  "cpuMeter": false,
  "threadCount": ${THREADS:-1},
  "cableOpacity": ${CABLES:-0.5},
  "preferDarkPanels": $([ "${DARK:-0}" = 1 ] && echo true || echo false),
  "frameRateLimit": 60.0,
  "autosaveInterval": 3600.0,
  "autoCheckUpdates": false,
  "verifyHttpsCerts": false,
  "showTipsOnLaunch": false,
  "tipIndex": 0,
  "allowCursorLock": false,
  "pluginSettings": $PLUGIN_SETTINGS_J
}
EOF

# --- Run -------------------------------------------------------------------
rc=0
xvfb-run -a -s "-screen 0 ${W}x${H}x24 -nolisten tcp" \
	"$HERE/shot.sh" --inner "$WORK" "$PATCH" "$OUT" "$SECS" "$W" "$H" || rc=$?

# Surface problems from Rack's log
if [ -f "$U/log.txt" ]; then
	grep -E '^\[[0-9.]+ (warn|fatal)' "$U/log.txt" | grep -vE 'src/(rtmidi|rtaudio).cpp' >&2 || true
fi
if [ $rc -ne 0 ] || [ ! -s "$OUT" ]; then
	echo "shot.sh: FAILED (rc=$rc). Rack log: $U/log.txt" >&2
	KEEP=1
	exit 1
fi
awk -v a="$t0" -v b="$(date +%s.%N)" -v o="$OUT" 'BEGIN { printf "shot.sh: wrote %s (%.1fs)\n", o, b - a }' >&2
