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

#include "settings.hh"
#include "tools/settingid.hh"
#include "util/util.hh"
#include "objpolygon.hh"
#include "objutil.hh"
#include "faintdc.hh"
const std::string s_TypePolygon = "Polygon";

FaintSettings GetPolygonSettings(){
  FaintSettings s;
  s.Set( ts_LineWidth, 1 );
  s.Set( ts_LineStyle, 1 );
  s.Set( ts_FillStyle, 0 );
  s.Set( ts_FgCol, faint::Color(0,0,0) );
  s.Set( ts_BgCol, faint::Color( 255, 255, 255 ) );
  s.Set( ts_SwapColors, false );
  s.Set( ts_LineJoin, faint::JOIN_MITER );
  return s;
}

ObjPolygon::ObjPolygon( const Points& points, const FaintSettings& settings )
  : Object( &s_TypePolygon, points.GetTri(), settings ),
    m_points( points )
{}

ObjPolygon::ObjPolygon( const ObjPolygon& other )
  : Object( &s_TypePolygon, other.GetTri(), other.m_settings ),
    m_points( other.m_points )
{}

void ObjPolygon::Draw( FaintDC& dc ){
  dc.Polygon( m_points.GetPointsDumb( GetTri() ), m_settings );
}

void ObjPolygon::DrawMask( FaintDC& dc ) {
  FaintSettings s(m_settings);
  s.Set( ts_FgCol, mask_edge );
  s.Set( ts_BgCol, mask_fill );
  dc.Polygon( m_points.GetPointsDumb( GetTri() ), s );
}

IntRect ObjPolygon::GetRefreshRect(){
  const float lineWidth( static_cast<float>( m_settings.Get( ts_LineWidth ) ) );
  return truncated( Inflated(GetRect(), lineWidth * 2 ) );
}

bool ObjPolygon::HitTest( const Point& p ){
  return GetRect().Contains(p);
}

std::vector<Point> ObjPolygon::GetResizePoints(){
  return Corners(GetRect());
}

std::vector<Point> ObjPolygon::GetAttachPoints(){
  return Vertices();
}

Object* ObjPolygon::Clone() const{
  return new ObjPolygon( *this );
}

Point ObjPolygon::GetPoint( int ){
  assert( false );
  return Point(0,0);
}

const std::vector<Point> ObjPolygon::Vertices() const{
  return m_points.GetPointsDumb( GetTri() );
}
