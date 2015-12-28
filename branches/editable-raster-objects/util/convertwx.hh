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
#include "util/path.hh"

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
class wxFileName;
class wxImage;
class wxKeyEvent;
class wxMouseEvent;
class wxPoint;
class wxRect;
class wxSize;
class wxString;

wxFileName absoluted( const wxFileName& );

const wxCursor& to_wx_cursor( Cursor );
wxColour to_wx( const faint::ColRGB& );
wxRect to_wx( const IntRect& );
wxPoint to_wx( const IntPoint& );
wxSize to_wx( const IntSize& );

// Converts a faint::Bitmap to a wxBitmap.
// Note that wxImage (see to_wx_image) may be preferable, especially
// for wxImage::SaveFile.
wxBitmap to_wx_bmp( const faint::Bitmap& );

// Convert a faint::Bitmap to a wxImage.
//
// Note: wxImage seems less platform-dependent than wxBitmap. This is
// preferable, as it will cause less surprises depending on for
// example the native bit-depth support. Also when saving, wxBitmap in
// some cases converts to wxImage, so to_wx_image may be a more direct
// route.
wxImage to_wx_image( const faint::Bitmap& );

faint::Color to_faint( const wxColour& );
IntRect to_faint( const wxRect& );
IntPoint to_faint( const wxPoint& );
IntSize to_faint( const wxSize& );
faint::Bitmap to_faint( wxBitmap );
faint::FileList to_FileList( const wxArrayString& );

int get_tool_modifiers();
int mouse_modifiers( const wxMouseEvent& );

// Getting raw data for pasted bitmaps fails in MSW unless this
// function is called.
wxBitmap clean_bitmap( const wxBitmap& dirtyBmp );

faint::utf8_char to_faint( const wxChar& );
faint::utf8_string to_faint( const wxString& );

wxString to_wx( const faint::utf8_string& );

wxString get_clipboard_text();
std::vector<wxString> wx_split_lines(const wxString&);
#endif
