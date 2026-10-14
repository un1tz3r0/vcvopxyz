. /tmp/claude-0/-home-claude/217f56e6-394f-5c4b-9d79-429eb491079b/scratchpad/opztest/lib.sh
at() { python3 -c "print(round(19.5 + 5.906 * $1), round(50.5 + 5.906 * $2))"; }
move() { xdotool mousemove $(at $1 $2); sleep 0.35; }
px() { xdotool mousemove $1 $2; sleep 0.35; xdotool click 1; sleep 0.5; }
sleep 1.5
move 218 8; xdotool click 3; sleep 0.5
px 1360 421
sleep 1
move 218 8; xdotool click 3; sleep 0.5; snap loaded-menu.png
xdotool key Escape; sleep 0.3
move 100 140; sleep 0.2
tap PLAY_PARAM; sleep 2; snap loaded-play.png
tap PLAY_PARAM; sleep 0.3
xdotool key ctrl+s; sleep 2
