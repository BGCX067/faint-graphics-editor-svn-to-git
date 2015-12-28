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

    def bind( self, function ):
        """Interactively connect a Python-function to a keyboard key.

        Example use:
        >>> bind( zoom_in )
        [ Press key ]"""

        if function.__class__ in _validFunctions:
            self._func = function
            int_get_key()
        else:
            raise TypeError("Invalid function")

    def bind2( self, key, modifiers=0 ):
        """[Faint internal] Bind is a two step operation, since it requires
        intervention from the console (a keypress). This is step two, which
        delegates to bindk"""
        if self._func is None:
            raise TypeError("No function ready for binding.")
        bindk( key, self._func, modifiers )
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

def bindk( keycode, function, modifiers=0 ):
    """Connect a function to the key specified by keycode.
    For interactive binding, try bind( function ) instead."""
    ifaint._binds[ (keycode,modifiers) ] = function
    int_bind_key( keycode, modifiers )
ifaint.bindk = bindk

def bindc( char, function, modifier=0 ):
    """Connect a function to the key specified by char.
    For interactive binding, try bind( function ) instead."""
    bindk( ord(char.upper()), function, modifier)
ifaint.bindc = bindc

def unbindc( char ):
    """Connect a function to the key specified by char.
    For interactive binding, try bind( function ) instead."""
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

def get_special_label(keycode):
    keys = {32: "Space",
            167: "Pgph",
            312: "End",
            313: "Home",
            366: "PgUp",
            367: "PgDn",

            }
    label = keys.get(keycode, None)
    if label == None:
        return "<%d>" % keycode
    return label

def key_text( keycode, modifiers, numeric ):
    if numeric:
        keyLabel = str(keycode)
    elif special_char(keycode):
        keyLabel = get_special_label(keycode)
    else:
        keyLabel = unichr( keycode ).encode('UTF-8')

    if modifiers == 0:
        return keyLabel
    if modifiers & 4 == 4:
        keyLabel = "alt+" + keyLabel
    if modifiers & 2 == 2:
        keyLabel = "shift+" + keyLabel
    if modifiers & 1 == 1:
        keyLabel = "ctrl+" + keyLabel
    return keyLabel[0].capitalize() + keyLabel[1:]



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
                print " See " + str(printed[func]) + " above)"
            else:
                printed[func] = (key, modifiers)
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

def load_test( filename ):
    print "Load: " + filename

def save_test( filename ):
    print "Save: " + filename

def undo():
    get_active_image().undo()

def redo():
    get_active_image().redo()

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

def Path( *args, **kwArgs ):
    return get_active_image().Path( *args, **kwArgs )

# Other Forwarding functions
def set_image_rect( *args, **kwArgs ):
    return get_active_image().set_rect( *args, **kwArgs )

def set_image_size( *args, **kwArgs ):
    return get_active_image().set_size( *args, **kwArgs )

def set_selection( *args, **kwArgs ):
    return get_active_image().set_selection( *args, **kwArgs )

def shrink_selection( *args, **kwArgs ):
    return get_active_image().shrink_selection( *args, **kwArgs )

def get_selection( *args, **kwArgs ):
    return get_active_image().get_selection( *args, **kwArgs )

def desaturate():
    return get_active_image().desaturate()

def desaturate_weighted():
    return get_active_image().desaturate_weighted()

def context_delete():
    return get_active_image().context_delete()

def invert():
    return get_active_image().invert()

def replace_color(*args, **kwArgs):
    return get_active_image().replace_color(*args, **kwArgs)

def line( *args, **kwArgs ):
    return get_active_image().line( *args, **kwArgs )

def rect( *args, **kwArgs ):
    return get_active_image().rect( *args, **kwArgs )

def ellipse( *args, **kwArgs ):
    return get_active_image().ellipse( *args, **kwArgs )

def select( *args, **kwArgs ):
    return get_active_image().select( *args, **kwArgs )

def set_pixel( *args, **kwArgs ):
    return get_active_image().set_pixel( *args, **kwArgs )

def get_pixel( *args, **kwArgs ):
    return get_active_image().get_pixel( *args, **kwArgs )

def get_objects( *args, **kwArgs ):
    return get_active_image().get_objects( *args, **kwArgs )

def get_selected( *args, **kwArgs ):
    return get_active_image().get_selected( *args, **kwArgs )

def get_mouse_pos( *args, **kwArgs ):
    return get_active_image().get_mouse_pos( *args, **kwArgs )

def delete_objects( *args, **kwArgs ):
    return get_active_image().delete_objects( *args, **kwArgs )

def get_image_size( *args, **kwArgs ):
    return get_active_image().get_size( *args, **kwArgs )

def context_flip_horizontal( *args, **kwArgs ):
    return get_active_image().context_flip_horizontal( *args, **kwArgs )

def context_flip_vertical( *args, **kwArgs ):
    return get_active_image().context_flip_vertical( *args, **kwArgs )

def context_rotate_90CW( *args, **kwArgs ):
    return get_active_image().context_rotate_90CW( *args, **kwArgs )

def center( *args, **kwArgs ):
    return get_active_image().center( *args, **kwArgs )

def center_on_cursor():
    image = get_active_image()
    pos = image.get_mouse_pos()
    image.center( *pos )

def scroll_page_up(*args,**kwArgs):
    return get_active_image().scroll_page_up( *args, **kwArgs )

def scroll_page_down(*args,**kwArgs):
    return get_active_image().scroll_page_down( *args, **kwArgs )

def scroll_page_left(*args,**kwArgs):
    return get_active_image().scroll_page_left( *args, **kwArgs )

def scroll_page_right(*args,**kwArgs):
    return get_active_image().scroll_page_right( *args, **kwArgs )

def scroll_max_left(*args,**kwArgs):
    return get_active_image().scroll_max_left( *args, **kwArgs )

def scroll_max_up(*args, **kwArgs):
    return get_active_image().scroll_max_up(*args, **kwArgs)

def scroll_max_down(*args, **kwArgs):
    return get_active_image().scroll_max_down(*args, **kwArgs)

def scroll_max_right(*args, **kwArgs):
    return get_active_image().scroll_max_right(*args, **kwArgs)

def scroll_top_left(*args, **kwArgs):
    scroll_max_left()
    scroll_max_up()

def scroll_bottom_right(*args, **kwArgs):
    scroll_max_right()
    scroll_max_down()

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

class key:
    space=32
    paragraph=167
    pgup=366
    pgdn=367
    home=313
    end=312
    num_plus=388
    num_minus=390

class mod:
    ctrl=1
    shift=2
    alt=4

bindk( key.end, scroll_bottom_right, mod.alt )
bindk( key.end, scroll_max_down, mod.ctrl )
bindk( key.end, scroll_max_right )
bindk( key.home, scroll_max_left )
bindk( key.home, scroll_max_up, mod.ctrl )
bindk( key.home, scroll_top_left, mod.alt )
bindk( key.pgdn, scroll_page_down )
bindk( key.pgdn, scroll_page_right, mod.ctrl )
bindk( key.pgup, scroll_page_left, mod.ctrl )
bindk( key.pgup, scroll_page_up )
bindk( key.num_minus, zoom_out )
bindk( key.num_plus, zoom_in )


import py.testformat as testfmt
ifaint.add_format( testfmt.load, testfmt.save, "Testformat (*.tst)", "tst" )
del testfmt

import py.formatsvg as formatsvg
ifaint.add_format( formatsvg.load, formatsvg.save, "Scalable Vector Graphics(*.svg)", "svg")
del formatsvg

import py.formatpdf as formatpdf
ifaint.add_format( formatpdf.load, formatpdf.save, "Portable Document Format (*.pdf)", "pdf" )
