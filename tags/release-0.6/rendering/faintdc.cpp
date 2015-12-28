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
#include <cstddef>
#include "geo/pathpt.hh"
#include "geo/tri.hh"
#include "rendering/cairocontext.hh"
#include "rendering/faintdc.hh"
#include "util/angle.hh"
#include "util/char.hh"
#include "util/settings.hh"
#include "util/settingutil.hh"
#include "util/util.hh"

// Check if the point is outside the bitmap on the right or bottom sides
bool overextends( const Point& p, const faint::Bitmap& bmp ){
  IntSize sz(bmp.GetSize());
  return p.x >= sz.w || p.y >= sz.h;
}

inline void realign_cairo_fill( faint::coord& x, faint::coord& y, faint::coord scale ){
  x = static_cast<int>(x * scale + 0.5) / scale;
  y = static_cast<int>(y * scale + 0.5) / scale;
}

Point realign_cairo_fill( const Point& p, faint::coord scale ){
  Point p2(p);
  realign_cairo_fill(p2.x, p2.y, scale );
  return p2;
}

inline void realign_cairo_odd_stroke( faint::coord& x, faint::coord& y, faint::coord scale ){
  x = (static_cast<int>(x * scale + 0.5) + 0.5) / scale;
  y = (static_cast<int>(y * scale + 0.5) + 0.5) / scale;
}

Point realign_cairo_odd_stroke( const Point& p, faint::coord scale ){
  return Point( (static_cast<int>(p.x * scale + 0.5) + 0.5) / scale,
    (static_cast<int>(p.y * scale + 0.5) + 0.5) / scale );
}

void realign_cairo( faint::coord& x, faint::coord& y, const Settings& s, faint::coord scale ){
  // Cairo aligns fill-operations and strokes with even strokewidths
  // at integer coordinates.  Strokes and fills at half-integer
  // coordinates appear smeared (especially obvious is the case of
  // single pixel horizontal and vertical lines).
  faint::coord lineWidth = s.Get( ts_LineWidth );
  const bool fillOnly = s.Has( ts_FillStyle ) && s.Get( ts_FillStyle ) == FillStyle::FILL;
  const bool even = int(lineWidth * scale + 0.5) % 2 == 0;
  if ( even || fillOnly ){
    // Round to nearest integer coordinate for strokes with even linewidth
    // or fills
    realign_cairo_fill(x, y, scale );
  }
  else {
    // Round to nearest .5 for strokes with odd linewidth
    realign_cairo_odd_stroke(x, y, scale );
  }
}

Point realign_cairo( const Point& p, const Settings& s, faint::coord scale ){
  faint::coord x = p.x;
  faint::coord y = p.y;
  realign_cairo( x, y, s, scale );
  return Point(x,y);
}

void draw_arrow_head( faint::CairoContext* cr, const ArrowHead& arrowHead, faint::coord lineWidth, faint::coord sc ){
  Settings s;
  s.Set( ts_LineWidth, lineWidth );
  Point p0( realign_cairo(arrowHead.P0(), s, sc) );
  Point p1( realign_cairo(arrowHead.P1(), s, sc ) );
  Point p2( realign_cairo(arrowHead.P2(), s, sc) );

  cr->move_to( p0 );
  cr->line_to( p1 );
  cr->line_to( p2 );
  cr->close_path();
  cr->fill();
}

inline void fill_and_or_stroke( faint::CairoContext* cr, const Settings& s ){
  assert(s.Has(ts_FillStyle));
  cr->set_line_cap(to_cap(s.GetDefault(ts_LineCap, LineCap::DEFAULT)));
  cr->set_line_join(to_join(s.GetDefault(ts_LineJoin, LineJoin::DEFAULT)));
  faint::coord lineWidth = s.Has( ts_LineWidth ) ? s.Get( ts_LineWidth ) : LITCRD(1.0);
  cr->set_line_width( lineWidth );

  if ( filled(s) ){
    cr->set_source_rgba( get_color_bg(s) );
    if ( border(s)){
      cr->fill_preserve();
    }
    else {
      cr->fill();
      return; // No stroke, return early
    }
  }

  cr->set_source_rgba( get_color_fg(s) );
  cr->stroke();
}

inline void from_settings( faint::CairoContext* cr, const Settings& s ){
  cr->set_line_cap( to_cap(s.GetDefault(ts_LineCap, LineCap::DEFAULT) ) );
  cr->set_line_join( to_join(s.GetDefault(ts_LineJoin, LineJoin::DEFAULT) ) );
  faint::coord lineWidth = s.GetDefault(ts_LineWidth, LITCRD(1.0));
  cr->set_line_width(lineWidth);
  cr->set_source_rgba( get_color_fg(s) );
  if ( dashed(s) ) {
    faint::coord dashes[] = {2 * lineWidth, 2 * lineWidth};
    cr->set_dash( dashes, 2, 0 );
  }
  else {
    cr->set_dash( 0, 0, 0 );
  }
}

faint::Brush get_brush( const size_t size, int shape ){
  if ( shape == BrushShape::SQUARE ){
    return faint::Brush( size, size );
  }
  else if ( shape == BrushShape::CIRCLE ){
    return faint::circle_brush( size );
  }
  else {
    assert( false );
    return faint::Brush( size, size );
  }
}

FaintDC::FaintDC( faint::Bitmap& bmp )
  : m_bitmap(bmp),
    m_cr(new faint::CairoContext(m_bitmap)),
    m_origin(0,0),
    m_sc(LITCRD(1.0))
{}

FaintDC::FaintDC( faint::Bitmap& bmp, const origin_t& origin )
  : m_bitmap(bmp),
    m_cr(new faint::CairoContext(m_bitmap)),
    m_origin(0,0),
    m_sc(LITCRD(1.0))
{
  SetOrigin(origin.Get());
}

FaintDC::~FaintDC(){
  delete m_cr;
}

void FaintDC::Bitmap( const faint::Bitmap& bmp, const Point& topLeft, const Settings& settings ){
  if ( overextends(topLeft + m_origin, m_bitmap) ){
    return;
  }
  BackgroundStyle::type bgStyle = (BackgroundStyle::type)settings.Get(ts_BackgroundStyle);
  bool alphaBlend = alpha_blending(settings);
  faint::Color bgCol(settings.Get(ts_BgCol));
  if ( bgStyle  == BackgroundStyle::MASKED ){
    if ( alphaBlend ){
      BitmapBlendAlphaMasked(bmp, bgCol, topLeft);
    }
    else{
      BitmapSetAlphaMasked(bmp, bgCol, topLeft);
    }
  }
  else {
    assert(bgStyle == BackgroundStyle::SOLID);
    if ( alphaBlend ){
      BitmapBlendAlpha(bmp, topLeft);
    }
    else {
      BitmapSetAlpha(bmp, topLeft);
    }
  }
}

void FaintDC::BitmapBlendAlpha( const faint::Bitmap& drawnBitmap, const Point& topLeft ){
  assert( !overextends(topLeft + m_origin, m_bitmap) );
  const IntPoint imagePt( truncated(topLeft * m_sc + m_origin) );
  if ( m_sc < 1 ){
    const faint::Bitmap scaled( scale_bilinear( drawnBitmap, Scale(m_sc) ) );
    blend( scaled, onto(m_bitmap), imagePt );
  }
  else if ( m_sc > 1 ){
    const faint::Bitmap scaled( scale_nearest( drawnBitmap, truncated(m_sc) ) );
    blend( scaled, onto(m_bitmap), imagePt );
  }
  else {
    blend( drawnBitmap, onto(m_bitmap), imagePt );
  }
}

void FaintDC::BitmapBlendAlphaMasked( const faint::Bitmap& drawnBitmap, const faint::Color& maskColor, const Point& topLeft ){
  assert( !overextends(topLeft + m_origin, m_bitmap) );

  const IntPoint& imagePt( truncated(topLeft * m_sc + m_origin ) );
  if ( m_sc < 1 ){
    const faint::Bitmap scaled( scale_bilinear( drawnBitmap, Scale(m_sc) ) );
    blend_masked( scaled, onto(m_bitmap), maskColor, imagePt );
  }
  else if ( m_sc > 1 ){
    const faint::Bitmap scaled( scale_nearest( drawnBitmap, truncated(m_sc) ) );
    blend_masked( scaled, onto(m_bitmap), maskColor, imagePt );
  }
  else {
    blend_masked( drawnBitmap, onto(m_bitmap), maskColor, imagePt );
  }
}

void FaintDC::BitmapSetAlpha( const faint::Bitmap& drawnBitmap, const Point& topLeft ){
  assert( !overextends(topLeft + m_origin, m_bitmap) );
  const IntPoint imagePt(truncated(topLeft * m_sc + m_origin));
  if ( m_sc < 1 ){
    const faint::Bitmap scaled( scale_bilinear( drawnBitmap, Scale(m_sc) ) );
    blit( scaled, onto(m_bitmap), imagePt );
  }
  else if ( m_sc > 1 ){
    const faint::Bitmap scaled( scale_nearest( drawnBitmap, truncated(m_sc) ) );
    blit( scaled, onto(m_bitmap), imagePt );
  }
  else {
    blit( drawnBitmap, onto(m_bitmap), imagePt );
  }
}

void FaintDC::BitmapSetAlphaMasked( const faint::Bitmap& drawnBitmap, const faint::Color& maskColor, const Point& topLeft ){
  assert( !overextends(topLeft + m_origin, m_bitmap) );
  const IntPoint imagePt(truncated(topLeft * m_sc + m_origin));
  if ( m_sc < 1 ){
    faint::Bitmap scaled( scale_bilinear( drawnBitmap, Scale(m_sc) ) );
    blit_masked( scaled, onto(m_bitmap), maskColor, imagePt );
  }
  else if ( m_sc > 1 ){
    faint::Bitmap scaled( scale_nearest( drawnBitmap, truncated(m_sc) ) );
    blit_masked( scaled, onto(m_bitmap), maskColor, imagePt );
  }
  else {
    blit_masked( drawnBitmap, onto(m_bitmap), maskColor, imagePt );
  }
}

void FaintDC::Clear( const faint::Color& color ){
  faint::clear( m_bitmap, color );
}

void FaintDC::DrawRasterEllipse( const Tri& tri, const Settings& s ){
  IntRect r(truncated(tri.P0() * m_sc + m_origin ),
    truncated(tri.P3() * m_sc + m_origin) );

  if ( filled(s) ){
    fill_ellipse( m_bitmap, r, get_color_bg(s) );
  }
  if ( border(s) ){
    draw_ellipse( m_bitmap, r, truncated(s.Get( ts_LineWidth )), get_color_fg(s) );
  }
}

void FaintDC::DrawRasterLine( const Point& fl_p0, const Point& fl_p1, const Settings& s ){
  IntPoint p0(truncated(truncated(fl_p0) * m_sc) + truncated(m_origin)); // Fixme: Truncating origin? Not done elsewhere
  IntPoint p1(truncated(truncated(fl_p1) * m_sc) + truncated(m_origin));
  faint::Color color(get_color_fg(s));
  faint::coord lw(s.Get(ts_LineWidth));
  if ( s.Has( ts_LineArrowHead ) && s.Get( ts_LineArrowHead ) == 1 ){
    ArrowHead a( get_arrowhead( ::Line(floated(p0), floated(p1)), s.Get( ts_LineWidth ) ) ); // fixme: Do computations on unspoiled coordinates
    Point anchor = a.LineAnchor();
    draw_line( m_bitmap, p0, round_away_from_zero(a.LineAnchor()),
      color, truncated(lw), dashed(s), get_line_cap(s) );
    fill_triangle( m_bitmap, a.P0(), a.P1(), a.P2(), color );
  }
  else {
    draw_line( m_bitmap, p0, p1, color, truncated(lw * m_sc), dashed(s), get_line_cap(s) );
  }
}

void FaintDC::DrawRasterPolygon( const std::vector<Point>& points, const Settings& s ){
  if ( points.size() < 2 ){
    return;
  }
  std::vector<IntPoint> points2(truncated(transform_points(points, m_sc, m_origin ) ));

  if ( filled(s) ){
    fill_polygon(m_bitmap, points2, get_color_bg(s));
  }
  if ( border(s) ){
    draw_polygon(m_bitmap, points2, get_color_fg(s), truncated(s.Get(ts_LineWidth)), dashed(s));
  }
}

void FaintDC::DrawRasterPolyLine( const std::vector<Point>& points, const Settings& s ){
  assert(points.size() >= 2);
  bool skipLast = points.size() > 2 && points[points.size() - 2] == points[points.size() -1];
  std::vector<Point> points2(transform_points(points, m_sc, m_origin));
  if ( skipLast ){
    // Remove the last point if the two last points are identical
    // to avoid random arrowhead-direction for this case.
    points2.pop_back();
  }
  faint::coord lineWidth = s.Get(ts_LineWidth);
  faint::Color color(get_color_fg(s));
  if ( s.GetDefault( ts_LineArrowHead, false ) ){
    Point p0 = points2[points2.size() - 2];
    const Point& p1 = points2.back();
    ArrowHead a( get_arrowhead( ::Line(p0, p1), lineWidth) );
    points2.back() = a.LineAnchor();
    fill_triangle( m_bitmap, a.P0(), a.P1(), a.P2(), color );
  }
  draw_polyline( m_bitmap,
    truncated(points2),
    color,
    truncated(lineWidth),
    dashed(s),
    get_line_cap(s));
}

void FaintDC::DrawRasterRect( const Tri& tri, const Settings& s ){
  IntRect r(truncated(tri.P0() * m_sc + m_origin ),
    truncated(tri.P3() * m_sc + m_origin) );

  if ( filled(s) ){
    fill_rect( m_bitmap, r, get_color_bg(s) );
  }
  if ( border(s) ){
    draw_rect( m_bitmap, r, get_color_fg(s),
      truncated(s.Get( ts_LineWidth ) * m_sc),
      dashed(s) );
  }
}

void FaintDC::Ellipse( const Tri& tri0, const Settings& s ){
  if ( !anti_aliasing(s) ){
    DrawRasterEllipse( tri0, s );
    return;
  }

  faint::coord skew = tri0.Skew();
  Tri tri( skewed( tri0, -skew ) );

  faint::coord angle = tri.Angle();
  tri = rotated( tri, - angle, tri.P0() );

  from_settings( m_cr, s );
  std::vector<Point> v = ellipse_as_path( tri0 );

  m_cr->save();
  m_cr->move_to(v[0]);

  for ( size_t i = 1; i != v.size(); i += 3 ){
    m_cr->curve_to( v[i], v[i + 1], v[i+2]);
  }
  fill_and_or_stroke( m_cr, s );
  m_cr->restore();
}

faint::Color FaintDC::GetPixel( const Point& pos ) const{
  return get_color( m_bitmap, truncated(pos * m_sc + m_origin) );
}

void FaintDC::Line( const Point& in_p0, const Point& in_p1, const Settings& s ){
  if ( !anti_aliasing(s) ){
    DrawRasterLine(in_p0, in_p1, s);
    return;
  }

  if ( in_p0 == in_p1 ){
    // Don't draw zero-length lines as vector graphics
    return;
  }

  faint::coord dummy; // Avoid smearing the edges of axis-aligned lines on integer coordinates
  faint::coord x0(in_p0.x);
  faint::coord y0(in_p0.y);
  faint::coord x1(in_p1.x);
  faint::coord y1(in_p1.y);
  if ( y0 == y1 ){
    realign_cairo( dummy, y0, s, m_sc );
    realign_cairo( dummy, y1, s, m_sc );
  }
  // FIXME: All wrong.
  else if ( x0 == x1 ){
    faint::coord x0(in_p0.x);
    faint::coord x1(in_p1.x);
    realign_cairo( x0, dummy, s, m_sc );
    realign_cairo( x1, dummy, s, m_sc );
  }
  else {
    realign_cairo( x0, y0, s, m_sc );
    realign_cairo( y0, y1, s, m_sc );
  }

  from_settings( m_cr, s );
  m_cr->move_to( Point(x0, y0) );

  if ( s.Has( ts_LineArrowHead ) && s.Get( ts_LineArrowHead ) == 1 ){
    ArrowHead a( get_arrowhead( ::Line(Point(x0, y0), Point(x1, y1)), s.Get( ts_LineWidth ) ) );
    Point anchor = realign_cairo(a.LineAnchor(), s, m_sc);
    m_cr->line_to( anchor );
    m_cr->stroke();
    draw_arrow_head( m_cr, a, s.Get(ts_LineWidth), m_sc );
  }
  else {
    m_cr->line_to(Point(x1, y1));
    m_cr->stroke();
  }
}

void FaintDC::Path( const std::vector<PathPt>& points, const Settings& s ){
  if ( points.empty() ){
    return;
  }

  from_settings( m_cr, s );
  faint::coord curr_x = points.front().x;
  faint::coord curr_y = points.front().y;

  for ( size_t i = 0; i != points.size(); i++ ){
    const PathPt& pt = points[i];
    if ( pt.type == PathPt::MoveTo ){
      m_cr->move_to(Point(pt.x, pt.y)); // Redo PathPt
      curr_x = pt.x;
      curr_y = pt.y;
    }
    else if ( pt.type == PathPt::LineTo ){
      m_cr->line_to( Point(pt.x, pt.y) );
      curr_x = pt.x;
      curr_y = pt.y;
    }
    else if ( pt.type == PathPt::CubicBezier ){
      m_cr->curve_to(Point(pt.cx, pt.cy), Point(pt.dx, pt.dy), Point(pt.x, pt.y)); // oops, redo Pathpt
      curr_x = pt.x;
      curr_y = pt.y;
    }
    else if ( pt.type == PathPt::Close ){
      m_cr->close_path();
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
	m_cr->curve_to(Point(cx, cy), Point(dx, dy), Point(x1, y1) );
      }

      else if ( pt.large_arc_flag == 0 ){
	faint::coord cx = x0 + ( x1 - x0 ) / LITCRD(2.0);
	faint::coord cy = y0;
	faint::coord dx = x1;
	faint::coord dy = y0 + ( y1 - y0 ) / LITCRD(2.0);
        m_cr->curve_to(Point(cx, cy), Point(dx, dy), Point(x1, y1));
      }

      curr_x = x1;
      curr_y = y1;
    }
  }
  fill_and_or_stroke( m_cr, s );
}

void FaintDC::PenStroke( const std::vector<IntPoint>& points, const Settings& s ){
  if ( points.empty() ){
    return;
  }

  const faint::Color c = get_color_fg(s);
  const int width = 1;
  if ( points.size() == 1 ){
    const IntPoint p(truncated(points[0] * m_sc + m_origin));
    draw_line( m_bitmap, p, p, c, width, dashed(s), LineCap::BUTT );
    return;
  }

  IntPoint prev = truncated(floated(points[0]) * m_sc + m_origin);
  for ( size_t i = 1; i != points.size(); i++ ){
    const IntPoint pt = truncated(points[i] * m_sc + m_origin);
    draw_line( m_bitmap, prev, pt, c,
      truncated(m_sc), // ?
      false, LineCap::BUTT );
    prev = pt;
  }
}

void FaintDC::PolyLine( const std::vector<Point>& points, const Settings& s ){
  if ( points.empty() ){
    return;
  }

  if ( !anti_aliasing(s) ){
    DrawRasterPolyLine( points, s );
    return;
  }

  from_settings( m_cr, s );

  // Move to the first point
  const Point p0(realign_cairo(points[0], s, m_sc));
  m_cr->move_to(p0);

  // Add line segments to all consecutive points except (normally-)
  // the last to allow adjustment for arrowhead. If the two last
  // points are identical, only add lines up to the last two points,
  // to avoid adding a line segment that extends over the arrow head.
  bool skipTwo = points.size() > 2 && points[points.size() - 1] == points[points.size() - 2];
  size_t firstEnd = skipTwo ? points.size() - 2 : points.size() - 1;
  // Add line segments to all consecutive points except the last
  for ( size_t i = 0; i != firstEnd; i++ ){
    const Point pt(realign_cairo(points[i], s, m_sc));
    m_cr->line_to(pt);
  }

  // The source point for the last line segment.  If the last two
  // points are identical, use the pen-penultimate point as source to
  // avoid random arrowhead direction.
  const Point from(skipTwo ? points[points.size() - 3] :
    points[points.size() - 2]);
  const Point to(points.back());
  if ( s.Has( ts_LineArrowHead ) && s.Get( ts_LineArrowHead ) == 1 ){
    ArrowHead a( get_arrowhead( ::Line(from, to), s.Get( ts_LineWidth ) ) );
    Point anchor = realign_cairo(a.LineAnchor(), s, m_sc);
    m_cr->line_to(anchor);
    m_cr->stroke();
    draw_arrow_head( m_cr, a, s.Get(ts_LineWidth), m_sc );
  }
  else {
    Point p(realign_cairo(to, s, m_sc));
    m_cr->line_to(p);
    m_cr->stroke();
  }
}

void FaintDC::Polygon( const std::vector<Point>& points, const Settings& s ){
  if ( points.size() <= 1 ){
    return;
  }

  if ( !anti_aliasing(s) ){
    DrawRasterPolygon( points, s );
    return;
  }

  from_settings( m_cr, s );
  const Point& p0 = points[0];
  faint::coord x( p0.x );
  faint::coord y( p0.y );
  realign_cairo( x, y, s, m_sc );
  m_cr->move_to( Point(x, y) ); // Fixme
  for ( size_t i = 1; i != points.size(); i++ ){
    const Point& p( points[i] );
    x = p.x;
    y = p.y;
    realign_cairo( x, y, s, m_sc );
    m_cr->line_to( Point(x, y) );
  }
  m_cr->close_path();
  fill_and_or_stroke( m_cr, s );
}

void FaintDC::Rectangle( const Tri& tri, const Settings& s ){
  if ( !anti_aliasing(s) ){
    DrawRasterRect( tri, s );
    return;
  }

  std::vector<Point> points;
  points.push_back( tri.P0() );
  points.push_back( tri.P1() );
  points.push_back( tri.P3() );
  points.push_back( tri.P2() );
  Polygon( points, s );
}

void FaintDC::SetOrigin( const Point& origin ){
  m_origin = origin;
  m_cr->translate(origin);
}

void FaintDC::SetScale( faint::coord scale ){
  m_sc = scale;
  m_cr->scale( scale, scale );
}

void FaintDC::Spline( const std::vector<Point>& points, const Settings& s ){
  if ( points.size() <= 2 ){
    return;
  }

  from_settings( m_cr, s );
  const Point& p0 = points[0];
  faint::coord x1 = p0.x;
  faint::coord y1 = p0.y;

  const Point& p2 = points[1];
  faint::coord c = p2.x;
  faint::coord d = p2.y;

  faint::coord x3 = ( x1 + c ) / 2;
  faint::coord y3 = ( y1 + d ) / 2;

  m_cr->move_to(Point(x1, y1));
  m_cr->line_to( Point(x3, y3));
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

    m_cr->curve_to( Point(x1, y1),
      Point(x2, y2),
      Point(x3, y3));
  }
  m_cr->line_to(Point(c,d));
  m_cr->stroke();
}

void FaintDC::Stroke( const std::vector<IntPoint>& points, const Settings& s ){
  if ( points.size() < 1 ){
    return;
  }

  if ( s.Lacks(ts_BrushSize) ){
    PenStroke(points, s);
    return;
  }

  faint::Color c = get_color_fg(s);
  faint::Brush b( get_brush( s.Get( ts_BrushSize ), s.Get( ts_BrushShape ) ) );

  if ( points.size() == 1 ){
    const IntPoint p(truncated(points[0] * m_sc + m_origin));
    faint::stroke( m_bitmap, p, p, b, c );
    return;
  }

  IntPoint prev = points[0];
  for ( size_t i = 1; i != points.size(); i++ ){
    faint::stroke( m_bitmap, truncated(prev * m_sc + m_origin),
      truncated(points[i] * m_sc + m_origin), b, c );
    prev = points[i];
  }
}

void FaintDC::Text( const Tri& t, const faint::utf8_string& text, const Settings& s ){
  m_cr->pango_text(t, text, s);
}

Size FaintDC::TextExtents( const faint::utf8_string& text, const Settings& s ) const{
  return m_cr->pango_text_extents(text, s);
}
