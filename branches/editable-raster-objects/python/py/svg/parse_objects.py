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

"""Helper for the parse_svg module. Parses SVG elements and returns
descriptions of Faint objects."""

import ifaint
from py.svg.parse_transform import parse_transform
import py.svg.util as util
from py.svg.util import parse_value, actual_line_end, rad_angle, maybe_id_ref
from py.svg.util import parse_embedded_image_data

def spline_points_from_path( path_def ):
    """Returns a tuple of points parsed from the SVG path definition string"""
    items = path_def.split(' ')
    points = []
    if items[0] != 'M':
        return None
    x1 = float(items[1])
    y1 = float(items[2])
    points.extend([x1 , y1])
    if items[3] != 'L':
        return None

    x3 = float(items[4])
    y3 = float(items[5])
    c = x3 * 2.0 - x1
    d = y3 * 2.0 - y1
    points.extend([c, d])

    i = 6
    while i != len(items):
        item = items[i]
        if item == 'C':
            x1 = float(items[i + 1])
            y1 = float(items[i + 2])
            x2 = float(items[i + 3])
            y2 = float(items[i + 4])
            x3 = float(items[i + 5])
            y3 = float(items[i + 6])
            c = x3 * 2 - x2
            d = y3 * 2 - y2
            points.extend([c, d])
            i += 7
        elif item == 'L':
            points.append( float(items[i + 1]) )
            points.append( float(items[i + 2]) )
            break
        else:
            break
    return points

def parse_tri( node ):
    """Parses a faint:tri.attribute (used for ellipses)"""
    s_tri = node.attributes["faint:tri"].value
    p0, p1, p2 = s_tri.split(" ")
    p0 = [float(v) for v in p0.split(",")]
    p1 = [float(v) for v in p1.split(",")]
    p2 = [float(v) for v in p2.split(",")]
    return ifaint.Tri( p0, p1, p2 )

def node_fill_style( node, state ):
    """Returns a Settings object initialized with fill and stroke
    based on the attributes of the SVG element.

    """
    if node.hasAttribute( 'fill' ):
        fill = node.attributes['fill'].value
        if node.hasAttribute( 'stroke' ):
            stroke = node.attributes['stroke'].value
            return fill_style( stroke, fill, state )
        else:
            return fill_style( "none", fill, state )
    elif node.hasAttribute( 'stroke' ):
        stroke = node.attributes['stroke'].value
        return fill_style( stroke, "none", state )
    return ifaint.Settings()

def fill_style( stroke, fill, state ):
    """Parses SVG stroke and fill attributes, returns a Faint Settings
    object."""
    settings = ifaint.Settings()
    if stroke.lower() == "none" and fill.lower() != "none":
        settings.fillstyle = 'f'
        settings.fg = parse_color(fill, "1.0", state)
    elif stroke.lower() != "none" and fill.lower() == "none":
        settings.fillstyle = 'b'
        settings.fg = parse_color(stroke, "1.0", state)
    else:
        settings.fillstyle = 'bf'
        settings.fg = parse_color(stroke, "1.0", state)
        settings.bg = parse_color(fill, "1.0", state)
    return settings

def to_faint_cap_str( svg_cap ):
    """Parses an svg line cap string and returns the equivalent Faint
    line cap string.

    """
    if svg_cap == 'butt':
        return 'flat'
    elif svg_cap == 'round':
        return 'round'
    else:
        return 'flat'

def get_style_dict( style ):
    """Parses an SVG style attribute string,
    returning it as a key/value dictionary"""
    style_items = style.split(";")
    style_dict = {}
    for item in style_items:
        if len( item ) > 0:
            key, value = item.split(":")
            key, value = key.strip(), value.strip()
            style_dict[key] = value
    return style_dict

def percent_to_rgb( r, g, b ):
    """Transforms color values expressed as 0-100% into an rgb-tuple
    of 0-255 integers"""
    r_fl = float(r[:-1])
    g_fl = float(g[:-1])
    b_fl = float(b[:-1])

    r = int( 255 * r_fl / 100.0 )
    g = int( 255 * g_fl / 100.0 )
    b = int( 255 * b_fl / 100.0 )
    return r, g, b

def parse_rgb_color( svg_color ):
    """Parses an svg rgb color"""
    assert(svg_color.startswith("rgb"))
    color = svg_color[3:]
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
        return int(r), int(g), int(b), 255

def parse_color( svg_color_str, opacity, state ):
    """Parses an svg color attribute string."""
    if svg_color_str.startswith("url"):
        ref_id = util.extract_url_reference(svg_color_str)
        if ref_id is not None:
            try:
                return state.items[ref_id]
            except:
                state.add_warning("Referenced id not found: %s" % ref_id)
        return 255, 0, 255, 255 # Fixme
    else:
        return util.parse_color_noref(svg_color_str, float(opacity)) # Fixme

def parse_node_transform( node ):
    """Returns the transform for this node, or None"""
    if node.hasAttribute("transform"):
        return parse_transform( node.attributes['transform'].value )
    return None

def center_based_to_rect( cx, cy, rx, ry ):
    """Returns x, y, w, h from the center and radius parameters."""
    return cx - rx, cy - ry, rx  * 2, ry * 2

def node_default_settings():
    settings = ifaint.Settings()
    settings.fg = (0,0,0)
    settings.fillstyle = 'fill'
    return settings

def get_settings( node, state ):
    """Returns a faint Settings object based on the passed in node."""
    settings = node_default_settings()
    if node.hasAttribute( 'style' ):
        settings.update_all(parse_style( node.attributes['style'].value, state))
    settings.update_all(node_fill_style(node, state))
    return settings

def parse_style( style, state ):
    """Returns a Faint Settings object based on the svg style
    string"""
    style_dict = get_style_dict( style )
    font_size = style_dict.get( "font-size", None )
    settings = ifaint.Settings()
    if font_size is not None:
        if font_size.strip() == 'medium':
            settings.fontsize = 12.0
        else:
            settings.fontsize = int(float(font_size.replace("px", "")) ) # Fixme

    font_family = style_dict.get( "font-family", None )
    if font_family is not None:
        settings.fontface = str(font_family) # Fixme: uh oh

    font_style = style_dict.get ( "font-style", None )
    if font_style is not None:
        settings.fontitalic = font_style == "italic"

    font_weight = style_dict.get ( "font-weight", None )
    if font_weight is not None:
        settings.fontbold = font_weight == "bold"

    stroke_width = style_dict.get( "stroke-width", None )
    if stroke_width is not None:
        sw = parse_value( stroke_width )
        sw1, sw2 = util.multiply_matrix_pt(state.current_transform(), (sw, sw))
        settings.linewidth = sw1

    stroke_dash_array = style_dict.get( "stroke-dasharray", "none" )
    if stroke_dash_array != "none":
        settings.linestyle = "ld"
    else:
        settings.linestyle = "s"

    stroke = style_dict.get( "stroke", "none" )
    fill = style_dict.get( "fill", "none" )
    stroke_opacity = style_dict.get( "stroke-opacity", "1.0" )
    fill_opacity = style_dict.get( "fill-opacity", "1.0" )
    if stroke == "none" and fill != "none":
        settings.fillstyle = 'fill'
        settings.fg = parse_color( fill, fill_opacity, state )
    elif stroke != "none" and fill == "none":
        settings.fillstyle = 'border'
        settings.fg = parse_color( stroke, stroke_opacity, state )
    elif stroke != "none" and fill != "none":
        settings.fillstyle = 'border+fill'
        settings.fg = parse_color( stroke, stroke_opacity, state )
        settings.bg = parse_color( fill, fill_opacity, state )

    line_join = style_dict.get( "stroke-linejoin", None )
    if line_join is not None:
        settings.join = str(line_join)

    line_cap = style_dict.get( "stroke-linecap", None )
    if line_cap is not None:
        settings.cap = to_faint_cap_str(line_cap)

    return settings

def parse_rect_node( node, state ):
    x = y = width = height = 0
    if node.hasAttribute( 'x' ):
        x = float(node.attributes['x'].value)
    if node.hasAttribute( 'y' ):
        y = float(node.attributes['y'].value)
    if node.hasAttribute( 'width' ):
        width = float(node.attributes['width'].value)
    if node.hasAttribute( 'height' ):
        height = float(node.attributes['height'].value)

    return [ "rect",
             (x,y,width,height),
             get_settings(node, state),
             parse_node_transform(node) ]

def parse_polygon_node( node, state ):
    points = node.attributes['points'].value.strip()
    points = points.replace(" ", "," ) # Fixme
    vals = [ float(coord) for coord in points.split(',') if len(coord) > 0 ]

    if node.hasAttribute('faint:type') and node.attributes["faint:type"].value == "rect":
        return ["rect",
                ifaint.Tri( (vals[0], vals[1]), (vals[2], vals[3]), (vals[6], vals[7])),
                get_settings(node, state),
                parse_node_transform(node)]

    return ["polygon",
            vals,
            get_settings(node, state),
            parse_node_transform(node)]

def parse_marker_attribute( node, settings ):
    arrowhead = node.hasAttribute("marker-end") and node.attributes["marker-end"].value == "url(#Arrowhead)"
    arrowtail = node.hasAttribute("marker-start") and node.attributes["marker-start"].value == "url(#Arrowtail)"
    if arrowhead and arrowtail:
        settings.arrow = 'both'
    elif arrowtail:
        settings.arrow = 'back'
    elif arrowhead:
        settings.arrow = 'front'
    else:
        settings.arrow = 'none'

def parse_polyline_node( node, state ):
    settings = get_settings(node, state)
    parse_marker_attribute( node, settings )

    points = node.attributes['points'].value.strip()
    points = points.replace(" ", ",") # Fixme
    vals = [ float(coord) for coord in points.split(',') if len(coord) > 0 ]

    if settings.arrow in ('front', 'both'):
        angle = rad_angle(vals[-2], vals[-1], vals[-4], vals[-3])
        vals[-2], vals[-1] = actual_line_end(vals[-2], vals[-1], angle, settings.linewidth)

    return ["polyline",
            vals,
            settings,
            parse_node_transform(node)]

def parse_circle_node( node, state ):
    cx = float(node.attributes['cx'].value)
    cy = float(node.attributes['cy'].value)
    r = float(node.attributes['r'].value)

    return [ "ellipse",
             center_based_to_rect(cx, cy, r, r ),
             get_settings(node, state),
             parse_node_transform(node) ]

def parse_ellipse_node( node, state ):
    pos = {"cx" : 0.0,
           "cy" : 0.0,
           "rx" : 0.0,
           "ry" : 0 }
    for name in pos:
        if node.hasAttribute(name):
            pos[name] = float(node.attributes[name].value)

    return [ "ellipse",
             center_based_to_rect( pos["cx"], pos["cy"], pos["rx"], pos["ry"] ),
             get_settings( node, state ),
             parse_node_transform( node ) ]

def parse_group_node( node, state ):
    objs = []
    for child in node.childNodes:
        obj = parse_node( child, state )
        if obj is not None:
            # Child nodes are not necessarily objects
            objs.append(obj)

    if len(objs) == 0:
        return ["empty_group"]

    return ["group",
            None,
            get_settings( node, state ),
            parse_node_transform( node ),
            objs ]


def clean_path_definition( path_def ):
    return path_def.replace(",", " ")

def _parse_path_ellipse( node, state ):
    return ["ellipse",
            parse_tri(node),
            get_settings(node, state ),
            parse_node_transform( node )]

def _parse_path_spline( node, state ):
    path_def = clean_path_definition( node.attributes['d'].value )
    points = spline_points_from_path( path_def )
    return ["spline",
            points,
            get_settings( node, state ),
            parse_node_transform( node ) ]

def _parse_path_path( node, state ):
    if not node.hasAttribute('d'):
        state.add_warning("Ignored path-element without definition attribute%s." % maybe_id_ref(node))
        return None

    path_def = clean_path_definition( node.attributes['d'].value )
    if len(path_def) == 0:
        state.add_warning("Ignored path-element with empty definition attribute%s." % maybe_id_ref(node))
        return None

    settings = get_settings(node, state)
    return ["path",
            path_def,
            get_settings( node, state ),
            parse_node_transform( node )]

def parse_path_node( node, state ):
    if node.hasAttribute( 'faint:type' ):
        faint_type = node.attributes['faint:type'].value.strip()
        if faint_type == 'spline':
            return _parse_path_spline( node, state )
        elif faint_type == 'ellipse':
            return _parse_path_ellipse( node, state )
        else:
            assert( False )
    else:
        return _parse_path_path( node, state )

def parse_line_node( node, state ):
    settings = get_settings( node, state )
    x1 = float(node.attributes['x1'].value)
    y1 = float(node.attributes['y1'].value)
    x2 = float(node.attributes['x2'].value)
    y2 = float(node.attributes['y2'].value)

    parse_marker_attribute(node, settings)
    if settings.arrow in ('front', 'both'):
        # Adjust the arrow line end ( Fixme: More work required )
        line_width = settings.linewidth
        angle = rad_angle( x2, y2, x1, y1 )
        x2, y2 = actual_line_end( x2, y2, angle, line_width )
    return ["line", (x1, y1, x2, y2), settings, parse_node_transform( node ) ]

def parse_text_node( node, state ):
    settings = get_settings( node, state )

    x, y = 0, 0
    if node.hasAttribute('x'):
        x = float(node.attributes['x'].value)
    if node.hasAttribute('y'):
        y = float(node.attributes['y'].value)
    if node.hasAttribute("width"): # Fixme: Text shouldn't have width
        w = float(node.attributes['width'].value)
    else:
        w = 200.0 # Fixme

    if node.hasAttribute("height"): # Fixme: Text shouldn't have height
        h = float(node.attributes['height'].value)
    else:
        h = 200.0 #Fixme

    if node.hasAttribute("font-size"):
        sz = int( float( node.attributes['font-size'].value ) )
        settings.fontsize = sz

    if node.hasAttribute('style'):
        style_dict = get_style_dict( node.attributes['style'].value )
        if style_dict.has_key("fill"):
            settings.fg = parse_color( style_dict["fill"], "1.0", state )

    anchor = 'left'
    if node.hasAttribute('text-anchor'):
        anchor = node.attributes['text-anchor'].value

    text_str = u""
    for child in node.childNodes:
        if child.nodeName == 'tspan':
            if child.hasAttribute('style'):
                settings.update_all(parse_style( child.attributes['style'].value, state))
            if len( child.childNodes ) > 0:
                text_str = text_str + child.childNodes[0].nodeValue # Encode?
            if child.hasAttribute('faint:hardbreak') and child.attributes['faint:hardbreak'].value == "1":
                text_str += "\n"


        elif str(child.nodeName.strip()) == '#text':
            text_str = text_str + child.nodeValue.encode('utf-8')

    return ["text", (x, y, w, h), settings, parse_node_transform( node ), text_str, anchor ]

def parse_image_node( node, state ):
    assert(not node.hasAttribute("faint:background"))
    x, y = 0, 0
    if node.hasAttribute('x'):
        x = float(node.attributes['x'].value)
    if node.hasAttribute('y'):
        y = float(node.attributes['y'].value)

    w = float(node.attributes['width'].value)
    h = float(node.attributes['height'].value)
    settings = ifaint.Settings()
    settings.bgstyle = 'opaque'
    if node.hasAttribute('faint:bg_style'):
        bgstyle = node.attributes['faint:bg_style'].value.strip()
        if bgstyle == "masked":
            settings.bgstyle = 'masked'
        else:
            settings.bgstyle = 'opaque'
    if node.hasAttribute('faint:mask_color'):
        mask_color = parse_color( node.attributes['faint:mask_color'].value.strip(), "1.0", state ) # Fixme: Hard coded opacity
        settings.bg = mask_color
    else:
        settings.bg = (255, 255, 255)
    if node.hasAttribute("xlink:href"):
        data = parse_embedded_image_data(node.getAttribute('xlink:href'))
    else:
        data = None, ""
    return ["image", (x, y, w, h), settings, parse_node_transform(node), data]

_PARSE_FUNCTIONS = {'rect': parse_rect_node,
                    'ellipse': parse_ellipse_node,
                    'circle': parse_circle_node,
                    'text': parse_text_node,
                    'line': parse_line_node,
                    'polyline': parse_polyline_node,
                    'polygon': parse_polygon_node,
                    'path': parse_path_node,
                    'g': parse_group_node,
                    'image': parse_image_node }


def get_parse_function( node_name ):
    """Returns the function that should be used for parsing an SVG
    element with the given name, or None.

    The functions accept an svg node and an SVGParseState,
    and return a Faint object description.
    """
    return _PARSE_FUNCTIONS.get(node_name,None)

def has_parse_function( node_name ):
    return _PARSE_FUNCTIONS.has_key(node_name)


def parse_node( node, state ):
    """Parses a node that can be translated into a Faint object.
    Returns an object description, or None if the node cannot be
    parsed as an object.

    """
    parse_function = get_parse_function(node.nodeName)
    if parse_function is None:
        return None
    return parse_function(node, state)
