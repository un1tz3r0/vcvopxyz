"""The screen: a display beside an OP-Z engine, like the phone the OP-Z borrows for one. It's all glass."""
from panelkit import HP, Panel

WIDTH, HEIGHT = 12 * HP, 128.5


def panel():
    p = Panel(WIDTH, HEIGHT)
    p.rect(0, 0, WIDTH, HEIGHT, 0, 'panel')
    p.title('Screen')
    p.rect(2.5, 10.5, WIDTH - 5.0, 107.5, 3.6, 'bezel')
    p.rect(3.5, 11.5, WIDTH - 7.0, 105.5, 2.8, 'glass')
    p.place('DISPLAY_WIDGET', 3.5, 11.5, WIDTH - 7.0, 105.5)
    return p
