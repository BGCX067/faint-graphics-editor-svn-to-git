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

#include "app.hh"
#include "wx/bitmap.h"
#include "wx/rawbmp.h"
#include "wx/event.h"
#include "wx/mstream.h"
#include "geo/geotypes.hh"
#include "tools/toolbehavior.hh"
#include "bitmap/bitmap.h"
#include "bitmap/cairo_util.h"

wxCursor& to_wx_cursor( int cursorId ){
  return wxGetApp().GetCursor( cursorId );
}

wxColour to_wx( const faint::Color& c ){
  return wxColour( c.r, c.g, c.b );
}

wxRect to_wx( const IntRect& r ){
  return wxRect( r.x, r.y, r.w, r.h );
}

wxPoint to_wx( const IntPoint& p ){
  return wxPoint( p.x, p.y );
}

wxPoint to_wx( const Point& p ){
  return wxPoint( rounded(p.x + 0.5), rounded(p.y + 0.5) ); // Fixme: Weirdness, why round here? :(
}

wxSize to_wx( const Size& sz ){
  return wxSize( sz.w, sz.h );
}

IntPoint to_faint( const wxPoint& p ){
  return IntPoint( p.x, p.y );
}

faint::Color to_faint( const wxColour& c ){
  return faint::Color( c.Red(), c.Green(), c.Blue() );
}

IntRect to_faint( const wxRect& r ){
  return IntRect( IntPoint(r.x, r.y), IntSize( r.width, r.height ) );
}

IntSize to_faint( const wxSize& sz ){
  return IntSize( sz.GetWidth(), sz.GetHeight() );
}

int KeyModifiers(){
  int modifiers = 0;
  if ( wxGetKeyState( WXK_CONTROL ) ){
    modifiers |= TOOLMODIFIER1;
  }
  if ( wxGetKeyState( WXK_SHIFT ) ){
    modifiers |= TOOLMODIFIER2;
  }
  return modifiers;
}

int GetToolModifiers(){
  int modifiers = KeyModifiers();
  wxMouseState mouseState = wxGetMouseState();
  if ( mouseState.LeftIsDown() ){
    modifiers |= LEFT_MOUSE;
  }
  else if ( mouseState.RightIsDown() ){
    modifiers |= RIGHT_MOUSE;
  }
  return modifiers;
}

int MouseModifiers( const wxMouseEvent& event ){
  int modifiers = KeyModifiers();
  if ( event.LeftUp() || event.LeftDown() ){
    modifiers |= LEFT_MOUSE;
  }
  else if ( event.RightUp() || event.RightDown() ){
    modifiers |= RIGHT_MOUSE;
  }
  return modifiers;
}

bool IsToolModifier( int keycode ){
  return keycode == WXK_CONTROL || keycode == WXK_SHIFT;
}

typedef wxPixelData<wxBitmap, wxAlphaPixelFormat> AlphaPixelData;
typedef AlphaPixelData PixelData;
wxBitmap to_wx( const faint::Bitmap& bmp ){
  wxBitmap wxBmp( bmp.m_w, bmp.m_h, 32 );
  PixelData pData( wxBmp );
  assert( pData );
  PixelData::Iterator p = pData;

  faint::uchar* data = bmp.m_data;
  const int stride = bmp.m_row_stride;
  const int bpp = 4;

  for ( size_t y = 0; y != bmp.m_h; y++ ){
    PixelData::Iterator rowStart = p;
    for ( size_t x = 0; x != bmp.m_w; x++ ){
      int pos = y *  stride + x * bpp;
      #ifdef __WXMSW__
      const faint::uchar alpha = *(data + pos + 3);
      p.Alpha() = alpha;
      p.Red() = ( *(data + pos + 2 ) * ( 255 - ( 255 - alpha ) ) ) / 255;
      p.Green() = ( *(data + pos + 1 ) * ( 255 - ( 255 - alpha  ) ) ) / 255;
      p.Blue() = ( *(data + pos ) * ( 255 - ( 255 - alpha ) ) ) / 255;
      #else
      p.Alpha()   = *(data + pos + 3);
      p.Red()   = *(data + pos + 2 );
      p.Green() = *(data + pos + 1);
      p.Blue()  = *(data + pos );
      #endif
      ++p;
    }
    p = rowStart;
    p.OffsetY( pData, 1 );
  }
  return wxBmp;
}

faint::Bitmap to_faint( wxBitmap wxBmp ){
  faint::Bitmap bmp( faint::CairoCompatibleBitmap( wxBmp.GetWidth(), wxBmp.GetHeight() ) );
  if ( wxBmp.GetDepth() == 24 ){
    wxNativePixelData pixelData( wxBmp );
    if ( !pixelData ){
      goto alpha_label;
    }
    wxNativePixelData::Iterator p = pixelData;
    faint::uint stride = bmp.m_row_stride;
    faint::uchar* data = bmp.GetRaw();
    for ( size_t y = 0; y != bmp.m_h; y++ ){
      wxPixelData<wxBitmap, wxNativePixelFormat>::Iterator rowStart = p;
      for ( size_t x = 0; x != bmp.m_w; x++ ){
        int pos = y * stride + x * 4;
        data[ pos + 3 ] = 255;
        data[ pos + 2 ] = p.Red();
        data[ pos + 1 ] = p.Green();
        data[ pos + 0 ] = p.Blue();
        ++p;
      }
      p = rowStart;
      p.OffsetY( pixelData, 1 );
    }
  }
  else {
  alpha_label:
    AlphaPixelData pixelData( wxBmp );
    assert( pixelData );
    AlphaPixelData::Iterator p = pixelData;
    faint::uint stride = bmp.m_row_stride;
    faint::uchar* data = bmp.GetRaw();
    for ( size_t y = 0; y != bmp.m_h; y++ ){
      AlphaPixelData::Iterator rowStart = p;
      for ( size_t x = 0; x != bmp.m_w; x++ ){
        int pos = y * stride + x * 4;
        #ifdef __WXMSW__
        // Convert back from premultiplied alpha
        data[ pos + 3 ] = p.Alpha();
        if ( p.Alpha() != 0 ){
          data[ pos + 2 ] = ( p.Red() * 255 ) / ( 255 - ( 255 - p.Alpha() ) );
          data[ pos + 1 ] = ( p.Green() * 255 ) / ( 255 - ( 255 - p.Alpha() ) );
          data[ pos + 0 ] = ( p.Blue() * 255 ) / ( 255 - ( 255 - p.Alpha() ) );
        }
        #else
        data[ pos + 3 ] = p.Alpha();
        data[ pos + 2 ] = p.Red();
        data[ pos + 1 ] = p.Green();
        data[ pos + 0 ] = p.Blue();
        #endif
        ++p;
      }
      p = rowStart;
      p.OffsetY( pixelData, 1 );
    }
  }
  return bmp;
}

#ifdef __WXMSW__
#include "wx/dcmemory.h"
// Fixme:
// Without this, pasting from clipboard in MSW causes this error:
// msw\bitmap.cpp(1287): assert "Assert failure" failed in
// wxBitmap::GetRawData(): failed to get DIBSECTION from a DIB?
//
// Probably this bug: http://trac.wxwidgets.org/ticket/11640
wxBitmap CleanBitmap( wxBitmap dirtyBmp ){
  // 24bpp depth loses alpha information, but wxMemoryDC does not
  // support alpha Setting this to 32 or retrieving from the bitmap
  // works for pastes between applications and within Faint, but
  // fails for pastes from print screen (gives very weird effects,
  // probably random alpha values).
  // I've also tried GCDC Blit and DrawBitmap, neither retained alpha
  wxBitmap cleanBmp( dirtyBmp.GetWidth(), dirtyBmp.GetHeight(), 24 );
  wxMemoryDC cleanDC( cleanBmp );
  cleanDC.DrawBitmap( dirtyBmp, 0, 0 );
  cleanDC.SelectObject( wxNullBitmap );
  return cleanBmp;
}

#endif

#ifndef __WXMSW__
wxBitmap CleanBitmap( wxBitmap dirtyBmp ){
  return dirtyBmp;
}

#endif


