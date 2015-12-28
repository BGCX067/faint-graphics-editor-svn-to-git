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

import xml.dom
import ifaint
from math import pi, atan2, tan
from py.svg.util import arrow_line_end, rad_angle, rad2deg
import base64

class svg_state:
    def __init__(self):
        self.linear_gradients = []
        self.radial_gradients = []
        self.patterns = []
        self.num_png = 0

        # Map of Faint fill types to their list and id-prefix
        self._class_map = {
            ifaint.LinearGradient : ("lgradient", self.linear_gradients),
            ifaint.Pattern : ("pattern", self.patterns),
            ifaint.RadialGradient : ("rgradient", self.radial_gradients), }

    def new_external_png_filename():
        self.num_png += 1
        return "raster%d.png" % self.num_png

    def get_items( self ):
        """Returns all items with their id as a list of tuples"""
        combined_list = []
        for prefix, item_list in self._class_map.values():
            combined_list.extend(zip(self._get_id_range(prefix, len(item_list)),
                                 item_list))
        return combined_list

    def get_defs_id( self, item ):
        prefix, item_list = self._class_map[item.__class__]
        return self._get_id(item, prefix, item_list)

    def should_link( self, item ):
        return item.__class__ in self._class_map.keys()

    def _get_id( self, item, prefix, item_list ):
        try:
            index = item_list.index(item)
        except ValueError:
            index = len(item_list)
            item_list.append(item)
        return self._id_for_index( prefix, index )

    def _get_id_range( self, prefix, item_count ):
        return [self._id_for_index(prefix, i) for i in range(item_count)]

    def _id_for_index( self, prefix, index ):
        return "%s%d" % (prefix, index + 1)

def createAttr( doc, name, value ):
    attr = doc.createAttribute( name )
    attr.nodeValue = value
    return attr

def createPosAttrs( doc, object ):
    return [ createAttr( doc, elem, str(val) ) for elem, val in
             zip(('x','y','width','height'), object.rect() ) ]

def setTransform( doc, obj, node ):
    t = obj.tri()
    angle_rad = t.angle()
    ori_x, ori_y = t.p0()
    x_off = 0
    tf = ""
    if angle_rad != 0:
        tf = tf + 'rotate(%f,%f,%f)' % (rad2deg(angle_rad),ori_x, ori_y )
        # Rotate back so the rest of the written values are "un-rotated"
        t.rotate( - angle_rad )
    #skew = t.get_skew()
    #if skew != 0:
        #if len(tf) > 0:
        #    tf = tf + " "
        #skew_angle_rad = atan2(skew, t.p0()[1] - t.p2()[1])
        #tf = tf + 'skewX(%f)' % rad2deg(skew_angle_rad)
        #x_off = - t.p0()[1] * tan(skew_angle_rad )

    if len( tf ) > 0:
        node.setAttributeNode( createAttr( doc, 'transform', tf ) )
    return x_off

def setPosAndTransform( doc, obj, node ):
    t = obj.tri()
    x_off = setTransform( doc, obj, node )

    x, y = t.p0()
    x += x_off
    w = t.width()
    h = t.height()
    posAttrs = [ createAttr( doc, elem, str(val) ) for elem, val in
                 zip(('x','y','width','height'), ( x, y, w, h )) ]
    for attr in posAttrs:
        node.setAttributeNode( attr )

def SVGLineDashStyle( obj ):
    lineStyle = obj.get_linestyle()
    if lineStyle == 'long_dash':
        w = obj.get_linewidth()
        # Dash and space length is twice the width
        return "stroke-dasharray: %d, %d;" % (w * 2, w * 2) # Fixme: Check Cairo stuff, avoid duplication, this will probably be a setting.
    return ""

def to_rgb_color( color ):
    return "rgb%s" % str( color[0:3] )

def to_svg_color( color, state ):
    if state.should_link(color):
        return "url(#%s)" % state.get_defs_id(color)
    # Fixme: Where is opacity handled?
    return "rgb%s" % str( color[0:3] )

def to_opacity_str( color ):
    if color.__class__ == ifaint.LinearGradient:
        return "1.0" # Fixme: Hard coded opacity for gradient
    elif color.__class__ == ifaint.RadialGradient:
        return "1.0"
    elif color.__class__ == ifaint.Pattern:
        return "1.0" # Fixme: Hard coded opacity for pattern
    elif len( color ) == 3:
        return "1.0"
    return "%s" % str( color[3] / 255.0 )

def to_svg_join_str( join ):
    return join

def to_svg_cap_str( cap ):
    if cap == 'flat':
        return 'butt'
    elif cap == 'round':
        return 'round'
    elif cap == 'square':
        return 'butt' # Fixme

def SVGNoFill():
    return "fill:none;"

def SVGLineStyle( obj, state ):
    color = obj.get_fg()
    lineWidth = obj.get_linewidth()
    cap = to_svg_cap_str( obj.get_settings().cap )
    return "stroke:%s;stroke-width:%s;stroke-opacity:%s;stroke-linecap:%s;" % ( to_svg_color(color, state), str(lineWidth), to_opacity_str(color), cap )

def SVGLineJoinStyle( obj ):
    join = obj.get_settings().join
    return "stroke-linejoin: %s;" % to_svg_join_str( join )

def SVGFillStyle( object, state ):
    fillStyle = object.get_fillstyle()
    strokeWidth = object.get_linewidth()
    fgCol = object.get_fg()
    bgCol = object.get_bg()
    if fillStyle == "border":
        return "fill:none;stroke-width:%s;stroke:%s;stroke-opacity:%s;"%(str(strokeWidth), to_svg_color(fgCol, state), to_opacity_str(fgCol))
    elif fillStyle == "fill":
        return "stroke:none;fill:%s;fill-opacity:%s;"% (to_svg_color(fgCol, state), to_opacity_str( fgCol ) )
    elif fillStyle == "border+fill":
        return "stroke:%s; stroke-width:%s; fill:%s;fill-opacity:%s;stroke-opacity:%s;"%( to_svg_color(fgCol, state), str(strokeWidth), to_svg_color(bgCol, state), to_opacity_str(bgCol), to_opacity_str(fgCol) )

def rad2deg( angle ):
    return angle * ( 360.0 / ( 2 * pi ) )

def createRectNode( doc, rectObj, state ):
    rectNode = doc.createElement( "polygon" )
    ptStr = str(rectObj.get_points())[1:-1]
    rectNode.setAttributeNode( createAttr( doc, "faint:type", "rect" ) );
    rectNode.setAttributeNode( createAttr( doc, "points", ptStr ) )
    style = SVGFillStyle( rectObj, state ) + SVGLineDashStyle( rectObj ) + SVGLineJoinStyle( rectObj )
    rectNode.setAttributeNode( createAttr( doc, "style", style ) )
    return rectNode

def createTextNode( doc, textObj, state ):
    # Create the outer text-element
    textNode = doc.createElement( "text" )
    tri = textObj.tri()
    tri.rotate( -tri.angle() )
    x0,y0 = tri.p0()
    w = tri.width()
    h = tri.height()

    textHeight = textObj.get_text_height()
    baseline = textObj.get_text_baseline()
    textNode.setAttributeNode( createAttr( doc, "x", str(x0) ) )
    textNode.setAttributeNode( createAttr( doc, "y", str(y0 + baseline) ) ) # Fixme
    textNode.setAttributeNode( createAttr( doc, "width", str(w) ) )
    textNode.setAttributeNode( createAttr( doc, "height", str(h) ) )

    # Set the style to the outer text-element
    fgCol = textObj.get_fg()
    fill = "fill:" + to_svg_color(fgCol, state) + ";"
    fontSize = "font-size: " + str( textObj.get_fontsize() ) + "px;"
    fontFamily = "font-family: " + textObj.get_font() + ";"

    if textObj.get_settings().fontitalic:
        fontStyle = 'font-style: italic;'
    else:
        fontStyle = 'font-style: normal;'

    if textObj.get_settings().fontbold:
        fontWeight = 'font-weight: bold;'
    else:
        fontWeight = 'font-weight: normal;'

    style = fill + fontSize + fontFamily + fontStyle + fontWeight
    textNode.setAttributeNode( createAttr( doc, "style", style ) )

    # Add the lines as separate tspan elements
    lines = textObj.get_text_lines()
    lineTri = textObj.tri()
    lineTri.rotate( -lineTri.angle() )

    for num, item in enumerate(lines):
        hardBreak = item[0] == 1
        line = item[1]
        tspanNode = doc.createElement( "tspan" )
        textNode.appendChild( tspanNode )
        lx, ly = lineTri.p0()
        tspanNode.setAttributeNode( createAttr( doc, "x", str( lx ) ) )
        tspanNode.setAttributeNode( createAttr( doc, "y", str( ly + baseline ) ) ) # Fixme: Use dy instead

        if hardBreak:
            # Fixme: I for some reason add extra whitespace to the end when splitting,
            # presumably for caret placement
            tspanNode.appendChild( doc.createTextNode( line[:-1] ))
            tspanNode.setAttributeNode( createAttr( doc, "faint:hardbreak", str("1") ) )
        else:
            tspanNode.appendChild( doc.createTextNode( line ) )
        lineTri.offset_aligned(0, textHeight)

    angle = textObj.tri().angle()
    if angle != 0:
        piv_x, piv_y = tri.p3()
        transform = createAttr( doc, 'transform', 'rotate(%s,%s,%s)'%( rad2deg(angle), piv_x, piv_y ) )
        textNode.setAttributeNode( transform )
    return textNode

def createTwoPointLine( doc, lineObj, state ):
    lineNode = doc.createElement( "line" )
    points = lineObj.get_points()
    assert(len(points) == 4)
    lineNode.setAttributeNode( createAttr( doc, "x1", str( points[0] ) ) )
    lineNode.setAttributeNode( createAttr( doc, "y1", str( points[1] ) ) )
    arrowhead = lineObj.get_arrowhead()
    if arrowhead == 1:
        angle = rad_angle( points[2], points[3], points[0], points[1] )
        x, y = arrow_line_end( points[2], points[3], angle, lineObj.get_linewidth() )
        lineNode.setAttributeNode( createAttr( doc, "x2", str(x) ) )
        lineNode.setAttributeNode( createAttr( doc, "y2", str(y) ) )
    else:
        lineNode.setAttributeNode( createAttr( doc, "x2", str( points[2] ) ) )
        lineNode.setAttributeNode( createAttr( doc, "y2", str( points[3] ) ) )

    style = SVGLineStyle( lineObj, state ) + SVGLineDashStyle( lineObj )
    lineNode.setAttributeNode( createAttr( doc, "style", style ) )

    if arrowhead == 1 or arrowhead == 3:
        lineNode.setAttributeNode( createAttr( doc, "marker-end", "url(#Arrowhead)") )
    if arrowhead == 2 or arrowhead == 3:
        lineNode.setAttributeNode( createAttr( doc, "marker-start", "url(#Arrowtail)") )
    return lineNode

def createPolyLine( doc, lineObj, state ):
    polyLine = doc.createElement("polyline")
    points = lineObj.get_points()
    # Fixme: Duplication of createTwoPointLine
    arrowhead = lineObj.get_arrowhead()
    if arrowhead == 1:
        angle = rad_angle( points[-2], points[-1], points[-4], points[-3] )
        x, y = arrow_line_end( points[-2], points[-1], angle, lineObj.get_linewidth() )
        points[-2] = x
        points[-1] = y
    ptStr = ",".join([str(pt) for pt in points])
    polyLine.setAttributeNode(createAttr(doc, "points", ptStr))
    style = SVGLineStyle( lineObj, state ) + SVGLineDashStyle( lineObj )
    polyLine.setAttributeNode( createAttr( doc, "style", style + ";fill:none" ) )
    if arrowhead == 1 or arrowhead == 3:
        polyLine.setAttributeNode( createAttr( doc, "marker-end", "url(#Arrowhead)") )
    if arrowhead == 2 or arrowhead == 3:
        polyLine.setAttributeNode( createAttr( doc, "marker-start", "url(#Arrowtail)") )
    return polyLine

def createLineNode( doc, lineObj, state ):
    points = lineObj.get_points()
    if len(points) == 4:
        return createTwoPointLine(doc, lineObj, state)
    else:
        return createPolyLine(doc, lineObj, state)

def triAttr( doc, tri ):
    p0 = tri.p0()
    p1 = tri.p1()
    p2 = tri.p2()
    return createAttr( doc, "faint:tri", "%f,%f %f,%f %f,%f" % (p0[0], p0[1],
                                                                p1[0],p1[1],
                                                                p2[0],p2[1]))

def createEllipseNode( doc, ellipseObj, state ):
    ellipseNode = doc.createElement( "path" )
    ellipseNode.setAttributeNode( createAttr( doc, "d", ifaint.to_svg_path( ellipseObj ) ) )
    ellipseNode.setAttributeNode( createAttr( doc, "style",
       SVGFillStyle( ellipseObj, state ) + SVGLineDashStyle( ellipseObj ) ) )
    ellipseNode.setAttributeNode( triAttr( doc, ellipseObj.tri() ) )
    ellipseNode.setAttributeNode( createAttr( doc, "faint:type", "ellipse" ) )
    return ellipseNode

def createPolygonNode( doc, polygonObj, state ):
    polygonNode = doc.createElement( "polygon" )
    ptStr = str( polygonObj.get_points() )[1:-1] # Fixme: Strip Python-indication from string. Nasty.
    polygonNode.setAttributeNode( createAttr( doc, "points", ptStr ) )

    style = SVGFillStyle( polygonObj, state ) + SVGLineDashStyle( polygonObj ) + SVGLineJoinStyle( polygonObj )
    polygonNode.setAttributeNode( createAttr( doc, "style", style ) )
    return polygonNode

def createSplineNode( doc, splineObj, state ):
    splineNode = doc.createElement( "path" )
    splineNode.setAttributeNode( createAttr( doc, "d", ifaint.to_svg_path( splineObj ) ) );
    splineNode.setAttributeNode( createAttr( doc, 'faint:type', 'spline') )
    style = SVGLineStyle( splineObj, state ) + SVGLineDashStyle( splineObj ) + SVGNoFill();
    splineNode.setAttributeNode( createAttr( doc, "style", style ) );
    return splineNode

def faint_bg_attr(doc, obj ):
    return createAttr(doc, 'faint:bg_style', obj.get_transparency())

def faint_mask_color_attr(doc, obj):
    return createAttr(doc, 'faint:mask-color', "rgb" + str(obj.get_bg()))

def createRasterNode( doc, rasterObj, state, embed=False ):
    rasterNode = doc.createElement("image")
    setPosAndTransform( doc, rasterObj, rasterNode )
    rasterNode.setAttributeNode(faint_bg_attr(doc,rasterObj))
    rasterNode.setAttributeNode(faint_mask_color_attr(doc,rasterObj))

    if not embed:
        filename = state.new_external_png_filename()
        rasterNode.setAttributeNode( createAttr( doc, "xlink:href", filename) )
    else:
        data = base64.b64encode( rasterObj.get_png_string() )
        data = "data:image/png;base64," + data
        rasterNode.setAttributeNode( createAttr( doc, "xlink:href", data ) )

    return rasterNode

def createRasterNodeEmbed( doc, rasterObj, state ):
    return createRasterNode( doc, rasterObj, state, True )

def createGroupNode( doc, groupObj, state ):
    groupNode = doc.createElement( "g" )
    for i in range( groupObj.num_objs() ):
        object = groupObj.get_obj(i)
        func = creators.get( object.get_type(), createOther )
        groupNode.appendChild( func( doc, object, state ) )
    return groupNode

def createPathNode( doc, pathObj, state ):
    pathNode = doc.createElement( "path" )
    path_str = ifaint.to_svg_path( pathObj )

    if len(path_str) == 0:
        # Fixme: Need a way to warn from save?
        print "Warning: Skipping empty path"
        return None
    pathNode.setAttributeNode( createAttr( doc, "d", ifaint.to_svg_path( pathObj ) ) );
    style = SVGFillStyle( pathObj, state ) + SVGLineDashStyle( pathObj )
    pathNode.setAttributeNode( createAttr( doc, "style", style ) )
    return pathNode


def createOther( doc, obj ):
    return doc.createElement( "other" )

creators = { "Rectangle" : createRectNode,
             "Text Region" : createTextNode,
             "Line" : createLineNode,
             "Polygon" : createPolygonNode,
             "Spline" : createSplineNode,
             "Raster" : createRasterNode,
             "Ellipse" : createEllipseNode,
             "Group" : createGroupNode,
             "Path" : createPathNode
             }

def arrowheadDef( doc ):
    arrowNode = doc.createElement( "marker" )
    for attr in ( ("id", "Arrowhead"),
                  ("markerUnits", "strokeWidth"),
                  ("markerWidth", "7.5"),
                  ("markerHeight", "6.6"),
                  ("orient", "auto"),
                  ("refX", "0"),
                  ("refY", "3.3")): # Offset by half width
        arrowNode.setAttributeNode( createAttr( doc, attr[0], attr[1] ) )
    arrowPath = doc.createElement( "path" )
    arrowPath.setAttributeNode( createAttr( doc, "d", "M 0 0 L 7.5 3.3 L 0 6.6 z" ) )
    arrowNode.appendChild( arrowPath )
    return arrowNode

def arrowtailDef( doc ):
    arrowNode = doc.createElement( "marker" )
    for attr in ( ("id", "Arrowtail"),
                  ("viewBox", "0 0 10 10"),
                  ("markerUnits", "strokeWidth"),
                  ("markerWidth", "10"),
                  ("markerHeight", "10"),
                  ("orient", "auto"),
                  ("refX", "10"),
                  ("refY", "5")):
        arrowNode.setAttributeNode( createAttr( doc, attr[0], attr[1] ) )
    arrowPath = doc.createElement( "path" )
    arrowPath.setAttributeNode( createAttr( doc, "d", "M 10 0 L 0 5 L 10 10 z" ) )
    arrowNode.appendChild( arrowPath )
    return arrowNode


def createDefsNode( doc ):
    defsNode = doc.createElement( "defs" )
    defsNode.appendChild( arrowheadDef( doc ) )
    defsNode.appendChild( arrowtailDef( doc ) )
    return defsNode

def createBackgroundNode( doc, image, state ):
    data = base64.b64encode( image.get_background_png_string(stamp=True) )
    data = "data:image/png;base64," + data
    size = image.get_size()
    imageNode = doc.createElement("image")
    imageNode.setAttributeNode( createAttr( doc, 'faint:background', '1') )
    imageNode.setAttributeNode( createAttr( doc, 'x', '0' ) )
    imageNode.setAttributeNode( createAttr( doc, 'y', '0' ) )
    imageNode.setAttributeNode( createAttr( doc, 'width', str( size[0] ) ) )
    imageNode.setAttributeNode( createAttr( doc, 'height', str( size[1] ) ) )
    imageNode.setAttributeNode( createAttr( doc, "xlink:href", data ) )
    return imageNode

def add_gradient_stops( doc, gr_node, stops ):
    # Sorting the stops by offset is required in SVG to get the same
    # appearance as a Cairo gradient
    stops = sorted(stops, key = lambda x : x[0])

    for stop in stops:
        offset, color = stop
        stopNode = doc.createElement("stop")
        stopNode.setAttributeNode( createAttr( doc, "offset", '%d%%' % int(offset * 100) ) )
        stopNode.setAttributeNode( createAttr( doc, "style", "stop-color:%s;stop-opacity:1" % to_rgb_color(color) ) )
        gr_node.appendChild(stopNode)

def create_linear_gradient_node( doc, node_id, gr ):
    gr_node = doc.createElement("linearGradient")
    gr_node.setAttributeNode ( createAttr( doc, 'x1', '0' ) )
    gr_node.setAttributeNode ( createAttr( doc, 'y1', '0' ) )
    gr_node.setAttributeNode ( createAttr( doc, 'x2', '1' ) )
    gr_node.setAttributeNode ( createAttr( doc, 'y2', '0' ) )
    gr_node.setAttributeNode ( createAttr( doc, 'id', node_id ) )

    angle = gr.get_angle()
    if angle != 0:
        gr_node.setAttributeNode( createAttr( doc, 'gradientTransform', 'rotate(%f,%f,%f)' % ( rad2deg(angle), 0.5, 0 ) ) )

    stops = [gr.get_stop(i) for i in range(gr.get_num_stops())]
    add_gradient_stops(doc, gr_node, stops)
    return gr_node

def create_pattern_node( doc, node_id, pattern ):
    patternNode = doc.createElement("pattern")
    bmp = pattern.get_bitmap()
    w, h = bmp.get_size()
    patternNode.setAttributeNode( createAttr( doc, 'id', node_id ) )
    patternNode.setAttributeNode( createAttr( doc, 'x', '0' ) )
    patternNode.setAttributeNode( createAttr( doc, 'y', '0' ) )
    patternNode.setAttributeNode( createAttr( doc, 'width', str(w)) )
    patternNode.setAttributeNode( createAttr( doc, 'height', str(h)) )

    if not pattern.get_object_aligned():
        # Image aligned pattern
        patternNode.setAttributeNode( createAttr( doc, 'patternUnits', 'userSpaceOnUse' ) )
        patternNode.setAttributeNode( createAttr( doc, 'patternContentUnits', 'userSpaceOnUse' ) )
    else:
        pass # Fixme: This case isn't handled yet.

    data = "data:image/png;base64," + base64.b64encode( bmp.get_png_string() )
    imageNode = doc.createElement("image")
    imageNode.setAttributeNode( createAttr( doc, "width", str(w) ) )
    imageNode.setAttributeNode( createAttr( doc, "height", str(h) ) )
    imageNode.setAttributeNode( createAttr( doc, "xlink:href", data ) )
    patternNode.appendChild(imageNode)
    return patternNode

def create_radial_gradient_node( doc, node_id, gr ):
    cx, cy = gr.get_center()
    rx, ry = gr.get_radii()

    gr_node = doc.createElement("radialGradient")
    gr_node.setAttributeNode( createAttr( doc, 'id', node_id ) )
    gr_node.setAttributeNode( createAttr( doc, 'cx', str(cx) ) )
    gr_node.setAttributeNode( createAttr( doc, 'cy', str(cy) ) )
    gr_node.setAttributeNode( createAttr( doc, 'rx', str(rx) ) )
    gr_node.setAttributeNode( createAttr( doc, 'ry', str(ry) ) )

    add_gradient_stops(doc, gr_node, gr.get_stops())
    return gr_node

def add_defs( doc, defs_node, state ):
    _item_funcs = { ifaint.LinearGradient : create_linear_gradient_node,
      ifaint.Pattern : create_pattern_node,
      ifaint.RadialGradient : create_radial_gradient_node,}

    for node_id, item in state.get_items():
        creator = _item_funcs[item.__class__]
        defs_node.appendChild(creator( doc, node_id, item ))

def write( path, canvas, prettyprint=False, embedRaster=False ):
    state = svg_state()

    domImpl = xml.dom.getDOMImplementation()
    docType = domImpl.createDocumentType("svg", "-//W3C//DTD SVG 1.1//EN", "http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd")
    doc = domImpl.createDocument("http://www.w3.org/2000/svg", "svg", docType )
    size = canvas.get_size()

    svg = doc.getElementsByTagName("svg")[0]
    svg.setAttributeNode( createAttr( doc, "width", str( size[0] ) ) )
    svg.setAttributeNode( createAttr( doc, "height", str( size[1] ) ) )

    # Why must I do this for namespaces??
    svg.setAttributeNode( createAttr( doc, "xmlns", "http://www.w3.org/2000/svg" ) )
    svg.setAttributeNode( createAttr( doc, "xmlns:xlink", "http://www.w3.org/1999/xlink" ) )
    svg.setAttributeNode( createAttr( doc, "xmlns:faint", "http://www.code.google.com/p/faint-graphics-editor" ) )
    svg.setAttributeNode( createAttr( doc, "xmlns:svg", "http://www.w3.org/2000/svg" ) )

    defsNode = createDefsNode(doc)
    svg.appendChild( defsNode )

    if embedRaster:
        creators[ "Raster" ] = createRasterNodeEmbed
    else:
        creators[ "Raster" ] = createRasterNode

    # Save the background image if it is not single colored
    if not ifaint.one_color_bg( canvas.get_frame() ):
        svg.appendChild( createBackgroundNode( doc, canvas, state ) )
    else:
        # Fixme: If the image is uniform, it could either mean the
        # background color should be used for the SVG, or the
        # background should be transparent. Use transparent bg for now
        # (for transparency, add a way to remove the background layer)
        pass

    objects = canvas.get_objects()
    for object in objects:
        func = creators.get( object.get_type(), createOther )
        svg.appendChild( func( doc, object, state ) )
        add_defs( doc, defsNode, state )

    if path is None:
        print doc.toprettyxml()
    else:
        svgFile = open( path, 'w' )
        if prettyprint:
            svgFile.write( doc.toprettyxml(encoding="utf-8") )
        else:
            svgFile.write( doc.toxml(encoding="utf-8") )
