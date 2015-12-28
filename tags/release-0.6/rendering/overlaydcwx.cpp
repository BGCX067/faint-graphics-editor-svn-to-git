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
#include "geo/color.hh"
#include "geo/grid.hh"
#include "geo/tri.hh"
#include "overlaydcwx.hh"
#include "rendering/cairocontext.hh"
#include "util/angle.hh"
#include "util/convertwx.hh"

const int g_movableRadius = 2;

void draw_corner( wxDC& dc, faint::radian angle, const Point& pt, bool left, bool top ){
  faint::coord len(LITCRD(10.0));
  // Fixme: clean this up. :)
  faint::coord dx = ( left ? len : -len ) * cos( left ? angle : -angle);
  faint::coord dy = len * sin( left ? angle : -angle);
  dc.DrawLine( pt.x, pt.y, pt.x + dx, pt.y + dy );
  if ( (left && top) || !(left || top) ){
    dy *= -1;
  }
  if ( (!left && top) || (left && !top)){
    dx *= -1;
  }
  dc.DrawLine( rounded(pt.x), rounded(pt.y), rounded(pt.x + dy), rounded(pt.y + dx) );
}

void draw_handle( wxDC& dc, const Point& p, faint::coord scale ){
  dc.DrawRectangle( truncated(p.x * scale - 8 / LITCRD(2.0) + LITCRD(0.5)), truncated(p.y * scale - 8 / LITCRD(2.0) + LITCRD(0.5) ), 8, 8 );
}

BITMAPRETURN get_faint_bitmap( const IntSize& size ){
  faint::Bitmap bmp = faint::cairo_compatible_bitmap(size);
  clear(bmp, faint::Color(0,0,0,0));
  return bmp;
}

wxBitmap get_grid_bitmap( const Grid& grid, const IntPoint& imageRegionTopLeft, double scale, const IntSize& size ){
  faint::Bitmap bmp = get_faint_bitmap(IntSize(size + IntSize(5,5)));
  int spacing = grid.Spacing() * scale + 0.5;
  const faint::Color color(grid.Color());
  bool dashed = grid.Dashed() && spacing >= 8;
    int xOff = truncated((imageRegionTopLeft.x ) * scale ) % (spacing);
  int yOff = truncated((imageRegionTopLeft.y ) * scale ) % (spacing);
  int maxPos = std::max( ( size.w + 10 ) / grid.Spacing(), ( size.h + 10 ) / grid.Spacing()) + 2;
  const int width = 1;
  for ( int i = 0; i != maxPos; i++ ){
    draw_line(bmp, truncated(Point(i * spacing - xOff, 0)), truncated(Point(i * spacing - xOff, bmp.m_h)), color, width, dashed, LineCap::BUTT );
    draw_line(bmp, truncated(Point(0, i * spacing - yOff)), truncated(Point(bmp.m_w, i * spacing - yOff)), color, width, dashed, LineCap::BUTT );
  }
  wxBitmap wxBmp(to_wx(bmp));
  return wxBmp;
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

void OverlayDC_WX::Caret( const Line& line ){
  m_dc.SetPen( wxPen(wxColour(0, 0, 0), 1 ) );
  Point p0(line.P0());
  Point p1(line.P1());
  int x0 = truncated(p0.x * m_scale); // Fixme: Rounding?
  int x1 = truncated(p1.x * m_scale);
  int y0 = truncated(p0.y * m_scale);
  int y1 = truncated(p1.y * m_scale);
  m_dc.DrawLine( x0, y0, x1, y1 );
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

void OverlayDC_WX::GridLines( const Grid& grid, const IntPoint& imageRegionTopLeft, const IntSize& size ){
  if ( m_scale < 1 ){
    return; // Fixme
  }
  if ( m_scale == 1 && grid.Spacing() == 1 ){
    return;
  }
  wxBitmap bmp(get_grid_bitmap(grid, imageRegionTopLeft, m_scale, size));
  // + 1 so that a grid-line on the far edge is visible (wxRect does not include far edge)
  m_dc.SetClippingRegion( wxRect(0, 0,
      truncated( m_imageSize.w * m_scale + LITCRD(1.0) ),
      truncated( m_imageSize.h * m_scale + LITCRD(1.0) ) ) );
  m_dc.DrawBitmap( bmp, imageRegionTopLeft.x * m_scale, imageRegionTopLeft.y * m_scale);
}

void OverlayDC_WX::Handles( const Rect& rect ){
  overlay_pen_and_brush(m_dc);
  draw_handle(m_dc, rect.TopLeft(), m_scale);
  draw_handle(m_dc, rect.TopRight(), m_scale);
  draw_handle(m_dc, rect.BottomLeft(), m_scale);
  draw_handle(m_dc, rect.BottomRight(), m_scale);
}

void OverlayDC_WX::HorizontalLine( faint::coord in_y ){
  m_dc.SetPen( wxPen(wxColour(255, 0, 255), 1, wxPENSTYLE_DOT ) );
  int y = truncated(in_y * m_scale);
  int x0 = truncated(m_imageRect.x * m_scale);
  int x1 = truncated( (m_imageRect.x + m_imageRect.w) * m_scale);
  m_dc.DrawLine( x0, y, x1, y ); // Fixme: Rounding?
}

void OverlayDC_WX::MovablePoint( const Point& center ){
  overlay_pen_and_brush( m_dc );
  m_dc.DrawCircle( rounded(center.x * m_scale), rounded(center.y * m_scale), g_movableRadius );
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
    m_dc.DrawLine(p.x, p.y - dn, p.x, p.y + dp);
    m_dc.DrawLine(p.x - dn, p.y, p.x + dp, p.y);
  }
  else {
    // Draw a "hash"-box around the pixel
    m_dc.DrawLine(p.x, p.y - dn, p.x, p.y + m_scale + dp); // Left
    m_dc.DrawLine(p.x - dn, p.y, p.x + m_scale + dp, p.y ); // Top
    m_dc.DrawLine(p.x - dn, p.y + m_scale, p.x + m_scale + dp, p.y + m_scale ); // Bottom
    m_dc.DrawLine(p.x + m_scale, p.y - dn, p.x + m_scale, p.y + m_scale + dp ); // Right
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
