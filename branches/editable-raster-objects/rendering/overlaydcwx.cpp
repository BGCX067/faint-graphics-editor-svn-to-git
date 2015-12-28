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

#include "wx/wx.h"
#include "geo/grid.hh"
#include "geo/tri.hh"
#include "rendering/overlaydcwx.hh"
#include "util/angle.hh"
#include "util/color.hh"
#include "util/convertwx.hh"

const int g_movableRadius = 2;

static void draw_wx_line( wxDC& dc, const IntPoint& p0, const IntPoint& p1 ){
  dc.DrawLine( p0.x, p0.y, p1.x, p1.y );
}

static void draw_wx_bitmap( wxDC& dc, const faint::Bitmap& bmp, const IntPoint& topLeft ){
  dc.DrawBitmap( to_wx_bmp(bmp), topLeft.x, topLeft.y );
}

static void draw_corner( wxDC& dc, faint::radian angle, const Point& pt, bool left, bool top ){
  faint::coord len(LITCRD(10.0));
  Point delta(( left ? len : -len ) * cos( left ? angle : -angle),
    len * sin( left ? angle : -angle));

  draw_wx_line( dc, rounded(pt), rounded(pt + delta) );
  if ( (left && top) || !(left || top) ){
    delta.y *= -1;
  }
  if ( (!left && top) || (left && !top)){
    delta.x *= - 1;
  }
  draw_wx_line(dc, rounded(pt), rounded(pt + transposed(delta)));
}

static void draw_handle( wxDC& dc, const Point& p, faint::coord scale ){
  dc.DrawRectangle( truncated(p.x * scale - 8 / LITCRD(2.0) + LITCRD(0.5)), truncated(p.y * scale - 8 / LITCRD(2.0) + LITCRD(0.5) ), 8, 8 );
}

faint::Bitmap get_faint_bitmap( const IntSize& size ){
  return faint::Bitmap(size, faint::Color(0,0,0,0));
}

faint::Bitmap get_grid_bitmap( const Grid& grid, const IntPoint& imageRegionTopLeft, double scale, const IntSize& size ){
  faint::Bitmap bmp = get_faint_bitmap(IntSize(size + IntSize(5,5)));
  int spacing = rounded(grid.Spacing() * scale);
  const faint::Color color(grid.Color());
  bool dashed = grid.Dashed() && spacing >= 8;
    int xOff = truncated((imageRegionTopLeft.x ) * scale ) % (spacing);
  int yOff = truncated((imageRegionTopLeft.y ) * scale ) % (spacing);
  int maxPos = std::max( ( size.w + 10 ) / grid.Spacing(), ( size.h + 10 ) / grid.Spacing()) + 2;
  const int width = 1;
  for ( int i = 0; i != maxPos; i++ ){
    draw_line_color(bmp, floored(Point(i * spacing - xOff, 0)), floored(Point(i * spacing - xOff, bmp.m_h)), color, width, dashed, LineCap::BUTT );
    draw_line_color(bmp, floored(Point(0, i * spacing - yOff)), floored(Point(bmp.m_w, i * spacing - yOff)), color, width, dashed, LineCap::BUTT );
  }
  return bmp;
}

void overlay_pen_and_brush( wxDC& dc ){
  static wxPen pen(wxColour(0,0,0),1);
  static wxBrush brush( wxColour(128,128,200) );
  dc.SetPen( pen );
  dc.SetBrush( brush );
}

void overlay_transparent_brush( wxDC& dc ){
  wxBrush brush( wxColour(0,0,0));
  brush.SetStyle( wxTRANSPARENT );
  static wxPen pen(wxColour(0,0,0),1);
  dc.SetPen( pen );
  dc.SetBrush( brush );
}

OverlayDC_WX::OverlayDC_WX( wxDC& dc, faint::coord scale, const Rect& imageRect, const IntSize& imageSize )
  : m_dc(dc),
    m_imageRect(imageRect),
    m_imageSize(imageSize),
    m_scale(scale)
{}

void OverlayDC_WX::Caret( const LineSegment& line ){
  wxRasterOperationMode oldMode( m_dc.GetLogicalFunction() );
  m_dc.SetLogicalFunction( wxINVERT );
  m_dc.SetPen( wxPen(wxColour(0, 0, 0), 1 ) );
  Point p0(line.P0());
  Point p1(line.P1());

  draw_wx_line(m_dc, floored(line.P0() * m_scale), floored(line.P1() * m_scale));
  m_dc.SetLogicalFunction(oldMode);
}

void OverlayDC_WX::ConstrainPos( const Point& pos ){
  overlay_transparent_brush( m_dc );
  const int constrainRadius = 2;
  m_dc.DrawCircle( rounded(pos.x * m_scale), rounded(pos.y * m_scale), constrainRadius );
}

void OverlayDC_WX::Corners( const Tri& tri ){
  m_dc.SetPen( wxPen(wxColour(255, 0, 255), 1, wxPENSTYLE_DOT ) );
  wxBrush brush( wxColour(0,0,0));
  brush.SetStyle( wxTRANSPARENT );
  m_dc.SetBrush( brush );
  faint::radian angle(tri.Angle());
  Point topLeft(tri.P0() * m_scale );
  Point topRight(tri.P1() * m_scale );
  Point bottomLeft(tri.P2() * m_scale);
  Point bottomRight(tri.P3() * m_scale);
  draw_corner(m_dc, angle, topLeft, true, true);
  draw_corner(m_dc, angle, topRight, false, true);
  draw_corner(m_dc, angle, bottomRight, false, false);
  draw_corner(m_dc, angle, bottomLeft, true, false );
  m_dc.SetPen( wxPen(wxColour(0, 0, 0), 1, wxPENSTYLE_SOLID ) );
}

void OverlayDC_WX::ExtensionPoint( const Point& center ){
  overlay_pen_and_brush( m_dc );
  m_dc.SetBrush(wxBrush(wxColour(255,255,255)));
  m_dc.DrawCircle( rounded(center.x * m_scale), rounded(center.y * m_scale), g_movableRadius );
}

static bool grid_useless_at_scale( const Grid& g, faint::coord scale ){
  return faint::coord(g.Spacing()) * scale < 2.0;
}

void OverlayDC_WX::GridLines( const Grid& grid, const IntPoint& imageRegionTopLeft, const IntSize& size ){

  if ( grid_useless_at_scale(grid, m_scale) ){
    return;
  }

  faint::Bitmap bmp(get_grid_bitmap(grid, imageRegionTopLeft, m_scale, size));

  // + 1 so that a grid-line on the far edge is visible (wxRect does not include far edge)
  m_dc.SetClippingRegion( wxRect(0, 0,
      truncated( m_imageSize.w * m_scale + LITCRD(1.0) ),
      truncated( m_imageSize.h * m_scale + LITCRD(1.0) ) ) );
  draw_wx_bitmap( m_dc, bmp, floored(imageRegionTopLeft * m_scale) );
  m_dc.DestroyClippingRegion();
}

void OverlayDC_WX::Handles( const Rect& rect ){
  overlay_pen_and_brush(m_dc);
  draw_handle(m_dc, rect.TopLeft(), m_scale);
  draw_handle(m_dc, rect.TopRight(), m_scale);
  draw_handle(m_dc, rect.BottomLeft(), m_scale);
  draw_handle(m_dc, rect.BottomRight(), m_scale);
}

void OverlayDC_WX::Handles( const Tri& tri ){
  overlay_pen_and_brush(m_dc);
  draw_handle(m_dc, tri.P0(), m_scale);
  draw_handle(m_dc, tri.P1(), m_scale);
  draw_handle(m_dc, tri.P2(), m_scale);
  draw_handle(m_dc, tri.P3(), m_scale);
}

void OverlayDC_WX::HorizontalLine( faint::coord in_y ){
  m_dc.SetPen( wxPen(wxColour(255, 0, 255), 1, wxPENSTYLE_DOT ) );
  int y = truncated(in_y * m_scale);
  int x0 = floored(m_imageRect.x * m_scale);
  int x1 = floored( (m_imageRect.x + m_imageRect.w) * m_scale);
  m_dc.DrawLine( x0, y, x1, y );
}

void OverlayDC_WX::MovablePoint( const Point& center ){
  overlay_pen_and_brush( m_dc );
  m_dc.DrawCircle( rounded(center.x * m_scale), rounded(center.y * m_scale), g_movableRadius );
}

void OverlayDC_WX::Parallelogram( const Tri& tri ){
  m_dc.SetPen( wxPen(wxColour(255, 0, 255), 1, wxPENSTYLE_DOT ) );
  wxBrush brush( wxColour(0,0,0));
  brush.SetStyle( wxTRANSPARENT );
  m_dc.SetBrush( brush );
  wxPoint points[] = {to_wx(rounded(tri.P0())), to_wx(rounded(tri.P1())), to_wx(rounded(tri.P3())), to_wx(rounded(tri.P2()))};
  m_dc.DrawPolygon( 4, points );
  m_dc.SetPen( wxPen(wxColour(0, 0, 0), 1, wxPENSTYLE_SOLID ) );
}

void OverlayDC_WX::Pivot( const Point& center ){
  overlay_pen_and_brush( m_dc );
  const int pivotRadius = 5;
  m_dc.DrawCircle( rounded(center.x * m_scale), rounded(center.y * m_scale), pivotRadius );
}

void OverlayDC_WX::Pixel( const IntPoint& imagePoint ){
  m_dc.SetPen( wxPen(wxColour(255, 0, 255), 1, wxPENSTYLE_SOLID ) );
  Point p = imagePoint * m_scale;

  // Negative and positive offsets, wxWidgets excludes the second
  // pixel of lines.
  faint::coord dn = 5.0;
  faint::coord dp = 6.0;

  if ( m_scale <= 1 ){
    // Draw a simple crosshair for 1:1 and less
    draw_wx_line(m_dc, floored(p - delta_y(dn)), floored(p + delta_y(dp)));
    draw_wx_line(m_dc, floored(p - delta_x(dn)), floored(p + delta_x(dp)));
  }
  else {
    // Draw a "hash"-box around the pixel
    draw_wx_line(m_dc, floored(p - delta_y(dn)), floored(p + delta_y(m_scale + dp))); // Left
    draw_wx_line(m_dc, floored(p - delta_x(dn)), floored(p + delta_x(m_scale + dp))); // Top
    draw_wx_line(m_dc, floored(p + delta_xy(-dn, m_scale)), floored(p + delta_xy(m_scale + dp, m_scale))); // Bottom
    draw_wx_line(m_dc, floored(p + delta_xy(m_scale, -dn)), floored(p + delta_xy(m_scale, m_scale + dp)));
  }
}

void OverlayDC_WX::Rectangle( const Rect& r ){
  m_dc.SetPen( wxPen(wxColour(255, 0, 255), 1, wxPENSTYLE_DOT ) );
  wxBrush brush( wxColour(0,0,0));
  brush.SetStyle( wxTRANSPARENT );
  m_dc.SetBrush( brush );
  m_dc.DrawRectangle( rounded(r.x * m_scale), rounded(r.y * m_scale), rounded(r.w * m_scale), rounded(r.h * m_scale ));
  m_dc.SetPen( wxPen(wxColour(0, 0, 0), 1, wxPENSTYLE_SOLID ) );
}

void OverlayDC_WX::VerticalLine( faint::coord in_x ){
  m_dc.SetPen( wxPen(wxColour(255, 0, 255), 1, wxPENSTYLE_DOT ) );
  int x = truncated(in_x * m_scale);
  int y0 = truncated(m_imageRect.y * m_scale);
  int y1 = truncated(( m_imageRect.y + m_imageRect.h ) * m_scale);
  m_dc.DrawLine( x, y0, x, y1  );
}
