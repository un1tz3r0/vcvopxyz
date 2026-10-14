. $(dirname $ACTIONS)/lib.sh
# Screen display origin in panel mm: Screen at 16 HP, glass at (3.5, 11.5).
sx() { python3 -c "print(16 * 5.08 + 3.5 + $1)"; }
sy() { python3 -c "print(11.5 + $1)"; }
cell() { echo "$(sx $(python3 -c "print(2.6 + (60.96 - 7 - 5.2) / 4 * ($2 + 0.5))")) $(sy $(python3 -c "print(60.5 + 8.6 * ($1 + 0.5))"))"; }
sleep 1.2; snap sc_sound.png
click 7 42.5; sleep 0.8; snap sc_env.png
click 7 47.5; sleep 0.8; snap sc_lfo.png
click $(cell 3 3); sleep 0.5; snap sc_mix.png
hold $(sx 25) $(sy 30); for i in $(seq 1 12); do xdotool mousemove_relative -- 0 -8; sleep 0.03; done; release; sleep 0.5; snap sc_drag.png
click $(cell 4 1); sleep 0.8; snap sc_track.png
xdotool mousemove 1295 5
