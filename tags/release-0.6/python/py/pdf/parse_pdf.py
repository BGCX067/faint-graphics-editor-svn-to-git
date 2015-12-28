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
from convert_to_pdf import points_pdf_to_faint
re_mediabox = re.compile(r"/MediaBox \[(\d+ \d+ \d+ \d+)\]")
re_obj = re.compile(r"\d \d obj")
re_stream = re.compile(r"stream.*endstream", re.DOTALL)
re_metadata = re.compile("% faint meta-data\n(.*)\n% end of faint meta-data", re.DOTALL)

def from_pdf_color( r, g, b ):
    return int(r * 255), int(g * 255), int(b * 255)

def parse_obj_id( s ):
    i1, i2, obj = s.split(" ")
    return int(i1), int(i2)

class DummyFunc:
    def __init__( self, name ):
        self.name = name

    def __call__( self, *args, **kwArgs ):
        print "canvas." + self.name + "(" + ", ".join([str(arg) for arg in args]) + ")"

class DummyClass:
    def __getattr__(self, attr):
        return DummyFunc(attr)

if __name__ != "__main__":
    from ifaint import Settings
else:
    Settings = DummyClass

def parse_stream( text, canvas, doc_w, doc_h, metadata ):
    lines = text.split("\n")
    assert( lines[0] == "stream" )
    pos_x, pos_y = 0, 0
    points = []
    settings = Settings()
    settings.linewidth = 1
    settings.linestyle ='s'
    closed = False
    arrowHead = False
    skip = 0
    arrowEndX = 0 # Fixme
    arrowEndY = 0 # Fixme
    for num, line in enumerate(lines):
        if skip > 0:
            skip -= 1
            continue
        if metadata.has_key(num):            
            skip = 3
            settings.arrow = 1
            arrowHead = True
            arrowEndX = float(metadata[num][1])
            arrowEndY = float(metadata[num][2])

            continue        
        items = line.split(" ")
        if items[-1] == 'm':
            points.extend((float(items[0]), float(items[1])))
        elif items[-1] == 'l':
            points.extend([ float(item) for item in items[:-1]] )

        elif items[-1] == 'S' or items[-1] == "B": # Fixme: stroke/fill etc.
            if len(points) == 4:
                if arrowHead:
                    points[-2] = arrowEndX
                    points[-1] = arrowEndY
                p = canvas.Line( points_pdf_to_faint(points, doc_h, 1.0, 1.0 ), settings )
            elif len(points) > 4:
                if closed:
                    if items[-1] == "B":
                        settings.fillstyle = 'bf'
                    p = canvas.Polygon( points_pdf_to_faint(points, doc_h, 1.0, 1.0 ), settings )
                else:
                    p = canvas.Line( points_pdf_to_faint(points, doc_h, 1.0, 1.0) , settings )
            closed = False
            points = []
            settings.linewidth = 1
            settings.linestyle = 's'
            settings.fillstyle = 'b'
            settings.arrow = 0
        elif items[0] == 'f*':
            if len(points) > 4:
                settings.fillstyle = 'f'
                p = canvas.Polygon( points_pdf_to_faint(points, doc_h, 1.0, 1.0 ), settings )
            points = []
        elif items[-1] == 'w':
            settings.linewidth = float( items[0] )
        elif items[-1] == 'd':
            if items[0] == "[]":
                settings.linestyle = "s"
            else:
                settings.linestyle = "ld"
        elif items[0] == 'h':
            closed = True
        elif items[-1] == 'RG':
            settings.fgcol = from_pdf_color( *[float(item) for item in items[:-1]])
        elif items[-1] == 'rg':
            settings.bgcol = from_pdf_color( *[float(item) for item in items[:-1]] )
        elif items[-1] == 'f':
            settings.fillstyle = 'f'
            settings.fgcol, settings.bgcol = settings.bgcol, settings.fgcol
            p = canvas.Polygon( points_pdf_to_faint(points, doc_h, 1.0, 1.0 ), settings )
        else:
            pass


def parse_metadata( metadata ):
    d = {}
    for line in metadata.split('\n'):
        lineNum, data = line[2:].split(":")
        d[int(lineNum)] = data.split(" ")
    return d

def parse( path, canvas ):
    f = open(path, "rb")
    text = f.read()
    m = re_mediabox.search(text)
    assert(m)
    x,y,w,h = [int(item) for item in m.group(1).split(" ")]
    canvas.set_size(w,h)
    m2 = re_stream.search(text)
    assert(m2)

    m3 = re_metadata.search(text)
    if m3 is None:
        metadata = {}
    else: 
        metadata = parse_metadata(m3.group(1))
    parse_stream( text[m2.start():m2.end()], canvas, w, h, metadata )

if __name__ == '__main__':
    parse("test.pdf", DummyClass())
