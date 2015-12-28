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

#include "overlaydcwx.hh"
#include "wx/wx.h"
#include "util/convertwx.hh"
OverlayDC_WX::OverlayDC_WX( wxDC& dc, faint::coord scale, const Rect& imageRect, const IntSize& imageSize )
  : m_dc(dc),
    m_imageRect(imageRect),
    m_imageSize(imageSize),
    m_scale(scale)
{}

const int pivotRadius = 5;

void OverlayDC_WX::Pivot( const Point& center ){
  OverlayPenAndBrush( m_dc );
  m_dc.DrawCircle( truncated(center.x * m_scale), truncated(center.y * m_scale), pivotRadius );
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

void OverlayDC_WX::HorizontalLine( faint::coord in_y ){
  m_dc.SetPen( wxPen(wxColour(255, 0, 255), 1, wxPENSTYLE_DOT ) );
  int y = truncated(in_y * m_scale);
  int x0 = truncated(m_imageRect.x * m_scale);
  int x1 = truncated( (m_imageRect.x + m_imageRect.w) * m_scale);
  m_dc.DrawLine( x0, y, x1, y );
}

void OverlayDC_WX::Caret( const Line& line ){
  m_dc.SetPen( wxPen(wxColour(0, 0, 0), 1 ) );
  Point p0(line.P0());
  Point p1(line.P1());
  int x0 = rounded(p0.x * m_scale); // Fixme: Is there a reason I rounded the x:s but not the y:s?
  int x1 = rounded(p1.x * m_scale);
  int y0 = truncated(p0.y * m_scale);
  int y1 = truncated(p1.y * m_scale);
  m_dc.DrawLine( x0, y0, x1, y1 );
}

void OverlayDC_WX::GridLines( const Grid& grid ){  
  // + 1 so that a grid-line on the far edge is visible (wxRect does not include far edge)
  m_dc.SetClippingRegion( wxRect(0, 0, 
    truncated( m_imageSize.w * m_scale + LITCRD(1.0) ), 
    truncated( m_imageSize.h * m_scale + LITCRD(1.0) ) ) );
  m_dc.SetPen( wxPen(to_wx( grid.Color() ), 1 ) );

  int spacing = grid.Spacing();
  int xOff = truncated(m_imageRect.x / spacing - LITCRD(1.0));
  int yOff = truncated(m_imageRect.y / spacing - LITCRD(1.0));
  int x_steps = truncated(m_imageRect.w / spacing + LITCRD(3.0));
  int y_steps = truncated(m_imageRect.h / spacing + LITCRD(3.0));
  int steps = std::max(x_steps, y_steps );

  int x0 = truncated(m_imageRect.x * m_scale);
  int y0 = truncated(m_imageRect.y * m_scale);
  int x1 = truncated(( m_imageRect.x + m_imageRect.w ) * m_scale);
  int y1 = truncated(( m_imageRect.y + m_imageRect.h ) * m_scale);
  for ( int i = 0; i != steps; i++ ){
    int x = truncated(( xOff + i ) * spacing * m_scale);
    int y = truncated(( yOff + i ) * spacing * m_scale);
    m_dc.DrawLine( x, y0, x, y1 );
    m_dc.DrawLine( x0, y, x1, y );
  }
  m_dc.DestroyClippingRegion();
}

void OverlayPenAndBrush( wxDC& dc ){
  static wxPen pen(wxColour(0,0,0),1);
  static wxBrush brush( wxColour(128,128,200) );
  dc.SetPen( pen );
  dc.SetBrush( brush );
}
