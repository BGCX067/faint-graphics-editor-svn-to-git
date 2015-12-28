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
namespace faint{
  class Bitmap;
  class Color;
}

class wxBitmap;
class wxColour;
class wxCursor;
class wxKeyEvent;
class wxMouseEvent;
class wxPoint;
class wxRect;
class wxSize;
wxCursor& to_wx_cursor( int cursorId );
wxColour to_wx( const faint::Color& );
wxRect to_wx( const IntRect& );
wxPoint to_wx( const Point& ); // Fixme: Remove?
wxPoint to_wx( const IntPoint& );
wxSize to_wx( const Size& );
wxBitmap to_wx( const faint::Bitmap& );

faint::Color to_faint( const wxColour& );
IntRect to_faint( const wxRect& );
IntPoint to_faint( const wxPoint& );
IntSize to_faint( const wxSize& );

bool IsToolModifier( int keycode );
int GetToolModifiers();
int MouseModifiers( const wxMouseEvent& );

faint::Bitmap to_faint( wxBitmap );

// Getting raw data for pasted bitmaps fails in MSW unless this
// function is called.
wxBitmap CleanBitmap( wxBitmap dirtyBmp );

#endif
