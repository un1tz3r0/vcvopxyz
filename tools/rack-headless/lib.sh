# Rack takes a button event at the pointer position of the frame before, so move first and let a frame pass.
at() { python3 -c "print(round(19.5 + 7.382 * $1), round(50.5 + 7.382 * $2))"; }
move() { xdotool mousemove $(at $1 $2); sleep 0.1; }
click() { move $1 $2; xdotool click 1; }
hold() { move $1 $2; xdotool mousedown 1; }
release() { xdotool mouseup 1; }
key() { echo "$(python3 -c "print(3 + 75.28 / 14 * ($1 + 0.5))") $(python3 -c "print(81.4 if $1 == int($1) else 72.6)")"; }
