#!/usr/bin/env python3
# -*- coding: utf-8 -*-

# Copyright 2014 Lukas Kemmer
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

"""Builds Faint images from for SVG read from string or file."""

__all__ = ["parse_doc", "parse_svg_string"]

from math import atan2
import re
import faint.svg.wrapped_etree as ET
import faint.svg.svg_re as svg_re
from . util import (Matrix, mul_matrix_tri,
                    apply_transforms, clean_path_definition,
                    parse_color, to_faint_cap_str,
                    parse_color_noref, dict_union,
                    center_based_to_rect, maybe_id_ref,
                    extract_local_xlink_href,
                    match_children,
                    parse_embedded_image_data, deg2rad,
                    ABSOLUTE_UNIT_FACTORS)
import ifaint


_DEBUG_SVG = False

class SvgParseState:
    """The state when arriving at a node. Contains the inheritable
    settings from above, as well as the current transformation matrix
    (ctm) and the props.

    """

    def __init__(self, frame_props, ctm=None, settings=None, ids=None, currentColor=None, containerSize=None, system_language="en"):
        self.props = frame_props
        self.ctm = Matrix.identity() if ctm is None else ctm
        self.settings = (node_default_settings() if settings is None
                         else settings)
        self.ids = {} if ids is None else ids
        self.currentColor = ((255, 0, 0) if currentColor is None
                             else currentColor)
        self.containerSize = ((0.0, 0.0)
                              if containerSize is None else containerSize)

        # ISO 639-1 language code
        self.system_language = system_language


    def modified(self, ctm=None, settings=None, currentColor=None, containerSize=None):
        """Returns a new SvgParseState for recursing, with optionally updated
        transformation matrix or settings. The props is unchanged.

        """
        if ctm is None:
            ctm = self.ctm
        if settings is None:
            settings = self.settings
        if currentColor is None:
            currentColor = self.currentColor
        if containerSize is None:
            containerSize = self.containerSize
        return SvgParseState(self.props, ctm, settings, self.ids,
                             currentColor, containerSize, self.system_language)


    def transform_object(self, object_id):
        """Transform the object with object_id in props with the CTM."""

        tri = self.props.get_obj_tri(object_id)
        tri = mul_matrix_tri(self.ctm, tri)
        self.props.set_obj_tri(object_id, tri)


    def transform_tri(self, tri):
        """Return the tri transformed by the CTM"""
        return mul_matrix_tri(self.ctm, tri)


    def updated(self, node):
        """Return a new SvgParseState updated by the node attributes (e.g.
        transform, settings etc.)

        """
        assert self.ctm is not None
        transforms = parse_transform_list(node.get('transform', ''))
        ctm = apply_transforms(transforms, self.ctm)


        cc = node.get('color')
        if cc is not None:
            cc = parse_color_noref(cc, 1.0, self)

        # Fixme: Ugly. Create the new state instead
        currentCurrentColor = self.currentColor
        if cc is not None:
            self.currentColor = cc
        settings = self._updated_settings(node, ctm)
        self.currentColor = currentCurrentColor

        return self.modified(ctm=ctm, settings=settings, currentColor=cc)


    def _updated_settings(self, node, ctm):
        # Fixme: Move all stuff from parse_style_dict here.
        # Fixme: Traverse the keys instead
        # Fixme: Handle 'inherit'
        settings = ifaint.Settings()
        settings.update_all(self.settings)
        settings.fg = self.settings.fg
        settings.bg = self.settings.bg
        attributes = get_style_dict(node.get('style', ''))
        attributes.update(node.attrib)

        settings.fillstyle = self.settings.fillstyle
        #settings.update_all(self.settings)

        stroke_opacity = attributes.get("stroke-opacity", "1.0")
        fill_opacity = attributes.get("fill-opacity", "1.0")
        stroke = attributes.get("stroke")
        fill = attributes.get("fill")
        fillstyle_to_settings(settings, stroke, fill, stroke_opacity, fill_opacity, self)

        stroke_width = attributes.get('stroke-width')
        if stroke_width is not None:
            # Fixme: Should this be the new state rather than self?
            sw1 = svg_length_attr(stroke_width, self)
            # Fixme: Using the new ctm, verify.
            # Fixme: using .a is simplistic
            sw1 *= ctm.a
            try:
                settings.linewidth = sw1
            except ValueError as e:
                # Todo: Allow larger stroke width in Faint
                # also, check how to handle negative.
                self.props.add_warning(str(e))

        stroke_dasharray = attributes.get('stroke-dasharray', None)
        if stroke_dasharray is not None:
            if stroke_dasharray == 'none':
                # Fixme: Verify "none"
                settings.linestyle = 's'
            else:
                # Fixme: actually use the dash-array
                settings.linestyle = 'ld'

        stroke_linejoin = attributes.get('stroke-linejoin', None)
        if stroke_linejoin is not None and stroke_linejoin != 'inherit':
            # Fixme: settings.join probably not 100% aligned with
            # svg join
            settings.join = stroke_linejoin

        stroke_linecap = attributes.get('stroke-linecap', None)
        if stroke_linecap is not None:
            settings.cap = to_faint_cap_str(stroke_linecap)

        font_size = attributes.get('font-size', None)
        if font_size is not None:
            # Fixme: What other names are there?
            if font_size == 'medium':
                settings.fontsize = 12.0 # Fixme
            else:
                # Fixme: Terrible
                settings.fontsize = svg_length_attr(font_size, self)

        font_family = attributes.get("font-family", None)
        if font_family is not None:
            # Fixme: face vs family eh
            settings.fontface = font_family

        font_style = attributes.get("font-style", None)
        if font_style is not None:
            # Fixme: Extend
            settings.fontitalic = font_style == "italic"

        font_weight = attributes.get('font-weight', None)
        if font_weight is not None:
            settings.fontbold = font_weight == 'bold'

        parse_marker_attr(node, settings)
        return settings


    def add_warning(self, text):
        """Add a load warning to the contained props."""
        self.props.add_warning(text)


    def add_ref_item(self, item):
        """Add an item accessible by id (item is a tuple, id to object)"""
        ref_id, obj = item
        self.ids[ref_id] = obj

def parse_color_stop_offset(svg_string):
    """Parses an offset for a gradient color stop"""

    match = re.match(svg_re.percentage, svg_string)
    if match:
        return float(match.group(0)) / 100

    match = re.match(svg_re.number_attr, svg_string)
    if match:
        # Ratio offset
        # Fixme: I think standard says clamp to 0.0-1.0
        return float(match.group(0))
    else:
        raise ifaint.LoadError("Failed parsing gradient offset")


def parse_color_stops(nodes, state):
    """Parses the color stop nodes for a gradient node.
    Returns a list of offsets to colors.

    """
    stop_list = []
    for stop in [st for st in nodes if st.tag == ns_svg('stop')]:
        color_str, opacity_str = parse_color_stop_style(stop.get('style', ''))
        color_str = stop.get('stop-color', color_str)
        opacity_str = stop.get('stop-opacity', opacity_str)
        offset = parse_color_stop_offset(stop.get("offset"))

        # Fixme: Parse opacity str instead of just floating it
        faint_color = parse_color_noref(color_str, float(opacity_str), state)
        stop_list.append((offset, faint_color))
    return stop_list


# Fixme: Rename id_to_node, it maps to parsed objects
def parse_defs(node, state, id_to_etree_node=None):
    """Recursively parses an SVG defs-node."""

    if id_to_etree_node is None:
        id_to_etree_node = {}
    for child in node:
        ref_id = child.get('id')
        if ref_id is not None:
            id_to_etree_node[ref_id] = child

    for child, func in match_children(node, svg_defs_content_functions):
        item = func(child, state, id_to_etree_node)
        if item is not None:
            state.add_ref_item(item)


class SvgLiteralError(ifaint.LoadError):
    """Exception raised for invalidly expressed SVG literals."""
    def __init__(self, literal_type, literal):
        super().__init__("Invalid %s: %s" % (literal_type, literal))
        self.literal = literal


class SvgLengthError(SvgLiteralError):
    """Exception raised for invalidly expressed SVG lengths."""
    def __init__(self, literal):
        super().__init__("length", literal)


class SvgCoordError(SvgLiteralError):
    """Exception raised for invalidly expressed SVG coordinates."""
    def __init__(self, literal):
        super().__init__("coordinate", literal)


def add_fill(settings, fill, fill_opacity, state):
    """Adds the specified fill-style to the settings object, preserving
    border.

    """
    if settings.fillstyle == 'border':
        settings.fillstyle = 'border+fill'
        settings.bg = parse_color(fill, fill_opacity, state)
    else:
        settings.fillstyle = 'fill'
        settings.fg = parse_color(fill, fill_opacity, state)


def add_stroke(settings, stroke, stroke_opacity, state):
    """Adds the specified stroke-style to the settings object, preserving
    fill

    """
    if settings.fillstyle == 'fill':
        settings.fillstyle = 'border+fill'
        settings.bg = settings.fg
        settings.fg = parse_color(stroke, stroke_opacity, state)
    else:
        settings.fillstyle = 'border'
        settings.fg = parse_color(stroke, stroke_opacity, state)


def remove_fill(settings):
    """Removes fill from the settings object, preserving stroke (i.e.
    border in Faint).

    """
    if settings.fillstyle == 'fill':
        settings.fillstyle = 'none'

    elif settings.fillstyle == 'fill+border':
        settings.fillstyle = 'border'
        return


def remove_stroke(settings):
    """Removes the stroke (i.e. border in Faint) from the settings object,
    preserving fill.

    """
    if settings.fillstyle == 'border':
        settings.fillstyle = 'none'
    elif settings.fillstyle == 'fill+border':
        settings.fg = settings.bg
        settings.fillstyle = 'fill'


def fillstyle_to_settings(settings, stroke, fill, stroke_opacity, fill_opacity, state):
    """Converts from SVG stroke and fill to the combined faint fillstyle +
    fgcol, bgcol.

    """

    # Fixme: Simplify
    if stroke == None and fill != None:
        if fill == "none":
            remove_fill(settings)
        else:
            add_fill(settings, fill, fill_opacity, state)
        return
    elif stroke != None and fill == None:
        if stroke == "none":
            remove_stroke(settings)
        else:
            add_stroke(settings, stroke, stroke_opacity, state)
        return

    elif stroke != None and fill != None:
        if stroke == "none" and fill == "none":
            settings.fillstyle = 'none'
            return

        elif stroke == "none" and fill != "none":
            settings.fillstyle = 'fill'
            settings.fg = parse_color(fill, fill_opacity, state)
            return

        elif stroke != "none" and fill == "none":
            settings.fillstyle = 'border'
            settings.fg = parse_color(stroke, stroke_opacity, state)
            return

        elif stroke != "none" and fill != "none":
            settings.fillstyle = 'border+fill'
            settings.fg = parse_color(stroke, stroke_opacity, state)
            settings.bg = parse_color(fill, fill_opacity, state)
            return


def get_style_dict(style):
    """Parses an SVG style attribute string, returning it as a key/value
    dictionary

    """
    # Fixme: Review, from old
    style_items = style.split(";")
    style_dict = {}
    for item in style_items:
        if len(item) > 0:
            key, value = item.split(":")
            key, value = key.strip(), value.strip()
            style_dict[key] = value
    return style_dict


def node_default_settings():
    """Returns the initial default Settings."""
    # Fixme: Review, from old
    settings = ifaint.Settings()
    settings.linewidth = 1.0
    settings.cap = 'flat'
    settings.fg = (0, 0, 0)
    settings.bg = (0, 0, 0)
    settings.fillstyle = 'fill'
    return settings


def parse_transform(s):
    """Parses an entry in a transform-list."""
    def _parse_args(s):
        assert s.startswith("(")
        assert s.endswith(")")
        # Fixme: parse number (i.e. incl. Ee etc)

        str_args = [arg for arg in s.replace(" ", ",")[1:-1].split(",") if len(arg) != 0]
        return [float(arg) for arg in str_args]

    op, args = s.split("(")
    args = _parse_args('(' + args)

    if op == "skewX":
        return Matrix.skewX(deg2rad(*args))
    elif op == "skewY":
        return Matrix.skewY(deg2rad(*args))
    elif op == "rotate":
        rad = deg2rad(args[0])
        pivot = args[1:] if len(args) == 3 else None
        return Matrix.rotation(rad, pivot)
    elif op == "translate":
        x = args[0]
        y = args[1] if len(args) == 2 else 0
        return Matrix.translation(x,y)
    elif op == "matrix":
        return Matrix(*args)
    elif op == "scale":
        sx = args[0]
        sy = args[1] if len(args) == 2 else sx
        return Matrix.scale(sx, sy)
    else:
        raise ifaint.LoadError("Unsupported transform: %s" % op)


def parse_transform_list(s):
    """Parses an SVG transform attribute"""
    transform_list = re.findall(svg_re.transform_list, s)
    return [parse_transform(tr) for tr in transform_list]


def parse_points(points_str):
    """Parses the points specification for polyline and polygon
    elements (SVG 1.1, 9.7.1).

    """
    return [float(s) for s in re.split("[\x20\x09\x0D\x0A]+|[,]",
      points_str) if s != ""]


def parse_pattern_node(node, state):
    """Parses a pattern element"""
    # Fixme: Review, from old
    node_id = node.get('id')
    for child in node.childNodes:
        if child.tag == ns_svg('image'):
            encoding, data = parse_embedded_image_data(child.get(ns_xlink('href')))
            if encoding == "png":
                return node_id, ifaint.Pattern(ifaint.bitmap_from_png_string(data))
    state.add_warning('Failed parsing pattern with id=%s' % node_id)
    return None


def get_linked_stops(xlink, state, id_to_etree_node):
    """Returns a referenced list of color-stops for a gradient.

    Retrieves the referenced id from the xlink, and retrieves its
    stops, if already available as a faint Gradient object in the state,
    otherwise looks up the target node in id_to_etree node, and tries to
    parse it as a gradient.

    """

    ref_id = extract_local_xlink_href(xlink)
    linked = state.ids.get(ref_id, None)

    if linked is None:
        if ref_id in id_to_etree_node:
            other = id_to_etree_node[ref_id]
            if other.tag == ns_svg('linearGradient'):
                linked_id, linked = parse_linear_gradient_node(other, state, id_to_etree_node)
            elif other.tag == ns_svg('radialGradient'):
                linked_id, linked = parse_radial_gradient_node(other, state, id_to_etree_node)
        if linked is None:
            state.add_warning("Referenced gradient not found: %s " % ref_id)

    if linked is not None:
        stops = []
        for num in range(linked.get_num_stops()):
            stops.append(linked.get_stop(num))
        return stops
    return []


def parse_linear_gradient_node(node, state, id_to_etree_node):
    """Parses a linear gradient element"""

    state = state.updated(node)

    node_id = node.get('id')
    if node_id is None:
        # Ignore gradients without identifiers
        return None

    x1 = float(node.get('x1', '0'))
    x2 = float(node.get('x2', '0'))
    y1 = float(node.get('y1', '0'))
    y2 = float(node.get('y2', '0'))

    dx = x2 - x1
    dy = y2 - y1
    angle = atan2(dy, dx)

    stops = []
    stops = parse_color_stops(list(node), state)

    if len(stops) == 0:
        # No color stops, follow references
        # FIXME: Can there both be stops and a reference?
        xlink = node.get(ns_xlink('href'))
        if xlink is not None:
            stops = get_linked_stops(xlink, state, id_to_etree_node)
        else:
            # Ignore gradients without stops
            # (Note: Need to add handling of referenced gradients)
            state.add_warning("linearGradient with id=%s has no color-stops" % node_id)
            return None
    if len(stops) == 0:
        state.add_warning("linearGradient with id=%s has no color-stops" % node_id)
        return None

    return node_id, ifaint.LinearGradient(angle, *stops)


def parse_radial_gradient_node(node, state, id_to_etree_node):
    """Parses a radial gradient node."""
    # Fixme: Review, from old
    node_id = node.get('id')
    if node_id is None:
        # Ignore gradients without identifiers
        return None

    state = state.updated(node)

    stops = []
    stops = parse_color_stops(list(node), state)

    if len(stops) == 0:
        # No color stops, follow references
        # FIXME: Can there both be stops and a reference?
        xlink = node.get(ns_xlink('href'))
        if xlink is not None:
            stops = get_linked_stops(xlink, state, id_to_etree_node)
        else:
            # Ignore gradients without stops
            state.add_warning("radialGradient with id=%s has no color-stops" % node_id)
            return None
    if len(stops) == 0:
        state.add_warning("radialGradient with id=%s has no color-stops" % node_id)
        return None

    return node_id, ifaint.RadialGradient(stops)


def parse_color_stop_style(style):
    """Return the color and opacity from the style as strings.
    Returns None for color-string and 1.0 for opacity if not present

    """

    style_dict = get_style_dict(style)
    stop_opacity = style_dict.get('stop-opacity', '1.0')
    color_str = style_dict.get('stop-color', None)
    return color_str, stop_opacity


def parse_faint_tri_attr(node):
    """Parses a faint:tri attribute from the node"""
    tri_str = node.get(ns_faint("tri"))
    p0, p1, p2 = pairs(parse_points(tri_str))
    return ifaint.Tri(p0, p1, p2)


def parse_ellipse(node, state):
    """Parses an SVG <ellipse>-element."""
    state = state.updated(node)
    bounding_rect = center_based_to_rect(
        svg_coord_attr(node.get("cx", "0.0"), state),
        svg_coord_attr(node.get("cy", "0.0"), state),
        svg_coord_attr(node.get("rx", "0.0"), state),
        svg_coord_attr(node.get("ry", "0.0"), state))

    ellipse_id = state.props.Ellipse(bounding_rect, state.settings)
    state.transform_object(ellipse_id)
    return ellipse_id

def parse_group(node, state):
    """Parses an SVG <g>-element."""
    state = state.updated(node)

    object_ids = [func(child, state) for child, func in match_children(node, svg_group_content_functions)]
    object_ids = [objid for objid in object_ids if objid is not None]

    if len(object_ids) == 0:
        return

    # Fixme: Transform the group?
    return state.props.Group(object_ids)


def parse_marker_attr(node, settings):
    """Parses the node's SVG 'marker-start', 'marker-end' attributes

    """
    arrowhead_str = node.get('marker-end')
    arrowtail_str = node.get('marker-start')
    # Fixme: actually refer to marked structure
    arrowhead = (arrowhead_str == 'url(#Arrowhead)')
    arrowtail = (arrowtail_str == 'url(#Arrowtail)')

    if arrowhead and arrowtail:
        settings.arrow = 'both'
    elif arrowtail:
        settings.arrow = 'back'
    elif arrowhead:
        settings.arrow = 'front'
    else:
        settings.arrow = 'none'


def parse_path(node, state):
    """Parses an SVG <path>-element"""

    state = state.updated(node)
    path_def = node.get('d')
    if path_def is None:
        state.add_warning("Ignored path-element without definition attribute%s." % maybe_id_ref(node))
        return

    path_def = clean_path_definition(path_def)
    if len(path_def) == 0:
        state.add_warning("Ignored path-element without definition attribute%s." % maybe_id_ref(node))
        return

    try:
        path_id = state.props.Path(path_def, state.settings)
    except ValueError:
        ifaint.copy_text(node.get('d')) # Fixme: Remove
        state.add_warning("Failed parsing a path definition%s." % maybe_id_ref(node))
        return

    state.transform_object(path_id)
    return path_id


def parse_path_as_ellipse(node, state):
    """Parses an ellipse with a faint:tri attribute to an ellipse.

    Faint saves ellipses as paths for some reason (probably to
    skew/transform only the points, not the stroked edge).

    """
    state = state.updated(node)
    tri = parse_faint_tri_attr(node)
    # Fixme: Accept Tri in props.Ellipse
    ellipse_id = state.props.Ellipse((0, 0, 1, 1), state.settings)
    state.props.set_obj_tri(ellipse_id, tri)
    state.transform_object(ellipse_id)
    return ellipse_id


def parse_polygon(node, state):
    """Parses an SVG <path>-element."""
    state = state.updated(node)
    polygon_id = state.props.Polygon(parse_points(node.get('points', '')),
                                     state.settings)
    state.transform_object(polygon_id)
    return polygon_id


def parse_polyline(node, state):
    """Parses an SVG <polyline>-element."""
    state = state.updated(node)
    line_id = state.props.Line(parse_points(node.get('points')),
                            state.settings)
    state.transform_object(line_id)
    return line_id


def parse_faint_text_size_attrs(node, state):
    """Parses the faint-added width, height attributes"""

    # Get name-spaced or un-namespaced width, height
    # (added by Faint regardless)
    width_str = (node.get(ns_faint('width')) or
                  node.get('width'))
    height_str = (node.get(ns_faint('height')) or
                  node.get('height'))
    if width_str is not None:
        w = svg_length_attr(width_str, state)
    else:
        w = 200 # Fixme
    if height_str is not None:
        h = svg_length_attr(height_str, state)
    else:
        h = 200
    return w, h


def svg_baseline_to_faint(text_id, state):

    """Convert the position of the text object in props with text_id.

    SVG anchors at baseline, Faint at top of text.
    """
    dy = state.props.get_obj_text_height(text_id)
    tri = state.props.get_obj_tri(text_id)
    tri.translate(0, -dy)
    state.props.set_obj_tri(text_id, tri)


def parse_text(node, state):
    """Parses an SVG <text>-element."""
    state = state.updated(node)

    # Fixme: Should support <coordinate-list>
    x = svg_length_attr(node.get('x', '0'), state)
    y = svg_length_attr(node.get('y', '0'), state)

    w, h = parse_faint_text_size_attrs(node, state)
    # Fixme: Parse dx, dy lists etc.

    settings = ifaint.Settings()
    settings.update_all(state.settings)

    # Fixme: These assignments will overwrite inherited settings
    #        even if node doesn't have the retrieved setting
    settings.bounded = node.get(ns_faint('bounded'), '1') == '1'

    # Fixme: Made redundant by anchor?
    settings.halign = node.get(ns_faint('halign'), 'left')
    settings.valign = node.get(ns_faint('valign'), 'top')

    # TODO: If no unit specified size is processed as a height in the user
    #       coordinate system.
    #       Otherwise converts the length into user coordinate system

    fontsize_str = node.get('font-size')
    if fontsize_str is not None:
        settings.fontsize = svg_length_attr(fontsize_str, state)

    style = get_style_dict(node.get('style', ''))
    if 'fill' in style:
        settings.fg = parse_color(style['fill'], "1.0", state)

    anchor = node.get('text-anchor', 'start')
    if anchor == 'start':
        settings.halign = 'left'
    elif anchor == 'middle':
        settings.halign = 'center'
        x -= w / 2 # fixme
    elif anchor == 'end':
        settings.halign = 'right'

    text_str = ""
    if node.text:
        text_str += node.text

    for child in node:
        if child.tag == ns_svg('tspan'):
            # Fixme: Parse settings from the tspans
            if len(child.text) > 0:
                text_str += child.text

            hardbreak = child.get(ns_faint('hardbreak'), '0')
            if hardbreak == 1:
                text_str += "\n"

    text_id = state.props.Text((x, y, w, h), text_str, settings)
    svg_baseline_to_faint(text_id, state)
    state.transform_object(text_id)
    return text_id


def ns_svg(item_name):
    """Prepends the svg xml-namespace to the item name."""
    return '{http://www.w3.org/2000/svg}' + item_name


def ns_xlink(item_name):
    """Prepends the xlink xml-namespace to the item name."""
    return '{http://www.w3.org/1999/xlink}' + item_name


def ns_faint(item_name):
    """Prepends the faint xml-namespace to the item name."""
    return '{http://www.code.google.com/p/faint-graphics-editor}' + item_name


def is_faint_type(node, expected):
    """True if the node has a faint:type attribute with value matching
    expected. Allows mapping SVG shapes etc. to specific Faint object
    types.

    """
    return node.get(ns_faint('type')) == expected


def pairs(iterable):
    """Return a list which pairs up the neighbouring items in tuples.
    Will lose the last item if uneven item count.
    """
    i = iter(iterable)
    return list(zip(i, i))


def parse_circle(node, state):
    """Parses an SVG <circle>-element."""
    state = state.updated(node)
    cx = svg_coord_attr(node.get('cx', '0'), state)
    cy = svg_coord_attr(node.get('cy', '0'), state)
    r = svg_length_attr(node.get('r', '0'), state)
    rect = center_based_to_rect(cx, cy, r, r)
    ellipse_id = state.props.Ellipse(rect, state.settings)
    state.transform_object(ellipse_id)
    return ellipse_id


def parse_polygon_as_rect(node, state):
    """Skewed Faint-rectangles are saved as polygons (marked with
    faint:type="rect").

    """
    state = state.updated(node)
    rect_id = state.props.Rect((0, 0, 1, 1), state.settings) # Fixme: Pass Tri to props.rect instead of defaulting like this
    p0, p1, p3, p2 = pairs(parse_points(node.get('points')))
    del p3
    state.props.set_obj_tri(rect_id, ifaint.Tri(p0, p1, p2))
    state.transform_object(rect_id)
    return rect_id


def parse_image_as_background(node, state):
    """Parses the embedded content of image node with a faint:background
    attribute and sets it as the background.

    """
    image_str = node.get(ns_xlink('href'))
    image_type, data = parse_embedded_image_data(image_str)
    assert image_type == 'png'
    state.props.set_background_png_string(data)


def parse_image(node, state):
    """Parses an SVG <image>-element."""
    assert node.get(ns_faint('background')) is None
    state = state.updated(node)

    x = float(node.get('x', 0.0))
    y = float(node.get('y', 0.0))
    w = float(node.get('width', 0.0))
    h = float(node.get('height', 0.0))

    image_str = node.get(ns_xlink('href'))
    if image_str is None:
        state.add_warning("Ignored image element with no data.")
        return

    image_type, image_data = parse_embedded_image_data(image_str)
    if image_type not in ('jpeg', 'png'):
        state.add_warning('Ignored image element with unsupported type: %s' % image_type)
        return
    # Fixme: Handle faint:bg-style/faint:bg_style?
    raster_id = state.props.Raster((x, y, w, h), image_type, image_data,
                                   state.settings)

    state.transform_object(raster_id)
    return raster_id


def parse_image_custom(node, state):
    """Parses an SVG <image> element into a faint raster background if
    marked-up as such, otherwise as a Faint raster object.

    """
    return (parse_image_as_background(node, state)
            if node.get(ns_faint('background')) is not None
            else parse_image(node, state))


def parse_line(node, state):
    """Parses an SVG <line> element."""
    state = state.updated(node)
    pts = [svg_coord_attr(node.get(s, '0'), state)
           for s in ('x1', 'y1', 'x2', 'y2')]
    line_id = state.props.Line(pts, state.settings)
    state.transform_object(line_id)
    return line_id


def parse_path_custom(node, state):
    """Parses an SVG <path> element as a Faint Ellipse if marked-up as
    such, otherwise as a Faint Path.

    """
    return (parse_path_as_ellipse(node, state) if is_faint_type(node, 'ellipse')
            else parse_path(node, state))


def parse_polygon_custom(node, state):
    """Parses an SVG <polygon> element as a Faint Rectangle if marked-up
    as such, otherwise as a Faint Polygon.

    """
    return (parse_polygon_as_rect(node, state) if is_faint_type(node, 'rect')
            else parse_polygon(node, state))


def parse_rect_as_background(node, state):
    """Parses a marked-up rect-node to a background color.

    """
    color = parse_color(node.get('fill'), '1.0', state)
    state.props.set_background_color(color)

def parse_rect(node, state):
    """Parses an SVG <rect> element."""
    state = state.updated(node)
    pts = [svg_coord_attr(node.get(s, '0'), state)
           for s in ('x', 'y', 'width', 'height')]

    rect_id = state.props.Rect(pts, state.settings)
    state.transform_object(rect_id)
    # TODO: rx, ry
    return rect_id


def parse_rect_custom(node, state):
    """Parses an SVG <rect> element as the Canvas background color if
    marked-up as such, otherwise as a regular Rectangle.

    """
    return (parse_rect_as_background(node, state)
            if node.get(ns_faint('background')) is not None
            else parse_rect(node, state))


def parse_switch(node, state):
    """Parses a switch node"""
    for child in node:
        if child.tag not in svg_switch_element_content_functions:
            continue

        requiredExtensions = child.get('requiredExtensions')
        if requiredExtensions is not None and requiredExtensions != "":
            # Faint supports no extensions.
            continue

        requiredFeatures = child.get('requiredFeatures')
        if requiredFeatures is not None:
            rfs = requiredFeatures.split(" ")
            unsupported = [rf.strip() for rf in rfs if rf not in supported_svg_features]
            if len(unsupported) != 0:
                # At least one required feature was unsupported.
                continue

        systemLanguage = child.get('systemLanguage', state.system_language)
        if systemLanguage == state.system_language:
            return svg_switch_element_content_functions[child.tag](child, state)


    state.add_warning("No supported child node of SVG switch-element")


def _not_implemented(node, state, *args): #pylint:disable=unused-argument
    if _DEBUG_SVG:
        state.add_warning("Support not implemented: %s" % node.tag)

# pylint:disable=invalid-name
_not_implemented_dict = {}

svg_animation_element_functions = {
    ns_svg('animate') : _not_implemented,
    ns_svg('animateColor') : _not_implemented,
    ns_svg('animateMotion') : _not_implemented,
    ns_svg('animateTransform'): _not_implemented,
    ns_svg('set'): _not_implemented
}

svg_descriptive_element_functions = {
    ns_svg('desc'): _not_implemented,
    ns_svg('metadata'): _not_implemented,
    ns_svg('title'): _not_implemented
}

svg_gradient_element_functions = {
    # SVG 1.1
    ns_svg('linearGradient') : parse_linear_gradient_node,
    ns_svg('radialGradient') : parse_radial_gradient_node
}

svg_shape_element_functions = {
    ns_svg('circle') : parse_circle,
    ns_svg('ellipse') : parse_ellipse,
    ns_svg('line') : parse_line,
    ns_svg('path') : parse_path_custom,
    ns_svg('polygon') : parse_polygon_custom,
    ns_svg('polyline') : parse_polyline,
    ns_svg('rect') : parse_rect_custom,
}

svg_structural_element_functions = {
    ns_svg('defs'): parse_defs,
    ns_svg('g'): parse_group,
    ns_svg('svg'): _not_implemented, # nested SVG
    ns_svg('symbol'): _not_implemented,
    ns_svg('use'): _not_implemented
}

svg_content_functions = dict_union(
    svg_animation_element_functions,
    svg_descriptive_element_functions,
    svg_shape_element_functions,
    svg_structural_element_functions,
    svg_gradient_element_functions,
    {
        ns_svg('a'): _not_implemented,
        ns_svg('altGlyphDef'): _not_implemented,
        ns_svg('clipPath'): _not_implemented,
        ns_svg('color-profile'): _not_implemented,
        ns_svg('cursor'): _not_implemented,
        ns_svg('filter'): _not_implemented,
        ns_svg('font'): _not_implemented,
        ns_svg('font-face'): _not_implemented,
        ns_svg('foreignObject'): _not_implemented,
        ns_svg('image') : parse_image_custom,
        ns_svg('marker'): _not_implemented,
        ns_svg('mask'): _not_implemented,
        ns_svg('pattern'): _not_implemented,
        ns_svg('script'): _not_implemented,
        ns_svg('style'): _not_implemented,
        ns_svg('switch'): parse_switch,
        ns_svg('text') : parse_text,
        ns_svg('view') : _not_implemented
    }
)

svg_group_content_functions = dict_union(
    svg_animation_element_functions,
    svg_descriptive_element_functions,
    svg_shape_element_functions,
    svg_structural_element_functions,
    svg_gradient_element_functions,
    {
        ns_svg('a'): _not_implemented,
        ns_svg('altGlyphDef'): _not_implemented,
        ns_svg('clipPath') : _not_implemented,
        ns_svg('color-profile') : _not_implemented,
        ns_svg('cursor'): _not_implemented,
        ns_svg('filter'): _not_implemented,
        ns_svg('font'): _not_implemented,
        ns_svg('font-face'): _not_implemented,
        ns_svg('foreignObject'): _not_implemented,
        ns_svg('image') : parse_image_custom,
        ns_svg('marker'): _not_implemented,
        ns_svg('mask'): _not_implemented,
        ns_svg('pattern'): _not_implemented,
        ns_svg('script'): _not_implemented,
        ns_svg('style'): _not_implemented,
        ns_svg('switch'): parse_switch,
        ns_svg('text') : parse_text,
        ns_svg('view'): _not_implemented,
    })

svg_switch_element_content_functions = dict_union(
    svg_animation_element_functions,
    svg_descriptive_element_functions,
    svg_shape_element_functions,
    {
        ns_svg('a'): _not_implemented,
        ns_svg('foreignObject'): _not_implemented,
        ns_svg('g'): parse_group,
        ns_svg('image') : parse_image,
        ns_svg('svg'): _not_implemented, # Nested SVG
        ns_svg('switch'): parse_switch,
        ns_svg('text'): parse_text,
        ns_svg('use'): _not_implemented,
    })

svg_descriptive_element_functions = {
    ns_svg('desc') : _not_implemented,
    ns_svg('metadata') : _not_implemented,
    ns_svg('title') : _not_implemented,
}

svg_defs_content_functions = dict_union(
    svg_animation_element_functions,
    svg_descriptive_element_functions,
    # todo: shape
    # todo: structural
    svg_structural_element_functions,
    svg_gradient_element_functions,
    {
      ns_svg('a'): _not_implemented,
      ns_svg('altGlyphDef'): _not_implemented,
      ns_svg('clipPath'): _not_implemented,
      ns_svg('colorProfile'): _not_implemented,
      ns_svg('cursor'): _not_implemented,
      ns_svg('filter'): _not_implemented,
      ns_svg('font'): _not_implemented,
      ns_svg('font-face'): _not_implemented,
      ns_svg('foreignObject'): _not_implemented,
      ns_svg('image'): _not_implemented,
      ns_svg('marker'): _not_implemented,
      ns_svg('mask'): _not_implemented,
      ns_svg('pattern'): parse_pattern_node,
      ns_svg('script'): _not_implemented,
      ns_svg('style'): _not_implemented,
      ns_svg('switch'): parse_switch,
      ns_svg('text'): _not_implemented,
      ns_svg('view'): _not_implemented,
    })

supported_svg_features = [

    # Not sure, group
    "http://www.w3.org/TR/SVG11/feature#SVG",

    # Not sure
    "http://www.w3.org/TR/SVG11/feature#SVGDOM",

    # Not totally.
    "http://www.w3.org/TR/SVG11/feature#SVG-static",

    # Probably untrue
    "http://www.w3.org/TR/SVG11/feature#SVGDOM-static",

    # No animation support at all
    #"http://www.w3.org/TR/SVG11/feature#SVG-animation",
    #"http://www.w3.org/TR/SVG11/feature#SVGDOM-animation",
    #"http://www.w3.org/TR/SVG11/feature#SVG-dynamic",
    #"http://www.w3.org/TR/SVG11/feature#SVGDOM-dynamic",

    # Support for attributes:
    # id, xml:base, xml:space attributes
    #"http://www.w3.org/TR/SVG11/feature#CoreAttribute",

    # Support for elements:
    # svg, g, defs, desc, title, metadata, symbol, use
    "http://www.w3.org/TR/SVG11/feature#Structure",

    # Support for elements:
    # svg, g, defs, desc, title, metadata, use
    "http://www.w3.org/TR/SVG11/feature#BasicStructure",

    # Support for enable-background property
    #"http://www.w3.org/TR/SVG11/feature#ContainerAttribute",

    # Support for the switch element, and the attributes:
    # requiredFeatures, requiredExtensions, systemLanguage
    "http://www.w3.org/TR/SVG11/feature#ConditionalProcessing",

    # Support for the image element
    "http://www.w3.org/TR/SVG11/feature#Image",

    # Support for the style element
    # (Not true)
    "http://www.w3.org/TR/SVG11/feature#Style",

    # Support for clip and overflow properties of viewport
    #"http://www.w3.org/TR/SVG11/feature#ViewportAttribute",

    # Support for the shape elements:
    # rect, circle, line, polyline, polygon, ellipse, path.
    "http://www.w3.org/TR/SVG11/feature#Shape",

    # Support for text, tspan, tref, textPath, altGlyph, altGlyphDef,
    # altGlyphItem, glyphRef.
    "http://www.w3.org/TR/SVG11/feature#Text",

    # Support for the text element.
    "http://www.w3.org/TR/SVG11/feature#BasicText",

    # Support for the properties:
    # color, fill, fill-rule, stroke, stroke-dasharray,
    # stroke-dashoffset, stroke-linecap, stroke-linejoin,
    # stroke-miterlimit, stroke-width, color-interpolation,
    # color-rendering.
    "http://www.w3.org/TR/SVG11/feature#PaintAttribute",

    # Support for the properties: color, fill, fill-rule, stroke,
    # stroke-dasharray, stroke-dashoffset, stroke-linecap,
    # stroke-linejoin, stroke-miterlimit, stroke-width,
    # color-rendering.
    "http://www.w3.org/TR/SVG11/feature#BasicPaintAttribute",

    # Support for opacity, stroke-opacity, fill-opacity
    # Fixme: 'opacity'? Check that out.
    "http://www.w3.org/TR/SVG11/feature#OpacityAttribute",

    # Support for the properties display, image-rendering,
    # pointer-events, shape-rendering, text-rendering visibility
    "http://www.w3.org/TR/SVG11/feature#GraphicsAttribute",

    # Support for display and visibility properties
    "http://www.w3.org/TR/SVG11/feature#BasicGraphicsAttribute",

    # Support for the marker element
    #"http://www.w3.org/TR/SVG11/feature#Marker",

    # Support for the color-profile element
    #"http://www.w3.org/TR/SVG11/feature#ColorProfile",

    # Support for elements linearGradient, radialGradient and stop
    "http://www.w3.org/TR/SVG11/feature#Gradient",

    # Support for the pattern element
    #"http://www.w3.org/TR/SVG11/feature#Pattern",

    # Support for the clipPath element and clip-path, clip-rule
    # properties
    #"http://www.w3.org/TR/SVG11/feature#Clip",

    # Support for the clipPath element and clip-path property
    #"http://www.w3.org/TR/SVG11/feature#BasicClip",

    # Support for the mask element
    #"http://www.w3.org/TR/SVG11/feature#Mask",

    # Support for the elements filter, feBlend, feColorMatrix,
    # feComponentTransfer, feComposite, feConvolveMatrix,
    # feDiffuseLighting, feDisplacementMap, feFlood, feGaussianBlur,
    # feImage, feMerge, feMergeNode, feMorphology, feOffset,
    # feSpecularLighting, feTile, feDistantLight, fePointLight,
    # feSpotLight, feFuncR, feFuncG, feFuncB, feFuncA.
    #"http://www.w3.org/TR/SVG11/feature#Filter",

    # Support for the elements filter, feBlend, feColorMatrix,
    # feComponentTransfer, feComposite, feFlood, feGaussianBlur,
    # feImage, feMerge, feMergeNode, feOffset, feTile, feFuncR,
    # feFuncG, feFuncB, feFuncA.
    #"http://www.w3.org/TR/SVG11/feature#BasicFilter",

    # Support for attributes onunload, onabort, onerror, onresize,
    # onscroll and onzoom.
    #"http://www.w3.org/TR/SVG11/feature#DocumentEventsAttribute",

    # Support for the attributes onfocusin, onfocusout, onactivate, onclick,
    # onmousedown, onmouseup, onmouseover, onmousemove, onmouseout,
    # onload.
    #"http://www.w3.org/TR/SVG11/feature#GraphicalEventsAttribute",

    # Support for the attributes onbegin, onend, onrepeat, onload
    #"http://www.w3.org/TR/SVG11/feature#AnimationEventsAttribute",

    # Support for the cursor element
    #"http://www.w3.org/TR/SVG11/feature#Cursor",

    # Support for the a element.
    #"http://www.w3.org/TR/SVG11/feature#Hyperlinking",

    # Support for the xlink:type, xlink:href, xlink:role,
    # xlink:arcrole, xlink:title, xlink:show and xlink:actuate
    # elements.
    # (Fixme: Only xlink:href in rare cases)
    "http://www.w3.org/TR/SVG11/feature#XlinkAttribute",

    # Support for the externalResourcesRequired attribute
    #"http://www.w3.org/TR/SVG11/feature#ExternalResourcesRequired",

    # Support for the view element.
    #"http://www.w3.org/TR/SVG11/feature#View",

    # Support for the script element.
    #"http://www.w3.org/TR/SVG11/feature#Script",

    # Support for the animate, set, animateMotion, animateTransform,
    # animateColor and mpath elements.
    #"http://www.w3.org/TR/SVG11/feature#Animation",

    # Support for the font, font-face, glyph, missing-glyph, hkern,
    # vkern, font-face-src, font-face-uri, font-face-format,
    # font-face-name elements.
    # (Fixme: Very limited support)
    "http://www.w3.org/TR/SVG11/feature#Font",

    # Support for the elements font, font-face, glyph, missing-glyph,
    # hkern, font-face-src, font-face-name.
    "http://www.w3.org/TR/SVG11/feature#BasicFont",

    # The foreignObject element
    #"http://www.w3.org/TR/SVG11/feature#Extensibility",
]

# pylint:enable=invalid-name

def svg_coord_attr(coord_str, state):
    """Parses an svg coordinate attribute."""
    # Fixme: Duplicates length_attr
    m = re.match(svg_re.coordinate, coord_str)
    if m is None:
        raise SvgCoordError(coord_str)
    value, unit = m.groups()
    if unit == '%':
        # Fixme: How should this work?
        return (state.containerSize[0] * 100) / float(value)
    elif unit in ABSOLUTE_UNIT_FACTORS:
        return float(value) * ABSOLUTE_UNIT_FACTORS[unit]
    elif unit in ('em', 'ex'):
        state.add_warning("Unsupported unit: %s" % unit)
        return float(value)
    else:
        state.add_warning("Invalid unit: %s" % unit)
        return float(value)


def svg_length_attr_dumb(value_str, props, full_span):
    """Parses an svg length-attribute from the value_str. full_span is
    used as reference for percentages."""

    m = re.match(svg_re.length_attr, value_str)
    if m is None:
        raise SvgLengthError(value_str)

    value, unit = m.groups()
    if unit == "%":
        # Fixme: More work required.
        if full_span.__class__ == float:
            return (full_span * 100) / float(value)
        else:
            return (full_span[0] * 100) / float(value)
    elif unit in ABSOLUTE_UNIT_FACTORS:
        return float(value) * ABSOLUTE_UNIT_FACTORS[unit]
    elif unit in ('em','ex'):
        # Fixme: Implement.
        props.add_warning("Unsupported unit: %s" % unit)
        return float(value)
    else:
        props.add_warning("Invalid unit: %s" % unit)
        return float(value)


def svg_length_attr(value_str, state):
    """Parses an svg length attribute."""
    return svg_length_attr_dumb(value_str, state.props, state.containerSize)


def check_svg_size(size):
    """Raises exception if invalid size for SVG node."""
    if size[0] <= 0:
        raise ifaint.LoadError("SVG element has negative width")
    elif size[1] <= 0:
        raise ifaint.LoadError("SVG element has negative height")
    return size


def svg_size(svg_node, image_props, view_box_str):
    """Returns w, h from the svg node."""
    # Fixme: Truncating the svg width, height (Faint limitation)
    # Fixme: Use what for container size here? Easier if not outermost, I guess

    default_w = view_box_str[2]
    default_h = view_box_str[3]

    svg_w = svg_node.get('width', default_w)
    svg_h = svg_node.get('height', default_h)

    # Fixme: I guess viewbox should be converted, not float:ed
    w2 = int(svg_length_attr_dumb(svg_w, image_props, full_span=float(default_w)))
    h2 = int(svg_length_attr_dumb(svg_h, image_props, full_span=float(default_h)))
    return check_svg_size((w2, h2))

def get_viewbox(node, default=('0','0','640','480')):
    """Returns the viewbox from the node as a sequence of four strings,
    x, y, w, h."""

    # Available for: svg, symbol (via use), image, foreignObject
    viewBox = node.get('viewBox') # pylint:disable=invalid-name
    if viewBox is None:
        return default

    # Fixme: Handle preserveAspectRatio
    x, y, w, h = [v for v in viewBox.split(' ')]
    return x, y, w, h

def parse_svg_root_node(svg_node, image_props, system_language):
    """Parses the SVG-root node and recurses into child nodes. Creates the
    frame from the SVG root node properties.

    """

    # Fixme: Check if SVG element can have transform

    view_box = get_viewbox(svg_node)
    props = image_props.add_frame(svg_size(svg_node, image_props, view_box))
    state = SvgParseState(props, system_language=system_language)

    # Fixme: Convert instead of float
    x0, y0 = float(view_box[0]), float(view_box[1])
    if x0 != 0 or y0 != 0:
        state.ctm = Matrix.translation(-x0, -y0) * state.ctm

    for child in svg_node:
        if child.tag in svg_content_functions:
            # Fixme: I guess there could be a transform already?
            svg_content_functions[child.tag](child, state)

def parse_doc(path, image_props, system_language="en"):
    """Parses the SVG document at the given file path, using the
    image_props to build an image.

    """
    ET.register_namespace("svg", "{http://www.w3.org/2000/svg}")
    tree = ET.parse(path)
    root = tree.getroot()
    if root.tag == ns_svg("svg"):
        parse_svg_root_node(root, image_props, system_language)
    else:
        raise ifaint.LoadError("Root element was not <svg>.")

def parse_svg_string(xml_string, image_props, system_language="en"):
    """Parses the SVG document in the given string, using the image_props
    to build an image.

    """

    ET.register_namespace("svg", "{http://www.w3.org/2000/svg}")
    root = ET.fromstring(xml_string)
    if root.tag == ns_svg("svg"):
        parse_svg_root_node(root, image_props, system_language)
    else:
        raise ifaint.LoadError("Root element was not <svg>.")
