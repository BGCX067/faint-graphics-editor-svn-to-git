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

from code import InteractiveConsole
import sys
from ifaint import *
import ifaint

# Will be True while a command is run silently (e.g. from a keybind, not the interpreter)
_g_silent = False

# Will be True when a command has printed output, until a new prompt
# is displayed
_g_printed = False

class _mywriter:
    """[Faint internal] Replacement for stderr and stdout which
    directs output to the Faint interpreter window."""

    def write(self, str):
        if _g_silent:
            # Output is being made through a keypress or similar.  The
            # interpreter is probably in enter command mode (>>>).
            # So: Create a new line if this is the first print of a
            # batch (or the only print), so the output appears on a
            # prompt-free line
            global _g_printed
            if not _g_printed:
                int_faint_print("\n")
                # Signal that a print occured, to show that
                # a new prompt is needed etc.
                _g_printed = True

        int_faint_print(str)
sys.stdout = _mywriter()
sys.stderr = _mywriter()

# Python-interpreter, used for submitting Python strings for compilation.
# Also accepts partial commands which can be added on until completed
_console = InteractiveConsole();

def interpreter_new_prompt():
    """[Faint internal] Creates a new prompt (>>>) in the interpreter,
    and resets command states _g_printed and _g_silent"""
    global _g_printed
    global _g_silent
    _g_printed = False
    _g_silent = False
    int_ran_command()

def push( str_ ):
    """[Faint internal] Push a line of text to the Python interpreter process"""
    try:
        if _console.push( str_ ):
            # More strings must be pushed before
            # command can execute
            int_incomplete_command()
        else:
            # Command executed. Inform interested parties.
            if str_ != "":
                push("")
            else:
                interpreter_new_prompt()
    except AssertionError, e:
        int_faint_print('Assertion.\n')
        interpreter_new_prompt()
    except SystemExit:
        # post exit event to wx
        faint_quit()
        interpreter_new_prompt()
    except AttributeError, e:
        int_faint_print('Invalid attribute.')
        interpreter_new_prompt()
    except:
        int_faint_print('Unhandled exception: ' + str(sys.exc_info()[0]) )
        interpreter_new_prompt()

# Fixme: Must silent commands be complete? What happens if not?
def push_silent( str ):
    """[Faint internal] Push comands without outputting any feedback
    (e.g. for commands bound to keys)"""
    global _g_silent
    global _g_printed

    try:
        # Make the fact that the command is run silently
        # globally visible
        _g_silent = True

        for line in str.split('\n'):
            _console.push( line )
    finally:
        _g_silent = False

        if _g_printed:
            # Output was made, so the interpreter must
            # add a new prompt-line
            interpreter_new_prompt()

# Dictionary of keyboard codes connected to functions
ifaint._binds = {}

def keypress( key, modifier=0):
    """[Faint internal] Called when a key is pressed in Faint"""
    fn = ifaint._binds.get((key, modifier), None)
    if fn is not None:
        fn()

# Required for access outside envsetup.py
ifaint.keypress = keypress

class binder:
    """[Faint internal] Used for interactive function binding:

    This is required because function binding is done in steps:
    A global instance of this class is used to store the function that
    is to be bound until the user presses the key the function will be
    bound to to."""
    def __init__( self ):
        self._func = None
        self.bindGlobal = False

    def bind( self, function, bindGlobal=False ):
        """Interactively connect a Python-function to a keyboard key.

        Example use:
        >>> bind( zoom_in )
        [ Press key ]"""
        self.bindGlobal = bindGlobal

        if function.__class__ in _validFunctions:
            self._func = function
            int_get_key()
        else:
            raise TypeError("Invalid function")

    def bind_global( self, function ):
        """Like bind, but ignores which Faint window has focus."""
        self.bind( function, True )

    def bind2( self, key, modifiers=0 ):
        """[Faint internal] Bind is a two step operation, since it requires
        intervention from the console (a keypress). This is step two, which
        delegates to bindk"""
        if self._func is None:
            raise TypeError("No function ready for binding.")
        bindk( key, self._func, modifiers, self.bindGlobal )
        self._func = None

    def c_dummy( self ): pass

_b = binder()

# Create a tuple of bindable function types
def dummy(): pass
_builtIn = set_linewidth
_validFunctions = ( dummy.__class__, _b.c_dummy.__class__ , _builtIn.__class__)
del _builtIn
del dummy

ifaint.bind = _b.bind
ifaint.bind2 = _b.bind2
ifaint.bind_global = _b.bind_global

# Why named like this? Also: Not used
def __inifile__( path ):
    ifaint.inipath = path
    import os
    if not os.path.isfile( path ):
        push_silent("print 'User configuration file %s not found.'" %path)
        return
    try:
        execfile( path )
    except:
        push_silent("print 'An error occured reading user configuration file %s'" %path)

# Does not work because __inifile__ is not used
def reload_ini():
    """Reloads the users ini file"""
    __inifile__(ifaint.inipath)

def bindk( keycode, function, modifiers=0, bindGlobal=False ):
    """Connect a function to the key specified by keycode.
    For interactive binding, try bind( function ) instead."""
    ifaint._binds[ (keycode,modifiers) ] = function
    if bindGlobal:
        int_bind_key_global( keycode, modifiers )
    else:
        int_bind_key( keycode, modifiers )
ifaint.bindk = bindk

def bindk_global( keycode, function, modifiers=0 ):
    bindk( keycode, function, modifiers, True )

ifaint.bindk_global = bindk_global

def bindc( char, function, modifier=0 ):
    """Connect a function to the key specified by char.
    For interactive binding, try bind( function ) instead."""
    bindk( ord(char.upper()), function, modifier)
ifaint.bindc = bindc

def unbindc( char ):
    """Connect a function to the key specified by char.
    For interactive binding, try bind( function ) instead.""" # Fixme: interactive unbinding
    unbindk( ord(char) )

def unbindk( key, ask=False, verbose=True, modifiers=0 ):
    """Remove the bind from the given keycode"""
    if ifaint._binds.has_key((key,modifiers)):
        if verbose:
            print "Unbinding: " + str(key) + " " + ifaint._binds[ (key, modifiers) ].__name__
        del ifaint._binds[ (key, modifiers) ]
        int_unbind_key( key, modifiers )
ifaint.unbindk = unbindk

def unbindf( function ):
    """Removes all keybinds to the specified function."""
    d = ifaint._binds
    for key in d.keys():
        if d[key].__name__ == function:
            del d[key]
ifaint.unbindf = unbindf

def printDoc( docstr, indent ):
    lines = docstr.split('\n')
    if len(lines) == 1:
        print '  "' + lines[0].strip() + '"'
        return

    for line in lines[:-1]:
        print '  ' + line.strip()
    print '  ' + lines[-1] + '"'

def special_char(keycode):
    return not (40 < keycode < 150)

# Fixme: Duplicates keycode.cpp
class key:
    asterisk=39
    backspace=8
    delete=127
    end=312
    f1=340
    f8=347
    home=313
    num_minus=390
    num_plus=388
    paragraph=167
    pgdn=367
    pgup=366
    space=32

class mod:
    ctrl=1
    shift=2
    alt=4

def key_text( keycode, modifiers, numeric ):
    if numeric:
        keyLabel = str(keycode)
    else:
        keyLabel = int_get_key_name(keycode)

    if modifiers == 0:
        return keyLabel
    if modifiers & 4 == 4:
        keyLabel = "Alt+" + keyLabel
    if modifiers & 2 == 2:
        keyLabel = "Shift+" + keyLabel
    if modifiers & 1 == 1:
        keyLabel = "Ctrl+" + keyLabel
    return keyLabel

def binds( verbose=False, numeric=False ):
    """List what keyboard shortcuts are connected to Python functions.
    With verbose=True, doc strings are included.
    With numeric=True, key code values are shown instead of characters or
    key names. See also "bind" for help on binding."""

    # Dict to keep track of printed doc-strings under verbose,
    # to avoid repeating them for multiple binds
    printed = {}
    for key, modifiers in sorted(ifaint._binds, key=lambda bind : bind[0] ):
        func = ifaint._binds[(key, modifiers)]
        print " " + key_text( key, modifiers, numeric ) + ": " + func.__name__

        if verbose and func.__doc__ is not None:
            if ( func in printed ):
                print " (See " + str(printed[func]) + " above)"
            else:
                printed[func] = key_text(key, modifiers, numeric)
                printDoc( func.__doc__, 2 )
    if not verbose and not numeric:
        print "Use binds(t) to print doc-strings for functions."
ifaint.binds = binds

# For convenience
ifaint.f = False
ifaint.t = True

ifaint.window = FaintWindow()
ifaint.interpreter = FaintInterpreter()
ifaint.palette = FaintPalette()
ifaint.app = FaintApp()

def load_test( filename ):
    print "Load: " + filename

def save_test( filename ):
    print "Save: " + filename

def pick_color_fg():
    try:
        set_fgcol(get_pixel(*get_mouse_pos()))
    finally:
        return

def pick_color_bg():
    try:
        set_fgcol(get_pixel(*get_mouse_pos()))
    finally:
        return

def swap_colors():
    fgcol, bgcol = get_fgcol(), get_bgcol()
    set_fgcol( bgcol )
    set_bgcol( fgcol )

def select_top_object():
    objects = get_objects()
    if len(objects) > 0:
        tool(1)
        i = get_active_image()
        i.select( objects[-1] )

def other_selection():
    if tool() == 0:
        return 1
    return 0

def center_on_selected():
    objects = get_selected()
    if len(objects) > 0:
        center(objects[0].tri().center())

def tool_pen(): tool(2)
def tool_brush(): tool(3)
def tool_picker(): tool(4)
def tool_line(): tool(5)
def tool_spline(): tool(6)
def tool_rectangle(): tool(7)
def tool_ellipse(): tool(8)
def tool_polygon(): tool(9)
def tool_text(): tool(10)
def tool_fill(): tool(11)
def toggle_selection_type():
    tool(other_selection())

def raster_layer(): set_layer(0)
def object_layer(): set_layer(1)

#
# Forwarding functions for object creation to the active canvas
#
def Spline( *args, **kwArgs ):
    return get_active_image().Spline( *args, **kwArgs )

def Rect( *args, **kwArgs ):
    return get_active_image().Rect( *args, **kwArgs )

def Polygon( *args, **kwArgs ):
    return get_active_image().Polygon( *args, **kwArgs )

def Line( *args, **kwArgs ):
    return get_active_image().Line( *args, **kwArgs )

def Ellipse( *args, **kwArgs ):
    return get_active_image().Ellipse( *args, **kwArgs )

def Group( *args, **kwArgs ):
    return get_active_image().Group( *args, **kwArgs )

def Raster( *args, **kwArgs ):
    return get_active_image().Raster( *args, **kwArgs )

def Text( *args, **kwArgs ):
    return get_active_image().Text( *args, **kwArgs )
Text.__doc__ = "Adds a text object to the active image. Equivalent to get_active_images().Text(*args, **kwArgs)\n\nCanvas.Text:\n" + "".join(["  %s\n" % line for line in Canvas.Text.__doc__.split("\n")])

def Path( *args, **kwArgs ):
    return get_active_image().Path( *args, **kwArgs )

# Custom
def center_on_cursor():
    image = get_active_image()
    pos = image.get_mouse_pos()
    image.center( *pos )

def scroll_top_left(*args, **kwArgs):
    scroll_max_left()
    scroll_max_up()

def scroll_bottom_right(*args, **kwArgs):
    scroll_max_right()
    scroll_max_down()

# Other Forwarding functions

def _apply_images( func, images ):
    if len(images) == 0:
        return func(get_active_image())
    if len(images) == 1:
        firstObj = images[0]
        if firstObj.__class__ == ifaint.Canvas:
            func(firstObj)
            return
        else:
            for img in firstObj:
                if img.__class__ != ifaint.Canvas:
                    raise TypeError("Non-canvas specified")
            for img in firstObj:
                func(img)
    else:
        for img in images:
            if img.__class__ != ifaint.Canvas:
                raise TypeError("Non-canvas specified")
        for img in images:
            func(img)

def _forwarder( func ):
    l = lambda *images : _apply_images(func, images)
    l.__name__ = func.__name__
    l.__doc__ = "Forward to Canvas." + func.__doc__
    return l

def _active( func ):
    l = lambda *args, **kwArgs : func(get_active_image(), *args, **kwArgs)
    l.__name__ = func.__name__
    l.__doc__ = "Single-forward to Canvas." + func.__doc__
    return l

_c = ifaint.Canvas

# Forwarding functions for no-argument functions provide
# func() - apply to active image
# func(i1, i2) and func(imagelist) - apply to specified images
auto_crop = _forwarder(_c.auto_crop)
blur = _forwarder(_c.blur)
context_crop = _forwarder(_c.context_crop)
desaturate = _forwarder(_c.desaturate)
desaturate_weighted = _forwarder(_c.desaturate_weighted)
invert = _forwarder(_c.invert)
next_frame = _forwarder(_c.next_frame)
prev_frame = _forwarder(_c.prev_frame)
redo = _forwarder(_c.redo)
undo = _forwarder(_c.undo)
zoom_default = _forwarder(_c.zoom_default)
zoom_fit = _forwarder(_c.zoom_fit)
zoom_in = _forwarder(_c.zoom_in)
zoom_out = _forwarder(_c.zoom_out)

# Forwarding to the active image, supports arguments
aa_line = _active(_c.aa_line)
add_frame = _active(_c.add_frame)
center = _active(_c.center)
context_delete = _active(_c.context_delete)
context_flip_horizontal = _active(_c.context_flip_horizontal)
context_flip_vertical = _active(_c.context_flip_vertical)
context_rotate_90CW = _active(_c.context_rotate_90CW)
copy_rect = _active(_c.copy_rect)
delete_objects = _active(_c.delete_objects)
deselect = _active(_c.deselect)
dwim = _active(_c.dwim)
ellipse = _active(_c.ellipse)
erase_but_color = _active(_c.erase_but_color)
flatten = _active(_c.flatten)
get_image_size = _active(_c.get_size)
get_mouse_pos = _active(_c.get_mouse_pos)
get_objects = _active(_c.get_objects)
get_colors = _active(_c.get_colors)
get_pixel = _active(_c.get_pixel)
get_selected = _active(_c.get_selected)
get_selection = _active(_c.get_selection)
line = _active(_c.line)
paste = _active(_c.paste)
rect = _active(_c.rect)
replace_color = _active(_c.replace_color)
scroll_max_down = _active(_c.scroll_max_down)
scroll_max_left = _active(_c.scroll_max_left)
scroll_max_right = _active(_c.scroll_max_right)
scroll_max_up = _active(_c.scroll_max_up)
scroll_page_down = _active(_c.scroll_page_down)
scroll_page_left = _active(_c.scroll_page_left)
scroll_page_right = _active(_c.scroll_page_right)
scroll_page_up = _active(_c.scroll_page_up)
select = _active(_c.select)
set_image_rect = _active(_c.set_rect)
set_image_size = _active(_c.set_size)
set_pixel = _active(_c.set_pixel)
set_selection = _active(_c.set_selection)
shrink_selection = _active(_c.shrink_selection)

def delete_selected_objects():
    img = get_active_image()
    objects = img.get_selected()
    if len(objects) > 0:
        img.delete_objects(objects)

def scroll_top_left_all():
    for img in ifaint.images:
        img.scroll_max_left()
        img.scroll_max_up()

bindc( 'y', dwim, mod.alt )
bindc( 'r', prev_frame )
bindc( 't', next_frame )
bindk_global( key.f1, dialog_help )
bindk_global( key.f8, dialog_python_console )
bindk( key.end, scroll_bottom_right, mod.alt )
bindk( key.end, scroll_max_down, mod.ctrl )
bindk( key.end, scroll_max_right )
bindk( key.home, scroll_max_left )
bindk( key.home, scroll_max_up, mod.ctrl )
bindk( key.home, scroll_top_left, mod.alt )
bindk( key.home, scroll_top_left_all, mod.alt|mod.ctrl )
bindk( key.pgdn, scroll_page_down )
bindk( key.pgdn, scroll_page_right, mod.ctrl )
bindk( key.pgup, scroll_page_left, mod.ctrl )
bindk( key.pgup, scroll_page_up )
bindk( key.num_minus, zoom_out )
bindk( key.num_plus, zoom_in )
bindk( key.backspace, delete_selected_objects )

import py.testformat as testfmt
ifaint.add_format( testfmt.load, testfmt.save, "Testformat (*.tst)", "tst" )
del testfmt

import py.formatsvg as formatsvg
ifaint.add_format( formatsvg.load, formatsvg.save, "Scalable Vector Graphics(*.svg)", "svg")
del formatsvg

import py.formatpdf as formatpdf
ifaint.add_format( formatpdf.load, formatpdf.save, "Portable Document Format (*.pdf)", "pdf" )

def snowman():
    Text((20,20,40,40),u'\u2603', get_settings())

ifaint.grid = active_grid()

class ContextSettings:
    def __getattr__(self,key):
        return get_settings().__getattr__(key)
    def __setattr__(self,key, value):
        # Fixme: Not workable way of doing this. :)
        if key == 'linewidth':
            s = get_settings()
            s.linewidth = value
            update_settings(s)

import collections

class ContextList(collections.Sequence):
    """Behaves like a list of objects, but is initialized on demand by
     calling a function"""

    def __init__(self, func, doc=None):
        self.func = func
        self.__doc__ = doc
    def __getitem__(self,key):
        return self.func()[key]
    def __len__(self):
        return len(self.func())
    def __repr__(self):
        return str(self.func())
    def __iter__(self):
        return self.func().__iter__()

ifaint.settings = ContextSettings()
ifaint.images = ContextList(list_images, "A list of all opened images.")
ifaint.objects = ContextList(get_objects, "A list of all objects in the active image.")
ifaint.selected = ContextList(get_selected, "A list of all selected objects in the active image.")

# Somewhat temporary experiment
def anchor( images, anchors ):
    assert(len(images) == len(anchors))
    assert(len(images) > 1)
    max_x = anchors[0][0]
    max_y = anchors[0][1]
    for p in anchors:
        max_x = max(p[0], max_x)
        max_y = max(p[1], max_y)

    for img, anchor in zip(images, anchors):
        w, h = img.get_size()
        dx = anchor[0] - max_x
        dy = anchor[1] - max_y
        if ( dx != 0 or dy != 0 ):
            img.set_rect(dx, dy, w - dx, h - dy)

flagged = {}
def flag_pixel():
    image = get_active_image()
    pos = get_mouse_pos()
    flagged[image] = pos
    image.set_point_overlay(*pos)

def anchor_flagged():
    anchor( flagged.keys(), flagged.values() )
    for image in flagged.keys():
        image.clear_point_overlay()
    flagged.clear()

def clear_flagged():
    flagged.clear()

bindc('n', flag_pixel)

def listimages(path):
    # Fixme: Filter on extensions
    import os
    return [ os.path.join(path, name) for name in os.listdir(path)]

def faint_resolve( item_str ):
    try:
        object = _console.locals[item_str]
        if object.__class__ == Something:
            object.set_fgcol((255,0,0))
    except:
        pass

def help_class(c):
    print c.__name__ + ":"
    for line in c.__doc__.split('\n'):
        print " " + line

    print
    print " Methods:"
    for name in sorted([item for item in c.__dict__ if not item.startswith("_")]):
        doc = c.__dict__[name].__doc__.split('\n')
        if len(doc) < 2:
            continue
        print "  " + doc[0], doc[1]
        print "    " + "\n    ".join(doc[2:])

def help_method(m):
    print m.__objclass__.__name__ + '.' + m.__name__ + ":"
    doc = m.__doc__.split('\n')
    for line in doc:
        print " " + line

def help_object(o):
    print o.__doc__

def help_intro():
    print "Faint Python Help"
    print
    print "Use help(<name>) for help about named objects."
    print "Example: help(Canvas)"

def help(item=None):
    """Python Help, customized for Faint"""
    if item == None:
        help_intro()
    elif item.__class__ == type:
        help_class(item)
    elif hasattr(item, '__objclass__'):
        help_method(item)
    else:
        help_object(item)
