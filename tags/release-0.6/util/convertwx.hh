// Copyright 2012 Lukas Kemmer
//
// Licensed under the Apache License, Version 2.0 (the "License"); you
// may not use this file except in compliance with the License. You
// may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied. See the License for the specific language governing
// permissions and limitations under the License.

#ifndef FAINT_CONVERTWX_HH
#define FAINT_CONVERTWX_HH
#include <string>
#include <vector>
#include "wx/chartype.h"
#include "wx/arrstr.h"
#include "gui/cursors.hh"
#include "util/char.hh"
#include "util/commonfwd.hh"

namespace faint{
  class Bitmap;
  class Color;
}
class IntPoint;
class IntRect;
class IntSize;
class Point;
class Size;
class wxBitmap;
class wxColour;
class wxCursor;
class wxKeyEvent;
class wxMouseEvent;
class wxPoint;
class wxRect;
class wxSize;
class wxString;

wxCursor& to_wx_cursor( Cursor::type );
wxColour to_wx( const faint::Color& );
wxRect to_wx( const IntRect& );
wxPoint to_wx( const IntPoint& );
wxSize to_wx( const IntSize& );

wxBitmap to_wx( const faint::Bitmap& );

faint::Color to_faint( const wxColour& );
IntRect to_faint( const wxRect& );
IntPoint to_faint( const wxPoint& );
IntSize to_faint( const wxSize& );
faint::Bitmap to_faint( wxBitmap );
std::vector<std::string> to_vector( const wxArrayString& );

bool is_tool_modifier( int keycode );
int get_tool_modifiers();
int mouse_modifiers( const wxMouseEvent& );

// Getting raw data for pasted bitmaps fails in MSW unless this
// function is called.
wxBitmap clean_bitmap( wxBitmap dirtyBmp );

faint::utf8_char to_faint( const wxChar& );
faint::utf8_string to_faint( const wxString& );

wxString to_wx( const faint::utf8_string& );
#endif
