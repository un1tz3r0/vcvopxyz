"""The Sequencer's face with a numbered badge on each part, for docs/controls.md: render_keys.py draws it over the
face of state "off" as docs/assets/keys/face-numbered-<theme>.png. The numbers follow the table in controls.md."""
from PIL import ImageDraw, ImageFont

# Each badge's centre, in panel mm. A key's badge sits on its top left corner.
POINTS = [
    (54.5, 10.5),     # 1 the index keys
    (56.0, 20.8),     # 2 the plug readout
    (89.3, 20.5),     # 3 the tempo, INT and EXT
    (119.6, 21.7),    # 4 the pages
    (12.5, 47.4),     # 5 the track keys
    (126.0, 12.0),    # 6 the dials
    (6.6, 61.0),      # 7 TRACK
    (6.6, 76.5),      # 8 REC
    (6.6, 92.1),      # 9 −
    (21.3, 76.5),     # 10 PLAY
    (36.0, 76.5),     # 11 STOP
    (21.3, 92.1),     # 12 +
    (36.0, 92.1),     # 13 SHIFT
    (21.3, 61.0),     # 14 the steps
    (58.1, 76.5),     # 15 the black keys
    (50.8, 92.1),     # 16 the white keys
    (5.3, 117.5),     # 17 the transport inputs
    (79.9, 117.5),    # 18 the note inputs
    (154.5, 117.5),   # 19 the transport outputs
    (229.2, 117.5),   # 20 L and R
]
FONT = '/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf'


def numbered(face, px_mm):
    """The face, a Pillow image at px_mm pixels a millimetre, with the badges drawn on a copy."""
    image = face.copy()
    draw = ImageDraw.Draw(image)
    r = 2.7 * px_mm
    font = ImageFont.truetype(FONT, round(r * 1.15))
    for n, (x, y) in enumerate(POINTS, 1):
        x, y = x * px_mm, y * px_mm
        draw.ellipse((x - r, y - r, x + r, y + r), fill='#2563eb', outline='#ffffff', width=max(2, round(r / 5)))
        draw.text((x, y), str(n), fill='#ffffff', font=font, anchor='mm')
    return image
