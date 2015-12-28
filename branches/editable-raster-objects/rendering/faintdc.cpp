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
#include "bitmap/filter.hh"
#include "geo/arc.hh"
#include "geo/pathpt.hh"
#include "geo/points.hh" // Fixme: For tri_from_points, which shouldn't be required here
#include "geo/tri.hh"
#include "rendering/cairocontext.hh"
#include "rendering/faintdc.hh"
#include "util/angle.hh"
#include "util/char.hh"
#include "util/color.hh"
#include "util/settings.hh"
#include "util/settingutil.hh"
#include "util/util.hh"
using faint::DrawSource;

static DrawSource get_bg( const Settings& s, const Point& origin ){
  DrawSource src = get_bg(s);
  return offsat(src, -floored(origin));
}

static DrawSource get_bg( const Settings& s, const Point& origin, const IntPoint& clickPos ){
  DrawSource src = get_bg(s);
  return offsat(src, -floored(origin), clickPos);
}

static DrawSource get_fg( const Settings& s, const Point& origin ){
  DrawSource src = get_fg(s);
  return offsat(src, -floored(origin));
}

static DrawSource get_fg( const Settings& s, const Point& origin, const IntPoint& clickPos ){
  DrawSource src = get_fg(s);
  return offsat(src, -floored(origin), clickPos);
}

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
  cr->set_line_cap(s.GetDefault(ts_LineCap, LineCap::DEFAULT));
  cr->set_line_join(s.GetDefault(ts_LineJoin, LineJoin::DEFAULT));
  faint::coord lineWidth = s.Has( ts_LineWidth ) ? s.Get( ts_LineWidth ) : LITCRD(1.0);
  cr->set_line_width( lineWidth );

  if ( filled(s) ){
    faint::DrawSource src(get_bg(s));
    cr->set_source(src);
    if ( border(s)){
      cr->fill_preserve();
    }
    else {
      cr->fill();
      return; // No stroke, return early
    }
  }

  cr->set_source( get_fg(s) );
  cr->stroke();
}

inline void from_settings( faint::CairoContext* cr, const Settings& s ){
  cr->set_line_cap( s.GetDefault(ts_LineCap, LineCap::DEFAULT) );
  cr->set_line_join( s.GetDefault(ts_LineJoin, LineJoin::DEFAULT) );
  faint::coord lineWidth = s.GetDefault(ts_LineWidth, LITCRD(1.0));
  cr->set_line_width(lineWidth);

  cr->set_source(get_fg(s)); // Fixme: This might duplicate pattern creation, compare fill_and_or_stroke
  if ( dashed(s) ) {
    faint::coord dashes[] = {2 * lineWidth, 2 * lineWidth};
    cr->set_dash( dashes, 2, 0 );
  }
  else {
    cr->set_dash( 0, 0, 0 );
  }
}

faint::Brush get_brush( const size_t size, BrushShape shape ){
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

static bool has_front_arrow( const Settings& s ){
  return s.Has( ts_LineArrowHead ) && s.Get(ts_LineArrowHead) == LineArrowHead::FRONT;
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

void FaintDC::Arc( const Tri& tri, const AngleSpan& span, const Settings& s ){
  if ( !anti_aliasing(s) ){
    return;
  }

  m_cr->set_source_tri(tri);
  from_settings(m_cr, s);
  std::vector<Point> v = arc_as_path(tri, span);
  Point c(center_point(tri));
  m_cr->move_to(c);
  m_cr->line_to(v[0]);
  for ( size_t i = 1; i < v.size(); i+= 3 ){
    m_cr->curve_to( v[i], v[i+1], v[i+2] );
  }
  m_cr->close_path();
  fill_and_or_stroke( m_cr, s );
}

void FaintDC::Bitmap( const faint::Bitmap& bmp, const Point& topLeft, const Settings& settings ){
  if ( overextends(topLeft + m_origin, m_bitmap) ){
    return;
  }
  bool alphaBlend = alpha_blending(settings);
  BackgroundStyle bgStyle = settings.Get(ts_BackgroundStyle);

  faint::DrawSource bg(settings.Get(ts_BgCol));
  if ( bg.IsColor() && bgStyle  == BackgroundStyle::MASKED ){
    faint::Color bgCol(bg.GetColor());
    if ( alphaBlend ){
      BitmapBlendAlphaMasked(bmp, bgCol, topLeft);
    }
    else{
      BitmapSetAlphaMasked(bmp, bgCol, topLeft);
    }
  }
  else {
    if ( alphaBlend ){
      BitmapBlendAlpha(bmp, topLeft);
    }
    else {
      BitmapSetAlpha(bmp, topLeft);
    }
  }
}

void FaintDC::BitmapBlendAlpha( const faint::Bitmap& drawnBitmap, const Point& topLeft ){
  assert( !overextends(topLeft + m_origin, m_bitmap) ); // Fixme: Shouldn't this check be scaled too?
  const IntPoint imagePt( floored(topLeft * m_sc + m_origin) ); // Fixme: Is flooring correct here, or is rounding more reasonable?
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
  assert( !overextends(topLeft + m_origin, m_bitmap) ); // Fixme: Shouldn't this check be scaled too?
  const IntPoint& imagePt( floored(topLeft * m_sc + m_origin ) ); // Fixme: Is flooring correct here?
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
  assert( !overextends(topLeft + m_origin, m_bitmap) ); // Fixme: Shouldn't this check be scaled too?
  const IntPoint imagePt(floored(topLeft * m_sc + m_origin)); // Fixme: Is flooring correct here?
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
  assert( !overextends(topLeft + m_origin, m_bitmap) ); // Fixme: Shouldn't this check be scaled too?
  const IntPoint imagePt(floored(topLeft * m_sc + m_origin)); // Fixme: Is flooring correct here?
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
  IntRect r(floored(tri.P0() * m_sc + m_origin ),
    floored(tri.P3() * m_sc + m_origin) );

  if ( filled(s) ){
    fill_ellipse( m_bitmap, r, get_bg(s, m_origin, rounded(center_point(tri))) );
  }
  if ( border(s) ){
    draw_ellipse( m_bitmap, r, floored(s.Get( ts_LineWidth )), get_fg(s, m_origin), dashed(s) );
  }
}

void FaintDC::DrawRasterLine( const Point& fl_p0, const Point& fl_p1, const Settings& s ){
  IntPoint p0(floored(floored(fl_p0) * m_sc) + floored(m_origin)); // Fixme: Truncating/flooring origin? Not done elsewhere
  IntPoint p1(floored(floored(fl_p1) * m_sc) + floored(m_origin));
  faint::DrawSource src(get_fg(s, m_origin));
  faint::coord lw(s.Get(ts_LineWidth));
  if ( has_front_arrow(s) ){
    ArrowHead a( get_arrowhead( LineSegment(floated(p0), floated(p1)), s.Get( ts_LineWidth ) ) ); // fixme: Do computations on unspoiled coordinates
    draw_line( m_bitmap, p0, round_away_from_zero(a.LineAnchor()),
      src, floored(lw), dashed(s), get_line_cap(s));
    fill_triangle( m_bitmap, a.P0(), a.P1(), a.P2(), src );
  }
  else {
    draw_line( m_bitmap, p0, p1, src, truncated(lw * m_sc), dashed(s), get_line_cap(s) );
  }
}

void FaintDC::DrawRasterPolygon( const std::vector<Point>& points, const Settings& s ){
  if ( points.size() < 2 ){
    return;
  }
  std::vector<IntPoint> points2(floored(transform_points(points, m_sc, m_origin ) ));

  if ( filled(s) ){
    fill_polygon(m_bitmap, points2, get_bg(s, m_origin));
  }
  if ( border(s) ){
    draw_polygon(m_bitmap, points2, get_fg(s, m_origin), floored(s.Get(ts_LineWidth)), dashed(s));
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
  faint::DrawSource src(get_fg(s, m_origin));
  std::vector<IntPoint> points3(floored(points2));
  if ( s.GetDefault( ts_LineArrowHead, LineArrowHead::NONE ) == LineArrowHead::FRONT ){
    Point p0 = points2[points2.size() - 2];
    const Point& p1 = points2.back();
    ArrowHead a( get_arrowhead( LineSegment(p0, p1), lineWidth) );
    points3.back() = rounded(a.LineAnchor());
    fill_triangle( m_bitmap, a.P0(), a.P1(), a.P2(), src );
  }
  draw_polyline( m_bitmap,
    points3,
    src,
    truncated(lineWidth),
    dashed(s),
    get_line_cap(s));
}

void FaintDC::DrawRasterRect( const Tri& tri, const Settings& s ){
  IntRect r(floored(tri.P0() * m_sc + m_origin ),
    floored(tri.P3() * m_sc + m_origin) );

  if ( filled(s) ){
    fill_rect( m_bitmap, r, get_bg(s, m_origin));
  }
  if ( border(s) ){
    draw_rect( m_bitmap, r, get_fg(s, m_origin), truncated(s.Get( ts_LineWidth ) * m_sc), dashed(s) );
  }
}

void FaintDC::Ellipse( const Tri& tri, const Filter& f, const Settings& s ){
  // Fixme: Filter variant is raster only
  IntRect r(floored(tri.P0() * m_sc + m_origin ),
    floored(tri.P3() * m_sc + m_origin) );

  IntSize extraSize(40,40);
  faint::Bitmap bmp(r.GetSize() + extraSize, faint::color_transparent_white());
  IntPoint offset(10,10);
  IntRect r2(translated(r, -r.TopLeft() + offset));
  if ( filled(s) ){
    fill_ellipse( bmp, r2, get_bg(s, m_origin) );
  }
  if ( border(s) ){
    draw_ellipse( bmp, r2, truncated(s.Get( ts_LineWidth )), get_fg(s, m_origin), dashed(s) );
  }
  f.Apply(bmp);
  BitmapBlendAlpha(bmp, tri.P0() - floated(offset));
}

void FaintDC::Ellipse( const Tri& tri, const Settings& s ){
  if ( !anti_aliasing(s) ){
    DrawRasterEllipse( tri, s );
    return;
  }
  m_cr->set_source_tri(tri);
  from_settings( m_cr, s );
  std::vector<Point> v = ellipse_as_path( tri );

  m_cr->save();
  m_cr->move_to(v[0]);
  for ( size_t i = 1; i != v.size(); i += 3 ){
    m_cr->curve_to( v[i], v[i + 1], v[i+2]);
  }
  fill_and_or_stroke( m_cr, s );
  m_cr->restore();
}

faint::Color FaintDC::GetPixel( const Point& posView ) const{
  IntPoint posBmp(floored(posView * m_sc + m_origin));
  assert(point_in_bitmap(m_bitmap, posBmp));
  return get_color( m_bitmap, posBmp );
}

bool FaintDC::IsOk() const{
  return m_cr->IsOk();
}

std::string FaintDC::ErrorString() const{
  return m_cr->ErrorString();
}

void FaintDC::Line( const Point& in_p0, const Point& in_p1, const Filter& f, const Settings& s ){
  // Fixme: Raster only (filter variant)
  IntSize extraSize(40,40);
  faint::Bitmap bmp(truncated(Size(fabs(in_p1.x - in_p0.x), fabs(in_p1.y - in_p0.y))) + extraSize,
    faint::color_transparent_white());
  IntPoint origin = floored(min_coords(in_p0, in_p1));
  IntPoint offset(10,10);
  faint::draw_line(bmp, floored(in_p0) - origin + offset, floored(in_p1) - origin + offset, get_fg(s, m_origin),
    floored(s.Get(ts_LineWidth)), dashed(s), get_line_cap(s));
  f.Apply(bmp);
  BitmapBlendAlpha(bmp, floated(origin - offset));
}

void FaintDC::Path( const std::vector<PathPt>& points, const Settings& s ){
  if ( points.empty() ){
    return;
  }

  from_settings( m_cr, s );
  faint::coord curr_x = points.front().x;
  faint::coord curr_y = points.front().y;

  Tri patternTri(tri_from_points(points)); // Fixme: Pass tri instead
  if ( !rather_zero(patternTri) ){
    m_cr->set_source_tri(patternTri);
  }

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

  const faint::DrawSource src = get_fg(s, m_origin);
  const int width = 1;
  if ( points.size() == 1 ){
    const IntPoint p(floored(points[0] * m_sc + m_origin));
    draw_line( m_bitmap, p, p, src, width, dashed(s), LineCap::BUTT );
    return;
  }

  IntPoint prev = floored(floated(points[0]) * m_sc + m_origin);
  for ( size_t i = 1; i != points.size(); i++ ){
    const IntPoint pt = floored(points[i] * m_sc + m_origin);
    draw_line( m_bitmap, prev, pt, src,
      truncated(m_sc), // ?
      false, LineCap::BUTT );
    prev = pt;
  }
}

void FaintDC::PolyLine( const Tri& tri, const std::vector<Point>& points, const Settings& s ){
  if ( points.empty() ){
    return;
  }

  if ( !anti_aliasing(s) ){
    DrawRasterPolyLine( points, s );
    return;
  }

  from_settings( m_cr, s );
  m_cr->set_source_tri(tri);

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
  if ( has_front_arrow(s) ){
    ArrowHead a( get_arrowhead( LineSegment(from, to), s.Get( ts_LineWidth ) ) );
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

void FaintDC::Polygon( const Tri& tri, const std::vector<Point>& points, const Settings& s ){
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
  m_cr->set_source_tri(tri);
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


void FaintDC::Rectangle( const Tri& tri, const Filter& f, const Settings& s ){
  // Fixme: Filter version is raster only
  IntRect r(floored(tri.P0() * m_sc + m_origin ),
    floored(tri.P3() * m_sc + m_origin) );

  IntSize extraSize(40,40);
  faint::Bitmap bmp(r.GetSize() + extraSize,
    faint::color_transparent_white());
  IntPoint offset(10,10);
  IntRect r2(translated(r, -r.TopLeft() + offset));
  if ( filled(s) ){
    fill_rect( bmp, r2, get_bg(s, m_origin) );
  }
  if ( border(s) ){
    draw_rect( bmp, r2, get_fg(s, m_origin),
      truncated(s.Get( ts_LineWidth ) * m_sc),
      dashed(s) );
  }
  f.Apply(bmp);
  BitmapBlendAlpha(bmp, tri.P0() - floated(offset));
}

void FaintDC::Rectangle( const Tri& tri, const Settings& s ){
  if ( !anti_aliasing(s) ){
    DrawRasterRect( tri, s );
    return;
  }
  m_cr->set_source_tri(tri);
  Polygon( tri, rectangle_as_clockwise_polygon(tri), s );
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
  if ( points.empty() ){
    return;
  }

  if ( s.Lacks(ts_BrushSize) ){
    PenStroke(points, s);
    return;
  }

  faint::DrawSource src( get_fg(s, m_origin, points.front()) );
  faint::Brush b( get_brush( s.Get( ts_BrushSize ), s.Get( ts_BrushShape ) ) );
  std::vector<IntPoint> scaledPoints;
  for ( size_t i = 0; i != points.size(); i++ ){
    scaledPoints.push_back(floored(points[i] * m_sc + m_origin));
  }
  faint::stroke( m_bitmap, scaledPoints, b, src );
}

void FaintDC::Text( const Tri& t, const faint::utf8_string& text, const Settings& s ){
  m_cr->set_source_tri(t);
  m_cr->pango_text(t, text, s);
}

Size FaintDC::TextSize( const faint::utf8_string& text, const Settings& s ) const{
  return m_cr->pango_text_size(text, s);
}

TextMeasures<Rect> FaintDC::TextExtents( const faint::utf8_string& text, const Settings& s ) const{
  return m_cr->pango_text_extents(text, s);
}

std::vector<int> FaintDC::CumulativeTextWidth( const faint::utf8_string& text, const Settings& s ) const{
  return m_cr->cumulative_text_width(text, s);
}
