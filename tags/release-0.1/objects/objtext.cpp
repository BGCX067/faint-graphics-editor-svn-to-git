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

#include "objtext.hh"
#include "bitmap/cairo_util.h"
#include "faintdc.hh"
#include "objects/objline.hh"
#include "objects/objrectangle.hh"
#include "objutil.hh"
#include "settings.hh"
#include "split_string.hh"
#include "tools/settingid.hh"
#include "commands/tricommand.hh"
#include "util/util.hh"

const std::string s_TypeText = "Text";

FaintSettings GetTextSettings(){
  FaintSettings s;
  s.Set( ts_FgCol, faint::Color(0,0,0) );
  s.Set( ts_BgCol, faint::Color( 255, 255, 255 ) );
  s.Set( ts_SwapColors, false );
  s.Set( ts_FontSize, 12 );
  s.Set( ts_FontFace, "Arial" ); // Fixme
  s.Set( ts_FontBold, false );
  s.Set( ts_FontItalic, false );
  s.Set( ts_TextAutoRect, false );
  return s;
}

faint::coord ComputeRowHeight( FaintDC& dc, const FaintSettings& settings ){
  return dc.TextExtents("M", settings).h;
}

faint::coord ComputeRowHeight( const FaintSettings& settings ){
  faint::Bitmap dummy( faint::CairoCompatibleBitmap( 10, 10 ) );
  FaintDC dc( dummy );
  return ComputeRowHeight( dc, settings );
}

ObjText::ObjText( const Rect& r, const wxString& text,  const FaintSettings& settings )
  : Object( &s_TypeText, TriFromRect(r), settings ),
    m_textBuf( text )
{
  Init();
}

ObjText::ObjText( const ObjText& other )
  : Object( &s_TypeText, other.GetTri(), other.GetSettings() ),
    m_textBuf(other.m_textBuf)
{
  Init();
}

void ObjText::Init(){
  assert( m_settings.Has( ts_FontSize ) );
  assert( m_settings.Has( ts_FontFace ) );
  m_beingEdited = false;

  m_highlightSettings = GetRectangleSettings();
  m_highlightSettings.Set( ts_FillStyle, FILL );
  faint::Color highlightColor = GetHighlightColor();
  m_highlightSettings.Set( ts_FgCol, highlightColor );
  m_highlightSettings.Set( ts_BgCol, highlightColor );

  m_rowHeight = ComputeRowHeight( m_settings );
  m_lastFontSize = m_settings.Get( ts_FontSize );
  m_lastFontFace = m_settings.Get( ts_FontFace );
}

void TranslatePos( const text_lines_t& lines, size_t pos, size_t& rowNum, size_t& charNum ){
  const size_t caret = pos;
  size_t caretRow = 0;
  size_t caretChar = 0;
  size_t chars = 0;
  for ( size_t i = 0; i != lines.size(); i++ ){
    caretRow = i;
    const wxString& line = lines[i].second;
    chars += line.size();
    if ( chars > caret ) {
      caretChar = line.size() - ( chars - caret );
      break;
    }
    caretChar = line.size() - 1;
  }

  rowNum = caretRow;
  charNum = caretChar;
}

text_lines_t split_text_buffer( const TextBuffer& buffer, faint::coord width, const FaintSettings& settings ){
  faint::Bitmap dummy( faint::CairoCompatibleBitmap( 10, 10 ) );
  FaintDC dc( dummy );
  return split_str( dc, buffer.get(), truncated(width), settings );
}

void DrawSelection( FaintDC& dc, const TextBuffer& textBuf, const text_lines_t& lines, const FaintSettings& settings, const Tri& tri, const FaintSettings& highlightSettings, const faint::coord ht ){
  text_rng selection = textBuf.get_sel_range();
  if ( selection.first == selection.second ){
    return;
  }

  // Find the start and end positions
  size_t row_s0, char_s0;
  size_t row_s1, char_s1;
  TranslatePos( lines, selection.first, row_s0, char_s0 );
  TranslatePos( lines, selection.second, row_s1, char_s1 );
  Size size0 = dc.TextExtents( std::string( lines[ row_s0 ].second.Mid(0, char_s0).ToUTF8()), settings );
  Size size1 = dc.TextExtents( std::string( lines[ row_s1 ].second.Mid(0, char_s1).ToUTF8()), settings );

  for ( size_t row = row_s0; row <= row_s1; row++ ){
    faint::coord dx = row * ht * sin( -tri.Angle() );
    faint::coord dy = row * ht * cos( tri.Angle() ) - 1;
    Size rowSize = dc.TextExtents( std::string( lines[ row ].second.ToUTF8() ), settings );

    // Start of the row
    Tri t2 = Translated( tri, dx, dy );

    // End of the selection
    faint::coord right_offset = row == row_s1 ? size1.w : rowSize.w;
    Tri t3 = OffsetAligned( t2, right_offset + LITCRD(1.0), LITCRD(0.0) );

    if ( row == row_s0 ){
      // Offset the selection start
      t2 = OffsetAligned( t2, size0.w, LITCRD(0.0) );
    }
    dc.Rectangle( Tri( t2.P0(), t3.P0(), ht ), highlightSettings );
  }
}

void ObjText::Draw( FaintDC& dc ){
  Tri tri(GetTri());
  text_lines_t lines = split_text_buffer( m_textBuf, tri.Width(), m_settings );
  m_rowHeight = ComputeRowHeight( dc, m_settings );
  faint::coord ht = m_rowHeight + LineSpacing();

  if ( m_textBuf.size() == 0 ){
    // Compute the caret position
    Tri caretTri( tri.P0(), tri.P1(), ht );
    caretTri = OffsetAligned( caretTri, LITCRD(1.0), LITCRD(0.0) );
    m_caret = Line(caretTri.P0(), caretTri.P2() );
    return;
  }

  if ( m_beingEdited ){
    DrawSelection( dc, m_textBuf, lines, m_settings, tri, m_highlightSettings, ht );
  }

  // Draw the text
  for ( size_t row = 0; row != lines.size(); row++ ){
    // Fixme: Selected text should be drawn with wxSYS_COLOUR_HIGHLIGHTTEXT
    faint::coord dx = row * ht * sin( -tri.Angle() );
    faint::coord dy = row * ht * cos( tri.Angle() );
    dc.Text( Translated(tri, dx, dy), std::string( lines[row].second.ToUTF8() ), m_settings );
  }

  if ( m_beingEdited ){
    text_rng selection = m_textBuf.get_sel_range();
    if ( selection.first != selection.second ){
      return;
    }
    m_caret = ComputeCaret( dc, tri, ht, lines );
  }
}

void ObjText::DrawMask( FaintDC& dc ){
  FaintSettings s(GetRectangleSettings());
  s.Set( ts_FillStyle, BORDER_AND_FILL );
  s.Set( ts_FgCol, mask_edge );
  s.Set( ts_BgCol, mask_fill );

  text_lines_t lines = split_text_buffer( m_textBuf, GetTri().Width(), m_settings );
  m_rowHeight = ComputeRowHeight( dc, m_settings );
  const faint::coord ht = m_rowHeight + LineSpacing();

  // Draw a rectangle from the left edge to the right for each line of text
  // Fixme: Only draw up to the line width
  for ( size_t row = 0; row != lines.size(); row++ ){
    // Fixme: Selected text should be drawn with wxSYS_COLOUR_HIGHLIGHTTEXT
    faint::coord dx = row * ht * sin( -GetTri().Angle() );
    faint::coord dy = row * ht * cos( GetTri().Angle() );

    Tri topLeft( Translated( GetTri(), dx, dy ) );
    Tri tri( topLeft.P0(), topLeft.P1(), ht );
    dc.Rectangle( tri, s );
  }
}

IntRect ObjText::GetRefreshRect(){
  return truncated(GetRect());
}

  bool ObjText::HitTest( const Point& p ){
  return GetRect().Contains(p);
}

std::vector<Point> ObjText::GetResizePoints(){
  return Corners(GetRect());
}

Object* ObjText::Clone() const{
  return new ObjText( *this );
}

const wxString& ObjText::GetString(){
  return m_textBuf.get();
}

TextBuffer* ObjText::GetTextBuffer(){
  return &m_textBuf;
}

void ObjText::SetEdited( bool edited ){
  m_beingEdited = edited;
}

Line ObjText::ComputeCaret( const FaintDC& dc, const Tri& tri, faint::coord ht, const text_lines_t& lines ){
  size_t caretRow, caretChar;
  TranslatePos( lines, m_textBuf.caret(), caretRow, caretChar );
  Size textSize = dc.TextExtents( std::string( lines[ caretRow ].second.Mid(0, caretChar ).ToUTF8() ), m_settings );
  textSize.h = ht;
  Tri caretTri( tri.P0(), tri.P1(), textSize.h );
  caretTri = OffsetAligned( caretTri, textSize.w, caretRow * ht );
  return Line( caretTri.P0(), caretTri.P2() );  
}

faint::coord ObjText::RowHeight() const{
  if ( m_settings.Get( ts_FontSize ) != m_lastFontSize || m_lastFontFace != m_settings.Get( ts_FontFace ) ){
    m_lastFontSize = m_settings.Get( ts_FontSize );
    m_lastFontFace = m_settings.Get( ts_FontFace );
    m_rowHeight = ComputeRowHeight( m_settings );
  }
  return m_rowHeight;
}

faint::coord ObjText::LineSpacing() const{
  return RowHeight() * 0.5;
}

Rect ObjText::GetAutoSizedRect() const {
  faint::Bitmap bmp(faint::CairoCompatibleBitmap(10,10));
  FaintDC dc( bmp );
  text_lines_t lines = split_text_buffer( m_textBuf, 65536, m_settings ); // Fixme: Use width-ignoring split
  faint::coord width = 0;
  faint::coord height = 0;
  faint::coord rowHeight = RowHeight();
  Size extents(0,0);
  for ( size_t i = 0; i!= lines.size(); i++ ){
    extents = dc.TextExtents( std::string( lines[i].second.ToUTF8() ), m_settings );
    width = std::max( width, extents.w );
    height += rowHeight;
  }

  return Rect(GetTri().P0(), Size(width, height));
}

// Fixme: A hack which places text slightly better when anchored 'middle' in SVG
void ObjText::AnchorMiddle(){
  faint::Bitmap dummy( faint::CairoCompatibleBitmap( 10, 10 ) );
  FaintDC dc( dummy );
  Size sz( dc.TextExtents( std::string(m_textBuf.get()), m_settings ) );
  SetTri( Translated( GetTri(), -sz.w / LITCRD(2.0), -sz.h / LITCRD(2.0) ) );
}

Line ObjText::GetCaret() const{
  return m_caret;
}

TextEntryCommand::TextEntryCommand( ObjText* textObj, const wxString& newText, const wxString& oldText )
  : Command( CMD_TYPE_OBJECT ),
    m_textObj( textObj ),
    m_old( oldText ),
    m_new( newText )
{}

void TextEntryCommand::Do( faint::Image& ){
  *(m_textObj->GetTextBuffer()) = TextBuffer( m_new );
}

void TextEntryCommand::Undo( faint::Image& ){
  *(m_textObj->GetTextBuffer()) = TextBuffer( m_old );
}

Command* AutoCropText( ObjText* text ){
  const Tri oldTri = text->GetTri();
  Tri tri( Rotated( TriFromRect( text->GetAutoSizedRect() ),
      oldTri.Angle(), oldTri.P0() ));
  return new TriCommand( text, tri, oldTri );
}
