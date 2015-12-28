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


# Parameters for gen_settinterface.py
# The keys are the Faint setting identifiers.
# The contents of the value tuples depend on their first item:
#
# int: the rest specify a function which takes an integer argument
#   item 1: The range, as a tuple (low, high)
#   item 2: The core of the python-accessible function name
#
# stringtoint: the rest specify a function which convert a string to a C++ integer
# (or typically, enumerator)
#   item 1: A map of strings specified in the call in Python to a C++ variable name or literal
#   item 2: The core of the python-accessible function name

shorthand = 1

class CppEnum:
    """Contains the fields for creating a C++-enum for an
    IntSetting."""
    def __init__( self, name, entries ):
        self.name = name
        self.entries = entries

    def get_name(self):
        return self.name

class Int:
    def __init__(self, cpp_name, py_name, pretty_name, min_value, max_value, doc_str):
        self.cpp_name = cpp_name
        self.py_name = py_name
        self.pretty_name = pretty_name
        self.min_value = min_value
        self.max_value = max_value
        self.doc_str = doc_str
        self.cpp_type = "IntSetting"

    def get_type(self): # For backwards compat. with gencpp.py
        return "int"

class Float:
    def __init__(self, cpp_name, py_name, pretty_name, min_value, max_value, doc_str):
        self.cpp_name = cpp_name
        self.py_name = py_name
        self.pretty_name = pretty_name
        self.min_value = min_value
        self.max_value = max_value
        self.doc_str = doc_str
        self.cpp_type = "FloatSetting"

    def get_type(self): # For backwards compat. with gencpp.py
        return "float"

class StringToInt:
    def __init__(self, cpp_name, py_name, pretty_name, py_to_cpp_map, doc_str, cpp_enum):
        self.cpp_name = cpp_name
        self.py_name = py_name
        self.pretty_name = pretty_name
        self.py_to_cpp_map = py_to_cpp_map
        self.doc_str = doc_str
        self.cpp_enum = cpp_enum
        self.cpp_type = "EnumSetting<%s>" % self.cpp_enum.name

    def get_type(self): # For backwards compat. with gencpp.py
        return "stringtoint"

class Color:
    def __init__(self, cpp_name, py_name, pretty_name, doc_str):
        self.cpp_name = cpp_name
        self.py_name = py_name
        self.pretty_name = pretty_name
        self.doc_str = doc_str
        self.cpp_type = "ColorSetting"
    def get_type(self): # For backwards compat. with gencpp.py
        return "color"

class Bool:
    def __init__(self, cpp_name, py_name, pretty_name, doc_str):
        self.cpp_name = cpp_name
        self.py_name = py_name
        self.pretty_name = pretty_name
        self.doc_str = doc_str
        self.cpp_type = "BoolSetting"
    def get_type(self): # For backwards compat. with gencpp.py
        return "bool"

class String:
    def __init__(self, cpp_name, py_name, pretty_name, doc_str):
        self.cpp_name = cpp_name
        self.py_name = py_name
        self.pretty_name = pretty_name
        self.doc_str = doc_str
        self.cpp_type = "StrSetting"
    def get_type(self): # For backwards compat. with gencpp.py
        return "string"

setters_and_getters = {
    "ts_LineArrowhead" : StringToInt(
        cpp_name="ts_LineArrowhead",
        py_name="arrow",
        pretty_name="Arrowhead",
        py_to_cpp_map={
            "none" : "LineArrowhead::NONE",
            "front" : "LineArrowhead::FRONT",
            "back" : "LineArrowhead::BACK",
            "both" : "LineArrowhead::BOTH",
            "f" : ("LineArrowhead::FRONT", shorthand),
            "b" : ("LineArrowhead::BACK", shorthand),
            "fb" : ("LineArrowhead::BOTH", shorthand)},
        doc_str="Arrowhead setting.\\nf=front, b=back, fb=both",
        cpp_enum=CppEnum("LineArrowhead", entries=("NONE", "FRONT", "BACK", "BOTH"))
        ),

    "ts_BrushSize" : Int(
        cpp_name="ts_BrushSize",
        py_name="brushsize",
        pretty_name="Brush Size",
        min_value=1, max_value=255,
        doc_str="Brush size (1-255)"),

    "ts_BrushShape" : StringToInt(
        cpp_name="ts_BrushShape",
        py_name="brushshape",
        pretty_name="Brush Shape",
        py_to_cpp_map={"square" : "BrushShape::SQUARE",
                       "circle" : "BrushShape::CIRCLE",
                       "experimental" : "BrushShape::EXPERIMENTAL"},
        doc_str="Brush shape setting (square or circle)",
        cpp_enum=CppEnum("BrushShape", entries=("SQUARE", "CIRCLE", "EXPERIMENTAL"))),

    # Fixme: Using int for now, This should use a special setting type, and a
    # Python class for filters.
    "ts_Filter" : Int(
        cpp_name="ts_Filter",
        py_name="filter",
        pretty_name="Filter",
        min_value=0, max_value=6,
        doc_str="Filter (0-6)"),

    "ts_LineCap" : StringToInt(
        cpp_name="ts_LineCap",
        py_name="cap",
        pretty_name="Line Cap",
        py_to_cpp_map={"round" : "LineCap::ROUND",
                       "r" : ("LineCap::ROUND",shorthand),
                       "flat": "LineCap::BUTT",
                       "f": ("LineCap::BUTT",shorthand)},
        doc_str="Line cap setting (line ends).\\nr=round, f: flat",
        cpp_enum=CppEnum("LineCap", entries=("BUTT", "ROUND"))),

    "ts_LineJoin" : StringToInt(
        cpp_name="ts_LineJoin",
        py_name="join",
        pretty_name="Line Join",
        py_to_cpp_map={"round" : "LineJoin::ROUND",
                       "bevel": "LineJoin::BEVEL",
                       "miter": "LineJoin::MITER"},
        doc_str="Line join setting (line connections).\\n('round', 'bevel' or 'miter')",
        cpp_enum=CppEnum("LineJoin", entries=("MITER", "BEVEL", "ROUND"))),

    "ts_FillStyle" : StringToInt(
        cpp_name="ts_FillStyle",
        py_name="fillstyle",
        pretty_name="Fill Style",
        py_to_cpp_map={"border" : "FillStyle::BORDER",
                       "b" : ("FillStyle::BORDER",shorthand),
                       "fill" : "FillStyle::FILL",
                       "f" : ("FillStyle::FILL",shorthand),
                       "border+fill" : "FillStyle::BORDER_AND_FILL",
                       "bf" : ("FillStyle::BORDER_AND_FILL",shorthand),
                       "none" : "FillStyle::NONE" },
        doc_str="Fill style\\nb: border, f: fill, bf: border and fill - otherwise none.",
        cpp_enum=CppEnum("FillStyle", entries=("BORDER", "BORDER_AND_FILL", "FILL", "NONE"))),

    "ts_LineWidth" : Float(
        cpp_name="ts_LineWidth",
        py_name="linewidth",
        pretty_name="Line Width",
        min_value=0,
        max_value=255,
        doc_str="Line width(0,255)"),

    "ts_LineStyle" : StringToInt(
        cpp_name="ts_LineStyle",
        py_name="linestyle",
        pretty_name="Line Style",
        py_to_cpp_map={"solid" : "LineStyle::SOLID",
                       "s" : ("LineStyle::SOLID",shorthand),
                       "long_dash" : "LineStyle::LONG_DASH",
                       "ld" : ("LineStyle::LONG_DASH",shorthand)},
        doc_str="Line style\\ns: solid, ld: long-dash",
        cpp_enum=CppEnum("LineStyle", entries=("SOLID", "LONG_DASH"))),

    "ts_BackgroundStyle" : StringToInt(
        cpp_name="ts_BackgroundStyle",
        py_name="bgstyle",
        pretty_name="Background Style",
        py_to_cpp_map={"masked" : "BackgroundStyle::MASKED",
                       "m" : ("BackgroundStyle::MASKED",shorthand),
                       "opaque" : "BackgroundStyle::SOLID",
                       "o" : ("BackgroundStyle::SOLID",shorthand) },
        doc_str="Background style:\\nm: masked background, o: opaque background.",
        cpp_enum=CppEnum("BackgroundStyle", entries=("MASKED", "SOLID"))),

    "ts_FontSize" : Int(
        cpp_name="ts_FontSize",
        py_name="fontsize",
        pretty_name="Font Size",
        min_value=1,
        max_value=255,
        doc_str="Font size (1-255)."),


    "ts_PointType" : StringToInt(
        cpp_name="ts_PointType",
        py_name="pointtype",
        pretty_name="Point Type",
        py_to_cpp_map={"line" : "PointType::LINE",
                       "curve" : "PointType::CURVE"},
        doc_str='The type of new points for the Path tool, line or curve.',
        cpp_enum=CppEnum("PointType", entries=("LINE", "CURVE"))),

    "ts_LayerStyle" : StringToInt(
        cpp_name="ts_LayerStyle",
        py_name="layerstyle",
        pretty_name="Layer Style",
        py_to_cpp_map={"raster" : "Layer::RASTER",
                       "object" : "Layer::OBJECT",
                       "r" : ("Layer::RASTER",shorthand),
                       "o" : ("Layer::OBJECT",shorthand)},
        doc_str="Layer choice\\nr: raster, o: object",
        cpp_enum=CppEnum("Layer", entries=("RASTER", "OBJECT"))),

    "ts_AlignedResize" : Bool(cpp_name="ts_AlignedResize", py_name="alignedresize", pretty_name="Aligned Resize", doc_str="Axis-aligned scaling?"),

    "ts_ClosedPath" : Bool(cpp_name="ts_ClosedPath", py_name="closedpath", pretty_name="Closed Path", doc_str="Closed path?"),

    "ts_Fg" : Color("ts_Fg", "fg", "Foreground Color", "Foreground color."),
    "ts_Bg" : Color("ts_Bg", "bg", "Background Color", "Background color."),
    "ts_FontBold" : Bool(cpp_name="ts_FontBold", py_name="fontbold", pretty_name="Bold Font", doc_str="Bold font?" ),
    "ts_FontItalic" : Bool(cpp_name="ts_FontItalic", py_name="fontitalic", pretty_name="Italic Font", doc_str="Italic font?" ),
    "ts_SwapColors" : Bool(cpp_name="ts_SwapColors", py_name="swapcolors", pretty_name="Swap Colors", doc_str="Swap foreground and background colors?" ),
    "ts_AntiAlias" : Bool(cpp_name="ts_AntiAlias", py_name="antialias", pretty_name="Anti Aliasing", doc_str="Anti-aliasing?"),
    "ts_EditPoints" : Bool(cpp_name="ts_EditPoints", py_name="editpoints", pretty_name="Edit Points", doc_str="Point-editing?"),
    "ts_FontFace" : String(cpp_name="ts_FontFace", py_name="fontface", pretty_name="Font Face", doc_str="Font face string."),
    "ts_AlphaBlending" : Bool(cpp_name="ts_AlphaBlending", py_name="alphablending", pretty_name="Alpha Blending", doc_str="Alpha blending?"),
    "ts_PolyLine" : Bool(cpp_name="ts_PolyLine", py_name="polyline", pretty_name="Poly-Lines", doc_str="Create poly-lines?"),
    "ts_BoundedText" : Bool(cpp_name="ts_BoundedText", py_name="bounded", pretty_name="Bounded Text", doc_str="True if the text is bounded by a rectangle"),
    "ts_HorizontalAlign" : StringToInt(
        cpp_name="ts_HorizontalAlign",
        py_name="halign",
        pretty_name="Horizontal text alignment",
        py_to_cpp_map={"left" : "HorizontalAlign::LEFT",
                       "right" : "HorizontalAlign::RIGHT",
                       "center" : ("HorizontalAlign::CENTER")},
        doc_str="Horizontal text alignment, left, right or center",
        cpp_enum=CppEnum("HorizontalAlign", entries=("LEFT", "RIGHT", "CENTER"))),
    "ts_VerticalAlign" : StringToInt(
        cpp_name="ts_VerticalAlign",
        py_name="valign",
        pretty_name="Vertical text alignment",
        py_to_cpp_map={"top" : "VerticalAlign::TOP",
                       "middle" : "VerticalAlign::MIDDLE",
                       "bottom" : ("VerticalAlign::BOTTOM")},
        doc_str="Vertical text alignment, top, middle or bottom",
        cpp_enum=CppEnum("VerticalAlign", entries=("TOP", "MIDDLE", "BOTTOM"))),

}
