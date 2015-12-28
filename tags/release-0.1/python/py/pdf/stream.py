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

class Stream:
    def __init__(self):
        self.stream = ""

    def keys( self ):
        return ["Length"]
    def __len__( self ):
        return len(self.keys())

    def __getitem__( self, item ):
        if item == "Length":
            return len( self.stream )

    def data( self ):
        return "stream\n" + self.stream + "endstream\n"

    def line( self, x0, y0, x1, y1 ):
        self.stream += "%f %f m\n%f %f l\nS\n" % ( x0, y0, x1, y1 )

    def rect( self, x0, y0, w, h ):
        self.stream += "%f %f %f %f re\n" % (x0, y0, w, h )

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
        self.stream += "%f %f m\n" % (x, y)

    def triangle( self, x0, y0, x1, y1, x2, y2 ):
        self.stream += "%f %f m\n" % ( x0, y0 )
        self.stream += "%f %f l\n" % ( x1, y1 )
        self.stream += "%f %f l\n" % ( x2, y2 )
        self.stream += "f* s\n"

    def text( self,  x, y, size, string, fontId=1 ):
        self.stream += "BT\n"
        self.stream += "/F%d %f Tf\n" % (fontId, size)
        self.stream += "%f %f Td\n" % (x, y )
        self.stream += "(%s) Tj\n" % string
        self.stream += "ET\n"

    def polygon( self, points ):
        self.stream += "%f %f m\n" % (points[0], points[1])
        for i in range( 1, len(points) / 2 ):
            point = points[i * 2], points[i*2 + 1]
            self.stream += "%f %f l\n" % point
        self.stream += "h\n"

    def stroke( self ):
        self.stream += "S\n"

    def fill( self ):
        self.stream += "f\n"

    def stroke_and_fill( self ):
        self.stream += "B\n"

    def spline( self, points ):
        p0 = points[0:2]
        x1 = p0[0]
        y1 = p0[1]

        p1 = points[2:4]
        c = p1[0]
        d = p1[1]

        x3 = ( x1 + c ) / 2.0
        y3 = ( y1 + d ) / 2.0

        self.stream += "%f %f m\n" % (x1, y1)
        self.stream += "%f %f l\n" % (x3, y3)

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

            self.stream += "%f %f %f %f %f %f c\n" % ( x1, y1, x2, y2, x3, y3 )
        self.stream += "S\n"

    # Extend the path with a cubic Bezier curve to (x3, y3) using
    # (x1,y1) and (x2, y2) as control points
    def cubicspline( self, x1, y1, x2, y2, x3, y3 ):
        self.stream += "%f %f %f %f %f %f c\n" % (x1, y1, x2, y2, x3, y3 )

    def fgcol( self, r, g, b ):
        self.stream += "%f %f %f RG\n" % ( r, g, b )

    def bgcol( self, r, g, b ):
        self.stream += "%f %f %f rg\n" % ( r, g, b )

    def linewidth( self, w ):
        self.stream += "%f w\n" % w

    def dash( self, on, off ):
        self.stream += "[%f %f] 0 d\n" % (on, off)

    def dash_off( self ):
        self.stream += "[] 0 d\n"
