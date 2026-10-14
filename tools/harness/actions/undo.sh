. /tmp/claude-0/-home-claude/217f56e6-394f-5c4b-9d79-429eb491079b/scratchpad/opztest/lib.sh
at() { python3 -c "print(round(19.5 + 5.906 * $1), round(50.5 + 5.906 * $2))"; }
move() { xdotool mousemove $(at $1 $2); sleep 0.35; }
sleep 1.5
snap u0.png
tap PROJECT_PARAM; sleep 0.4; tap KEY3_PARAM; sleep 0.6; tap PROJECT_PARAM; sleep 0.6
snap u1.png
xdotool key ctrl+z; sleep 0.8
snap u2.png
xdotool key ctrl+shift+z; sleep 0.8
snap u3.png
xdotool key ctrl+z; sleep 0.8
