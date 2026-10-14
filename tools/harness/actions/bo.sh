. /tmp/claude-0/-home-claude/217f56e6-394f-5c4b-9d79-429eb491079b/scratchpad/opztest/lib.sh
at() { python3 -c "print(round(19.5 + 5.906 * $1), round(50.5 + 5.906 * $2))"; }
move() { xdotool mousemove $(at $1 $2); sleep 0.35; }
sleep 1; tap PLAY_PARAM; sleep 3
snap breakout.png
