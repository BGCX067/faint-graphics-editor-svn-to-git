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
#include "rendering/faintdc.hh"
#include "util/objutil.hh"
#include "util/settingutil.hh"

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

ObjSpline::ObjSpline( const Points& pts, const Settings& settings )
  : Object( &s_TypeSpline, pts.GetTri(), settings ),
    m_points( pts )
{}

ObjSpline::ObjSpline( const ObjSpline& other )
  : Object(  &s_TypeSpline, other.GetTri(), other.m_settings ),
    m_points( other.m_points )
{}

Object* ObjSpline::Clone() const{
  return new ObjSpline( *this );
}

void ObjSpline::Draw( FaintDC& dc ){
  dc.Spline( m_points.GetPointsDumb( GetTri() ), m_settings);
}

void ObjSpline::DrawMask( FaintDC& dc ){
  dc.Spline( m_points.GetPointsDumb(GetTri()), mask_settings_line(m_settings) );
}

std::vector<Point> ObjSpline::GetAttachPoints() const{
  std::vector<Point> splinePts( m_points.GetPointsDumb(GetTri()) );
  std::vector<Point> pts;
  pts.push_back(splinePts.front());
  pts.push_back(splinePts.back());
  return pts;
}

IntRect ObjSpline::GetRefreshRect(){
  return truncated(inflated( GetRect(), m_settings.Get(ts_LineWidth)));
}

std::vector<Point> ObjSpline::GetSplinePoints() const{
  return m_points.GetPointsDumb( GetTri() );
}

bool ObjSpline::ShowSizeBox() const{
  return true;
}
