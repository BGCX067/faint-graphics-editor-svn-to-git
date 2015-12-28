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
import op

class Stream:
    def __init__(self):
        self.stream = ""

    def keys( self ): # For PDF-dictionary behavior
        return ["Length"]

    def __len__( self ): # For PDF-dictionary behavior
        return len(self.keys())

    def __getitem__( self, item ): # FOr PDF-dictionary behavior
        if item == "Length":
            return len( self.stream )

    def data( self ):
        return "stream\n" + self.stream + "endstream\n"

    def line( self, x0, y0, x1, y1 ):
        self.stream += op.join(
            op.moveto(x0,y0),
            op.lineto(x1,y1),
            op.stroke())

    def polyline( self, points ):
        self.stream += op.moveto(points[0], points[1])
        for i in range( 1, len(points) / 2 ):
            point = points[i*2], points[i*2+1]
            self.stream += op.lineto(*point)
        self.stream += op.stroke()

    def rect( self, x0, y0, w, h ):
        self.stream += op.rect(x0,y0,w,h)

    def ellipse( self, x, y, w, h ):
        t = 0.551784
        rx = w / 2.0
        ry = h / 2.0
        self.move( x + rx, y + h )
        self.cubicspline( x + rx + rx * t, y + h ,
                          x + w, y + ry + ry * t,
                          x + w, y + ry )
        self.cubicspline( x + w, y + ry - ry * t,
                          x + rx + rx * t, y,
                          x + rx, y)
        self.cubicspline( x + rx - rx * t, y,
                          x, y + ry - ry * t,
                          x, y + ry  )
        self.cubicspline( x, y + ry + ry *t,
                          x + rx - rx * t, y + h,
                          x + rx, y + h )

    def move( self, x, y ):
        self.stream += op.moveto(x,y)

    def triangle( self, x0, y0, x1, y1, x2, y2 ):
        self.stream += op.join(
            op.moveto(x0,y0),
            op.lineto(x1,y1),
            op.lineto(x2,y2),
            op.fill_even_odd(),
            op.close_and_stroke())

    def text( self,  x, y, size, string, fontId=1 ):
        self.stream += op.join(
            op.begin_text(),
            op.text_font(fontId,size),
            op.text_offset(x,y),
            op.show_string(string),
            op.end_text())

    def polygon( self, points ):
        self.stream += op.moveto(points[0], points[1])
        for i in range( 1, len(points) / 2 ):
            point = points[i * 2], points[i*2 + 1]
            self.stream += op.lineto( *point )
        self.stream += op.close()

    def stroke( self ):
        self.stream += op.stroke()

    def fill( self ):
        self.stream += op.fill_nonzero_winding()

    def stroke_and_fill( self ):
        self.stream += op.stroke_and_fill()

    def spline( self, points ):
        p0 = points[0:2]
        x1 = p0[0]
        y1 = p0[1]

        p1 = points[2:4]
        c = p1[0]
        d = p1[1]

        x3 = ( x1 + c ) / 2.0
        y3 = ( y1 + d ) / 2.0

        self.stream += op.join(
            op.moveto(x1,y1),
            op.lineto(x3,y3))

        for i in range( 2, len(points) / 2 ):
            point = points[ i * 2 ], points[ i * 2 + 1]
            x1 = x3
            y1 = y3
            x2 = c
            y2 = d
            c = point[0]
            d = point[1]
            x3 = ( x2 + c ) / 2.0
            y3 = ( y2 + d ) / 2.0
            self.stream += op.curveto(x1,y1,x2,y2,x3,y3)

        self.stream += op.stroke()

    # Extend the path with a cubic Bezier curve to (x3, y3) using
    # (x1,y1) and (x2, y2) as control points
    def cubicspline( self, x1, y1, x2, y2, x3, y3 ):
        self.stream += op.curveto(x1, y1, x2, y2, x3, y3)

    def fgcol( self, r, g, b ):
        self.stream += op.rgb_stroke_color(r,g,b)

    def bgcol( self, r, g, b ):
        self.stream += op.rgb_fill_color(r,g,b)

    def linewidth( self, w ):
        self.stream += op.linewidth(w)

    def dash( self, on, off ):
        self.stream += op.dash((on,off), 0)

    def dash_off( self ):
        self.stream += op.dash([], 0)

