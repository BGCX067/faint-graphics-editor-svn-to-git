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
import py.svg.parse_objects as parse_objects
from py.svg.util import deg2rad, parse_value, actual_line_end, rad_angle
from x11_colors import x11_colors
from math import tan, cos, sin
from py.svg.convert_base64 import decode_base64

tm = [ [1, 0, 0, 1, 0, 0 ] ]

def matrix_mul(m, pt):
    a,b,c,d,e,f = m
    x, y = pt
    x2 = x * a + y * c + 1 * e
    y2 = x * b + y * d + 1 * f
    return x2, y2

def multiply_matrices( m1, m2 ):
    a1,b1,c1,d1,e1,f1 = m1
    a2,b2,c2,d2,e2,f2 = m2
    return [ a1 * a2 + c1 * b2 + e1 * 0,# a
           b1 * a2 + d1 * b2 + f1 * 0, # b
           a1 * c2 + c1 * d2 + e1 * 0, # c
           b1 * c2 + d1 * d2 + f1 * 0, # d
           a1 * e2 + c1 * f2 + e1 * 1, # e
           b1 * e2 + d1 * f2 + f1 * 1, # f
        ]

def do_transform( m ):
    global tm
    ctm = tm[-1]
    tm.append(multiply_matrices(ctm, m) )

def pop_transform():
    global tm
    if len(tm) > 1:
        del tm[-1]

def applyCTF( obj_id, props ):
    #if str(obj) == "Group":
    #    # Fixme: Why?
    #    return
    tri = props.get_obj_tri(obj_id)
    ctm = tm[-1]
    p0 = tri.p0()
    p1 = tri.p1()
    p2 = tri.p2()
    p0 = matrix_mul(ctm, p0)
    p1 = matrix_mul(ctm, p1)
    p2 = matrix_mul(ctm, p2)
    props.set_obj_tri( obj_id, ifaint.Tri( p0, p1, p2 ) ) # ifaint necessary?

def applyTransforms( t ):
    if t is None:
        return
    did_transform = False

    m_change = [1, 0, 0, 1, 0, 0 ]
    for type, param  in t:
        if type == "skewX":
            rad = deg2rad( param[0])
            m = [1, 0, tan(rad), 1, 0, 0]
            m_change = multiply_matrices( m_change, m )
            did_transform = True
        elif type == "skewY":
            rad = deg2rad( param )
            m = [1, tan(rad), 0, 1, 0, 0]
            m_change = multiply_matrices( m_change, m )
            #do_transform(m)
            did_transform = True
        elif type == "rotate":
            rad = deg2rad(param[0])
            if len(param) > 1:
                temp = multiply_matrices( [1,0,0,1, param[1],param[2]], [cos(rad),sin(rad),-sin(rad),cos(rad), 0, 0])
                temp = multiply_matrices( temp, [1, 0, 0, 1, -param[1], -param[2]] )
                m_change = multiply_matrices( m_change, temp )
                did_transform = True
            else:
                m = [cos(rad),sin(rad),-sin(rad),cos(rad), 0, 0]
                m_change = multiply_matrices( m_change, m )
                did_transform = True
        elif type == "translate":
            m = [1,0,0,1, param[0], param[1]]
            m_change = multiply_matrices( m_change, m )
            did_transform = True
        elif type == "matrix":
            m_change = multiply_matrices( m_change, param )
            did_transform = True

    if did_transform:
        do_transform( m_change )
    return did_transform

def createObject( objInfo, props ):
    type = objInfo[0]

    if type == 'ellipse':
        type, pos, settings, transforms = objInfo
        if pos.__class__ == ifaint.Tri:
            # The node had a faint:Tri attribute
            ellipseId = ellipse = props.Ellipse( (0, 0, 1, 1), settings )
            props.set_obj_tri(ellipseId, pos)
        else:

            ellipseId = props.Ellipse( pos, settings )
            transformed = applyTransforms( transforms )
            applyCTF( ellipseId, props )
            if transformed:
                pop_transform()
        return ellipseId

    elif type == 'group':
        type, pos, settings, transform, childObjects = objInfo
        faintObjs = []
        transformed = applyTransforms( transform )
        for childInfo in childObjects:
            id = createObject( childInfo, props )
            if id is not None:
                faintObjs.append( id )
        if len( faintObjs ) > 0:
            groupId = props.Group( *faintObjs ) # Fixme: Allow sequence in props.Group
        else:
            groupId = None
        if transformed:
            pop_transform()
        return groupId

    elif type == 'line':
        type, pos, settings, transforms = objInfo # Fixme: Use transforms man
        lineId = props.Line( pos, settings )
        applyCTF( lineId, props )
        return lineId

    elif type == 'text':
        type, pos, settings, transforms, textStr, anchor = objInfo
        x, y, w, h = pos
        textId = props.Text((x,y,w,h), textStr, settings )
        ht = props.get_obj_text_height(textId)

        if anchor == 'middle':
            props.obj_text_anchor_middle(textId)

        # SVG anchors at baseline, Faint at top of text
        dy = props.get_obj_text_height(textId)
        tri = props.get_obj_tri(textId)
        tri.translate(0, -dy);
        props.set_obj_tri(textId, tri)
        transformed = applyTransforms( transforms )
        applyCTF(textId, props)
        if transformed:
            pop_transform()
        return textId

    elif type == 'rect':
        type, pos, settings, transforms = objInfo
        if pos.__class__ == ifaint.Tri:
            rectId = props.Rect( (0,0,1,1), settings)
            props.set_obj_tri(rectId, pos)
        else:
            rectId = props.Rect(pos, settings)
        transformed = applyTransforms( transforms )
        applyCTF( rectId, props )
        if transformed:
            pop_transform()
        return rectId
    elif type == 'polygon':
        type, pos, settings, transforms = objInfo
        polygonId = props.Polygon(pos, settings)
        transformed = applyTransforms( transforms )
        applyCTF( polygonId, props )
        if transformed:
            pop_transform()
        return polygonId

    elif type == 'spline':
        type, points, settings, transforms = objInfo
        splineId = props.Spline(points, settings)
        transformed = applyTransforms(transforms)
        applyCTF( splineId, props ) # if transfed
        if transformed:
            pop_transform()
        return splineId

    elif type == 'path':
        try:
            type, pathDef, settings, transforms = objInfo
            pathId = props.Path( pathDef, settings ) # Fixme: Handle exception
            transformed = applyTransforms( transforms )
            applyCTF( pathId, props )
            if transformed:
                pop_transform()
            return pathId
        except ValueError, e:
            print "Failed parsing path: %s" % pathDef
            print "(%s)" % str(e)

    elif type == 'image':
        type, (x,y,w,h), settings, transforms, data = objInfo
        imageType, imageData = data
        if ( imageType == "base64_jpg" ):
            imageId = props.Raster((x,y,w,h),"jpeg",decode_base64(imageData), settings)
        elif ( imageType == "base64_png" ):
            imageId = props.Raster((x,y,w,h),"png",decode_base64(imageData), settings)
        else:
            assert( False )

        transformed = applyTransforms( transforms )
        applyCTF( imageId, props )
        if transformed:
            pop_transform()
        return imageId
    else:
        assert( False )
    return None

def parseCircleNode( node, props ):
    return createObject( parse_objects.parseCircleNode( node ), props )

def parseEllipseNode( node, props ):
    return createObject( parse_objects.parseEllipseNode( node ), props )

def parseImageNode( node, props ):
    result = parse_objects.parseImageNode( node )
    if result[0] == "faint:background":
        base64png = result[1]
        props.set_background_png_string( decode_base64(base64png )) # Fixme: Add to props
        return None
    return createObject( parse_objects.parseImageNode( node ), props )

def parseLineNode( node, props ):
    return createObject( parse_objects.parseLineNode( node ), props )

def parsePathNode( node, props ):
    return createObject( parse_objects.parsePathNode( node ), props )

def parsePolygonNode( node, props ):
    return createObject( parse_objects.parsePolygonNode( node ), props )

def parseRectNode( node, props ):
    return createObject( parse_objects.parseRectNode( node ), props )

def parseTextNode( node, props ):
    return createObject( parse_objects.parseTextNode( node ), props )

def parseGroupNode( node, props ):
    groupInfo = parse_objects.parseGroupNode( node )
    if groupInfo is None:
        return None
    return createObject( groupInfo, props )

def parseNode( node, props ):
    if node.nodeName == 'rect':
        return parseRectNode( node, props )
    elif node.nodeName == 'ellipse':
        return parseEllipseNode( node, props )
    elif node.nodeName == 'circle':
        return parseCircleNode( node, props )
    elif node.nodeName == 'text':
        return parseTextNode( node, props )
    elif node.nodeName == 'line':
        return parseLineNode( node, props )
    elif node.nodeName == 'svg':
        strWidth = node.attributes['width'].value.strip()
        strHeight = node.attributes['height'].value.strip()
        if strWidth[-1] == '%':
            width = 1024 # FIXME
        else:
            width = parse_value(strWidth )

        if strHeight[-1] == '%':
            height= 768 # FIXME
        else:
            height = parse_value(strHeight)
        props.set_size( int(width), int(height) ) # Fixme: int-limitation seems rather arbitrary, and what about units?
        for child in node.childNodes:
            parseNode( child, props )
    elif node.nodeName == 'polygon':
        return parsePolygonNode( node, props )
    elif node.nodeName == 'path':
        return parsePathNode( node, props )
    elif node.nodeName =='g':
        return parseGroupNode( node, props )
    elif node.nodeName == 'image':
        return parseImageNode( node, props )
    return None

def parseDoc( path, props ):
    try:
        global tm
        tm = [ [1, 0, 0, 1, 0, 0 ] ]
        dom = parse(path)
        doc = dom.documentElement
        parseNode( doc, props )
    except Exception, e:
        props.set_error(str(e))


def parseString( svgStr, props ):
    global tm
    tm = [ [1, 0, 0, 1, 0, 0 ] ]
    dom = minidom.parseString( svgStr )
    doc = dom.documentElement
    parseNode( doc, props )
