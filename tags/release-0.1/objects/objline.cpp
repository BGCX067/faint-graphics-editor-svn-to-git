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

#include "objline.hh"
#include "util/angle.hh"
#include "util/util.hh"
#include "util/formatting.hh"
#include "tools/settingid.hh"
#include "settings.hh"
#include "faintdc.hh"
const std::string s_TypeLine = "Line";

using faint::coord;

Tri LineTri( const Point& p0, const Point& p1 ){
  return Tri(p0, p1, p0 );
}

Point LineMidPoint( const Tri& t ){
  return MidPoint( t.P0(), t.P1() );
}

FaintSettings GetLineSettings(){
  FaintSettings s;
  s.Set( ts_LineWidth, 1 );
  s.Set( ts_LineStyle, faint::SOLID );
  s.Set( ts_LineArrowHead, 0 );
  s.Set( ts_LineCap, faint::CAP_ROUND );
  s.Set( ts_FgCol, faint::Color(0,0,0) );
  s.Set( ts_SwapColors, false );
  return s;
}

ObjLine::ObjLine( const Point& p0, const Point& p1, const FaintSettings& settings )
  : Object( &s_TypeLine, LineTri(p0, p1), settings ),
    m_lastIndex(0)
{
  Tri t(GetTri());
  m_midPoint = LineMidPoint( GetTri() );
}

void ObjLine::Draw( FaintDC& dc ){
  dc.Line( GetTri(), m_settings );
}

void ObjLine::DrawMask( FaintDC& dc ){
  FaintSettings maskSettings( m_settings );
  maskSettings.Set( ts_FgCol, mask_edge);
  dc.Line( GetTri(), maskSettings );
}

Rect ObjLine::GetLineRect() const{
  faint::coord lineWidth(std::max( m_settings.Get( ts_LineWidth ), LITCRD(6.0) ));
  return Inflated(GetRect(), lineWidth / LITCRD(2.0) );
}

Rect ObjLine::GetArrowHeadRect() const{
  Tri tri(GetTri());
  return GetArrowHead(Line(tri.P0(), tri.P1()), m_settings.Get( ts_LineWidth ) ).BoundingBox();
}

IntRect ObjLine::GetRefreshRect(){
  if ( m_settings.Get( ts_LineArrowHead ) == 1 ){
    return truncated(Union(GetLineRect(), GetArrowHeadRect()));
  }
  return truncated(GetLineRect());
}

bool ObjLine::HitTest( const Point& pt ){
  if ( m_settings.Get( ts_LineArrowHead ) == 1 ){
    return GetLineRect().Contains(pt) || GetArrowHeadRect().Contains(pt);
  }
  return GetLineRect().Contains(pt);
}

Point ObjLine::GetStart() const{
  return GetTri().P0();
}

Point ObjLine::GetEnd() const{
  return GetTri().P1();
}

std::vector<Point> ObjLine::GetResizePoints(){
  return std::vector<Point>();
}

std::vector<Point> ObjLine::GetMovablePoints() const{
  Tri t(GetTri());
  return vector_of( t.P0(), t.P1() );
}

std::vector<Point> ObjLine::GetAttachPoints(){
  Tri t(GetTri());
  return vector_of( t.P0(), t.P1(), m_midPoint );
}

Object* ObjLine::Clone() const{
  Tri t(GetTri());
  return new ObjLine(t.P0(), t.P1(), m_settings);
}

Point ObjLine::GetPoint( size_t index ) const{
  assert( index <= 1 );
  return index == 0 ? GetStart() : GetEnd();
}

void ObjLine::OnSetTri(){
  m_midPoint = LineMidPoint(GetTri());
}

void ObjLine::SetPoint( const Point& pt, size_t index ){
  assert( index <= 1 );
  m_lastIndex = index;
  Tri t(GetTri());
  SetTri( LineTri( index == 0 ? pt : t.P0(),
      index == 0 ? t.P1() : pt ) );
}

std::string ObjLine::StatusString() const{
  Tri t(GetTri());

  return StrLineStatus( m_lastIndex == 0 ? t.P1() : t.P0(),
    m_lastIndex == 0 ? t.P0() : t.P1() );
}
