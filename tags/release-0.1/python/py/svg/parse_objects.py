# Copyright 2012 Lukas Kemmer
#
# Licensed under the Apache License, Version 2.0 (the "License"); you
# may not use this file except in compliance with the License. You
# may obtain a copy of the License at
#
# http:#www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
# implied. See the License for the specific language governing
# permissions and limitations under the License.

from xml.dom.minidom import parse
import xml.dom.minidom as minidom
import ifaint
from py.svg.parse_transform import parse_transform
from py.svg.util import deg2rad, parse_value, actual_line_end, rad_angle
from x11_colors import x11_colors
from math import tan
from py.svg.convert_base64 import strip_MIME

def splinePointsFromPath( pathDef ):
    items = pathDef.split(' ')
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
            points.extend([c,d])
            i += 7
        elif item == 'L':
            points.append( float(items[i + 1]) )
            points.append( float(items[i + 2]) )
            break
        else:
            break
    return points

def parseTri( node ):
    s_tri = node.attributes["faint:tri"].value
    p0, p1, p2 = s_tri.split(" ")
    p0 = [float(v) for v in p0.split(",")]
    p1 = [float(v) for v in p1.split(",")]
    p2 = [float(v) for v in p2.split(",")]
    return ifaint.Tri( p0, p1, p2 )

def NodeFillStyle( node ):
    if node.hasAttribute( 'fill' ):
        if node.hasAttribute( 'stroke' ):
            return FillStyle( node.attributes['stroke'].value, node.attributes['fill'].value )
        else:
            return FillStyle( "none", node.attributes['fill'].value )
    elif node.hasAttribute( 'stroke' ):
        return FillStyle( node.attributes['stroke'].value, "none" )
    elif node.hasAttribute( 'fill-rule' ):
        # I have not checked what evenodd means in the standard, but this
        # sets correct fill for johnny_automatic's images from Openclipart :)
        if node.attributes['fill-rule'].value == 'evenodd':
            s = ifaint.Settings()
            s.fillstyle='f'
            return s

    return ifaint.Settings()

def FillStyle( stroke, fill ):
    s = ifaint.Settings()
    if stroke.lower() == "none" and fill.lower() != "none":
        s.fillstyle = 'f'
        s.fgcol = parseColor(fill)
    elif stroke.lower() != "none" and fill.lower() == "none":
        s.fillstyle = 'b'
        s.fgcol = parseColor( stroke )
    else:
        s.fillstyle = 'bf'
        s.fgcol = parseColor( stroke )
        s.bgcol = parseColor( fill )
    return s

def to_faint_cap_str( svg_cap ):
    if svg_cap == 'butt':
        return 'flat'
    elif svg_cap == 'round':
        return 'round'
    else:
        return 'flat'


def getStyleDict( style ):
    styleItems = style.split(";")
    styleDict = {}
    for item in styleItems:
        if len( item ) > 0:
            key, value = item.split(":")
            key, value = key.strip(), value.strip()
            styleDict[key] = value
    return styleDict

def percentageColor( r, g, b ):
    r_fl = float(r[:-1])
    g_fl = float(g[:-1])
    b_fl = float(b[:-1])

    r = int( 255 * r_fl / 100.0 )
    g = int( 255 * g_fl / 100.0 )
    b = int( 255 * b_fl / 100.0 )
    return r,g,b

def hexColor( svgColor ):
    if len( svgColor ) == 4:
        # Thank you w3c for this convenient shorthand form
        svgColor = "#" + svgColor[1]*2 + svgColor[2]*2 + svgColor[3]*2

    assert( len( svgColor ) == 7 )

    r = int( svgColor[1:3], 16 )
    g = int( svgColor[3:5], 16 )
    b = int( svgColor[5:7], 16 )
    return r, g, b

def parseColor( svgColor, opacity="1.0" ):
    #fixme: handle "currentColor"

    opacity = float(opacity)
    a = int(255 * opacity)

    if svgColor[0:3] == "rgb":
        color = svgColor[3:]
        color = color.replace( "(", "" ).replace(")", "")

        parts = color.split(",")
        if len( parts ) == 4:
            # Fixme: Non-conformant terrible hack of doom
            return [ int(part) for part in parts ]

        r, g, b = color.split(",")
        if r[-1]=='%' or g[-1] == '%' or b[-1] == '%':
            r, g, b = percentageColor( r, g, b )
            return r, g, b, a
        else:
            return int(r), int(g), int(b), a
    elif svgColor[0] == "#":
        r, g, b = hexColor( svgColor )
        return r, g, b, a
    else:
        r,g,b = x11_colors.get( svgColor.lower(), (0,0,0))
        return r, g, b, a


def parseNodeTransform( node ):
    if node.hasAttribute("transform"):
        return parse_transform( node.attributes['transform'].value )
    return None


def centerBasedToRect( cx, cy, rx, ry ):
    return cx - rx, cy - ry, rx  * 2, ry * 2

def getSettings( node ):
    if node.hasAttribute( 'style' ):
        s = ParseStyle( node.attributes['style'].value )
    else:
        s= ifaint.Settings()
    s.update_all(NodeFillStyle( node ))
    return s

def ParseStyle( style ):
    s = ifaint.Settings()
    styleDict = getStyleDict( style )

    fontSize = styleDict.get( "font-size", None )

    if fontSize is not None:
        if fontSize.strip() == 'medium':
            fontSize = 12.0
        else:
            s.fontsize = int(float(fontSize.replace("px", "")) )

    fontFamily = styleDict.get( "font-family", None )
    if fontFamily is not None:
        s.fontface = str(fontFamily) # uh oh

    fontStyle = styleDict.get ( "font-style", None )
    if fontStyle is not None:
        s.fontitalic = fontStyle == "italic"

    fontWeight = styleDict.get ( "font-weight", None )
    if fontWeight is not None:
        s.fontbold = fontWeight == "bold"

    strokeWidth = styleDict.get( "stroke-width", None )
    if strokeWidth is not None:
        sw = parse_value( strokeWidth )
        s.linewidth = sw

    strokeDashArray = styleDict.get( "stroke-dasharray", "none" )
    if strokeDashArray != "none":
        s.linestyle = "ld"
    else:
        s.linestyle = "s"

    stroke = styleDict.get( "stroke", "none" )
    fill = styleDict.get( "fill", "none" )
    stroke_opacity = styleDict.get( "stroke-opacity", "1.0" )
    fill_opacity = styleDict.get( "fill-opacity", "1.0" )

    if stroke == "none" and fill != "none":
        s.fillstyle = 'fill'
        fg = parseColor( fill, fill_opacity )
        s.fgcol = fg

    elif stroke != "none" and fill == "none":
        s.fillstyle = 'border'
        fg = parseColor( stroke, stroke_opacity )
        s.fgcol = fg

    elif stroke != "none" and fill != "none":
        s.fillstyle = 'border+fill'
        fg = parseColor( stroke, stroke_opacity )
        s.fgcol = fg
        bg = parseColor( fill, fill_opacity )
        s.bgcol = bg

    lineJoin = styleDict.get( "stroke-linejoin", None )
    if lineJoin is not None:
        s.join = str(lineJoin)

    lineCap = styleDict.get( "stroke-linecap", None )
    if lineCap is not None:
        s.cap = to_faint_cap_str(lineCap)

    fillRule = styleDict.get( "fill-rule", None )
    if fillRule == 'evenodd':
        s.fillstyle = 'fill' # Hack

    return s

def parseRectNode( node ):
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
             getSettings(node),
             parseNodeTransform(node) ]

def parsePolygonNode( node ):
    points = node.attributes['points'].value.strip()
    points = points.replace(" ", "," ) # Fixme
    vals = [ float(coord) for coord in points.split(',') if len(coord) > 0 ]

    if node.hasAttribute('faint:type') and node.attributes["faint:type"].value == "rect":
        return ["rect",
                ifaint.Tri( (vals[0], vals[1]), (vals[2], vals[3]), (vals[6], vals[7])),
                getSettings(node),
                parseNodeTransform(node)]

    return ["polygon",
            vals,
            getSettings(node),
            parseNodeTransform(node)]

def parseCircleNode( node ):
    cx = float(node.attributes['cx'].value)
    cy = float(node.attributes['cy'].value)
    r = float(node.attributes['r'].value)

    return [ "ellipse",
             centerBasedToRect(cx, cy, r, r ),
             getSettings(node),
             parseNodeTransform(node) ]

def parseEllipseNode( node ):
    pos = {"cx" : 0.0,
           "cy" : 0.0,
           "rx" : 0.0,
           "ry" : 0 }
    for name in pos:
        if node.hasAttribute(name):
            pos[name] = float(node.attributes[name].value)

    return [ "ellipse",
             centerBasedToRect( pos["cx"], pos["cy"], pos["rx"], pos["ry"] ),
             getSettings( node ),
             parseNodeTransform( node ) ]

def parseGroupNode( node ):
    s = getSettings( node )
    objs = []
    for child in node.childNodes:
        obj = parseNode( child )
        if obj is not None:
            # Child nodes are not necessarily objects
            objs.append(obj)
    if len(objs) == 0:
        # Faint groups can not be empty. Perhaps some
        # warning or fix is required for this case
        return None
    return ["group",
            None,
            getSettings( node ),
            parseNodeTransform( node ),
            objs ]


def cleanPathDefinition( pathDef ):
    return pathDef.replace(",", " ")

def _parsePathEllipse( node ):
    return ["ellipse",
            parseTri(node),
            getSettings(node ),
            parseNodeTransform( node )]

def _parsePathSpline( node ):
    pathDef = cleanPathDefinition( node.attributes['d'].value )
    points = splinePointsFromPath( pathDef )
    return ["spline",
            points,
            getSettings( node ),
            parseNodeTransform( node ) ]

def _parsePathPath( node ):
    pathDef = cleanPathDefinition( node.attributes['d'].value )
    if len(pathDef) == 0:
        return None
    return ["path",
            pathDef,
            getSettings( node ),
            parseNodeTransform( node )]

def parsePathNode( node ):
    s = getSettings( node )

    if node.hasAttribute( 'faint:type' ):
        type = node.attributes['faint:type'].value.strip()
        if type == 'spline':
            return _parsePathSpline( node )
        elif type == 'ellipse':
            return _parsePathEllipse( node )
        else:
            assert( false )
    else:
        return _parsePathPath( node )

def parseLineNode( node ):
    s = getSettings( node )

    x1 = float(node.attributes['x1'].value)
    y1 = float(node.attributes['y1'].value)
    x2 = float(node.attributes['x2'].value)
    y2 = float(node.attributes['y2'].value)

    arrowhead = node.hasAttribute("marker-end") and node.attributes["marker-end"].value == "url(#Arrowhead)"
    arrowtail = node.hasAttribute("marker-start") and node.attributes["marker-start"].value == "url(#Arrowtail)"

    if arrowhead and arrowtail:
        s.arrow = 3
    elif arrowtail:
        s.arrow = 2
    elif arrowhead:
        s.arrow = 1
        # Adjust the arrow line end ( Fixme: More work required )
        lineWidth = s.linewidth
        angle = rad_angle( x2, y2, x1, y1 );
        x2, y2 = actual_line_end( x2, y2, angle, lineWidth )
    else:
        s.arrow = 0

    return ["line", (x1, y1, x2, y2), s, parseNodeTransform( node ) ]

def parseTextNode( node ):
    s = getSettings( node )

    x, y = 0, 0
    if node.hasAttribute('x'):
        x = float(node.attributes['x'].value)
    if node.hasAttribute('y'):
        y = float(node.attributes['y'].value)
    if node.hasAttribute("width"):
        w = float(node.attributes['width'].value)
    else:
        w = 200.0 # Fixme

    if node.hasAttribute("height"):
        h = float(node.attributes['height'].value)
    else:
        h = 200.0 #Fixme

    if node.hasAttribute("font-size"):
        sz = int( float( node.attributes['font-size'].value ) )
        s.fontsize = sz

    if node.hasAttribute('style'):
        styleDict = getStyleDict( node.attributes['style'].value )
        if styleDict.has_key("fill"):
            s.fgcol = parseColor( styleDict["fill"] )

    anchor = 'left'
    if node.hasAttribute('text-anchor'):
        anchor = node.attributes['text-anchor'].value

    textStr = u""
    for child in node.childNodes:
        if child.nodeName == 'tspan':
            if child.hasAttribute('style'):
                s.update_all(ParseStyle( child.attributes['style'].value ))
            if len( child.childNodes ) > 0:
                textStr = textStr + child.childNodes[0].nodeValue # Encode?
            if child.hasAttribute('faint:hardbreak') and child.attributes['faint:hardbreak'].value == "1":
                textStr += "\n"


        elif str(child.nodeName.strip()) == '#text':
            textStr = textStr + child.nodeValue.encode('utf-8')

    return ["text", (x,y,w,h), s, parseNodeTransform( node ), textStr, anchor ]

from py.svg.convert_base64 import decode_base64, base64_jpeg_prefix, base64_png_prefix

def parseImageNode( node ):
    x, y = 0, 0
    if node.hasAttribute('x'):
        x = float(node.attributes['x'].value)
    if node.hasAttribute('y'):
        y = float(node.attributes['y'].value)

    w = float(node.attributes['width'].value)
    h = float(node.attributes['height'].value)
    s = ifaint.Settings()
    if node.hasAttribute('faint:bg_style'):
        bgStyle = node.attributes['faint:bg_style'].value.strip()
        if bgStyle == "masked":
            s.transparency = 'transparent'
        else:
            s.transparency = 'opaque'
    if node.hasAttribute('faint:mask_color'):
        maskColor = parseColor( node.attributes['faint:mask_color'].value.strip() )
        s.bgcol = maskColor
    else:
        s.bgcol = (255,255,255)

    if node.hasAttribute("xlink:href"):
        data = strip_MIME( node.attributes['xlink:href'].value )
    else:
        data = None, ""

    if node.hasAttribute("faint:background"):
        return ["faint:background", data[1]]

    return ["image", (x, y, w, h), s, parseNodeTransform( node ), data]

def parseNode( node ):
    if node.nodeName == 'ellipse':
        return parseEllipseNode( node )
    if node.nodeName == 'g':
        return parseGroupNode( node )
    if node.nodeName == 'line':
        return parseLineNode( node )
    if node.nodeName == 'path':
        return parsePathNode( node )
    if node.nodeName == 'polygon':
        return parsePolygonNode( node )
    if node.nodeName == 'rect':
        return parseRectNode( node )
    if node.nodeName == 'spline':
        return parseLineNode( node )
    if node.nodeName == 'text':
        return parseTextNode( node )
    if node.nodeName == '#text':
        return None
    if node.nodeName == 'image':
        return parseImageNode( node )
    return None

def parseString( svgStr ):
    dom = minidom.parseString( svgStr )
    doc = dom.documentElement
    return parseNode( doc )
