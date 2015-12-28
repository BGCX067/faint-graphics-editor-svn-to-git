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

#include <cstddef>
#include <cassert>
#include "faintdc.hh"
#include "util/angle.hh"
#include "settings.hh"
#include "tools/settingid.hh"
#include "bitmap/bitmap.h"
#include "bitmap/cairo_util.h"
#include "util/util.hh"
#include "pango/pangocairo.h"
#include "objects/objrectangle.hh"
#include "objects/objellipse.hh"
#include "objects/objtext.hh"

faint::Color GetColorFg( const FaintSettings& );
faint::Color GetColorBg( const FaintSettings& );
void LineJoinToCairo( cairo_t*, const FaintSettings& );
void LineCapToCairo( cairo_t*, const FaintSettings& );

void DrawArrowHead( cairo_t* cr, const ArrowHead& arrowHead ){
  Point p0( arrowHead.P0() );
  Point p1( arrowHead.P1() );
  Point p2( arrowHead.P2() );

  cairo_move_to( cr, p0.x, p0.y );
  cairo_line_to( cr, p1.x, p1.y );
  cairo_line_to( cr, p2.x, p2.y );
  cairo_close_path( cr );
  cairo_fill( cr );
}

void DrawArrowHead( faint::Bitmap& bmp, const ArrowHead& a, const faint::Color& c ){
  Point p0( a.P0() );
  Point p1( a.P1() );
  Point p2( a.P2() );
  FillTriangle( bmp, p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
}

inline void RealignCairoFill( faint::coord& x, faint::coord& y, faint::coord scale ){
  x = static_cast<int>(x * scale + 0.5) / scale;
  y = static_cast<int>(y * scale + 0.5) / scale;
}

inline void RealignCairoOddStroke( faint::coord& x, faint::coord& y, faint::coord scale ){
  x = (static_cast<int>(x * scale + 0.5) + 0.5) / scale;
  y = (static_cast<int>(y * scale + 0.5) + 0.5) / scale;
}

void RealignCairo( faint::coord& x, faint::coord& y, const FaintSettings& s, faint::coord scale ){
  // Cairo aligns fill-operations and strokes with even strokewidths
  // at integer coordinates.  Strokes and fills at half-integer
  // coordinates appear smeared (especially obvious is the case of
  // single pixel horizontal and vertical lines).
  faint::coord lineWidth = s.Get( ts_LineWidth );
  const bool fillOnly = s.Has( ts_FillStyle ) && s.Get( ts_FillStyle ) == FILL;
  const bool even = int(lineWidth * scale + 0.5) % 2 == 0;
  if ( even || fillOnly ){
    // Round to nearest integer coordinate for strokes with even linewidth
    // or fills
    RealignCairoFill(x, y, scale );
  }
  else {
    // Round to nearest .5 for strokes with odd linewidth
    RealignCairoOddStroke(x, y, scale );
  }
}


inline void FillAndOrStroke( cairo_t* cr, const FaintSettings& s ){
  LineCapToCairo( cr, s );
  LineJoinToCairo( cr, s );
  faint::coord lineWidth = s.Has( ts_LineWidth ) ? s.Get( ts_LineWidth ) : LITCRD(1.0);
  cairo_set_line_width( cr, lineWidth );
  if ( s.Has( ts_FillStyle ) ){
    int fillStyle = s.Get( ts_FillStyle );
    if ( fillStyle == FILL ){
      // Fill with the foreground color
      faint::Color fgCol = GetColorFg(s);
      cairo_set_source_rgba( cr, fgCol.r / 255.0, fgCol.g / 255.0, fgCol.b / 255.0, fgCol.a / 255.0 );
      cairo_fill( cr );
      return; // No stroke, return early
    }
    if ( fillStyle == BORDER_AND_FILL ){
      // Fill with the background color
      faint::Color bgCol = GetColorBg(s);
      cairo_set_source_rgba( cr, bgCol.r / 255.0, bgCol.g / 255.0, bgCol.b / 255.0, bgCol.a / 255.0 );
      cairo_fill_preserve( cr );
    }
  }

  // Stroke with the foreground color
  faint::Color fgCol = GetColorFg(s);
  cairo_set_source_rgba( cr, fgCol.r / 255.0, fgCol.g / 255.0, fgCol.b / 255.0, fgCol.a / 255.0 );
  cairo_stroke( cr );
}

inline void FromSettings( cairo_t* cr, const FaintSettings& s ){
  LineCapToCairo( cr, s );
  LineJoinToCairo( cr, s );

  faint::coord lineWidth = s.Has( ts_LineWidth ) ? s.Get( ts_LineWidth ) : LITCRD(1.0);
  cairo_set_line_width( cr, lineWidth );

  faint::Color fgCol = s.Has( ts_SwapColors ) && s.Get( ts_SwapColors ) ?
    s.Get( ts_BgCol ) : s.Get( ts_FgCol );

  cairo_set_source_rgba( cr, fgCol.r / 255.0, fgCol.g / 255.0, fgCol.b / 255.0, fgCol.a / 255.0 );

  if ( s.Has( ts_LineStyle ) && s.Get( ts_LineStyle ) == faint::LONG_DASH ) {
    double dashes[] = {double(2 * lineWidth), double(2 * lineWidth)};
    cairo_set_dash( cr, dashes, 2, 0 );
  }
  else {
    cairo_set_dash( cr, 0, 0, 0 );
  }
}

faint::Brush GetBrush( const size_t size, int shape ){
  if ( shape == BRUSH_SQUARE ){
    return faint::Brush( size, size );
  }
  else if ( shape == BRUSH_CIRCLE ){
    return faint::CircleBrush( size );
  }
  else {
    assert( false );
    return faint::Brush( size, size );
  }
}

faint::Color GetColorBg( const FaintSettings& s ){
  return s.Has( ts_SwapColors ) && s.Get( ts_SwapColors ) ?
    s.Get( ts_FgCol ) : s.Get( ts_BgCol );
}

faint::Color GetColorFg( const FaintSettings& s ){
  return s.Has( ts_SwapColors ) && s.Get( ts_SwapColors ) ?
    s.Get( ts_BgCol ) : s.Get( ts_FgCol );
}

void LineCapToCairo( cairo_t* cr, const FaintSettings& s ){
  if ( !s.Has( ts_LineCap ) ){
    return;
  }

  const int cap = s.Get( ts_LineCap );
  if ( cap == faint::CAP_ROUND ){
    cairo_set_line_cap( cr, CAIRO_LINE_CAP_ROUND );
  }
  else if ( cap == faint::CAP_BUTT ){
    cairo_set_line_cap( cr, CAIRO_LINE_CAP_BUTT );
  }
  else {
    assert( false );
  }
}

void LineJoinToCairo( cairo_t* cr, const FaintSettings& s ){
  if ( !s.Has( ts_LineJoin ) ){
    return;
  }
  int join = s.Get( ts_LineJoin );
  if ( join == faint::JOIN_ROUND ){
    cairo_set_line_join( cr, CAIRO_LINE_JOIN_ROUND );
  }
  else if ( join == faint::JOIN_BEVEL ){
    cairo_set_line_join( cr, CAIRO_LINE_JOIN_BEVEL );
  }
  else if ( join == faint::JOIN_MITER ){
    cairo_set_line_join( cr, CAIRO_LINE_JOIN_MITER );
  }
  else {
    assert( false );
  }
}

FaintDC::FaintDC( faint::Bitmap& bmp )
  : m_bitmap(bmp)
{
  m_x0 = m_y0 = 0;
  m_sc = LITCRD(1.0);

  m_surface = GetCairoSurface( m_bitmap );
  m_cr = cairo_create( m_surface );
  cairo_select_font_face ( m_cr, "purisa", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_BOLD );
  cairo_set_font_size( m_cr, 20 );
}

FaintDC::~FaintDC(){
  cairo_destroy( m_cr );
  cairo_surface_destroy( m_surface );
}

void FaintDC::Bitmap( faint::Bitmap& drawnBitmap, const Point& topLeft ){
  faint::coord x = topLeft.x;
  faint::coord y = topLeft.y;
  if ( x + m_x0 >= static_cast<int>(m_bitmap.m_w) || y + m_y0 >= static_cast<int>(m_bitmap.m_h) ){
    // Outside target bitmap area
    return;
  }

  if ( m_sc < 1 ){
    faint::Bitmap scaled( ScaleBilinear( drawnBitmap, m_sc, m_sc ) );
    DrawBitmap( m_bitmap, scaled, x * m_sc + m_x0, y * m_sc + m_y0 );
  }
  else if ( m_sc > 1 ){
    faint::Bitmap scaled( ScaleNearest( drawnBitmap, m_sc ) );
    DrawBitmap( m_bitmap, scaled, x * m_sc + m_x0, y * m_sc + m_y0 );
  }
  else {
    DrawBitmap( m_bitmap, drawnBitmap, x + m_x0, y + m_y0 );
  }
}

void FaintDC::Bitmap( faint::Bitmap& drawnBitmap, const faint::Color& maskColor, const Point& topLeft ){
  faint::coord x = topLeft.x;
  faint::coord y = topLeft.y;
  if ( x + m_x0 >= static_cast<int>(m_bitmap.m_w) || y + m_y0 >= static_cast<int>(m_bitmap.m_h) ) {
    // Outside target bitmap area
    return;
  }

  if ( m_sc < 1 ){
    faint::Bitmap scaled( ScaleBilinear( drawnBitmap, m_sc, m_sc ) );
    DrawBitmap( m_bitmap, scaled, maskColor, x * m_sc + m_x0, y * m_sc + m_y0 );
  }
  else if ( m_sc > 1 ){
    faint::Bitmap scaled( ScaleNearest( drawnBitmap, m_sc ) );
    DrawBitmap( m_bitmap, scaled,  maskColor, x * m_sc + m_x0, y * m_sc + m_y0 );
  }
  else {
    DrawBitmap( m_bitmap, drawnBitmap, maskColor, x + m_x0, y + m_y0 );
  }
}

void FaintDC::Blend( faint::Bitmap& drawnBitmap, const Point& topLeft ){
  faint::coord x = topLeft.x;
  faint::coord y = topLeft.y;
  if ( x + m_x0 >= static_cast<int>(m_bitmap.m_w) || y + m_y0 >= static_cast<int>(m_bitmap.m_h) ) {
    // Outside target bitmap area
    return;
  }

  if ( m_sc < 1 ){
    faint::Bitmap scaled( ScaleBilinear( drawnBitmap, m_sc, m_sc ) );
    BlendBitmap( m_bitmap, scaled, x * m_sc + m_x0, y * m_sc + m_y0 );
  }
  else if ( m_sc > 1 ){
    faint::Bitmap scaled( ScaleNearest( drawnBitmap, m_sc ) );
    BlendBitmap( m_bitmap, scaled, x * m_sc + m_x0 + 0.5, y * m_sc + m_y0 + 0.5 );
  }
  else {
    BlendBitmap( m_bitmap, drawnBitmap, x * m_sc + m_x0 + 0.5, y * m_sc + m_y0 + 0.5 );
  }
}

void FaintDC::Blend( faint::Bitmap& drawnBitmap, const faint::Color& maskColor, const Point& topLeft ){
  faint::coord x = topLeft.x;
  faint::coord y = topLeft.y;
  if ( x + m_x0 >= static_cast<int>(m_bitmap.m_w) || y + m_y0 >= static_cast<int>(m_bitmap.m_h) ) {
    // Outside target bitmap area
    return;
  }

  if ( m_sc < 1 ){
    faint::Bitmap scaled( ScaleBilinear( drawnBitmap, m_sc, m_sc ) );
    BlendBitmap( m_bitmap, scaled, maskColor, x * m_sc + m_x0 + 0.5, y * m_sc + m_y0 + 0.5 );
  }
  else if ( m_sc > 1 ){
    faint::Bitmap scaled( ScaleNearest( drawnBitmap, m_sc ) );
    BlendBitmap( m_bitmap, scaled, maskColor, x * m_sc + m_x0 + 0.5, y * m_sc + m_y0 + 0.5 );
  }
  else {
    BlendBitmap( m_bitmap, drawnBitmap, maskColor, x * m_sc + m_x0 + 0.5, y * m_sc + m_y0 + 0.5 );
  }
}

void FaintDC::Clear( const faint::Color& color ){
  faint::Clear( m_bitmap, color );
}

void FaintDC::Ellipse( faint::coord x, faint::coord y, faint::coord w, faint::coord h, const FaintSettings& s ){
  if ( s.Has( ts_AntiAlias ) && !s.Get( ts_AntiAlias ) ){
    int fillStyle = s.Get( ts_FillStyle );
    faint::Color fgCol = s.Get( ts_FgCol );
    faint::Color bgCol = s.Get( ts_BgCol );
    const faint::coord lineWidth = s.Get( ts_LineWidth );

    using std::swap;
    if ( s.Has( ts_SwapColors ) && s.Get( ts_SwapColors ) ){
      std::swap( fgCol, bgCol );
    }
    if ( fillStyle == FILL ){
      swap( fgCol, bgCol );
    }

    faint::coord x_adj = x * m_sc + m_x0;
    faint::coord y_adj = y * m_sc + m_y0;
    faint::coord w_adj = w * m_sc;
    faint::coord h_adj = h * m_sc;

    if ( fillStyle == BORDER_AND_FILL || fillStyle == FILL ){
      faint::FillEllipse( m_bitmap, x_adj + w_adj / 2, y_adj + h_adj / 2, w_adj / 2, h_adj / 2, bgCol );
    }
    if ( fillStyle == BORDER || fillStyle == BORDER_AND_FILL ){
      faint::DrawEllipse( m_bitmap, x_adj + w_adj / 2, y_adj + h_adj / 2, w_adj / 2, h_adj / 2, lineWidth * m_sc, fgCol );
    }
  }
  else {
    FromSettings( m_cr, s );
    RealignCairo( x, y, s, m_sc );
    cairo_save ( m_cr );
    cairo_translate ( m_cr, x + w / 2., y + h / 2. );
    cairo_scale ( m_cr, w / 2., h / 2. );
    cairo_arc (m_cr, 0., 0., 1., 0., 2 * faint::pi );
    cairo_restore (m_cr);
    FillAndOrStroke( m_cr, s );
  }
}

void FaintDC::Ellipse( const Tri& tri0, const FaintSettings& s ){
  if ( s.Has( ts_AntiAlias ) && !s.Get( ts_AntiAlias ) ){
    DrawRasterEllipse( tri0, s );
    return;
  }

  faint::coord skew = tri0.Skew();
  Tri tri( Skewed( tri0, -skew ) );

  faint::coord angle = tri.Angle();
  tri = Rotated( tri, - angle, tri.P0() );

  FromSettings( m_cr, s );
  std::vector<Point> v = EllipseAsPath( tri0 );

  cairo_save ( m_cr );
  cairo_move_to( m_cr, v[0].x, v[0].y );

  for ( size_t i = 1; i != v.size(); i += 3 ){
    cairo_curve_to( m_cr,
      v[i].x, v[i].y,
      v[i + 1].x, v[i + 1].y,
      v[i +2].x, v[i + 2].y );
  }
  FillAndOrStroke( m_cr, s );
  cairo_restore(m_cr);
}

faint::Color FaintDC::GetPixel( faint::coord x, faint::coord y ) const{
  int xAdj = static_cast<int>( x * m_sc + m_x0 + 0.5 );
  int yAdj = static_cast<int>( y * m_sc + m_y0 + 0.5 );
  return GetColor( m_bitmap, xAdj, yAdj );
}

void FaintDC::Line( const Tri& t, const FaintSettings& s ){
  Point p0(t.P0());
  Point p1(t.P1());
  Line( p0.x, p0.y, p1.x, p1.y, s );
}

void FaintDC::Line( faint::coord x0, faint::coord y0, faint::coord x1, faint::coord y1, const FaintSettings& s ){
  faint::coord sz = s.Get( ts_LineWidth );
  if ( s.Has( ts_AntiAlias ) &&!s.Get( ts_AntiAlias ) ){
    x0 = x0 * m_sc + m_x0;
    y0 = y0 * m_sc + m_y0;
    x1 = x1 * m_sc + m_x0;
    y1 = y1 * m_sc + m_y0;
    faint::Color color = (s.Has( ts_SwapColors ) && s.Get( ts_SwapColors )) ?
      s.Get( ts_BgCol ) : s.Get( ts_FgCol );

    if ( s.Has( ts_LineArrowHead ) && s.Get( ts_LineArrowHead ) == 1 ){
      ArrowHead a( GetArrowHead( ::Line(Point(x0, y0), Point(x1, y1)), s.Get( ts_LineWidth ) ) );
      Point anchor = a.LineAnchor();
      DrawLine( m_bitmap, x0, y0, anchor.x, anchor.y, color, sz );
      DrawArrowHead( m_bitmap, a, color );
    }
    else {
      DrawLine( m_bitmap, x0, y0, x1, y1, color, sz * m_sc );
    }
    return;
  }

  if ( x0 == x1 && y0 == y1 ){
    // Don't draw zero-length lines as vector graphics
    return;
  }

  RealignCairo( x0, y0, s, m_sc );
  RealignCairo( x1, y1, s, m_sc );

  FromSettings( m_cr, s );
  cairo_move_to( m_cr, x0, y0 );

  if ( s.Has( ts_LineArrowHead ) && s.Get( ts_LineArrowHead ) == 1 ){
    ArrowHead a( GetArrowHead( ::Line(Point(x0, y0), Point(x1, y1)), s.Get( ts_LineWidth ) ) );
    Point anchor = a.LineAnchor();
    cairo_line_to( m_cr, anchor.x, anchor.y );
    cairo_stroke( m_cr );
    DrawArrowHead( m_cr, a );
  }
  else {
    cairo_line_to( m_cr, x1, y1 );
    cairo_stroke( m_cr );
  }
}

void FaintDC::Lines( const std::vector<IntPoint>& points, const FaintSettings& s ){
  if ( points.empty() ){
    return;
  }

  faint::Color c = ( s.Has( ts_SwapColors ) && s.Get( ts_SwapColors ) ) ?
    s.Get( ts_BgCol ) :
    s.Get( ts_FgCol );
  if ( points.size() == 1 ){
    DrawLine( m_bitmap, points[0].x * m_sc + m_x0, points[0].y * m_sc + m_y0, points[0].x * m_sc + m_x0, points[0].y * m_sc + m_y0, c );
    return;
  }

  Point prev = floated(points[0]) * m_sc + Point( m_x0, m_y0 );
  for ( size_t i = 1; i != points.size(); i++ ){
    Point pt = floated(points[i]) * m_sc + Point( m_x0, m_y0 );
    DrawLine( m_bitmap, prev.x, prev.y, pt.x, pt.y, c, m_sc );
    prev = pt;
  }
}

void FaintDC::Path( const std::vector<PathPt>& points, const FaintSettings& s ){
  if ( points.empty() ){
    return;
  }

  FromSettings( m_cr, s );
  faint::coord curr_x = points.front().x;
  faint::coord curr_y = points.front().y;

  for ( size_t i = 0; i != points.size(); i++ ){
    const PathPt& pt = points[i];
    if ( pt.type == PathPt::MoveTo ){
      cairo_move_to( m_cr, pt.x, pt.y );
      curr_x = pt.x;
      curr_y = pt.y;
    }
    else if ( pt.type == PathPt::LineTo ){
      cairo_line_to( m_cr, pt.x, pt.y );
      curr_x = pt.x;
      curr_y = pt.y;
    }
    else if ( pt.type == PathPt::CubicBezier ){
      cairo_curve_to( m_cr, pt.cx, pt. cy, pt.dx, pt.dy, pt.x, pt.y );
      curr_x = pt.x;
      curr_y = pt.y;
    }
    else if ( pt.type == PathPt::Close ){
      cairo_close_path( m_cr );
    }
    else if ( pt.type == PathPt::ArcTo ){
      // Fixme: Incorrect like nuts
      const faint::coord x0 = curr_x;
      const faint::coord y0 = curr_y;
      const faint::coord x1 = pt.x;
      const faint::coord y1 = pt.y;
      if ( pt.large_arc_flag == 1 ){
	faint::coord cx = x0;
	faint::coord cy = y0 + ( y1 - y0 ) / LITCRD(2.0);
	faint::coord dx = x0 + ( x1 - x0 ) / LITCRD(2.0);
	faint::coord dy = y1;
        cairo_curve_to( m_cr, cx, cy, dx, dy, x1, y1 );
      }

      else if ( pt.large_arc_flag == 0 ){
	faint::coord cx = x0 + ( x1 - x0 ) / LITCRD(2.0);
	faint::coord cy = y0;
	faint::coord dx = x1;
	faint::coord dy = y0 + ( y1 - y0 ) / LITCRD(2.0);
        cairo_curve_to( m_cr, cx, cy, dx, dy, x1, y1 );
      }

      curr_x = x1;
      curr_y = y1;
    }
  }
  FillAndOrStroke( m_cr, s );
}

void FaintDC::Polygon( const std::vector<Point>& points, const FaintSettings& s ){
  if ( points.size() <= 1 ){
    return;
  }

  FromSettings( m_cr, s );
  const Point& p0 = points[0];
  faint::coord x( p0.x );
  faint::coord y( p0.y );
  RealignCairo( x, y, s, m_sc );
  cairo_move_to( m_cr, x, y );
  for ( size_t i = 1; i != points.size(); i++ ){
    const Point& p( points[i] );
    x = p.x;
    y = p.y;
    RealignCairo( x, y, s, m_sc );
    cairo_line_to( m_cr, x, y );
  }
  cairo_close_path( m_cr );
  FillAndOrStroke( m_cr, s );
}

void FaintDC::Rectangle( const IntRect& r, const FaintSettings& s ){
  Rectangle( TriFromRect( floated(r) ), s );
}

void FaintDC::Rectangle( const Point& p0, const Point& p1, const FaintSettings& s ){
  Tri t( TriFromRect(Rect(p0,p1)) );
  Rectangle( t, s );
}

void FaintDC::Rectangle( const IntPoint& p1, const IntPoint& p2, const FaintSettings& s ){
  Rectangle( floated(p1), floated(p2), s ); // Fixme
}

void FaintDC::Rectangle( const Tri& tri, const FaintSettings& s ){
  if ( s.Has( ts_AntiAlias ) && !s.Get( ts_AntiAlias ) ){
    DrawRasterRect( tri, s );
    return;
  }

  //if ( fabs(tri.Skew()) > 0.0001 ){ // Fixme
    std::vector<Point> points;
    points.push_back( tri.P0() );
    points.push_back( tri.P1() );
    points.push_back( tri.P3() );
    points.push_back( tri.P2() );
    Polygon( points, s );
    //return;
    //}

//   FromSettings( m_cr, s );
//   faint::coord x0 = tri.P0().x;
//   faint::coord y0 = tri.P0().y;
//   faint::coord x1 = x0 + tri.Width(); // Fixme
//   faint::coord y1 = y0 + tri.Height();
//   RealignCairo( x0, y0, s, m_sc );
//   RealignCairo( x1, y1, s, m_sc );
//   faint::coord w = x1 - x0;
//   faint::coord h = y1 - y0;
//   cairo_save ( m_cr );
//   cairo_translate( m_cr, x0, y0 );
//   cairo_rotate( m_cr, tri.Angle() );
//   cairo_rectangle( m_cr, 0, 0, w + ( w < 0 ? 1 : -1), h + (h < 0 ? 1 : -1 )); // Cairo-style, -1 eh.
//   FillAndOrStroke( m_cr, s );
//   cairo_restore( m_cr );
}

void FaintDC::SetOrigin( faint::coord x, faint::coord y ){
  cairo_translate( m_cr, x, y );
  m_x0 = x;
  m_y0 = y;
}

void FaintDC::SetScale( faint::coord scale ){
  m_sc = scale;
  cairo_scale( m_cr, m_sc, m_sc );
}

void FaintDC::Spline( const std::vector<Point>& points, const FaintSettings& s ){
  if ( points.size() <= 2 ){
    return;
  }

  FromSettings( m_cr, s );
  const Point& p0 = points[0];
  faint::coord x1 = p0.x;
  faint::coord y1 = p0.y;

  const Point& p2 = points[1];
  faint::coord c = p2.x;
  faint::coord d = p2.y;

  faint::coord x3 = ( x1 + c ) / 2;
  faint::coord y3 = ( y1 + d ) / 2;


  cairo_move_to( m_cr, x1, y1 );
  cairo_line_to( m_cr, x3, y3 );
  faint::coord x2, y2;

  for ( size_t i = 2; i < points.size(); i++ ){
    const Point& pt = points[i];
    x1 = x3;
    y1 = y3;
    x2 = c;
    y2 = d;
    c = pt.x;
    d = pt.y;
    x3 = ( x2 + c ) / 2;
    y3 = ( y2 + d ) / 2;

    cairo_curve_to( m_cr, x1, y1,
      x2, y2,
      x3, y3 );
  }
  cairo_line_to( m_cr, c, d );
  cairo_stroke( m_cr );
}

void FaintDC::Stroke( const std::vector<IntPoint>& points, const FaintSettings& s ){
  if ( points.size() < 1 ){
    return;
  }

  faint::Color c = s.Has( ts_SwapColors ) && s.Get( ts_SwapColors ) ? s.Get( ts_BgCol ) : s.Get( ts_FgCol );

  faint::Brush b = ( s.Has( ts_BrushSize ) ?
    GetBrush( s.Get( ts_BrushSize ), s.Get( ts_BrushShape ) ) :
    GetBrush( 1, BRUSH_SQUARE ) );

  if ( points.size() == 1 ){
    faint::Stroke( m_bitmap, points[0].x * m_sc + m_x0, points[0].y * m_sc + m_y0,
      points[0].x * m_sc + m_x0, points[0].y * m_sc + m_y0, b, c );
    return;
  }

  IntPoint prev = points[0];
  for ( size_t i = 1; i != points.size(); i++ ){
    faint::Stroke( m_bitmap, points[0].x * m_sc + m_x0, points[0].y * m_sc + m_y0,
      points[0].x * m_sc + m_x0, points[0].y * m_sc + m_y0, b, c );

    faint::Stroke( m_bitmap, prev.x * m_sc + m_x0, prev.y * m_sc + m_y0,
      points[i].x * m_sc + m_x0, points[i].y * m_sc + m_y0, b, c );
    prev = points[i];
  }
}

void FaintDC::Text( const Tri& t, const std::string& text, const FaintSettings& s ){
  const bool bold = s.Has( ts_FontBold ) && s.Get( ts_FontBold );
  const bool italic = s.Has( ts_FontItalic ) && s.Get( ts_FontItalic );
  const faint::Color fgCol( s.Get( ts_FgCol ) );
  const faint::Color bgCol( s.Get( ts_BgCol ) );

  cairo_save( m_cr );
  faint::coord x0 = t.P0().x;
  faint::coord y0 = t.P0().y;

  PangoFontDescription* font_description = pango_font_description_new();
  pango_font_description_set_family (font_description, s.Get( ts_FontFace ).c_str());
  pango_font_description_set_weight (font_description, bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL );
  pango_font_description_set_style( font_description, italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL );
  pango_font_description_set_absolute_size (font_description, s.Get( ts_FontSize ) * PANGO_SCALE);

  // Offset to anchor at the top
  PangoLayout* referenceLayout = pango_cairo_create_layout (m_cr);
  pango_layout_set_font_description( referenceLayout, font_description );
  pango_layout_set_text( referenceLayout, "M", -1 );
  PangoRectangle ink;
  PangoRectangle logical;
  pango_layout_get_extents(referenceLayout, &ink, &logical);
  g_object_unref( referenceLayout );

  PangoLayout* layout = pango_cairo_create_layout (m_cr);
  pango_layout_set_font_description (layout, font_description);
  pango_layout_set_text (layout, text.c_str(), -1);

  RealignCairoFill(x0,y0,m_sc);
  cairo_translate( m_cr, x0, y0 );
  cairo_rotate( m_cr, t.Angle() );
  if ( t.Height() < 0 ){
    cairo_scale( m_cr, 1, -1 );
  }

  faint::coord xOffset = 0;
  faint::coord yOffset = ink.height / PANGO_SCALE;
  if ( rather_zero( t.Angle() ) ){
    RealignCairoFill(xOffset, yOffset, m_sc);
  }
  cairo_translate( m_cr, xOffset, yOffset );

  // Render text as a Cairo path. Rendering with Pango gives somewhat
  // nicer output on Windows (e.g. ClearType sub-pixel anti aliasing),
  // but I've found no convenient way to turn that of when
  // saving/flattening, and the Pango output also caused letters to
  // "wobble" when the text is rotated.
  PangoLayoutLine* line = pango_layout_get_line( layout, 0 );
  pango_cairo_layout_line_path( m_cr, line );
  cairo_set_source_rgba( m_cr, fgCol.r / 255.0, fgCol.g / 255.0, fgCol.b / 255.0, fgCol.a / 255.0 );
  cairo_fill( m_cr );
  g_object_unref (layout);
  pango_font_description_free (font_description);
  cairo_restore( m_cr );
}

Size FaintDC::TextExtents( const std::string& text, const FaintSettings& s ) const{
  const bool bold = s.Has( ts_FontBold ) && s.Get( ts_FontBold );
  const bool italic = s.Has( ts_FontItalic ) && s.Get( ts_FontItalic );
  PangoFontDescription* font_description = pango_font_description_new ();  font_description = pango_font_description_new();
  pango_font_description_set_family (font_description, s.Get( ts_FontFace ).c_str());
  pango_font_description_set_weight (font_description, bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL );
  pango_font_description_set_style( font_description, italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL );
  pango_font_description_set_absolute_size (font_description, s.Get( ts_FontSize ) * PANGO_SCALE);
  PangoLayout* layout = pango_cairo_create_layout (m_cr);
  pango_layout_set_font_description (layout, font_description);
  pango_layout_set_text (layout, text.c_str(), -1);

  PangoRectangle ink;
  PangoRectangle logical;
  pango_layout_get_extents(layout, &ink, &logical);
  pango_font_description_free( font_description );
  g_object_unref (layout);
  return Size( ink.width / PANGO_SCALE, ink.height / PANGO_SCALE );
}

void FaintDC::DrawPoint( const Point& pt ){
  Ellipse( pt.x - 5, pt.y - 5, 10, 10, GetEllipseSettings() );
}

void FaintDC::DrawRasterEllipse( const Tri& tri, const FaintSettings& s ){
  using std::swap;
  faint::Color fgCol = s.Get( ts_FgCol );
  faint::Color bgCol = s.Get( ts_BgCol );
  if ( s.Has( ts_SwapColors ) && s.Get( ts_SwapColors ) ){
    swap( fgCol, bgCol );
  }
  int fillStyle = s.Get( ts_FillStyle );
  if ( fillStyle == FILL ){
    swap( fgCol, bgCol );
  }

  const Point origin(m_x0, m_y0);
  if ( fillStyle == BORDER_AND_FILL || fillStyle == FILL ){
    Point origin( m_x0, m_y0 );
    FillEllipse( m_bitmap, tri.P0() * m_sc + origin, tri.P3() * m_sc + origin, bgCol );
  }
  if ( fillStyle == BORDER || fillStyle == BORDER_AND_FILL ){
    DrawEllipse( m_bitmap, tri.P0() * m_sc + origin, tri.P3() * m_sc + origin , s.Get( ts_LineWidth ), fgCol );
  }
}

void FaintDC::DrawRasterRect( const Tri& tri, const FaintSettings& s ){
  using std::swap;
  faint::Color fgCol = s.Get( ts_FgCol );
  faint::Color bgCol = s.Get( ts_BgCol );
  if ( s.Has( ts_SwapColors ) && s.Get( ts_SwapColors ) ){
    swap( fgCol, bgCol );
  }
  int fillStyle = s.Get( ts_FillStyle );
  if ( fillStyle == FILL ){
    swap( fgCol, bgCol );
  }

  const Point origin(m_x0, m_y0);
  if ( fillStyle == BORDER_AND_FILL || fillStyle == FILL ){
    FillRect( m_bitmap, tri.P0() * m_sc + origin, tri.P3() * m_sc + origin, bgCol );
  }
  if ( fillStyle == BORDER || fillStyle == BORDER_AND_FILL ){
    DrawRect( m_bitmap, tri.P0() * m_sc + origin, tri.P3() * m_sc + origin, fgCol, s.Get( ts_LineWidth ) * m_sc );
  }
}

