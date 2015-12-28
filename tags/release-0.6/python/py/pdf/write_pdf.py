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

import ifaint
from py.pdf.document import Document
from py.pdf.stream import Stream
from py.pdf.convert_to_pdf import object_to_stream
import math
from math import cos, sin

def write( path, canvas ):
    objects = canvas.get_objects()
    faint_size = canvas.get_size()
    doc_w = faint_size[0]
    doc_h = faint_size[1]
    scale_x = faint_size[0] / doc_w
    scale_y = faint_size[1] / doc_h

    stream = Stream()
    meta = []
    for object in objects:
        objMeta = object_to_stream( stream, object, doc_h, scale_x, scale_y )
        meta.extend(objMeta)

    doc = Document()
    page_id = doc.add_page( doc_w, doc_h )
    doc.add_comment("faint meta-data")
    for item in meta:
        doc.add_comment(item)
    doc.add_comment("end of faint meta-data")
    doc.add_stream( stream, page_id )

    f = open( path, 'wb' )
    try:
        f.write( str(doc) )
    finally:
        f.close()
