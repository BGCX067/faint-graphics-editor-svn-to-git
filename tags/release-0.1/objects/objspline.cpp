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

#include "objspline.hh"
#include "tools/settingid.hh"
#include "objutil.hh"
#include "faintdc.hh"
const std::string s_TypeSpline = "Spline";

Points spline_to_svg_path( const std::vector<Point>& points ){
  Points out;
  const Point& p0 = points[0];
  faint::coord x1 = p0.x;
  faint::coord y1 = p0.y;

  const Point& p1 = points[1];
  faint::coord c = p1.x;
  faint::coord d = p1.y;

  faint::coord x3 = ( x1 + c ) / 2;
  faint::coord y3 = ( y1 + d ) / 2;

  out.Append( PathPt::Move( x1, y1 ) );
  out.Append( PathPt::Line( x3, y3 ) );

  for ( size_t i = 2; i != points.size(); i++ ){
    x1 = x3;
    y1 = y3;
    faint::coord x2 = c;
    faint::coord y2 = d;

    const Point& pt = points[i];
    c = pt.x;
    d = pt.y;
    x3 = ( x2 + c ) / 2;
    y3 = ( y2 + d ) / 2;
    out.Append( PathPt::Curve( x3, y3, x1, y1, x2, y2 ) );
  }
  return out;
}

FaintSettings GetSplineSettings(){
  FaintSettings s;
  s.Set( ts_LineWidth, 1 );
  s.Set( ts_LineStyle, faint::SOLID );
  s.Set( ts_FgCol, faint::Color(0,0,0) );
  s.Set( ts_BgCol, faint::Color( 255, 255, 255 ) );
  s.Set( ts_SwapColors, false );
  s.Set( ts_LineCap, faint::CAP_BUTT );
  return s;
}

ObjSpline::ObjSpline( const Points& pts, const FaintSettings& settings )
  : Object( &s_TypeSpline, pts.GetTri(), settings ),
    m_points( pts )
{}

ObjSpline::ObjSpline( const ObjSpline& other )
  : Object(  &s_TypeSpline, other.GetTri(), other.m_settings ),
    m_points( other.m_points )    
{}

void ObjSpline::Draw( FaintDC& dc ){
  dc.Spline( m_points.GetPointsDumb( GetTri() ), m_settings);
}

void ObjSpline::DrawMask( FaintDC& dc ){
  FaintSettings s(m_settings);
  s.Set( ts_FgCol, mask_edge );
  dc.Spline( m_points.GetPointsDumb( GetTri() ), s );
}

IntRect ObjSpline::GetRefreshRect(){
  const float lineWidth( static_cast<float>( m_settings.Get( ts_LineWidth ) ) );
  return truncated(Inflated( GetRect(), lineWidth ));
}

bool ObjSpline::HitTest( const Point& p ){
  return GetRect().Contains(p);
}

std::vector<Point> ObjSpline::GetResizePoints(){
  return Corners(GetRect());
}

std::vector<Point> ObjSpline::GetAttachPoints(){
  // Fixme: Use first and last point
  return GetResizePoints();
}

std::vector<Point> ObjSpline::GetSplinePoints() const{
  return m_points.GetPointsDumb( GetTri() );
}

Object* ObjSpline::Clone() const{
  return new ObjSpline( *this );
}
