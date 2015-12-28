# PDF: BT
def begin_text():
    return "BT\n"

# PDF: h
def close():
    return "h\n"

# PDF: s
def close_and_stroke():
    return "s\n"

# PDF: c (bezier curve)
def curveto( x1, y1, x2, y2, x3, y3 ):
    return "%f %f %f %f %f %f c\n" % (x1, y1, x2, y2, x3, y3)

# PDF: d
def dash( array, phase ):
    return "[%s] %d d\n" %(" ".join([str(v) for v in array]), phase)

#PDF: ET
def end_text():
    return "ET\n"

# PDF: f*
def fill_even_odd():
    return "f*\n"

def fill_nonzero_winding():
    return "f\n"

def join( *ops ):
    return "".join(ops)

# PDF: l
def lineto(x,y):
    return "%f %f l\n" % (x,y)

# PDF: w
def linewidth(w):
    return "%f w\n" % w

def moveto(x,y):
    return "%f %f m\n" % (x,y)

# PDF: re
def rect(x,y,w,h):
    return "%f %f %f %f re\n"

# PDF: RG, parameters in range 0.0-1.0
def rgb_stroke_color(r,g,b):
    return "%f %f %f RG\n" % (r,g,b)

# PDF: rg, parameters in range 0.0-1.0
# Note: PDF32000-2008 gives this as "Set RGB color for nonstroking
# operations"
def rgb_fill_color(r,g,b):
    return "%f %f %f rg\n" % (r,g,b)

#PDF: Tj
def show_string( text ):
    return "(%s) Tj\n" % text

# PDF: S (no close)
def stroke():
    return "S\n"

# PDF: B
def stroke_and_fill():
    return "B\n"

# PDF: Tf
def text_font(fontId, size):
    return "/F%d %f Tf\n" % (fontId, size)

# PDF: Td
def text_offset( tx, ty ):
    return "%f %f Td\n" % (tx, ty)
