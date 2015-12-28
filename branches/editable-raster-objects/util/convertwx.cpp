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

#include "wx/bitmap.h"
#include "wx/event.h"
#include "wx/clipbrd.h"
#include "wx/filename.h"
#include "wx/mstream.h"
#include "wx/rawbmp.h"
#include "wx/tokenzr.h"
#include "app/app.hh"
#include "geo/geotypes.hh"
#include "tools/tool.hh"
#include "util/char.hh"

wxFileName absoluted( const wxFileName& filename ){
  wxFileName other(filename);
  other.MakeAbsolute();
  return other;
}

const wxCursor& to_wx_cursor( Cursor cursor ){
  return wxGetApp().GetCursor( cursor );
}

wxColour to_wx( const faint::ColRGB& c ){
  return wxColour( c.r, c.g, c.b );
}

wxRect to_wx( const IntRect& r ){
  return wxRect( r.x, r.y, r.w, r.h );
}

wxPoint to_wx( const IntPoint& p ){
  return wxPoint( p.x, p.y );
}

wxSize to_wx( const IntSize& sz ){
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

int get_tool_modifiers(){
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

int mouse_modifiers( const wxMouseEvent& event ){
  int modifiers = KeyModifiers();
  if ( event.LeftUp() || event.LeftDown() || event.LeftDClick() ){
    modifiers |= LEFT_MOUSE;
  }
  else if ( event.RightUp() || event.RightDown() ){
    modifiers |= RIGHT_MOUSE;
  }
  return modifiers;
}

wxImage to_wx_image( const faint::Bitmap& bmp ){
  using faint::uchar;
  const int bpp = 4;
  const int stride = bmp.m_row_stride;
  const uchar* bgraData = bmp.m_data;

  uchar* rgbData = (uchar*)malloc(bmp.m_w * bmp.m_h * 3); // Using malloc to match wxWidgets free
  uchar* aData = (uchar*)malloc(bmp.m_w * bmp.m_h); // Using malloc to match wxWidgets free
  for ( int y = 0; y != bmp.m_h; y++ ){
    for ( int x = 0; x != bmp.m_w; x++ ){
      size_t srcPos = y * stride + x * bpp;
      size_t dstPos_rgb = y * bmp.m_w * 3 + x * 3;
      size_t dstPos_alpha = y * bmp.m_w + x;
      rgbData[dstPos_rgb] = bgraData[srcPos + 2]; // R
      rgbData[dstPos_rgb + 1] = bgraData[srcPos + 1]; // G
      rgbData[dstPos_rgb + 2] = bgraData[srcPos + 0]; // B
      aData[dstPos_alpha] = bgraData[srcPos + 3]; // A
    }
  }

  // wxWidgets takes ownership of rgbData and aData, and uses free()
  // to release the memory.
  wxImage result(bmp.m_w, bmp.m_h, rgbData, aData );
  return result;
}

typedef wxPixelData<wxBitmap, wxAlphaPixelFormat> AlphaPixelData;
typedef AlphaPixelData PixelData;
wxBitmap to_wx_bmp( const faint::Bitmap& bmp ){
  wxBitmap wxBmp( bmp.m_w, bmp.m_h, 32 );
  PixelData pData( wxBmp );
  assert( pData );
  PixelData::Iterator p = pData;

  faint::uchar* data = bmp.m_data;
  const int stride = bmp.m_row_stride;
  const int bpp = 4;

  for ( int y = 0; y != bmp.m_h; y++ ){
    PixelData::Iterator rowStart = p;
    for ( int x = 0; x != bmp.m_w; x++ ){
      int pos = y *  stride + x * bpp;
      #ifdef __WXMSW__
      const faint::uchar alpha = *(data + pos + 3);
      p.Alpha() = alpha;
      p.Red() = ( *(data + pos + 2 ) * ( 255 - ( 255 - alpha ) ) ) / 255;
      p.Green() = ( *(data + pos + 1 ) * ( 255 - ( 255 - alpha  ) ) ) / 255;
      p.Blue() = ( *(data + pos ) * ( 255 - ( 255 - alpha ) ) ) / 255;
      #else
      p.Alpha() = *(data + pos + 3);
      p.Red()   = *(data + pos + 2);
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
  faint::Bitmap bmp(to_faint( wxBmp.GetSize()));
  if ( wxBmp.GetDepth() == 24 ){
    wxNativePixelData pixelData( wxBmp );
    if ( !pixelData ){
      goto alpha_label;
    }
    wxNativePixelData::Iterator p = pixelData;
    faint::uint stride = bmp.m_row_stride;
    faint::uchar* data = bmp.GetRaw();
    for ( int y = 0; y != bmp.m_h; y++ ){
      wxPixelData<wxBitmap, wxNativePixelFormat>::Iterator rowStart = p;
      for ( int x = 0; x != bmp.m_w; x++ ){
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
    for ( int y = 0; y != bmp.m_h; y++ ){
      AlphaPixelData::Iterator rowStart = p;
      for ( int x = 0; x != bmp.m_w; x++ ){
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
// Without this, pasting from clipboard in MSW causes this error:
// msw\bitmap.cpp(1287): assert "Assert failure" failed in
// wxBitmap::GetRawData(): failed to get DIBSECTION from a DIB?
//
// Probably this bug: http://trac.wxwidgets.org/ticket/11640
wxBitmap clean_bitmap( const wxBitmap& dirtyBmp ){
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

wxBitmap clean_bitmap( const wxBitmap& dirtyBmp ){
  return dirtyBmp;
}

#endif

faint::utf8_char to_faint( const wxChar& ch ){
  wxString s;
  s += ch;
  wxCharBuffer buf = s.utf8_str();
  return faint::utf8_char(std::string(buf, buf.length()));
}

faint::utf8_string to_faint( const wxString& str ){
  wxCharBuffer buf( str.utf8_str() );
  return faint::utf8_string(std::string(buf, buf.length()));
}

wxString to_wx( const faint::utf8_string& faintStr ){
  return wxString::FromUTF8(faintStr.c_str());
}

faint::FileList to_FileList( const wxArrayString& strings ){
  faint::FileList files;
  for ( wxArrayString::const_iterator it = strings.begin(); it != strings.end(); ++it ){
    files.push_back(faint::FilePath::FromAbsoluteWx(*it));
  }
  return files;
}

wxString get_clipboard_text(){
  wxTextDataObject textObject;
  if ( !wxTheClipboard->GetData(textObject) ){
    return "";
  }
  return textObject.GetText();
}

std::vector<wxString> wx_split_lines( const wxString& text ){
  wxStringTokenizer tokenizer(text, "\r\n");
  std::vector<wxString> v;
  while ( tokenizer.HasMoreTokens() ){
    v.push_back( tokenizer.GetNextToken() );
  }
  return v;
}
