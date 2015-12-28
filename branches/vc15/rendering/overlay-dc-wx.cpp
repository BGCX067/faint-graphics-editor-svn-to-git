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

#include <algorithm>
#include "wx/dc.h"
#include "wx/settings.h"
#include "bitmap/bitmap.hh"
#include "bitmap/color.hh"
#include "bitmap/draw.hh"
#include "geo/geo-func.hh"
#include "geo/line.hh"
#include "geo/tri.hh"
#include "rendering/overlay-dc-wx.hh"
#include "text/text-buffer.hh"
#include "util/grid.hh"
#include "util-wx/convert-wx.hh"

namespace faint{

const int g_movableRadius = 2;

static void draw_wx_line(wxDC& dc, const IntPoint& p0, const IntPoint& p1){
  dc.DrawLine(p0.x, p0.y, p1.x, p1.y);
}

static void draw_wx_bitmap(wxDC& dc, const Bitmap& bmp, const IntPoint& topLeft){
  dc.DrawBitmap(to_wx_bmp(bmp), topLeft.x, topLeft.y);
}

static void draw_corner(wxDC& dc,
  const Angle& angle,
  const Point& pt,
  bool left,
  bool top)
{
  coord len(10.0);
  Point delta((left ? len : -len) * cos(left ? angle : -angle),
    len * sin(left ? angle : -angle));

  draw_wx_line(dc, rounded(pt), rounded(pt + delta));
  if ((left && top) || !(left || top)){
    delta.y *= -1;
  }
  if ((!left && top) || (left && !top)){
    delta.x *= - 1;
  }
  draw_wx_line(dc, rounded(pt), rounded(pt + transposed(delta)));
}

static void draw_handle(wxDC& dc, const Point& p, coord scale, int width){
  dc.DrawRectangle(truncated(p.x * scale - width / 2.0 + 0.5),
    truncated(p.y * scale - width / 2.0 + 0.5), width, width);
}

static Bitmap get_grid_bitmap(const Grid& grid,
  const IntPoint& imageRegionTopLeft,
  double scale,
  const IntSize& size)
{
  Bitmap bmp(size + IntSize(5,5), Color(0,0,0,0));
  int spacing = rounded(grid.Spacing() * scale);
  const Color color(grid.GetColor());
  LineStyle style = grid.Dashed() && spacing >= 8 ? LineStyle::LONG_DASH :
    LineStyle::SOLID;
  Point anchor = grid.Anchor();
  int xOff = truncated(imageRegionTopLeft.x * scale - anchor.x * scale) % spacing;
  int yOff = truncated(imageRegionTopLeft.y * scale - anchor.y * scale) % spacing;
  int maxPos = std::max((size.w + 10) / grid.Spacing(),
    (size.h + 10) / grid.Spacing()) + 2;
  const int width = 1;

  const LineSettings s(color, width, style, LineCap::BUTT);
  for (int i = 0; i != maxPos; i++){
    coord x = i * spacing - xOff;
    draw_line(bmp, {floored(Point(x, 0)), floored(Point(x, bmp.m_h))}, s);

    coord y = i * spacing - yOff;
    draw_line(bmp, {floored(Point(0, y)), floored(Point(bmp.m_w, y))}, s);
  }

  return bmp;
}

static void overlay_pen_and_brush(wxDC& dc){
  static wxPen pen(wxColour(0,0,0),1);
  static wxBrush brush(wxColour(128,128,200));
  dc.SetPen(pen);
  dc.SetBrush(brush);
}

static void overlay_transparent_brush(wxDC& dc){
  wxBrush brush(wxColour(0,0,0));
  brush.SetStyle(wxBRUSHSTYLE_TRANSPARENT);
  static wxPen pen(wxColour(0,0,0),1);
  dc.SetPen(pen);
  dc.SetBrush(brush);
}

OverlayDC_WX::OverlayDC_WX(wxDC& dc,
  coord scale,
  const Rect& imageRect,
  const IntSize& imageSize,
  int objectHandleWidth)
  : m_dc(dc),
    m_imageRect(imageRect),
    m_imageSize(imageSize),
    m_objectHandleWidth(objectHandleWidth),
    m_scale(scale)
{}

void OverlayDC_WX::Caret(const LineSegment& line){
  wxRasterOperationMode oldMode(m_dc.GetLogicalFunction());
  m_dc.SetLogicalFunction(wxINVERT);
  m_dc.SetPen(wxPen(wxColour(0, 0, 0), 1));
  draw_wx_line(m_dc, floored(line.p0 * m_scale), floored(line.p1 * m_scale));
  m_dc.SetLogicalFunction(oldMode);
}

void OverlayDC_WX::ConstrainPos(const Point& pos){
  overlay_transparent_brush(m_dc);
  const int constrainRadius = 2;
  m_dc.DrawCircle(rounded(pos.x * m_scale), rounded(pos.y * m_scale),
    constrainRadius);
}

void OverlayDC_WX::Corners(const Tri& tri){
  m_dc.SetPen(wxPen(wxColour(255, 0, 255), 1, wxPENSTYLE_DOT));
  wxBrush brush(wxColour(0,0,0));
  brush.SetStyle(wxBRUSHSTYLE_TRANSPARENT);
  m_dc.SetBrush(brush);
  Angle angle(tri.GetAngle());
  Point topLeft(tri.P0() * m_scale);
  Point topRight(tri.P1() * m_scale);
  Point bottomLeft(tri.P2() * m_scale);
  Point bottomRight(tri.P3() * m_scale);
  draw_corner(m_dc, angle, topLeft, true, true);
  draw_corner(m_dc, angle, topRight, false, true);
  draw_corner(m_dc, angle, bottomRight, false, false);
  draw_corner(m_dc, angle, bottomLeft, true, false);
  m_dc.SetPen(wxPen(wxColour(0, 0, 0), 1, wxPENSTYLE_SOLID));
}

void OverlayDC_WX::ExtensionPoint(const Point& center){
  overlay_pen_and_brush(m_dc);
  m_dc.SetBrush(wxBrush(wxColour(255,255,255)));
  m_dc.DrawCircle(rounded(center.x * m_scale), rounded(center.y * m_scale),
    g_movableRadius);
}

static bool grid_useless_at_scale(const Grid& g, coord scale){
  return coord(g.Spacing()) * scale < 2.0;
}

void OverlayDC_WX::GridLines(const Grid& grid,
  const IntPoint& imageRegionTopLeft,
  const IntSize& size)
{
  if (grid_useless_at_scale(grid, m_scale)){
    return;
  }

  Bitmap bmp(get_grid_bitmap(grid, imageRegionTopLeft, m_scale, size));

  // + 1 so that a grid-line on the far edge is visible (wxRect does
  // not include far edge)
  m_dc.SetClippingRegion(wxRect(0, 0,
      truncated(m_imageSize.w * m_scale + 1.0),
      truncated(m_imageSize.h * m_scale + 1.0)));
  draw_wx_bitmap(m_dc, bmp, floored(imageRegionTopLeft * m_scale));
  m_dc.DestroyClippingRegion();
}

void OverlayDC_WX::Handles(const Rect& rect){
  overlay_pen_and_brush(m_dc);
  draw_handle(m_dc, rect.TopLeft(), m_scale, m_objectHandleWidth);
  draw_handle(m_dc, rect.TopRight(), m_scale, m_objectHandleWidth);
  draw_handle(m_dc, rect.BottomLeft(), m_scale, m_objectHandleWidth);
  draw_handle(m_dc, rect.BottomRight(), m_scale, m_objectHandleWidth);
}

void OverlayDC_WX::Handles(const Tri& tri){
  overlay_pen_and_brush(m_dc);
  draw_handle(m_dc, tri.P0(), m_scale, m_objectHandleWidth);
  draw_handle(m_dc, tri.P1(), m_scale, m_objectHandleWidth);
  draw_handle(m_dc, tri.P2(), m_scale, m_objectHandleWidth);
  draw_handle(m_dc, tri.P3(), m_scale, m_objectHandleWidth);
}

void OverlayDC_WX::HorizontalLine(coord in_y){
  m_dc.SetPen(wxPen(wxColour(255, 0, 255), 1, wxPENSTYLE_DOT));
  int y = truncated(in_y * m_scale);
  int x0 = floored(m_imageRect.x * m_scale);
  int x1 = floored((m_imageRect.x + m_imageRect.w) * m_scale);
  m_dc.DrawLine(x0, y, x1, y);
}

void OverlayDC_WX::Line(const LineSegment& line){
  m_dc.SetPen(wxPen(wxColour(255, 0, 255), 1, wxPENSTYLE_SOLID));
  IntPoint p0 = floored(line.p0 * m_scale);
  IntPoint p1 = floored(line.p1 * m_scale);
  m_dc.DrawLine(p0.x, p0.y, p1.x, p1.y);
}

void OverlayDC_WX::MovablePoint(const Point& center){
  overlay_pen_and_brush(m_dc);
  m_dc.DrawCircle(rounded(center.x * m_scale), rounded(center.y * m_scale),
    g_movableRadius);
}

void OverlayDC_WX::Parallelogram(const Tri& tri){
  m_dc.SetPen(wxPen(wxColour(255, 0, 255), 1, wxPENSTYLE_DOT));
  wxBrush brush(wxColour(0,0,0));
  brush.SetStyle(wxBRUSHSTYLE_TRANSPARENT);
  m_dc.SetBrush(brush);
  wxPoint points[] = {to_wx(rounded(tri.P0())), to_wx(rounded(tri.P1())),
                      to_wx(rounded(tri.P3())), to_wx(rounded(tri.P2()))};
  m_dc.DrawPolygon(4, points);
  m_dc.SetPen(wxPen(wxColour(0, 0, 0), 1, wxPENSTYLE_SOLID));
}

void OverlayDC_WX::Pivot(const Point& center){
  overlay_pen_and_brush(m_dc);
  const int pivotRadius = 5;
  m_dc.DrawCircle(rounded(center.x * m_scale), rounded(center.y * m_scale),
    pivotRadius);
}

void OverlayDC_WX::Pixel(const IntPoint& imagePoint){
  m_dc.SetPen(wxPen(wxColour(255, 0, 255), 1, wxPENSTYLE_SOLID));
  Point p = imagePoint * m_scale;

  // Negative and positive offsets, wxWidgets excludes the
  // end-coordinate of lines.
  coord dn = 5.0;
  coord dp = 6.0;

  if (m_scale <= 1){
    // Draw a simple crosshair for 1:1 and less
    draw_wx_line(m_dc, floored(p - delta_y(dn)), floored(p + delta_y(dp)));
    draw_wx_line(m_dc, floored(p - delta_x(dn)), floored(p + delta_x(dp)));
  }
  else {
    // Draw a "hash"-box around the pixel
    draw_wx_line(m_dc, floored(p - delta_y(dn)),
      floored(p + delta_y(m_scale + dp))); // Left

    draw_wx_line(m_dc, floored(p - delta_x(dn)), floored(p +
        delta_x(m_scale + dp))); // Top

    draw_wx_line(m_dc, floored(p + delta_xy(-dn, m_scale)),
      floored(p + delta_xy(m_scale + dp, m_scale))); // Bottom

    draw_wx_line(m_dc, floored(p + delta_xy(m_scale, -dn)),
      floored(p + delta_xy(m_scale, m_scale + dp))); // Right
  }
}

void OverlayDC_WX::Rectangle(const Rect& r){
  m_dc.SetPen(wxPen(wxColour(255, 0, 255), 1, wxPENSTYLE_DOT));
  wxBrush brush(wxColour(0,0,0));
  brush.SetStyle(wxBRUSHSTYLE_TRANSPARENT);
  m_dc.SetBrush(brush);
  m_dc.DrawRectangle(rounded(r.x * m_scale), rounded(r.y * m_scale),
    rounded(r.w * m_scale), rounded(r.h * m_scale));
  m_dc.SetPen(wxPen(wxColour(0, 0, 0), 1, wxPENSTYLE_SOLID));
}

void OverlayDC_WX::Textbox(const Point& topLeft, const TextBuffer& text,
  const utf8_string& sampleText)
{
  m_dc.SetPen(wxPen(wxColour(0,0,255), 1, wxPENSTYLE_SOLID));
  m_dc.SetBrush(wxBrush(wxColour(245,228,156)));
  wxFont font(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
  if (text.empty()){
    font = font.Italic();
  }
  m_dc.SetFont(font);
  IntPoint p(rounded(topLeft * m_scale));

  auto textWx(to_wx(text.get()));
  auto sampleWx(to_wx(sampleText));
  auto textSize(m_dc.GetTextExtent(textWx));
  auto sampleSize(m_dc.GetTextExtent(sampleWx));
  auto fontSize(font.GetPixelSize());

  wxArrayInt widths;
  bool ok = m_dc.GetPartialTextExtents(textWx, widths);
  assert(ok);

  int caretPos = text.caret() == 0 ?
    0 :
    widths[text.caret() - 1];

  const int r = 5;
  const int minWidth = std::max(sampleSize.GetWidth() + r * 2, 50);
  const int boxWidth = std::max(textSize.GetWidth() + r * 2, minWidth);
  const int boxHeight = fontSize.GetHeight() + r * 2; // Fixme

  m_dc.DrawRoundedRectangle(p.x, p.y, boxWidth, boxHeight, r);

  CaretRange sel = text.get_sel_range();
  if (sel.to != 0){
    m_dc.SetBrush(wxBrush(wxColour(100,100,255))); // Fixme: get highlight color
    int s0x = sel.from == 0 ?
      0 : widths[sel.from - 1];

    int s1x = sel.to == 0 ?
      0 : widths[sel.to - 1];

    m_dc.DrawRectangle(p.x + r + s0x, p.y + r, s1x - s0x,
      fontSize.GetHeight());
  }

  // Fixme: DrawText position refers to upper left and depends on
  // string content. Adjust with GetTextExtents and font-size
  m_dc.DrawText(textWx, p.x + r, p.y + r);

  m_dc.DrawLine(p.x + r + caretPos,
    p.y + r,
    p.x + r + caretPos,
    p.y + fontSize.GetHeight() + r);

  if (text.empty()){
    m_dc.SetTextForeground(wxColour(160,160,160));
    m_dc.DrawText(sampleWx, p.x + r, p.y + r);
  }
}

void OverlayDC_WX::VerticalLine(coord in_x){
  m_dc.SetPen(wxPen(wxColour(255, 0, 255), 1, wxPENSTYLE_DOT));
  int x = truncated(in_x * m_scale);
  int y0 = truncated(m_imageRect.y * m_scale);
  int y1 = truncated((m_imageRect.y + m_imageRect.h) * m_scale);
  m_dc.DrawLine(x, y0, x, y1);
}

} // namespace
