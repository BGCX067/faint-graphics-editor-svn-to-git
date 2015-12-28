# Copyright 2012 Lukas Kemmer
#
# Licensed under the Apache License, Version 2.0 (the "License"); you
# may not use this file except in compliance with the License. You
# may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
# implied. See the License for the specific language governing
# permissions and limitations under the License.

import re
from math import atan2, cos, sin, pi
from py.svg.convert_base64 import strip_MIME, decode_base64
from py.svg.x11_colors import x11_colors

_DEG_PER_RAD = 360 / ( 2 * pi )
_RAD_PER_DEG = 1 / _DEG_PER_RAD

def rad2deg( angle ):
    return angle * _DEG_PER_RAD

def deg2rad( angle ):
    return angle * _RAD_PER_DEG

def extract_url_reference( ref_string ):
    """Extracts the id part from an in-document url reference"""
    match = re.match(r"url\(#(.*?)\)", ref_string )
    if match is None:
        return None
    return match.group(1)

def extract_local_xlink_href( ref_string ):
    """Extracts an id reference from xlink:href to an item in the same
    document from the attribute value"""
    match = re.match(r"\#(.*)", ref_string )
    if match is None:
        return None
    return match.group(1)

def maybe_id_ref( node ):
    if node.hasAttribute("id"):
        return " (id=%s)" % node.getAttribute("id")
    return ""

def parse_value( v ):
    for item in ["px","pt","mm", "cm"]:
        if v.endswith(item):
            return float( v[:-len(item)] )
    return float(v)

def actual_line_end( lineEndX, lineEndY, angle, lineWidth ):
    x = lineEndX - cos( angle ) * 15 * (lineWidth / 2.0 )
    y = lineEndY - sin( angle ) * 15 * (lineWidth / 2.0 )
    return x, y

def arrow_line_end( arrowTipX, arrowTipY, angle, lineWidth ):
    x = arrowTipX + cos( angle ) * 15 * ( lineWidth / 2.0 )
    y = arrowTipY + sin( angle ) * 15 * ( lineWidth / 2.0 )
    return x, y

def rad_angle( x0, y0, x1, y1 ):
    return atan2( y1 - y0, x1 - x0 )

def percent_str_to_rate( s ):
    return float(s[:-1]) / 100.0

def multiply_matrix_pt(matrix, point):
    """Multiplies the point with the matrix somehow"""
    a, b, c, d, e, f = matrix # pylint: disable=C0103
    x, y = point
    x2 = x * a + y * c + 1 * e
    y2 = x * b + y * d + 1 * f
    return x2, y2

def multiply_matrices( m1, m2 ):
    """Multiplices two matrices, returns the result."""
    a1, b1, c1, d1, e1, f1 = m1
    a2, b2, c2, d2, e2, f2 = m2
    return [ a1 * a2 + c1 * b2 + e1 * 0,# a
           b1 * a2 + d1 * b2 + f1 * 0, # b
           a1 * c2 + c1 * d2 + e1 * 0, # c
           b1 * c2 + d1 * d2 + f1 * 0, # d
           a1 * e2 + c1 * f2 + e1 * 1, # e
           b1 * e2 + d1 * f2 + f1 * 1, # f
        ]

def rotation_matrix( rad ):
    """Returns a rotation matrix"""
    return [cos(rad), sin(rad), -sin(rad), cos(rad), 0, 0]

def translation_matrix( x, y ):
    """Returns a translation matrix"""
    return [1, 0, 0, 1, x, y]

def parse_embedded_image_data(image_string):
    """Returns a string describing the format, and a string with the
    raw image data."""
    encoding, data = strip_MIME(image_string)
    if encoding == "base64_jpg":
        return "jpeg", decode_base64(data)
    elif encoding == "base64_png":
        return "png", decode_base64(data)
    return None, ""

def parse_gradient_offset(svg_string):
    match = re.match(r"([0-9]+)%", str(svg_string))
    if match is not None:
        # Percentage
        return float(match.group(1)) / 100.0
    match = re.match(r"[1-9]+[0-9]*?\.[0-9]*?", svg_string)
    if match is not None:
        # Float
        return float(match.group(0)) # Fixme: I think standard says clamp to 0.0-1.0

    return 0.0 # Fixme: Report error

def hex_to_rgb( hex_color ):
    """Transforms a hexadecimal color string into an rgb-tuple of
    0-255 integers"""
    if len( hex_color ) == 4:
        # Thank you w3c for this convenient shorthand form
        # (#fb0 is equal to #ffbb00)
        hex_color = "#" + hex_color[1]*2 + hex_color[2]*2 + hex_color[3]*2

    assert( len( hex_color ) == 7 )
    r = int( hex_color[1:3], 16 )
    g = int( hex_color[3:5], 16 )
    b = int( hex_color[5:7], 16 )
    return r, g, b

def parse_color_noref(svg_color_str, opacity=1.0):
    """Parses an svg named, hex or rgb color specification"""
    #fixme: handle "currentColor"
    a = int(255 * opacity)
    if svg_color_str[0:3] == "rgb":
        color = svg_color_str[3:]
        color = color.replace( "(", "" ).replace(")", "")

        parts = color.split(",")
        if len( parts ) == 4:
            # Fixme: Non-conformant terrible hack of doom
            r, g, b, a = [ int(part) for part in parts ]
            return r, g, b, a

        r, g, b = color.split(",")
        if r[-1] == '%' or g[-1] == '%' or b[-1] == '%':
            r, g, b = percent_to_rgb( r, g, b )
            return r, g, b, a
        else:
            return int(r), int(g), int(b), a
    elif svg_color_str[0] == "#":
        r, g, b = hex_to_rgb( svg_color_str )
        return r, g, b, a
    else:
        r, g, b = x11_colors.get( svg_color_str.lower(), (0, 0, 0))
        return r, g, b, a

