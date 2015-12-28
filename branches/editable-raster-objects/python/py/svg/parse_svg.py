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
"""Faint SVG parsing.
The functions parse_doc and parse_string use the passed in ImageProps to
build a Faint-image from the SVG file or string."""

import xml.dom.minidom as minidom
from math import tan, cos, sin

import ifaint
import py.svg.expat_util as expat
from py.svg.util import  multiply_matrices, rotation_matrix, translation_matrix, multiply_matrix_pt
from py.svg.convert_base64 import decode_base64
from py.svg.util import deg2rad, parse_value
from py.svg.util import parse_embedded_image_data
from py.svg.util import maybe_id_ref
from py.svg.util import multiply_matrices, rotation_matrix, translation_matrix, multiply_matrix_pt
import py.svg.util as util
import py.svg.parse_objects as parse_objects

__all__ = ("parse_doc", "parse_svg_string")

def parse_doc( path, props ):
    """Use the ImageProps to build a Faint Image from the svg file at
    the specified path.

    """

    try:
        dom = minidom.parse(path)
    except expat.ExpatError, err:
        if expat.is_document_related(err):
            err_str = "Error in file:\n" + str(err)
            raise ifaint.LoadError(err_str)
        else:
            raise # Re-raise as internal error

    doc = dom.documentElement
    state = SVGParseState()
    parse_node( doc, props, state )
    for warning in state.warnings:
        props.add_warning(warning)

def parse_svg_string( string, props ):
    """Use the ImageProps to build a Faint Image from the svg-string."""
    dom = minidom.parseString( string )
    doc = dom.documentElement
    parse_node( doc, props, SVGParseState() )

class SVGParseState:
    """Stores the SVG state, e.g. definitions and a stack of
    transformation matrices."""
    def __init__(self):
        # Items that can be referenced by element ids
        self.items = {}

        # List of transformation matrices, recursive for groups and
        # similar SVG containers.
        self.matrices = [ [1, 0, 0, 1, 0, 0 ] ]

        # For state related warnings
        self.warnings = []
    def current_transform(self):
        """Returns the currently active transformation matrix."""
        return self.matrices[-1]

    def pop_transform(self):
        """Pops the latest transformation matrix"""
        if len(self.matrices) > 1:
            del self.matrices[-1]

    def push_transform(self, matrix):
        """Pushes the matrix, making it the current transformation"""
        self.matrices.append(matrix)

    def add_warning( self, warning ):
        self.warnings.append(warning)

def apply_matrix( obj_id, props, matrix ):
    """Multiplies the Tri of the object, specified by the obj_id, with
    the matrix."""
    tri = props.get_obj_tri(obj_id)
    p0 = tri.p0()
    p1 = tri.p1()
    p2 = tri.p2()
    p0 = multiply_matrix_pt(matrix, p0)
    p1 = multiply_matrix_pt(matrix, p1)
    p2 = multiply_matrix_pt(matrix, p2)
    props.set_obj_tri( obj_id, ifaint.Tri( p0, p1, p2 ) )

def apply_transforms( transforms, state ):
    """Pushes the current transformation matrix modified
    by the svg-transformations to the state"""
    if transforms is None:
        return False
    did_transform = False

    m_change = [1, 0, 0, 1, 0, 0 ]
    for transform_type, param in transforms:
        if transform_type == "skewX":
            rad = deg2rad( param[0])
            matrix = [1, 0, tan(rad), 1, 0, 0]
            m_change = multiply_matrices( m_change, matrix )
            did_transform = True
        elif transform_type == "skewY":
            rad = deg2rad( param )
            matrix = [1, tan(rad), 0, 1, 0, 0]
            m_change = multiply_matrices( m_change, matrix )
            did_transform = True
        elif transform_type == "rotate":
            rad = deg2rad(param[0])
            if len(param) > 1:
                x, y = param[1], param[2]
                matrix = multiply_matrices(
                    multiply_matrices(
                        translation_matrix(x, y),
                        rotation_matrix(rad)),
                    translation_matrix(-x, -y))
                m_change = multiply_matrices( m_change, matrix )
                did_transform = True
            else:
                matrix = rotation_matrix(rad)
                m_change = multiply_matrices( m_change, matrix )
                did_transform = True
        elif transform_type == "translate":
            matrix = translation_matrix(param[0], param[1])
            m_change = multiply_matrices( m_change, matrix )
            did_transform = True
        elif transform_type == "matrix":
            m_change = multiply_matrices( m_change, param )
            did_transform = True

    if did_transform:
        current = state.current_transform()
        state.push_transform( multiply_matrices(current, m_change) )
    return did_transform

def create_object( obj_info, props, state ):
    """Uses the obj_info to create an object. Adds it to the props and
    returns its in-props id."""

    obj_type = obj_info[0]
    if obj_type == 'empty_group':
        return None
    if obj_type == 'ellipse':
        obj_type, pos, settings, transforms = obj_info
        if pos.__class__ == ifaint.Tri:
            # The node had a faint:Tri attribute
            ellipse_id = props.Ellipse( (0, 0, 1, 1), settings )
            props.set_obj_tri(ellipse_id, pos)
        else:
            ellipse_id = props.Ellipse( pos, settings )
            transformed = apply_transforms( transforms, state )
            apply_matrix( ellipse_id, props, state.current_transform() )
            if transformed:
                state.pop_transform()
        return ellipse_id

    elif obj_type == 'group':
        obj_type, pos, settings, transform, child_objects = obj_info
        faint_objects = []
        transformed = apply_transforms( transform, state )
        for child_info in child_objects:
            obj_id = create_object( child_info, props, state )
            if obj_id is not None:
                faint_objects.append( obj_id )
        if len( faint_objects ) > 0:
            group_id = props.Group( faint_objects )
        else:
            group_id = None
        if transformed:
            state.pop_transform()
        return group_id

    elif obj_type == 'line':
        obj_type, pos, settings, transforms = obj_info # Fixme: Use transforms man
        line_id = props.Line( pos, settings )
        apply_matrix( line_id, props, state.current_transform() )
        return line_id

    elif obj_type == 'polyline':
        obj_type, pos, settings, transforms = obj_info # Fixme: Use transforms
        line_id = props.Line( pos, settings )
        return line_id

    elif obj_type == 'text':
        obj_type, text_rect, settings, transforms, text_str, anchor = obj_info
        text_id = props.Text(text_rect, text_str, settings )

        # SVG anchors at baseline, Faint at top of text
        dy = props.get_obj_text_height(text_id)
        tri = props.get_obj_tri(text_id)
        tri.translate(0, -dy)
        props.set_obj_tri(text_id, tri)
        transformed = apply_transforms( transforms, state )
        apply_matrix(text_id, props, state.current_transform())
        if transformed:
            state.pop_transform()
        return text_id

    elif obj_type == 'rect':
        obj_type, pos, settings, transforms = obj_info
        if pos.__class__ == ifaint.Tri:
            rect_id = props.Rect( (0, 0, 1, 1), settings)
            props.set_obj_tri(rect_id, pos)
        else:
            rect_id = props.Rect(pos, settings)
        transformed = apply_transforms( transforms, state )
        apply_matrix( rect_id, props, state.current_transform() )
        if transformed:
            state.pop_transform()
        return rect_id
    elif obj_type == 'polygon':
        obj_type, pos, settings, transforms = obj_info
        polygon_id = props.Polygon(pos, settings)
        transformed = apply_transforms( transforms, state )
        apply_matrix( polygon_id, props, state.current_transform() )
        if transformed:
            state.pop_transform()
        return polygon_id

    elif obj_type == 'spline':
        obj_type, points, settings, transforms = obj_info
        spline_id = props.Spline(points, settings)
        transformed = apply_transforms(transforms, state)
        apply_matrix( spline_id, props, state.current_transform() ) # if transfed
        if transformed:
            state.pop_transform()
        return spline_id

    elif obj_type == 'path':
        try:
            obj_type, path_def, settings, transforms = obj_info
            transformed = apply_transforms( transforms, state )

            # Fixme: Apparently, I must scale the line width with
            # the current transformation matrix
            try:
                lw = settings.linewidth
                ctm = state.current_transform()
                lw1 = lw * ctm[0]
                settings.linewidth = lw1
            except:
                pass # FIXME
            path_id = props.Path( path_def, settings ) # Fixme: Handle exception
            apply_matrix( path_id, props, state.current_transform() )

            if transformed:
                state.pop_transform()
            return path_id
        except ValueError, err:
            props.add_warning("Failed parsing a path definition.")
            return None

    elif obj_type == 'image':
        obj_type, (x, y, w, h), settings, transforms, data = obj_info
        image_type, image_data = data
        image_rect = x, y, w, h
        if image_type == "jpeg":
            image_id = props.Raster(image_rect, "jpeg", image_data, settings)
        elif image_type == "png":
            image_id = props.Raster(image_rect, "png", image_data, settings)
        else:
            props.add_warning("Failed creating image object")
            return None

        transformed = apply_transforms( transforms, state )
        apply_matrix( image_id, props, state.current_transform() )
        if transformed:
            state.pop_transform()
        return image_id
    else:
        assert( False )
    return None

def without_text_nodes(nodes):
    return [node for node in nodes if node.nodeName != "#text" and node.nodeName != "#comment"]

def parse_defs_node(node, props, state): # (props unused) pylint: disable=W0613
    """Recursively parses an SVG defs-node."""
    assert(node.nodeName == "defs")

    id_to_node = {}
    children = without_text_nodes(node.childNodes)
    for child in children:
        if child.hasAttribute('id'):
            id_to_node[child.getAttribute('id')] = child
    for child in children:
        item = None
        if child.nodeName == 'linearGradient':
            item = parse_linear_gradient_node(child, props, state, id_to_node)
        elif child.nodeName == 'radialGradient':
            item = parse_radial_gradient_node(child, props, state, id_to_node)
        elif child.nodeName == 'pattern':
            item = parse_pattern_node(child, props, state)

        if item is not None:
            state.items[item[0]] = item[1]

def parse_background_node( node, props, state ):
    assert(node.hasAttribute("faint:background"))
    assert(node.hasAttribute("xlink:href"))
    imageStr = node.getAttribute("xlink:href")
    imageType, data = parse_embedded_image_data(imageStr)
    assert (imageType == "png")
    props.set_background_png_string(data)

def parse_color_stops( nodes, props, state ):
    stop_list = []
    for stop in nodes:
        if stop.nodeName == "stop":
            # Fixme
            offset = util.parse_gradient_offset(stop.getAttribute("offset"))
            style_str = stop.getAttribute("style")
            color = parse_color_stop_style(style_str)
            stop_list.append((offset, color))
    return stop_list

def parse_linear_gradient_node( node, props, state, id_to_node ):
    """Parses a linearGradient element"""
    assert(node.nodeName == "linearGradient")
    if not node.hasAttribute("id"):
        # Ignore gradients without identifiers
        return None
    node_id = node.attributes["id"].value.strip()

    stops = []
    try:
        stops = parse_color_stops(node.childNodes, props, state)
        if len(stops) == 0:
            if node.hasAttribute("xlink:href"):
                xlink = node.getAttribute("xlink:href")
                ref_id = util.extract_local_xlink_href(xlink)
                linked = state.items.get(ref_id, None)
                if linked is None:
                    if ref_id in id_to_node.keys():
                        other = id_to_node[ref_id]
                        if other.nodeName == 'linearGradient':
                            linked_id, linked = parse_linear_gradient_node( other, props, state, id_to_node )
                        elif other.nodeName == 'radialGradient':
                            linked_id, linked = parse_radial_gradient_node( other, props, state, id_to_node )
                    if linked is None:
                        state.add_warning("Referenced gradient not found: %s " % ref_id)
                if linked is not None:
                    for num in range(linked.get_num_stops()):
                        stops.append( linked.get_stop(num) )
            else:
                # Ignore gradients without stops
                # (Note: Need to add handling of referenced gradients)
                props.add_warning("linearGradient with id=%s has no color-stops" % node_id)
                return None

        return node_id, ifaint.LinearGradient(stops)
    except StandardError, err:
        props.add_warning('Failed parsing linearGradient with id=%s\n"%s, %s"' % (node_id, str(err.__class__), str(err)))
        return None

def parse_pattern_node( node, props, state ):
    """Parses a pattern element"""
    node_id = node.getAttribute('id')
    for child in node.childNodes:
        if child.nodeName == 'image':
            encoding, data = parse_embedded_image_data(child.getAttribute("xlink:href"))
            if encoding == "png":
                return node_id, ifaint.Pattern(ifaint.bitmap_from_png_string(data))
    props.add_warning('Failed parsing pattern with id=%s' % node_id)
    return None

def parse_color_stop_style( style ):
    """Limited parsing of a style item string from a
    stop-element"""
    style_dict = parse_objects.get_style_dict(style)
    stop_opacity = 1.0
    if style_dict.has_key("stop-opacity"):
        stop_opacity = float(style_dict["stop-opacity"]) # Fixme: Parse, don't convert
    color = util.parse_color_noref(style_dict["stop-color"], stop_opacity)
    return color

def parse_radial_gradient_node(node, props, state, id_to_node):
    assert(node.nodeName == "radialGradient")
    if not node.hasAttribute("id"):
        # Ignore gradients without identifiers
        return None
    node_id = node.getAttribute("id")
    # Fixme: Add parsing of cx, cy, rx, ry

    stops = []
    try:
        stops = parse_color_stops(node.childNodes, props, state)
        if len(stops) == 0:
            # Fixme: Duplicated between radial and linear
            if node.hasAttribute("xlink:href"):
                xlink = node.getAttribute("xlink:href")
                ref_id = util.extract_local_xlink_href(xlink)
                linked = state.items.get(ref_id, None)
                if linked is None:
                    if ref_id in id_to_node.keys():
                        other = id_to_node[ref_id]
                        if other.nodeName == 'linearGradient':
                            linked_id, linked = parse_linear_gradient_node( other, props, state, id_to_node )
                        elif other.nodeName == 'radialGradient':
                            linked_id, linked = parse_radial_gradient_node( other, props, state, id_to_node )
                    if linked is None:
                        state.add_warning("Referenced gradient not found: %s " % ref_id)
                if linked is not None:
                    for num in range(linked.get_num_stops()):
                        stops.append( linked.get_stop(num) )
            else:
                # Ignore gradients without stops
                # (Note: Need to add handling of referenced gradients)
                props.add_warning("radialGradient with id=%s has no color-stops" % node_id)
                return None

        return node_id, ifaint.RadialGradient(stops)
    except StandardError, err:
        props.add_warning('Failed parsing radialGradient with id=%s\n"%s, %s"' % (node_id, str(err.__class__), str(err)))
        return None

def parse_switch_node(node, props, state):
    assert(node.nodeName == 'switch')
    # A reader should normally evaluate requiredFeatures,
    # requiredExtensions= and systemLanguage for all child nodes of a switch element
    # and select the most suitable item.
    # ...just try to find any recognized item.
    for childNode in node.childNodes:
        if parse_objects.has_parse_function(childNode.nodeName):
            parse_node( childNode, props, state )
            return
    props.add_warning("No supported child node of SVG switch-element")


def get_special_parse_function(node):
    """Returns a function for parsing the node. The function takes a
    node, props and state. Returns None if there's no suitable
    function."""

    if node.nodeName == 'image' and node.hasAttribute('faint:background'):
        return parse_background_node
    return {
        'defs' : parse_defs_node,
        'switch' : parse_switch_node,
        'svg' : parse_svg_node}.get(node.nodeName, None)

def parse_node( node, props, state ):
    """The general SVG node parsing function"""
    parse_special = get_special_parse_function(node)
    if parse_special is not None:
        parse_special(node, props, state)
        return None
    try:
        parse_function = parse_objects.get_parse_function(node.nodeName)
        if parse_function is not None:
            object_info = parse_function(node, state)
            if object_info is None:
                props.add_warning('Failed parsing node %s%s' % (node.nodeName, maybe_id_ref(node)))
                return None
            else:
                return create_object(object_info, props, state)
    except AssertionError:
        raise
    except StandardError, err:
        # Fixme: Rather generous exception handler, turning exceptions
        # into warnings. Remove.
        props.add_warning('Failed parsing node: %s%s\n"%s %s' % (node.nodeName,
          maybe_id_ref(node),
          str(err.__class__), str(err)))

def parse_svg_node( node, props, state ):
    """Recursively parses the svg-element"""
    if node.hasAttribute('width'):
        str_width = node.getAttribute('width')
    else:
        str_width = "100%"
    if node.hasAttribute('height'):
        str_height = node.getAttribute('height')
    else:
        str_height = "100%"

    if str_width[-1] == '%':
        width = 1024 # FIXME
    else:
        width = int(parse_value(str_width))

    if str_height[-1] == '%':
        height = 768 # FIXME
    else:
        height = int(parse_value(str_height))

    frameProps = props.add_frame(width, height)
    for child in node.childNodes:
        parse_node( child, frameProps, state )
