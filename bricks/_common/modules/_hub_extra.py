from pybricks.tools import wait


def light_matrix_text_async(display, text, on, off):
    for char in text:
        display.char(char)
        yield from wait(on)
        display.off()
        yield from wait(off)
