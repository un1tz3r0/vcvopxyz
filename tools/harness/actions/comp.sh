. /tmp/claude-0/-home-claude/217f56e6-394f-5c4b-9d79-429eb491079b/scratchpad/opztest/lib.sh
at() { python3 -c "print(round(19.5 + 5.906 * $1), round(50.5 + 5.906 * $2))"; }
move() { xdotool mousemove $(at $1 $2); sleep 0.35; }
sleep 1.5; tap PLAY_PARAM; sleep 0.5
mclick ctrl SHIFT_PARAM; sleep 0.5
tap STEP0_PARAM; sleep 0.3; tap STEP4_PARAM; sleep 0.3; tap STEP8_PARAM; sleep 0.3
tap KEY4_PARAM; sleep 0.3; tap KEY5_PARAM; sleep 0.3
tap KEY14_PARAM; sleep 0.3; tap KEY10_PARAM; sleep 0.5
snap comp.png
