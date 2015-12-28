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

import xml.dom
import ifaint
from math import pi, atan2, tan
from py.svg.util import arrow_line_end, rad_angle
import base64

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

def to_opacity_str( color ):
    if len( color ) == 3:
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

def SVGLineStyle( obj ):
    color = obj.get_fgcol()
    lineWidth = obj.get_linewidth()
    cap = to_svg_cap_str( obj.get_settings().cap )
    return "stroke:%s;stroke-width:%s;stroke-opacity:%s;stroke-linecap:%s;" % ( to_rgb_color(color), str(lineWidth), to_opacity_str(color), cap )

def SVGLineJoinStyle( obj ):
    join = obj.get_settings().join
    return "stroke-linejoin: %s;" % to_svg_join_str( join )

def SVGFillStyle( object ):
    fillStyle = object.get_fillstyle()
    strokeWidth = object.get_linewidth()
    fgCol = object.get_fgcol()
    bgCol = object.get_bgcol()
    if fillStyle == "border":
        return "fill:none;stroke-width:%s;stroke:%s;stroke-opacity:%s;"%(str(strokeWidth), to_rgb_color(fgCol), to_opacity_str(fgCol))
    elif fillStyle == "fill":
        return "stroke:none;fill:%s;fill-opacity:%s;"% (to_rgb_color(fgCol), to_opacity_str( fgCol ) )
    elif fillStyle == "border+fill":
        return "stroke:%s; stroke-width:%s; fill:%s;fill-opacity:%s;stroke-opacity:%s;"%( to_rgb_color(fgCol), str(strokeWidth), to_rgb_color(bgCol), to_opacity_str(bgCol), to_opacity_str(fgCol) )

def rad2deg( angle ):
    return angle * ( 360.0 / ( 2 * pi ) )

def createRectNode( doc, rectObj ):
    rectNode = doc.createElement( "polygon" )
    tri = rectObj.get_points()
    ptStr = str(rectObj.get_points())[1:-1]
    rectNode.setAttributeNode( createAttr( doc, "faint:type", "rect" ) );
    rectNode.setAttributeNode( createAttr( doc, "points", ptStr ) )
    style = SVGFillStyle( rectObj ) + SVGLineDashStyle( rectObj ) + SVGLineJoinStyle( rectObj )
    rectNode.setAttributeNode( createAttr( doc, "style", style ) )
    return rectNode

def createTextNode( doc, textObj ):
    # Create the outer text-element
    textNode = doc.createElement( "text" )
    tri = textObj.tri()
    tri.rotate( -tri.angle() )
    x0,y0 = tri.p0()
    w = tri.width()
    h = tri.height()

    textHeight = textObj.get_text_height()
    textNode.setAttributeNode( createAttr( doc, "x", str(x0) ) )
    textNode.setAttributeNode( createAttr( doc, "y", str(y0 + textHeight) ) )
    textNode.setAttributeNode( createAttr( doc, "width", str(w) ) )
    textNode.setAttributeNode( createAttr( doc, "height", str(h) ) )

    # Set the style to the outer text-element
    color = str( textObj.get_fgcol() )
    fill = "fill:rgb" + color + ";"
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
    lineSpacing = textObj.get_text_line_spacing()
    lineTri = textObj.tri()
    lineTri.rotate( -lineTri.angle() )

    for num, item in enumerate(lines):
        hardBreak = item[0] == 1
        line = item[1]
        tspanNode = doc.createElement( "tspan" )
        textNode.appendChild( tspanNode )
        lx, ly = lineTri.p0()
        tspanNode.setAttributeNode( createAttr( doc, "x", str( lx ) ) )
        tspanNode.setAttributeNode( createAttr( doc, "y", str( ly + textHeight ) ) )

        if hardBreak:
            # Fixme: I for some reason add extra whitespace to the end when splitting,
            # presumably for caret placement
            tspanNode.appendChild( doc.createTextNode( line[:-1] ))
            tspanNode.setAttributeNode( createAttr( doc, "faint:hardbreak", str("1") ) )
        else:
            tspanNode.appendChild( doc.createTextNode( line ) )
        lineTri.offset_aligned(0, textHeight + lineSpacing )

    angle = textObj.tri().angle()
    if angle != 0:
        piv_x, piv_y = tri.p3()
        transform = createAttr( doc, 'transform', 'rotate(%s,%s,%s)'%( rad2deg(angle), piv_x, piv_y ) )
        textNode.setAttributeNode( transform )
    return textNode

def createLineNode( doc, lineObj ):
    lineNode = doc.createElement( "line" )
    points = lineObj.get_points()
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

    style = SVGLineStyle( lineObj ) + SVGLineDashStyle( lineObj )
    lineNode.setAttributeNode( createAttr( doc, "style", style ) )

    if arrowhead == 1 or arrowhead == 3:
        lineNode.setAttributeNode( createAttr( doc, "marker-end", "url(#Arrowhead)") )
    if arrowhead == 2 or arrowhead == 3:
        lineNode.setAttributeNode( createAttr( doc, "marker-start", "url(#Arrowtail)") )
    return lineNode

def triAttr( doc, tri ):
    p0 = tri.p0()
    p1 = tri.p1()
    p2 = tri.p2()
    return createAttr( doc, "faint:tri", "%f,%f %f,%f %f,%f" % (p0[0], p0[1],
                                                                p1[0],p1[1],
                                                                p2[0],p2[1]))
def createEllipseNode( doc, ellipseObj ):
    ellipseNode = doc.createElement( "path" )
    ellipseNode.setAttributeNode( createAttr( doc, "d", ifaint.to_svg_path( ellipseObj ) ) )
    ellipseNode.setAttributeNode( createAttr( doc, "style",
       SVGFillStyle( ellipseObj ) + SVGLineDashStyle( ellipseObj ) ) )
    ellipseNode.setAttributeNode( triAttr( doc, ellipseObj.tri() ) )
    ellipseNode.setAttributeNode( createAttr( doc, "faint:type", "ellipse" ) )
    return ellipseNode

def createPolygonNode( doc, polygonObj ):
    polygonNode = doc.createElement( "polygon" )
    ptStr = str( polygonObj.get_points() )[1:-1]
    polygonNode.setAttributeNode( createAttr( doc, "points", ptStr ) )

    style = SVGFillStyle( polygonObj ) + SVGLineDashStyle( polygonObj ) + SVGLineJoinStyle( polygonObj )
    polygonNode.setAttributeNode( createAttr( doc, "style", style ) )
    return polygonNode

def createSplineNode( doc, splineObj ):
    splineNode = doc.createElement( "path" )
    splineNode.setAttributeNode( createAttr( doc, "d", ifaint.to_svg_path( splineObj ) ) );
    splineNode.setAttributeNode( createAttr( doc, 'faint:type', 'spline') )
    style = SVGLineStyle( splineObj ) + SVGLineDashStyle( splineObj ) + SVGNoFill();
    splineNode.setAttributeNode( createAttr( doc, "style", style ) );
    return splineNode

numRast = 0

def createRasterNodeEmbed( doc, rasterObj ):
    return createRasterNode( doc, rasterObj, True )

def createRasterNode( doc, rasterObj, embed=False ):
    global numRast
    rasterNode = doc.createElement( "image" )

    setPosAndTransform( doc, rasterObj, rasterNode )
    #for attr in createPosAttrs( doc, rasterObj ):
    #    rasterNode.setAttributeNode( attr )

    if rasterObj.get_transparency() == 'transparent':
        rasterNode.setAttributeNode( createAttr( doc, 'faint:bg_style', 'masked') )
    else:
        rasterNode.setAttributeNode( createAttr( doc, 'faint:bg_style', 'solid') )

    rasterNode.setAttributeNode( createAttr( doc, 'faint:mask_color', "rgb" + str(rasterObj.get_bgcol()) ) )

    if not embed:
        filename = "raster" + str(numRast) + ".png"
        # Fixme: should be xlink:href
        rasterNode.setAttributeNode( createAttr( doc, "href", filename) )
        numRast += 1
    else:
        data = base64.b64encode( rasterObj.get_image() )
        data = "data:image/png;base64," + data
        rasterNode.setAttributeNode( createAttr( doc, "xlink:href", data ) )

    return rasterNode

def createGroupNode( doc, groupObj ):
    groupNode = doc.createElement( "g" )
    for i in range( groupObj.num_objs() ):
        object = groupObj.get_obj(i)
        func = creators.get( object.get_type(), createOther )
        groupNode.appendChild( func( doc, object ) )
    return groupNode

def createPathNode( doc, pathObj ):
    pathNode = doc.createElement( "path" )
    pathNode.setAttributeNode( createAttr( doc, "d", ifaint.to_svg_path( pathObj ) ) );
    style = SVGFillStyle( pathObj ) + SVGLineDashStyle( pathObj )
    pathNode.setAttributeNode( createAttr( doc, "style", style ) )
    return pathNode


def createOther( doc, obj ):
    return doc.createElement( "other" )

creators = { "Rectangle" : createRectNode,
             "Text" : createTextNode,
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

def createBackgroundNode( doc, image ):
    data = base64.b64encode( image.get_background_png_string() )
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

def write( path, canvas, prettyprint=False, embedRaster=False ):
    global numRast
    numRast = 0

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

    svg.appendChild( createDefsNode( doc ) )

    if embedRaster:
        creators[ "Raster" ] = createRasterNodeEmbed
    else:
        creators[ "Raster" ] = createRasterNode

    # Save the background image if it is not single colored
    if not ifaint.one_color_bg( canvas ):
        svg.appendChild( createBackgroundNode( doc, canvas ) )
    else:
        # Fixme: If the image is uniform, it could either mean the
        # background color should be used for the SVG, or the
        # background should be transparent. Use transparent bg for now
        # (for transparency, add a way to remove the background layer)
        pass

    objects = canvas.get_objects()
    for object in objects:
        func = creators.get( object.get_type(), createOther )
        svg.appendChild( func( doc, object ) )

    if path is None:
        print doc.toprettyxml()
    else:
        svgFile = open( path, 'w' )
        if prettyprint:
            svgFile.write( doc.toprettyxml(encoding="utf-8") )
        else:
            svgFile.write( doc.toxml(encoding="utf-8") )
