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

from math import cos, sin, atan2, pi

def to_pdf_color( faint_color ):
    return faint_color[0] / 255.0, faint_color[1] / 255.0, faint_color[2] / 255.0

def set_colors( stream, object ):
    stream.fgcol( *to_pdf_color(object.get_fgcol() ) )
    stream.bgcol( *to_pdf_color(object.get_bgcol() ) )

def set_dash( stream, object ):
    if object.get_linestyle() == 'long_dash':
        w = object.get_linewidth() * 2
        stream.dash(w, w )
    else:
        stream.dash_off()

def rad_angle( x0, y0, x1, y1 ):
    x1b = x1 - x0
    y1b = y1 - y0
    return atan2( y1b, x1b )

def arrow_line_end( tip_x, tip_y, angle, linewidth ):
    return (tip_x + cos( angle ) * 15 * (linewidth / 2.0 ),
            tip_y + sin( angle ) * 15 * (linewidth / 2.0 ) )

def arrowhead( stream, lineWidth, x0, y0, x1, y1 ):
    angle = rad_angle( x0, y0, x1, y1 )
    orth = angle + pi / 2.0
    ax0 = cos( orth ) * 10 * ( lineWidth / 3.0 )
    ay0 = sin( orth ) * 10 * ( lineWidth / 3.0 )

    ax1 = cos( angle ) * 15 * ( lineWidth / 2.0 )
    ay1 = sin( angle ) * 15 * ( lineWidth / 2.0 )

    stream.triangle( x0 + ax0 + ax1, y0 + ay0 + ay1,
                     x0, y0,
                     x0 - ax0 + ax1, y0 - ay0 + ay1 )

    leX, leY = arrow_line_end( x0, y0, angle, lineWidth )
    stream.line( x1, y1, leX, leY )

def stroke_and_or_fill( stream, object ):
    fillStyle = object.get_fillstyle()
    if fillStyle == 'border':
        stream.fgcol( *to_pdf_color( object.get_fgcol() ) )
        stream.stroke()
    elif fillStyle == 'fill':
        stream.bgcol( *to_pdf_color( object.get_fgcol() ) )
        stream.fill()
    elif fillStyle == 'border+fill':
        stream.fgcol( *to_pdf_color( object.get_fgcol() ) )
        stream.bgcol( *to_pdf_color( object.get_bgcol() ) )
        stream.stroke_and_fill()

def transform_points( faint_points, doc_h, scale_x, scale_y ):
    pdf_points = []
    for i in range(len(faint_points)):
        if i % 2 == 0:
            pdf_points.append( faint_points[i] * scale_x )
        else:
            pdf_points.append( doc_h - faint_points[i] * scale_y )
    return pdf_points

def points_pdf_to_faint( pdf_points, doc_h, scale_x, scale_y ):
    faint_points = []
    for i in range(len(pdf_points)):
        if i % 2 == 0:
            faint_points.append( pdf_points[i] / scale_x )
        else:
            faint_points.append( (doc_h - pdf_points[i]) / scale_y )
    return faint_points


def object_to_stream( s, object, doc_h, scale_x, scale_y ):
        if object.get_type() == 'Line':
            x0, y0, x1, y1 = object.get_points()
            x0 *= scale_x
            x1 *= scale_x
            y0 *= scale_y
            y1 *= scale_y
            y0 = doc_h - y0
            y1 = doc_h - y1
            set_dash( s, object )
            s.fgcol( *to_pdf_color( object.get_fgcol() ) )

            s.linewidth( object.get_linewidth() )
            if object.get_arrowhead() == 1:
                s.bgcol( *to_pdf_color( object.get_fgcol() ) )
                arrowhead( s, object.get_linewidth(), x1, y1, x0, y0 )
            else:
                s.line( x0, y0, x1, y1 )

        elif object.get_type() == 'Rectangle':
            t = object.tri()
            p0 = t.p0()
            p1 = t.p1()
            p2 = t.p2()
            p3 = t.p3()
            set_dash( s, object )
            s.linewidth( object.get_linewidth() )
            points = transform_points( (p0[0], p0[1], p1[0], p1[1], p3[0], p3[1], p2[0], p2[1] ), doc_h, scale_x, scale_y )
            s.polygon( points )
            stroke_and_or_fill( s, object )

        elif object.get_type() == 'Text':
            s.bgcol( *to_pdf_color( object.get_fgcol() ) )
            x, y = object.tri().p0()
            textHeight = object.get_text_height()
            lineSpacing = object.get_text_line_spacing()
            y += textHeight
            x *= scale_x
            y *= scale_y
            y = doc_h - y
            for num, item in enumerate(object.get_text_lines()):
                s.text( x, y - num * ( textHeight + lineSpacing ), object.get_fontsize(), item[1] )
                

        elif object.get_type() == 'Spline':
            s.fgcol( *to_pdf_color( object.get_fgcol() ) )
            s.linewidth( object.get_linewidth() )
            set_dash( s, object )
            points = transform_points( object.get_points(), doc_h, scale_x, scale_y )
            s.spline( points )

        elif object.get_type() == 'Polygon':
            set_dash( s, object )
            s.linewidth( object.get_linewidth() )
            points = transform_points( object.get_points(), doc_h, scale_x, scale_y )
            s.polygon( points )
            stroke_and_or_fill( s, object )
        elif object.get_type() == 'Group':
            for i in range( object.num_objs() ):
                object_to_stream( s, object.get_obj(i), doc_h, scale_x, scale_y )

        elif object.get_type() == 'Ellipse':
            t = object.tri()
            x, y = t.p0()
            w = t.width()
            h = t.height()
            x *= scale_x
            y *= scale_y
            w *= scale_x
            h *= scale_y
            y = doc_h - y
            y -= h
            set_dash( s, object )
            s.linewidth( object.get_linewidth() )
            s.ellipse( x, y, w, h )
            stroke_and_or_fill( s, object )
