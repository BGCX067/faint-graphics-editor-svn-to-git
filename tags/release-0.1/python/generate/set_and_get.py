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

setters_and_getters = {
    "ts_LineArrowHead" :
        ( "int",
          (0,3),
          "arrow",
          "Arrowhead setting.\\n0: None, 1: Arrowhead front, 2: Arrowhead back, 3: Arrowhead both"),

    "ts_BrushSize" : (
        "int",
        (1,255),
        "brushsize",
        "Brush size (1-255)"),
    
    "ts_LineCap" : (
        "stringtoint", {
            "round" : "faint::CAP_ROUND",
            "r" : ("faint::CAP_ROUND",shorthand),
            "flat": "faint::CAP_BUTT",
            "f": ("faint::CAP_BUTT",shorthand)},
        "cap",
        "Line cap setting (line ends).\\nr=round, f: flat"),

    "ts_LineJoin" : (
        "stringtoint", {
            "round" : "faint::JOIN_ROUND",
            "bevel": "faint::JOIN_BEVEL",
            "miter": "faint::JOIN_MITER"},
        "join",
        "Line join setting (line connections).\\n('round', 'bevel' or 'miter')"),

    "ts_FillStyle" : (
        "stringtoint", {
            "border" : "BORDER",
            "b" : ("BORDER",shorthand),
            "fill" : "FILL",
            "f" : ("FILL",shorthand),
            "border+fill" : "BORDER_AND_FILL",
            "bf" : ("BORDER_AND_FILL",shorthand)},
        "fillstyle",
        "Fill style\\nb: border, f: fill, bf: border and fill"),

    "ts_LineWidth" : (
        "float",
        (0 ,255),
        "linewidth",
        "Line width (0.0-255.0)"),

    "ts_LineStyle" : (
        "stringtoint", {
            "solid" : "faint::SOLID",
            "s" : ("faint::SOLID",shorthand),

            "long_dash" : "faint::LONG_DASH",
            "ld" : ("faint::LONG_DASH",shorthand)},
        "linestyle",
        "Line style\\ns: solid, ld: long-dash"),

    "ts_Transparency" : (
        "stringtoint", {
            "transparent" : "TRANSPARENT_BG",
            "t" : ("TRANSPARENT_BG",shorthand),

            "opaque" : "OPAQUE_BG",
            "o" : ("OPAQUE_BG",shorthand) },
        "transparency",
        "Transparency setting:\\nt: transparent background, o: opaque background."),

    "ts_FontSize" : (
        "int",
        (1, 255),
        "fontsize",
        "Font size (1-255)."),

    "ts_LayerStyle" : (
        "stringtoint", {
            "raster" : "LAYER_RASTER",
            "object" : "LAYER_OBJECT",
            "r" : ("LAYER_RASTER",shorthand),
            "o" : ("LAYER_OBJECT",shorthand)},
        "layerstyle",
        "Layer choice\\nr: raster, o: object"),

    "ts_FgCol" : ( "color", "fgcol", "Foreground color."),
    "ts_BgCol" : ( "color", "bgcol", "Background color."),
    "ts_FontBold" : ("bool", "fontbold", "Bold font?" ),
    "ts_FontItalic" : ("bool", "fontitalic", "Italic font?" ),
    "ts_SwapColors" : ("bool", "swapcolors", "Swap foreground and background colors?" ),
    "ts_AntiAlias" : ("bool", "antialias", "Antialiasing?"),
    "ts_FontFace" : ("string", "fontface", "Font face string."),
}
