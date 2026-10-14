. /tmp/claude-0/-home-claude/217f56e6-394f-5c4b-9d79-429eb491079b/scratchpad/tools/lib.sh
IDS=/tmp/claude-0/-home-claude/217f56e6-394f-5c4b-9d79-429eb491079b/scratchpad/opztest/ids.py
on() { python3 $IDS $1; }
tap() { click $(on $1); }
mclick() { move $(on $2); xdotool keydown $1; sleep 0.05; xdotool click 1; sleep 0.05; xdotool keyup $1; }
press() { hold $(on $1); }
