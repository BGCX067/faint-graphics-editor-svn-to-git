// -*- coding: us-ascii-unix -*-
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
#include "bitmap/bitmap.hh"
#include "commands/tri-cmd.hh"
#include "geo/geo-func.hh"
#include "geo/intrect.hh"
#include "geo/intsize.hh"
#include "geo/size.hh"
#include "objects/objline.hh"
#include "objects/objtext.hh"
#include "rendering/faint-dc.hh"
#include "text/split-string.hh"
#include "text/utf8-string.hh"
#include "util/font.hh"
#include "util/object-util.hh"
#include "util/optional.hh"
#include "util/setting-util.hh"
#include "util/settings.hh"

namespace faint{

static Tri aligned(const Tri& tri, HorizontalAlign align, coord part, coord whole){
  if (align == HorizontalAlign::CENTER){
    return offset_aligned(tri, (whole - part) / 2, 0.0);
  }
  else if (align == HorizontalAlign::RIGHT){
    return offset_aligned(tri, (whole - part), 0.0);
  }
  else{
    return tri;
  }
}

coord compute_row_height(FaintDC& dc, const Settings& settings){
  return dc.TextSize(utf8_string("M"), settings).h;
}

coord ComputeRowHeight(const Settings& settings){
  Bitmap dummy(IntSize(10, 10)); // Fixme: Bitmap-creation like this is duplicated all over this file.
  FaintDC dc(dummy);
  return compute_row_height(dc, settings);
}

void translate_pos(const text_lines_t& lines, size_t pos, size_t& rowNum, size_t& charNum){
  const size_t caret = pos;
  size_t caretRow = 0;
  size_t caretChar = 0;
  size_t chars = 0;
  for (size_t i = 0; i != lines.size(); i++){
    caretRow = i;
    const utf8_string& line = lines[i].text;
    size_t numChars = line.size();
    chars += numChars;
    if (chars > caret) {
      caretChar = numChars - (chars - caret);
      break;
    }
    caretChar = numChars - 1;
  }
  rowNum = caretRow;
  charNum = caretChar;
}

text_lines_t split_text_buffer(const TextBuffer& buffer, const Settings& settings, const max_width_t& maxWidth){
  Bitmap dummy(IntSize(10, 10));
  FaintDC dc(dummy);
  return split_str(dc, buffer.get(), settings, maxWidth);
}

static void draw_selection(FaintDC& dc, const TextBuffer& textBuf, const text_lines_t& lines, const Settings& settings, const Tri& tri, const Settings& highlightSettings, const coord ht){
  text_rng selection = textBuf.get_sel_range();
  if (selection.first == selection.second){
    return;
  }

  // Find the start and end positions
  size_t row_s0, char_s0;
  size_t row_s1, char_s1;
  translate_pos(lines, selection.first, row_s0, char_s0);
  translate_pos(lines, selection.second, row_s1, char_s1);
  Size size0 = dc.TextSize(lines[ row_s0 ].text.substr(0, char_s0), settings);
  Size size1 = dc.TextSize(lines[ row_s1 ].text.substr(0, char_s1), settings);

  for (size_t row = row_s0; row <= row_s1; row++){
    coord dx = row * ht * sin(-tri.GetAngle());
    coord dy = row * ht * cos(tri.GetAngle()) - 1;
    Size rowSize = dc.TextSize(lines[ row ].text, settings);

    // Start of the row
    Tri t2 = translated(tri, dx, dy);

    // End of the selection
    coord right_offset = row == row_s1 ? size1.w : rowSize.w;
    Tri t3 = offset_aligned(t2, right_offset + 1.0, 0.0);

    if (row == row_s0){
      // Offset the selection start
      t2 = offset_aligned(t2, size0.w, 0.0);
    }

    Tri rowTri(aligned(Tri(t2.P0(), t3.P0(), ht), settings.Get(ts_HorizontalAlign), lines[row].width, tri.Width()));
    dc.Rectangle(rowTri, highlightSettings);
  }
}

ObjText::ObjText(const Rect& r, const utf8_string& text,  const Settings& settings)
  : Object(settings),
    m_textBuf(text),
    m_caret(LineSegment(Point(0,0),Point(0,0))),
    m_tri(tri_from_rect(r))
{
  Init();
}

ObjText::ObjText(const ObjText& other)
  : Object(other.GetSettings()),
    m_textBuf(other.m_textBuf),
    m_caret(LineSegment(Point(0,0),Point(0,0))),
    m_tri(other.GetTri())
{
  Init();
}

void ObjText::Init(){
  assert(m_settings.Has(ts_FontSize));
  assert(m_settings.Has(ts_FontFace));
  m_beingEdited = false;

  m_highlightSettings = default_rectangle_settings();
  m_highlightSettings.Set(ts_FillStyle, FillStyle::FILL);
  Color highlightColor = get_highlight_color();
  m_highlightSettings.Set(ts_Fg, Paint(highlightColor));
  m_highlightSettings.Set(ts_Bg, Paint(highlightColor));

  m_rowHeight = ComputeRowHeight(m_settings);
  m_lastFontSize = m_settings.Get(ts_FontSize);
  m_lastFontFace = m_settings.Get(ts_FontFace);
}

LineSegment ObjText::ComputeCaret(const FaintDC& dc, const Tri& tri, coord ht, const text_lines_t& lines){
  size_t caretRow, caretChar;
  translate_pos(lines, m_textBuf.caret(), caretRow, caretChar);
  const line_t& line = lines[caretRow];
  Size textSize = dc.TextSize(line.text.substr(0, caretChar), m_settings);
  textSize.h = ht;
  Tri caretTri(tri.P0(), tri.P1(), RowHeight()); // Fixme
  caretTri = aligned(offset_aligned(caretTri, textSize.w, caretRow * ht),
    m_settings.Get(ts_HorizontalAlign), line.width, tri.Width());
  return LineSegment(caretTri.P0(), caretTri.P2());
}

coord ObjText::BaselineOffset() const{
  Bitmap dummy(IntSize(10, 10));
  FaintDC dc(dummy);
  TextMeasures rects(dc.TextExtents(utf8_string(1, utf8_char("M")), m_settings));
  return rects.ink.h;
}

size_t ObjText::CaretPos(const Point& posUnaligned) const{
  Bitmap bmp(IntSize(10,10));
  FaintDC dc(bmp);

  Tri tri = m_tri;
  Point pos(rotate_point(posUnaligned, -tri.GetAngle(), tri.P0()));
  tri = rotated(tri, -tri.GetAngle(), tri.P0());

  if (pos.y < tri.P0().y){
    return 0;
  }
  text_lines_t lines = split_text_buffer(m_textBuf, m_settings, max_width_t(tri.Width()));
  coord rowHeight = RowHeight();
  size_t row = static_cast<size_t>((pos.y - tri.P0().y) / (rowHeight));
  if (row >= lines.size()){
    return m_textBuf.size();
  }
  size_t charNum = 0;
  for (size_t i = 0; i!= row; i++){
    charNum += lines[i].text.size();
  }
  std::vector<int> extents(dc.CumulativeTextWidth(lines[row].text, m_settings));
  for (size_t i = 0; i != extents.size(); i++){
    if (m_tri.P0().x + extents[i] > pos.x){
      break;
    }
    charNum += 1;
  }
  return std::min(charNum, m_textBuf.size());
}

Object* ObjText::Clone() const{
  return new ObjText(*this);
}

static coord get_y_offset(const Tri& tri, coord rowHeight, int numRows, VerticalAlign align){
  switch(align){
  case VerticalAlign::TOP:
    return 0.0;

  case VerticalAlign::MIDDLE:
    return (tri.Height() - rowHeight * numRows) / 2.0;

  case VerticalAlign::BOTTOM:
    return (tri.Height() - rowHeight * numRows);
  }

  assert(false);
  return 0.0;
}

void ObjText::Draw(FaintDC& dc){
  text_lines_t lines = split_text_buffer(m_textBuf, m_settings, m_settings.Get(ts_BoundedText) ? max_width_t(m_tri.Width()) : no_option());
  m_rowHeight = compute_row_height(dc, m_settings);
  coord ht = m_rowHeight;
  if (m_textBuf.size() == 0){
    // Fixme: Tricky that this is done in Draw
    // Compute the caret position
    Tri caretTri(m_tri.P0(), m_tri.P1(), ht);
    caretTri = offset_aligned(caretTri, 1.0, 0.0);
    m_caret = LineSegment(caretTri.P0(), caretTri.P2());
    return;
  }

  if (m_beingEdited){
    draw_selection(dc, m_textBuf, lines, m_settings, m_tri, m_highlightSettings, ht);
  }

  coord yOffset = get_y_offset(m_tri, ht, resigned(lines.size()), m_settings.Get(ts_VerticalAlign));

  // Draw the text
  for (size_t row = 0; row != lines.size(); row++){
    const line_t& line = lines[row];
    // Fixme: Selected text should be drawn with wxSYS_COLOUR_HIGHLIGHTTEXT
    coord dx = (row * ht + yOffset) * sin(-m_tri.GetAngle());
    coord dy = (row * ht + yOffset) * cos(m_tri.GetAngle());
    Tri rowTri(aligned(translated(m_tri, dx, dy), m_settings.Get(ts_HorizontalAlign), line.width, m_tri.Width()));
    dc.Text(rowTri, line.text, m_settings, m_tri);
  }
  if (m_beingEdited){
    text_rng selection = m_textBuf.get_sel_range();
    if (selection.first != selection.second){
      return;
    }
    m_caret = ComputeCaret(dc, m_tri, ht, lines);
  }
}

void ObjText::DrawMask(FaintDC& dc){
  // Fixme: Sort of duplicates Draw
  // Fixme: Thought: Fill rows with inside-blank, then draw text with inside
  Settings s(default_rectangle_settings());
  s.Set(ts_FillStyle, FillStyle::BORDER_AND_FILL);
  s.Set(ts_Fg, Paint(mask_edge));
  s.Set(ts_Bg, Paint(mask_fill));

  text_lines_t lines = split_text_buffer(m_textBuf, m_settings, max_width_t(m_tri.Width()));
  m_rowHeight = compute_row_height(dc, m_settings);
  const coord ht = m_rowHeight;

  // Draw a rectangle from the left edge to the right for each line of text
  // Fixme: Only draw up to the line width
  for (size_t row = 0; row != lines.size(); row++){
    // Fixme: Selected text should be drawn with wxSYS_COLOUR_HIGHLIGHTTEXT
    coord dx = row * ht * sin(-m_tri.GetAngle());
    coord dy = row * ht * cos(m_tri.GetAngle());

    Tri topLeft(translated(m_tri, dx, dy));
    Tri tri(topLeft.P0(), topLeft.P1(), ht);
    dc.Rectangle(tri, s);
  }
}

Rect ObjText::GetAutoSizedRect() const {
  Bitmap bmp(IntSize(10,10));
  FaintDC dc(bmp);
  text_lines_t lines = split_text_buffer(m_textBuf, m_settings, no_option());
  coord width = 0;
  coord height = 0;
  coord rowHeight = RowHeight();
  for (size_t i = 0; i!= lines.size(); i++){
    const Size extents = dc.TextSize(lines[i].text, m_settings);
    width = std::max(width, extents.w);
    height += rowHeight;
  }
  return Rect(m_tri.P0(), Size(width, height));
}


LineSegment ObjText::GetCaret() const{
  return m_caret;
}

std::vector<PathPt> ObjText::GetPath() const{
  Bitmap bmp(IntSize(20,20));
  FaintDC dc(bmp);
  std::vector<PathPt> path;

  text_lines_t lines = split_text_buffer(m_textBuf, m_settings, m_settings.Get(ts_BoundedText) ? max_width_t(m_tri.Width()) : no_option());
  m_rowHeight = compute_row_height(dc, m_settings);
  coord ht = m_rowHeight;

  coord yOffset = get_y_offset(m_tri, ht, resigned(lines.size()), m_settings.Get(ts_VerticalAlign));

  // Draw the text
  for (size_t row = 0; row != lines.size(); row++){
    const line_t& line = lines[row];
    coord dx = (row * ht + yOffset) * sin(-m_tri.GetAngle());
    coord dy = (row * ht + yOffset) * cos(m_tri.GetAngle());
    Tri rowTri(aligned(translated(m_tri, dx, dy), m_settings.Get(ts_HorizontalAlign), line.width, m_tri.Width()));
    auto subPath(dc.GetTextPath(rowTri, line.text, m_settings));
    for (auto p : subPath){
      path.push_back(p);
    }
  }
  return path;
}


IntRect ObjText::GetRefreshRect() const{
  return floiled(bounding_rect(m_tri));
}

utf8_string ObjText::GetString() const{
  return m_textBuf.get();
}

TextBuffer* ObjText::GetTextBuffer(){
  return &m_textBuf;
}

Tri ObjText::GetTri() const{
  if (m_settings.Get(ts_BoundedText)){
    return m_tri;
  }

  Bitmap bmp(IntSize(10,10));
  FaintDC dc(bmp);
  text_lines_t lines = split_text_buffer(m_textBuf, m_settings, no_option());
  coord width = 0;
  coord height = 0;
  coord rowHeight = RowHeight();
  for (size_t i = 0; i!= lines.size(); i++){
    const Size extents = dc.TextSize(lines[i].text, m_settings);
    width = std::max(width, extents.w);
    height += rowHeight;
  }
  return Tri(m_tri.P0(), m_tri.GetAngle(), Size(width, height));
}

utf8_string ObjText::GetType() const{
  return "Text Region";
}

bool ObjText::HasSelectedRange() const{
  text_rng range(m_textBuf.get_sel_range());
  return range.first != range.second;
}

coord ObjText::RowHeight() const{
  if (m_settings.Get(ts_FontSize) != m_lastFontSize || m_lastFontFace != m_settings.Get(ts_FontFace)){
    m_lastFontSize = m_settings.Get(ts_FontSize);
    m_lastFontFace = m_settings.Get(ts_FontFace);
    m_rowHeight = ComputeRowHeight(m_settings);
  }
  return m_rowHeight;
}

void ObjText::SetCaretPos(size_t pos, bool select){
  m_textBuf.caret(pos, select);
}

void ObjText::SetEdited(bool edited){
  m_beingEdited = edited;
}

void ObjText::SetTri(const Tri& t){
  m_tri = t;
}

bool ObjText::ShowSizeBox() const{
  // Since the text object boundaries aren't clear (like say a
  // rectangle), show a box while resizing.
  return true;
}

Command* crop_text_region_command(ObjText* text){
  const Tri oldTri = text->GetTri();
  Tri tri(rotated(tri_from_rect(text->GetAutoSizedRect()),
      oldTri.GetAngle(), oldTri.P0()));
  return new TriCommand(text, New(tri), Old(oldTri), "Auto-Size");
}

bool is_text(const Object* obj){
  return dynamic_cast<const ObjText*>(obj) != nullptr;
}

} // namespace
