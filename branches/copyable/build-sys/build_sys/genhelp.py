#!/usr/bin/env python3
# -*- coding: utf-8 -*-

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
import os
import sys
import codecs
import build_sys.util.parsecode as parsecode

# Section-title regexes
title1 = re.compile("^= (.*) =$")
title2 = re.compile("^== (.*) ==$")
title3 = re.compile("^=== (.*) ===$")

# Summary of a page
re_summary = re.compile('^summary\:"(.+)"$')

# Instruction to summarize child pages
re_summarize_children = re.compile('^child_summary$')

# Label for cross-referencing
label = re.compile("^label\:(.*?)$")

# Bullet points for bullet lists
bullet = re.compile("^\* (.*)$")

# Image filename from images/
re_image = re.compile(r"\\image\((.*?)\)")

# Image map description file
re_image_map = re.compile(r"\\imagemap\((.*?)\)")

# Image filename from graphics/
re_graphic = re.compile(r"\\graphic\((.*?)\)")

# Font styles
bold = re.compile("\*(.*?)\*")
italic = re.compile("\'(.*?)\'")

# Centered text
center = re.compile("/(.*?)/")

# Horizontal line
hr = re.compile("^---$")

# Reference to a label or page
re_ref = re.compile(r"\\ref\((.*?)\)")

# External reference
re_extref = re.compile(r"\\extref\((.*?)\)")

# Include Python code file
re_py_sample = re.compile(r"\\py_example\((.*)\)")

# A table row (with one or more cells)
table_row = re.compile("^\|\|.*\|\|$")

# Table style definition
table_style = re.compile("^tablestyle:([1-9])$")

# Table widths definition
table_widths = re.compile("^tablewidths:(.*)$")

re_withbind = re.compile("withbind\:(.*?);")
re_bind = re.compile("bind\:(.*?);")
re_bindlist = re.compile(r"^\\bindlist\(\)$")
re_configpath = re.compile(r"\\pyconfigpath")
re_py_methoddef = re.compile(r"^\\methoddef\((.*?)\)$")
re_py_getsetdef = re.compile(r"^\\getsetdef\((.*?)\)$")
# Verbatim text (ignore markup?)
verb_start = "{{{"
verb_end = "}}}"
g_table_style = 0
g_table_widths = []

# Even or odd table row, for styling rows
g_table_odd = False

# Page background
color_background = "#FAFDD5"
# Backgrounds for even and odd table rows
color_table_even = color_background
color_table_odd = "#F9FCC4"
color_link="#4D6DF3"

# Labels found in the parsed files, for cross-referencing
labels = {}

class missing_graphic:
    pass

def fail_with_message( description ):
    print("Error: " + description)
    sys.exit(1)

def match_title( line ):
    """Returns a match object if the line is matched by any title
    regex, otherwise None"""
    for num, title in enumerate( [title1, title2, title3]):
        match = title.match(line)
        if match:
            return "".join( [ item.to_html() for item in parse_rest(match.group(1)) ]), num + 1
    return None

def html_help_image(filename):
    return '<img src="images/%s"></img>' % filename

g_map_num = 0
def new_map_name():
    global g_map_num
    g_map_num += 1
    return "map%d" % g_map_num

def html_image_map(map_file_name, image_file_name):
    map_name = new_map_name()
    lines = codecs.open(_get_source_image_path(map_file_name)).read().decode("ascii")
    if lines.find('\r') != -1:
        raise ContentError("Non-unix endline (\\r) in %s" % map_file_name)

    res = '<img src="images/%s" usemap="#%s"></img>' % (image_file_name, map_name )
    res += '<map name="%s">' % map_name
    for line in lines.split('\n'):
        if len(line) != 0:
            coords, target = line.split(' ')
            target_label = labels.get(target)
            if target_label is None:
                raise(ContentError("Undefined label: %s in image map %s" % (target, map_file_name)))
            res += '<area SHAPE="rect" COORDS="%s" HREF="%s">' % (coords, target_label.url)
    res += '</map>'
    return res

def html_link(url, label=None):
    if label is None:
        label = url
    return '<a href="%s">%s</a>' % (url, label)

class ContentError(Exception):
    def __init__(self, error):
        Exception.__init__(self, error)


class Bullet:
    def __init__( self, items ):
        self.items = items

    def to_html( self, **kwArgs ):
        return '<li>%s</li>' % "".join( [item.to_html() for item in self.items])

    def __repr__( self ):
        return "Bullet()"

class PythonBindList:
    def to_html( self, **kwArgs ):
        return "<bindlist></bindlist>"
    def __repr__( self ):
        return "PythonBindList()"

def parse_methoddef_entries(s):
    re_methoddef_entry = re.compile(r"\{\s*\"(.*?)\"\s*,.*?,.*?,\s*\"(.*?)\"\s*\}\s*,", re.DOTALL)
    methods = {}
    for item in re.finditer(re_methoddef_entry, s):
        methods[item.group(1)] = item.group(2)

    # Find all forwards to Common_... and add the doc-strings
    common_entries = parsecode.parse_common_methoddef_entries(s)
    if len(common_entries) != 0:
        # Fixme: No need to recreate commonDict every time
        common_dict = parsecode.parse_py_common("../python/py-common.hh")
        name_to_doc= parsecode.methoddef_common_to_name_doc(common_entries, common_dict)
        methods.update(name_to_doc)

    return methods

def parse_methoddef(file_name, struct_name):
    """FIXME: Document"""

    re_methoddef = re.compile(r"static PyMethodDef ([a-zA-Z_]+)\[\]\s*=\s*\{(.*?)\};", re.DOTALL)
    cpp_file = open("../python/%s" % file_name)
    for item in re.finditer(re_methoddef, cpp_file.read()):
        if item.group(1) == struct_name:
            return parse_methoddef_entries(item.group(2))
    assert(False)

def _py_docstring_to_html(s):
    s = s.replace("\\n", "<br>")
    s = s.replace("\n", "")
    s = s.replace('"', '')
    return s

def get_public_functions(keys):
    return [m for m in keys if not m.startswith("int_")]

class PythonMethodDef:
    def __init__( self, rest ):
        self.file_name, self.struct_name = rest.split(",")
        self.methods = parse_methoddef(self.file_name, self.struct_name)

    def to_html( self, **kwArgs ):
        table = '<table border="0"><tr><td><b>Method</b></td><td><b>Description</b></td></tr>'

        for method in get_public_functions(sorted(self.methods.keys())):
            method_doc = _py_docstring_to_html(self.methods[method])
            table += '<tr><td valign="top">%s</td><td>%s</td></tr>' % (method, method_doc)
        table += '</table>'
        return table
    def __repr__(self):
        return "PythonMethodDef"

class PythonGetSetDef:
    def __init__( self, rest ):
        file_name = rest

        self.name_to_doc = parsecode.parse_PyGetSetDef(file_name.replace("\\","//"))

    def to_html( self, **kwArgs ):
        table = '<table border="0"><tr><td><b>Property</b></td><td><b>Description</b></td></tr>'

        for name in sorted(list(self.name_to_doc.keys())):
            doc = _py_docstring_to_html(self.name_to_doc[name])
            table += '<tr><td valign="top">%s</td><td>%s</td></tr>' % (name, doc)
        table += '</table>'
        return table
    def __repr__(self):
        return "PythonGetSetDef"

def _get_source_image_path( filename ):
    return os.path.join("../help/images", filename)

def _get_source_graphic_path( filename ):
    return os.path.join("../graphics/", filename)

class ConfigPath:
    def to_html(self, **kwArgs):
        return "<FAINTCONFIGPATH>"

class Image:
    _referenced_images = ["non-versioned",
                          "icon_extref.png",
                          "key_alt.png", # Fixme
                          "key_delete.png", # Fixme
                          ".svn",
                          ]
    def __init__( self, filename ):
        self.filename = filename
        Image._referenced_images.append(filename)
        relative_path = _get_source_image_path(filename)
        if not os.path.exists(relative_path):
            raise ContentError("Missing image: %s" % relative_path)
    def to_html( self, **kwArgs ):
        return html_help_image(self.filename)
    def __repr__( self ):
        return "Image(%s)" % self.filename

class ImageMap:
    def __init__( self, filename ):
        self.map_file_name = filename
        assert(self.map_file_name.endswith(".map"))
        self.image_file_name = filename[:-4] + ".png"
        Image._referenced_images.append(self.image_file_name)
        Image._referenced_images.append(self.map_file_name)
        imgRelPath = _get_source_image_path(self.image_file_name)
        if not os.path.exists(imgRelPath):
            raise ContentError("Missing image for image map: %s" % imgRelPath)
        imgMapRelPath = _get_source_image_path(self.map_file_name)
        if not os.path.exists(imgMapRelPath):
            raise ContentError("Missing image map: %s" % imgMapRelPath)

    def to_html(self, **kwArgs):
        return html_image_map(self.map_file_name, self.image_file_name)

    def __repr__( self ):
        return "ImageMap(%s)" % self.map_file_name

class PythonFunction:
    def __init__( self, signature, withBind=True ):
        parenPos = signature.find("(")
        assert(parenPos != -1)
        self.name = signature[:parenPos]
        self.args = signature[parenPos:]
        self.withBind = withBind
    def to_html( self, **kwArgs ):
        if self.withBind:
            return self._name_and_args() + " " + "<bind>%s</bind>" % self.name
        return self._name_and_args()

    def _name_and_args(self):
        return self.name + self.args

class PythonBind:
    """A Python function bind, rendering as the key-code"""
    def __init__(self, funcName):
        self.funcName = funcName

    def to_html( self, **kwArgs ):
        return "<bind>%s</bind>" % self.funcName

class Graphic:
    def __init__( self, filename ):
        self.filename = filename
        relative_path = _get_source_graphic_path(filename)
        if not os.path.exists(relative_path):
            raise ContentError("Missing graphic: %s" % relative_path)
    def to_html( self, **kwArgs ):
        return '<img src="../graphics/%s"></img>' % self.filename
    def __repr__( self ):
        return "Graphic(%s)" % self.filename

class Label:
    def __init__( self, label, title, url ):
        self.label = label
        self.title = title
        self.url = url

    def to_html( self, **kwArgs ):
        return '<a name="%s"></a>' % self.label

    def __repr__( self ):
        return "Label(%s)" % self.label


class Text:
    html_replacements = [("\\br", "<br>"),
                         ("->", "&rarr;"), # Ascii arrow as html-right arrow
                         (u"é", "&eacute;")]
    def __init__( self, text ):
        self.text = text
    def to_html( self, **kwArgs ):
        html_str = self.text
        for src, replacement in self.html_replacements:
            html_str = html_str.replace(src, replacement)

        return html_str
    def __repr__( self ):
        return "Text(...)"

class Page:
    def __init__( self, filename, sourcename, title, summary, items ):
        self.items = items
        self.title = title
        self.summary = summary
        self.filename = filename
        self.sourcename = sourcename

    def first_title( self ):
        for item in self.items:
            if item.__class__ == Title:
                return item.text
        return ""

class ChildSummary:
    def __init__(self):
        pass
    def to_html(self, **kwArgs ):
        return ""
    def __repr__( self ):
        return "ChildSummary"

class Title:
    def __init__( self, text, level ):
        self.text = text
        self.level = level
    def to_html( self, **kwArgs ):
        return "<h%d>%s</h%d>" % (self.level, self.text, self.level)
    def __repr__( self ):
        return "Title"

class Paragraph:
    def to_html( self, **kwArgs ):
        return "<p>"

class Tagged:
    def __init__( self, tag, content ):
        self.tag = tag
        self.content = content

    def to_html( self, **kwArgs ):
        html = "<%s>%s</%s>" % (self.tag, "".join( item.to_html() for item in parse_rest(self.content)), self.tag)
        return html

class Reference:
    def __init__( self, name, label=None ):
        self.name = name
        self.target_label = None
        self.custom_label = label

    def to_html( self, **kwArgs ):
        if self.target_label is None:
            self.target_label = labels.get( self.name, None )
        if self.target_label is None:
            raise(ContentError("Undefined label: %s" % self.name))
        if self.custom_label is not None:
            title = self.custom_label
        else:
            title = self.target_label.title
        return '<a href="%s">%s</a>' % (self.target_label.url, title )

class ExternalReference:
    def __init__( self, url ):
        if not url.startswith("http://"):
            raise ContentError('External url not prefixed by http://. "%s"' % url)
        self.url = url

    def to_html( self, **kwArgs ):
        return html_help_image("icon_extref.png") + html_link(self.url)

class PythonSample:
    """Insert and format Python code sample"""

    def __init__( self, filename ):
        self.filename = filename

    def _py_line( self, l ):
        return l.replace(" ", "&nbsp;").replace("<", "&lt;").replace(">", "&gt;").replace("##", "")

    def to_html( self, **kwArgs ):
        text = '<font face="courier">'
        f = open('../help/example_py/%s' % self.filename)
        lines = [l.rstrip() for l in f.readlines()]
        try:
            lines = lines[lines.index("#start") + 1:]
        except ValueError:
            raise ContentError('PythonSample %s missing #start-comment' % self.filename)
        text += '<br>'.join([self._py_line(l) for l in lines])
        text += '</font>'
        return text

def tbl_str_width(num, widths):
    """Returns the width-argument for an html table cell using the width from
    the list of widths.
    Returns width="[some number]" or the empty-string if no width is
    specified for this num"""
    if num >= len(widths):
        return ""
    return ' width="%s"' % widths[num]

class Table:
    def __init__( self, line ):
        self.cells = line.split("||")[1:-1]

    def to_html( self, **kwArgs ):
        fmt_cells = ["".join( [ item2.to_html() for item2 in item] ) for item in [ parse_rest(item) for item in self.cells ] ]
        row = kwArgs["row"]
        style = kwArgs["tablestyle"]
        tablewidths = kwArgs["tablewidths"]
        widths = []
        for num, cell in enumerate(fmt_cells):
            if num < len(tablewidths):
                widths.append(tablewidths[num])

        html = "<tr>"
        for num, cell in enumerate(fmt_cells):
            if style == 3 and row == 0:
                html = html + '<td bgcolor="#F5E49C"%s>%s</td>' % (tbl_str_width(num, widths), cell )
            elif row % 2 == 0 and ( ( style == 3 and row != 0 ) or style == 2 ):
                html = html + '<td bgcolor="%s"%s>%s</td>' % ( color_table_odd, tbl_str_width(num, widths), cell )
            else:
                html = html + '<td%s>%s</td>' % (tbl_str_width(num,widths), cell)
        html = html + "</tr>"
        return html

class TableStyle:
    def __init__( self, style ):
        self.style = int(style)
    def to_html( self, **kwArgs ):
        return ""

class Instruction:
    """A configuration item for the output"""
    def __init__( self, name, values ):
        self.name = name
        self.values = values
    def to_html( self, **kwArgs ):
        return ""

class Header:
    def __init__( self, targetname, link ):
        self.targetname = targetname
        self.link = link
        self.title = ""

    def set_title( self, title ):
        self.title = title

    def to_html( self, **kwArgs ):
        return '<a href="%s">Previous</a> - %s<hr>' % (self.link, self.title )

class Footer:
    def __init__( self, targetname, link ):
        self.targetname = targetname
        self.link = link
        self.title = ""

    def set_title( self, title ):
        self.title = title

    def to_html( self, **kwArgs ):
        return '<hr><a href="%s">Next</a> - %s' % (self.link, self.title )

class HR:
    def to_html(self, **kwArgs):
        return '<hr>'

def list_split( regex, data, marker ):
    ret = []
    for item in data:
        if item.startswith("["):
            ret.append(item)
            continue
        for num, splat in enumerate(regex.split(item)):
            if (num + 1) % 2 == 0:
                ret.append( marker + splat )
            elif len(splat) > 0:
                ret.append( splat )
    return ret

def to_object( txt ):
    if txt.startswith("[R]"):
        content = txt[3:]
        content = content.split(',')
        if len(content) == 1:
            return Reference( content[0])
        else:
            return Reference( content[0], content[1])

    if txt.startswith("[IMGMAP]"):
        return ImageMap( txt[8:] )
    if txt.startswith("[IMG]"):
        return Image( txt[5:] )
    if txt.startswith("[GRAPHIC]"):
        return Graphic( txt[9:] )
    if txt.startswith("[I]"):
        return Tagged( 'i', txt[3:] )
    if txt.startswith("[B]"):
        return Tagged( 'b', txt[3:] )
    if txt.startswith("[C]"):
        return Tagged( 'center', txt[3:] )
    if txt.startswith("[WBND]"):
        return PythonFunction(txt[6:], True)
    if txt.startswith("[BINDLIST]"):
        return PythonBindList()
    if txt.startswith("[CONFIGPATH]"):
        return ConfigPath()
    if txt.startswith("[BIND]"):
        return PythonBind(txt[6:])
    if txt.startswith("[METHODDEF]"):
        return PythonMethodDef(txt[11:])
    if txt.startswith("[GETSETDEF]"):
        return PythonGetSetDef(txt[11:])

    if txt.startswith("[EXTREF]"):
        return ExternalReference(txt[8:])
    if txt.startswith("[PY_SAMPLE]"):
        return PythonSample(txt[11:])
    return Text(txt)

def parse_rest( rest ):
    if rest.__class__ != [].__class__:
        rest = [rest]
    rest = list_split( re_ref, rest, "[R]")
    rest = list_split( re_image, rest, "[IMG]")
    rest = list_split( re_image_map, rest, "[IMGMAP]")
    rest = list_split( re_graphic, rest, "[GRAPHIC]")
    rest = list_split( re_withbind, rest, "[WBND]")
    rest = list_split( re_bind, rest, "[BIND]")
    rest = list_split( re_extref, rest, "[EXTREF]")
    rest = list_split( re_py_sample, rest, "[PY_SAMPLE]")
    rest = list_split( re_bindlist, rest, "[BINDLIST]")
    rest = list_split( re_configpath, rest, "[CONFIGPATH]")
    rest = list_split( italic, rest, "[I]")
    rest = list_split( bold, rest, "[B]")
    rest = list_split( center, rest, "[C]")
    return [to_object(item) for item in rest ]

def parse_table_widths( s ):
    """Returns a list of the widths in the string
    for group(1) of a  matched table_widths regex"""
    return [ item.strip() for item in s.split(",") ]

def parse_file( filename, prev, next, labels ):
    src = codecs.open(filename, 'r', 'utf-8')
    lines = src.readlines()
    src.close()

    if not lines[0].startswith("page:"):
        fail_with_message('First line does not start with "page:" (%s)')

    for line in lines:
        if line.find("\r") != -1:
            fail_with_message("Non-unix endline (\\r) in %s" % filename)

    page_title = lines[0].split(":")[1].strip()[1:-1]
    target_filename = os.path.split(filename)[-1].replace(".txt", ".html") # Fixme: Don't determine output format already
    summary = ""

    doc = [] # The list of items in the page
    if prev is not None:
        doc.append(Header( prev, prev.replace(".txt", ".html") ) )  # Fixme: Don't determine output format already
    in_list = False

    verbatim = False
    last_title = None

    for line in lines[1:]:
        if verbatim:
            if line.strip() == "}}}":
                verbatim = False
            else:
                doc.append(Text(line))
            continue

        if line.strip() == "{{{":
            verbatim = True
            continue

        m = re_summary.match( line )
        if m is not None:
            summary = m.group(1)
            continue

        m = re_summarize_children.match(line)
        if m is not None:
            doc.append( ChildSummary() )
            continue

        m = hr.match( line )
        if m is not None:
            doc.append( HR() )
            continue
        m = match_title( line )
        if m is not None:
            last_title = Title( m[0], m[1] )
            doc.append( last_title )
            continue
        m = bullet.match( line )
        if m is not None:
            doc.append( Bullet( parse_rest( m.group(1) ) ) )
            continue

        m = label.match( line )
        if m is not None:
            name = m.group(1).strip()
            if last_title is not None:
                labelTitle = last_title.text
            else:
                fail_with_message("No title for " + filename)
            lbl = Label( name, labelTitle, target_filename + "#" + name  )
            doc.append( lbl )
            labels[name] = lbl
            continue

        m = table_style.match( line )
        if m is not None:
            doc.append( TableStyle( m.group(1)) )
            continue

        m = table_widths.match( line )
        if m is not None:
            doc.append( Instruction( "tablewidths", parse_table_widths( m.group(1)) ) )
            continue

        if line.strip() == "":
            doc.append( Paragraph() )
            continue

        m = table_row.match(line)
        if m is not None:
            doc.append( Table( line ) )
            continue

        content = [line]
        content = list_split( re_extref, content, "[EXTREF]")
        content = list_split( re_py_sample, content, "[PY_SAMPLE]")
        content = list_split( center, content, "[C]")
        content = list_split( re_image, content,"[IMG]")
        content = list_split( re_image_map, content,"[IMGMAP]")
        content = list_split( re_withbind, content, "[WBND]")
        content = list_split( re_bind, content, "[BIND]")
        content = list_split( re_bindlist, content, "[BINDLIST]")
        content = list_split( re_configpath, content, "[CONFIGPATH]")
        content = list_split( re_py_methoddef, content, "[METHODDEF]")
        content = list_split( re_py_getsetdef, content, "[GETSETDEF]")
        content = list_split( re_graphic, content,"[GRAPHIC]")
        content = list_split( bold, content, "[B]")
        content = list_split( italic, content, "[I]")
        content = list_split( re_ref, content, "[R]")

        for item in content:
            if item.startswith("[IMG]"):
                doc.append( Image( item[5:] ) )
            elif item.startswith("[GRAPHIC]"):
                doc.append( Graphic( item[9:] ) )
            elif item.startswith("[B]"):
                doc.append( Tagged( "b", item[3:] ) )
            elif item.startswith("[U]"):
                doc.append( Tagged( "u", item[3:] ) )
            elif item.startswith("[I]"):
                doc.append( Tagged( "i", item[3:] ) )
            elif item.startswith("[C]"):
                doc.append( Tagged( "center", item[3:] ) )
            else:
                doc.append( to_object( item ) )

    if next is not None:
        doc.append(Footer( next, next.replace(".txt", ".html") ) )
    else:
        doc.append(HR())
    return Page(target_filename, os.path.split(filename)[-1], page_title, summary, doc)

def write_child_summary( out, pages, childPages ):
    out.write('<table border="0" cellpadding="5">')
    for child in childPages:
        out.write('<tr><td><a href="%s">%s</a></td><td width="10"></td><td>%s</td></tr>' % (pages[child].filename, pages[child].title, pages[child].summary))
    out.write('</table>')

def write( outDir, pages, pageHierarchy, labels ):
    global g_table_style
    global g_table_widths
    row_num = 0
    for sourcename in pages:
        g_table_style = 0
        g_table_widths = []
        page = pages[sourcename]
        outfile = open(os.path.join(outDir, page.filename), 'w')
        outfile.write('<html><head><title>%s</title></head><body bgcolor="%s" leftmargin="50" link="%s">' % (page.title, color_background, color_link))
        in_table = False
        in_list = False
        for item in page.items:
            if item.__class__ == TableStyle:
                g_table_style = item.style
            if item.__class__ == Instruction:
                if item.name == "tablewidths":
                    g_table_widths = item.values
            if item.__class__ == Table:
                if not in_table:
                    row_num = 0
                    if g_table_style == 3 or g_table_style == 2:
                        outfile.write('<table border="0" cellpadding="5" width="80%">' )
                    else:
                        outfile.write('<table border="0" cellpadding="5">' )

                    in_table = True
                    global g_table_odd
                    g_table_odd = not g_table_odd
                else:
                    row_num += 1
            elif in_table:
                outfile.write('</table>')
                row_num = 0
                in_table = False

            if item.__class__ == Bullet:
                if not in_list:
                    in_list = True
                    outfile.write("<ul>\n")
            elif in_list:
                outfile.write("</ul>\n")
                in_list = False
            if item.__class__ == ChildSummary:
                write_child_summary(outfile, pages, pageHierarchy[sourcename])
            try:
                outfile.write( item.to_html( tablestyle=g_table_style, tablewidths=g_table_widths,row=row_num ) )
            except ContentError as e:
                fail_with_message(str(e) + " (%s)" % page.sourcename)
        outfile.write("</body></html>")

def _need_generate(srcRoot, dstRoot, sources):
    dst_contents = os.path.join(dstRoot, "contents.dat")
    src_contents = os.path.join(srcRoot, "contents.txt")
    if not os.path.isfile(dst_contents) or os.path.getmtime(dst_contents) < os.path.getmtime(src_contents):
        return True

    for sourceFile, page in [ (os.path.join(srcRoot, page), page) for page in sources]:
        targetFile = os.path.join(dstRoot, page.replace(".txt", ".html"))
        if not os.path.isfile(targetFile) or os.path.getmtime(targetFile) < os.path.getmtime(sourceFile):
            # An html page is older than its source text or does not exist
            return True
    return False


def get_files(contentLines):
    lines = [line.strip() for line in contentLines]
    return [line.replace(">", "") for line in lines if not len(line) == 0 ]

def read_contents_source(contents_path):
    f = open(contents_path)
    lines = [line.strip() for line in f.readlines()]
    return [line for line in lines if not len(line) == 0]

def write_contents(contentLines, dst_path, pages):
    contents_file = open(dst_path, 'w')
    for line in contentLines:
        if line.startswith(">"):
            contents_file.write(">")
        pageInfo = pages[line.replace(">","")]
        contents_file.write("%s;%s\n" % (pageInfo.title, pageInfo.filename))

def parse_contents(contentLines):
    pages = {}
    mainPage = ""
    for line in contentLines:
        line = line.strip()
        if not line.startswith(">"):
            mainPage = line
            pages[line] = []
        else:
            pages[line[1:]] = []
            pages[mainPage].append(line[1:])
    return pages

def find_unused_pages( referencedPages, sourceRoot ):
    allPages = [ file for file in os.listdir(sourceRoot) if file.endswith(".txt") and file != "contents.txt"]
    return [page for page in allPages if page not in referencedPages]

def find_unused_images( imageRoot, referenced_images ):
    all_images = [image for image in os.listdir(imageRoot)]
    return [image for image in all_images if image not in referenced_images]

def run(force=False):
    helpRoot = "../help"
    helpSource = "../help/source"

    # Retrieve what files to parse as pages from the contents file
    content = read_contents_source(os.path.join(helpSource, "contents.txt"))
    source_pages = get_files(content)
    unused_pages = find_unused_pages(source_pages, helpSource)
    if len(unused_pages) != 0:
        fail_with_message("Unreferenced pages: %s" % ", ".join(unused_pages))

    pageHierarchy = parse_contents(content)
    if not force and not _need_generate(helpSource, helpRoot, source_pages):
        print(" Up to date.")
        return

    pages = {}
    for num, page in enumerate(source_pages):
        if num == 0:
            # No previous page for the first page
            prev = None
        else:
            prev = source_pages[num - 1]
        if num == len(source_pages) -1:
            # No next page for the last page
            next = None
        else:
            next = source_pages[num + 1]

        try:
            pages[page] = parse_file(os.path.join(helpSource, page), prev, next, labels)
        except ContentError as e:
            fail_with_message(str(e) + " (%s)" % page)
    for source_name in source_pages:
        page = pages[source_name]
        labels[ source_name ] = Label( source_name, page.title, page.filename )

    for sourcepage in pages:
        last = pages[sourcepage].items[-1]
        if last.__class__ == Footer:
            last.set_title( pages[last.targetname].first_title() )

        first = pages[sourcepage].items[0]
        if first.__class__ == Header:
            first.set_title( pages[first.targetname].first_title() )

    # Write all pages
    write( helpRoot, pages, pageHierarchy, labels )
    # Write the contents (for the tree view)
    write_contents( content, os.path.join(helpRoot, "contents.dat"), pages )

    unused_images = find_unused_images(os.path.join(helpRoot, "images"), Image._referenced_images)
    if len(unused_images) != 0:
        fail_with_message("Unused images: %s" % ", ".join(unused_images))

if __name__ == '__main__':
    import sys
    force = "--force" in sys.argv
    run(force)
