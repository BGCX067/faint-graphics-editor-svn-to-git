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
#include "objpath.hh"
#include "objutil.hh"

const std::string s_TypePath = "Path";

FaintSettings GetPathSettings(){
  FaintSettings s;
  s.Set( ts_LineWidth, 1 );
  s.Set( ts_LineStyle, 1 );
  s.Set( ts_FillStyle, 0 );
  s.Set( ts_FgCol, faint::Color( 0, 0, 0 ) );
  s.Set( ts_BgCol, faint::Color( 255, 255, 255 ) );
  s.Set( ts_SwapColors, false );
  return s;
}

ObjPath::ObjPath( const Points& points, const FaintSettings& settings )
  : Object( &s_TypePath, points.GetTri(), settings ),
    m_points( points )
{}

ObjPath::ObjPath( const ObjPath& other ) :
  Object( &s_TypePath, other.GetTri(), other.m_settings ),
  m_points( other.m_points )
{}

void ObjPath::Draw( FaintDC& dc ){
  dc.Path( m_points.GetPoints(GetTri()), m_settings );
}

void ObjPath::DrawMask( FaintDC& dc ){
  FaintSettings s(m_settings);
  s.Set( ts_FgCol, mask_edge );
  s.Set( ts_BgCol, mask_fill );
  dc.Path( m_points.GetPoints(GetTri()), s );
}

IntRect ObjPath::GetRefreshRect(){
  float lw( static_cast<float>( m_settings.Get( ts_LineWidth ) ) );
  return truncated(Inflated(GetRect(), lw ));
}

bool ObjPath::HitTest( const Point& p ){
  return GetRect().Contains(p);
}

std::vector<Point> ObjPath::GetResizePoints(){
  return Corners( GetRect() );
}

std::vector<Point> ObjPath::GetAttachPoints(){
  return std::vector<Point>();
}

Object* ObjPath::Clone() const{
  return new ObjPath( *this );
}

std::vector<PathPt> ObjPath::GetPathPoints() const{
  return m_points.GetPoints(GetTri());
}
