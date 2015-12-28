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

#include <cassert>
#include "objtext.hh"
#include "commands/tri-cmd.hh"
#include "rendering/cairocontext.hh"
#include "rendering/faintdc.hh"
#include "objects/objline.hh"
#include "util/char.hh"
#include "util/objutil.hh"
#include "util/settings.hh"
#include "util/settingutil.hh"
#include "util/split_string.hh"
#include "util/util.hh"

const std::string s_TypeText = "Text Region";

faint::coord compute_row_height( FaintDC& dc, const Settings& settings ){
  return dc.TextExtents(faint::utf8_string("M"), settings).h;
}

faint::coord ComputeRowHeight( const Settings& settings ){
  faint::Bitmap dummy( faint::cairo_compatible_bitmap(IntSize(10, 10))); // Fixme: Bitmap-creation like this is duplicated all over this file.
  FaintDC dc( dummy );
  return compute_row_height( dc, settings );
}

void translate_pos( const text_lines_t& lines, size_t pos, size_t& rowNum, size_t& charNum ){
  const size_t caret = pos;
  size_t caretRow = 0;
  size_t caretChar = 0;
  size_t chars = 0;
  for ( size_t i = 0; i != lines.size(); i++ ){
    caretRow = i;
    const faint::utf8_string& line = lines[i].second;
    size_t numChars = line.size();
    chars += numChars;
    if ( chars > caret ) {
      caretChar = numChars - ( chars - caret );
      break;
    }
    caretChar = numChars - 1;
  }
  rowNum = caretRow;
  charNum = caretChar;
}

text_lines_t split_text_buffer( const TextBuffer& buffer, faint::coord width, const Settings& settings ){
  faint::Bitmap dummy( faint::cairo_compatible_bitmap(IntSize(10, 10)) );
  FaintDC dc( dummy );
  return split_str( dc, buffer.get(), width, settings );
}

void DrawSelection( FaintDC& dc, const TextBuffer& textBuf, const text_lines_t& lines, const Settings& settings, const Tri& tri, const Settings& highlightSettings, const faint::coord ht ){
  text_rng selection = textBuf.get_sel_range();
  if ( selection.first == selection.second ){
    return;
  }

  // Find the start and end positions
  size_t row_s0, char_s0;
  size_t row_s1, char_s1;
  translate_pos( lines, selection.first, row_s0, char_s0 );
  translate_pos( lines, selection.second, row_s1, char_s1 );
  Size size0 = dc.TextExtents( lines[ row_s0 ].second.substr(0, char_s0), settings );
  Size size1 = dc.TextExtents( lines[ row_s1 ].second.substr(0, char_s1), settings );

  for ( size_t row = row_s0; row <= row_s1; row++ ){
    faint::coord dx = row * ht * sin( -tri.Angle() );
    faint::coord dy = row * ht * cos( tri.Angle() ) - 1;
    Size rowSize = dc.TextExtents( lines[ row ].second, settings );

    // Start of the row
    Tri t2 = translated( tri, dx, dy );

    // End of the selection
    faint::coord right_offset = row == row_s1 ? size1.w : rowSize.w;
    Tri t3 = offset_aligned( t2, right_offset + LITCRD(1.0), LITCRD(0.0) );

    if ( row == row_s0 ){
      // Offset the selection start
      t2 = offset_aligned( t2, size0.w, LITCRD(0.0) );
    }
    dc.Rectangle( Tri( t2.P0(), t3.P0(), ht ), highlightSettings );
  }
}

ObjText::ObjText( const Rect& r, const faint::utf8_string& text,  const Settings& settings )
  : Object( &s_TypeText, tri_from_rect(r), settings ),
    m_textBuf( text ),
    m_caret(Line(Point(0,0),Point(0,0)))
{
  Init();
}

ObjText::ObjText( const ObjText& other )
  : Object( &s_TypeText, other.GetTri(), other.GetSettings() ),
    m_textBuf(other.m_textBuf),
    m_caret(Line(Point(0,0),Point(0,0)))
{
  Init();
}

void ObjText::Init(){
  assert( m_settings.Has( ts_FontSize ) );
  assert( m_settings.Has( ts_FontFace ) );
  m_beingEdited = false;

  m_highlightSettings = default_rectangle_settings();
  m_highlightSettings.Set( ts_FillStyle, FillStyle::FILL );
  faint::Color highlightColor = get_highlight_color();
  m_highlightSettings.Set( ts_FgCol, highlightColor );
  m_highlightSettings.Set( ts_BgCol, highlightColor );

  m_rowHeight = ComputeRowHeight( m_settings );
  m_lastFontSize = m_settings.Get( ts_FontSize );
  m_lastFontFace = m_settings.Get( ts_FontFace );
}

Line ObjText::ComputeCaret( const FaintDC& dc, const Tri& tri, faint::coord ht, const text_lines_t& lines ){
  size_t caretRow, caretChar;
  translate_pos( lines, m_textBuf.caret(), caretRow, caretChar );
  const faint::utf8_string& line = lines[caretRow].second;
  Size textSize = dc.TextExtents( line.substr(0, caretChar), m_settings );
  textSize.h = ht;
  Tri caretTri( tri.P0(), tri.P1(), RowHeight() ); // Fixme
  caretTri = offset_aligned( caretTri, textSize.w, caretRow * ht );
  return Line( caretTri.P0(), caretTri.P2() );
}

// Fixme: A hack which places text slightly better when anchored 'middle' in SVG
void ObjText::AnchorMiddle(){
  faint::Bitmap dummy( faint::cairo_compatible_bitmap( IntSize(10, 10) ) );
  FaintDC dc( dummy );
  Size sz( dc.TextExtents( m_textBuf.get(), m_settings ) );
  SetTri( translated( GetTri(), -sz.w / LITCRD(2.0), -sz.h / LITCRD(2.0) ) );
}

size_t ObjText::CaretPos( const Point& posUnaligned ) const{
  faint::Bitmap bmp(faint::cairo_compatible_bitmap(IntSize(10,10)));
  FaintDC dc( bmp );

  Tri tri(GetTri());
  Point pos(rotate_point(posUnaligned, -tri.Angle(), tri.P0()));
  tri = rotated(tri, -tri.Angle(), tri.P0());

  if ( pos.y < tri.P0().y ){
    return 0;
  }
  text_lines_t lines = split_text_buffer( m_textBuf, tri.Width(), m_settings );
  faint::coord rowHeight = RowHeight();
  size_t row = static_cast<size_t>((pos.y - tri.P0().y) / (rowHeight + LineSpacing()));
  if ( row >= lines.size() ){
    return m_textBuf.size();
  }
  size_t charNum = 0;
  for ( size_t i = 0; i!= row; i++ ){
    charNum += lines[i].second.size();
  }
  std::vector<int> extents(dc.PartialExtents(lines[row].second, m_settings));
  for ( size_t i = 0; i != extents.size(); i++ ){
    if ( GetTri().P0().x + extents[i] > pos.x ){
      break;
    }
    charNum += 1;
  }
  return std::min(charNum, m_textBuf.size());
}

Object* ObjText::Clone() const{
  return new ObjText( *this );
}

void ObjText::Draw( FaintDC& dc ){
  Tri tri(GetTri());
  text_lines_t lines = split_text_buffer( m_textBuf, tri.Width(), m_settings );
  m_rowHeight = compute_row_height( dc, m_settings );
  faint::coord ht = m_rowHeight + LineSpacing();

  if ( m_textBuf.size() == 0 ){
    // Fixme: Tricky that this is done in Draw
    // Compute the caret position
    Tri caretTri( tri.P0(), tri.P1(), ht );
    caretTri = offset_aligned( caretTri, LITCRD(1.0), LITCRD(0.0) );
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
    dc.Text( translated(tri, dx, dy), lines[row].second, m_settings );
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
  Settings s(default_rectangle_settings());
  s.Set( ts_FillStyle, FillStyle::BORDER_AND_FILL );
  s.Set( ts_FgCol, mask_edge );
  s.Set( ts_BgCol, mask_fill );

  text_lines_t lines = split_text_buffer( m_textBuf, GetTri().Width(), m_settings );
  m_rowHeight = compute_row_height( dc, m_settings );
  const faint::coord ht = m_rowHeight + LineSpacing();

  // Draw a rectangle from the left edge to the right for each line of text
  // Fixme: Only draw up to the line width
  for ( size_t row = 0; row != lines.size(); row++ ){
    // Fixme: Selected text should be drawn with wxSYS_COLOUR_HIGHLIGHTTEXT
    faint::coord dx = row * ht * sin( -GetTri().Angle() );
    faint::coord dy = row * ht * cos( GetTri().Angle() );

    Tri topLeft( translated( GetTri(), dx, dy ) );
    Tri tri( topLeft.P0(), topLeft.P1(), ht );
    dc.Rectangle( tri, s );
  }
}

Rect ObjText::GetAutoSizedRect() const {
  faint::Bitmap bmp(faint::cairo_compatible_bitmap(IntSize(10,10)));
  FaintDC dc( bmp );
  text_lines_t lines = split_text_buffer( m_textBuf, 65536, m_settings ); // Fixme: Use width-ignoring split
  faint::coord width = 0;
  faint::coord height = 0;
  faint::coord rowHeight = RowHeight();
  Size extents(0,0);
  for ( size_t i = 0; i!= lines.size(); i++ ){
    extents = dc.TextExtents( lines[i].second, m_settings );
    width = std::max( width, extents.w );
    height += rowHeight;
  }
  return Rect(GetTri().P0(), Size(width, height));
}


Line ObjText::GetCaret() const{
  return m_caret;
}

IntRect ObjText::GetRefreshRect(){
  return truncated(GetRect());
}

faint::utf8_string ObjText::GetString() const{
  return m_textBuf.get();
}

TextBuffer* ObjText::GetTextBuffer(){
  return &m_textBuf;
}

bool ObjText::HasSelectedRange() const{
  text_rng range( m_textBuf.get_sel_range() );
  return range.first != range.second;
}

faint::coord ObjText::LineSpacing() const{
  return RowHeight() * 0.2;
}

faint::coord ObjText::RowHeight() const{
  if ( m_settings.Get( ts_FontSize ) != m_lastFontSize || m_lastFontFace != m_settings.Get( ts_FontFace ) ){
    m_lastFontSize = m_settings.Get( ts_FontSize );
    m_lastFontFace = m_settings.Get( ts_FontFace );
    m_rowHeight = ComputeRowHeight( m_settings );
  }
  return m_rowHeight;
}

void ObjText::SetCaretPos( size_t pos, bool select ){
  m_textBuf.caret( pos, select );
}

void ObjText::SetEdited( bool edited ){
  m_beingEdited = edited;
}

bool ObjText::ShowSizeBox() const{
  // Since the text object boundaries aren't clear (like say a
  // rectangle), show a box while resizing.
  return true;
}

Command* crop_text_region_command( ObjText* text ){
  const Tri oldTri = text->GetTri();
  Tri tri( rotated( tri_from_rect( text->GetAutoSizedRect() ),
      oldTri.Angle(), oldTri.P0() ));
  return new TriCommand( text, New(tri), Old(oldTri), "Auto-Size" );
}
