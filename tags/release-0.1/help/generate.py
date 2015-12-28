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

import re
title1 = re.compile("^= (.*) =$")
title2 = re.compile("^== (.*) ==$")
title3 = re.compile("^=== (.*) ===$")
label = re.compile("^label\:(.*?)$")
bullet = re.compile("^\* (.*)$")
image = re.compile("(image\:.*?\s)")
image_name = re.compile("image\:(.*?)\s")
bold = re.compile("\*(.*?)\*")
italic = re.compile("\'(.*?)\'")
center = re.compile("/(.*?)/")
hr = re.compile("^---$")
ref = re.compile("-(.*?)-")
table_row = re.compile("^\|\|.*\|\|$")
table_style = re.compile("^tablestyle:([1-9])$")
table_widths = re.compile("^tablewidths:(.*)$")
verb_start = "{{{"
verb_end = "}}}"
g_table_style = 0
g_table_widths = []
g_table_odd = False

labels = {}

def match_title( line ):
    for num, title in enumerate( [title1, title2, title3]):
        match = title.match(line)
        if match:
            return "".join( [ item.to_html() for item in parse_rest(match.group(1)) ]), num + 1
    return None

def match_image( line, outfile ):
    match = image.search( line )
    if match:
        outfile.write('<img src="%s"></img>' % match.group(1))
        return True
    return False

class Bullet:
    def __init__( self, items ):
        self.items = items

    def to_html( self, **kwArgs ):
        return '<li>%s</li>' % "".join( [item.to_html() for item in self.items])

    def __repr__( self ):
        return "Bullet()"

class Image:
    def __init__( self, path ):
        self.path = path
    def to_html( self, **kwArgs ):
        return '<img src="images/%s"></img> ' % self.path

    def __repr__( self ):
        return "Image(%s)" % self.path

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
    def __init__( self, text ):
        self.text = text

    def to_html( self, **kwArgs ):
        return self.text.replace("\\", "<br>")
    def __repr__( self ):
        return "Text(...)"

class Page:
    def __init__( self, filename, title, items ):
        self.items = items
        self.title = title
        self.filename = filename

    def first_title( self ):
        for item in self.items:
            if item.__class__ == Title:
                return item.text
        return ""

class Title:
    def __init__( self, text, level ):
        self.text = text
        self.level = level
    def to_html( self, **kwArgs ):
        return "<h%d>%s</h%d>" % (self.level, self.text, self.level)
    def __repr__( self ):
        return "Title"

class Linebreak:
    def to_html( self, **kwArgs ):
        return "<br>"

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
            return self.name + "??"
        if self.custom_label is not None:
            title = self.custom_label
        else:
            title = self.target_label.title
        return '<a href="%s">%s</a>' % (self.target_label.url, title )


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
            else:
                widths.append("*")

        html = "<tr>"
        for num, cell in enumerate(fmt_cells):
            if style == 3 and row == 0:
                html = html + '<td bgcolor="#F5E49C" width="%s">' % widths[num] + cell + '</td>'
            elif row % 2 == 0 and ( ( style == 3 and row != 0 ) or style == 2 ):
                html = html + '<td bgcolor="#E5E0E0" width="%s">%s</td>' % ( widths[num], cell )
            else:
                html = html + '<td width="%s">%s</td>' % (widths[num], cell)
        html = html + "</tr>"
        return html

class TableStyle:
    def __init__( self, style ):
        self.style = int(style)
    def to_html( self, **kwArgs ):
        return ""

class Instruction:
    def __init__( self, name, values ):
        self.name = name
        self.values = values
    def to_html( self, **kwArgs ):
        return ""


class Footer:
    def __init__( self, targetname, link ):
        self.targetname = targetname
        self.link = link
        self.title = ""

    def set_title( self, title ):
        self.title = title

    def to_html( self, **kwArgs ):
        return '<hr><a href="%s">Next</a> - %s' % (self.link, self.title )

class Header:
    def __init__( self, targetname, link ):
        self.targetname = targetname
        self.link = link
        self.title = ""

    def set_title( self, title ):
        self.title = title

    def to_html( self, **kwArgs ):
        return '<a href="%s">Previous</a> - %s<hr>' % (self.link, self.title )

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
    if txt.startswith("[IMG]"):
        return Image( txt[5:] )
    if txt.startswith("[I]"):
        return Tagged( 'i', txt[3:] )
    if txt.startswith("[B]"):
        return Tagged( 'b', txt[3:] )
    if txt.startswith("[C]"):
        return Tagged( 'center', txt[3:] )
    return Text(txt)


def parse_rest( rest ):
    if rest.__class__ != [].__class__:
        rest = [rest]
    rest = list_split( ref, rest, "[R]")
    rest = list_split( image_name, rest, "[IMG]")
    rest = list_split( italic, rest, "[I]")
    rest = list_split( bold, rest, "[B]")
    rest = list_split( center, rest, "[C]")

    return [to_object(item) for item in rest ]

def parse_table_widths( s ):
    return [ item.strip() for item in s.split(",") ]


def parse_file( filename, prev, next, labels ):
    src = open(filename)
    lines = src.readlines()
    src.close()
    page = lines[0]
    target_filename = page.split(":")[1].strip()
    title = target_filename.split('"')[1]
    target_filename = target_filename.split(' ')[0]

    doc = []
    if prev is not None:
        doc.append(Header( prev, prev.replace(".txt", ".html") ) )
    in_list = False

    verbatim = False
    last_title = None

    for line in lines[1:]:
        if verbatim:
            if line.strip() == "}}}":
                verbatim = False
                continue
            else:
                doc.append(Text(line))
                continue

        if line.strip() == "{{{":
            verbatim = True
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
                labelTitle = title
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
        content = list_split( center, content, "[C]")
        content = list_split( image_name, content,"[IMG]")
        content = list_split( bold, content, "[B]")
        content = list_split( italic, content, "[I]")
        content = list_split( ref, content, "[R]")

        for item in content:
            if item.startswith("[IMG]"):
                doc.append( Image( item[5:] ) )
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
    return Page(target_filename, title, doc)

def write( pages, labels ):
    global g_table_style
    global g_table_widths
    row_num = 0
    for sourcename in pages:
        g_table_style = 0
        g_table_widths = []
        page = pages[sourcename]
        outfile = open(page.filename, 'w')
        outfile.write('<html><head><title>%s</title></head><body bgcolor="#F8F8F0" leftmargin="50">' % page.title)
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

            outfile.write( item.to_html( tablestyle=g_table_style, tablewidths=g_table_widths,row=row_num ) )
        outfile.write("</body></html>")

if __name__ == '__main__':
    source_pages = ["main.txt", "introduction.txt", "basic_concepts.txt", "tools.txt", "objects.txt", "scripting_intro.txt", "ref_menu.txt", "ref_shortcuts.txt", "ref_python.txt", "formats.txt", "tutorials.txt", "tut_flag.txt", "tips.txt" ]
    pages = {}
    for num, page in enumerate(source_pages):
        if num == 0:
            prev = None
        else:
            prev = source_pages[num - 1]
        if num == len(source_pages) -1:
            next = None
        else:
            next = source_pages[num + 1]

        pages[page] = parse_file(page, prev, next, labels)

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

    write( pages, labels )
    contents_file = open( "contents.hhc", 'w')
    contents_file.write("<ul>")

    for source_name in source_pages[1:]:
        if source_name == "main.txt":
            continue
        page = pages[source_name]
        contents_file.write('<li> <object type="text/sitemap">\n')
        contents_file.write('     <param name="Name" value="%s">\n' % page.title )
        contents_file.write('     <param name="ID" value="0">\n')
        contents_file.write('     <param name="Local" value="%s">\n' % page.filename )
        contents_file.write('     </object>\n')

    contents_file.write("</ul>\n")
    contents_file.close()

    index_file = open("index.hhk", 'w')
    index_file.write("<ul>\n")
    index_file.write("</ul>\n")
    index_file.close()

    header_file = open("header.hhp", 'w')
    header_file.write("Contents file=contents.hhc\n")
    header_file.write("Index file=index.hhk\n")
    header_file.write("Title=Help topics\n")
    header_file.write("Default topic=main.html\n")
    header_file.close()
