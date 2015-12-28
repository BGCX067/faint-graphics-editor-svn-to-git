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

#include "faintdc.hh"
#include "objrectangle.hh"
#include "util/objutil.hh"
#include "tools/settingid.hh"

const std::string s_TypeRectangle = "Rectangle";

FaintSettings GetRectangleSettings(){
  FaintSettings s;
  s.Set( ts_LineWidth, 1 );
  s.Set( ts_LineStyle, 1 );
  s.Set( ts_FillStyle, BORDER );
  s.Set( ts_FgCol, faint::Color(0,0,0) );
  s.Set( ts_BgCol, faint::Color(255,255,255) );
  s.Set( ts_SwapColors, false );
  s.Set( ts_AntiAlias, true );
  s.Set( ts_LineJoin, faint::JOIN_MITER );
  return s;
}

ObjRectangle::ObjRectangle( const Tri& tri, const FaintSettings& s )
  : Object(&s_TypeRectangle, tri, s)
{}

void ObjRectangle::Draw( FaintDC& dc ){
  dc.Rectangle( GetTri(), m_settings );
}

void ObjRectangle::DrawMask( FaintDC& dc ){
  FaintSettings s(m_settings);
  s.Set( ts_FgCol, mask_edge );
  s.Set( ts_BgCol, mask_fill );
  dc.Rectangle( GetTri(), s );
}

IntRect ObjRectangle::GetRefreshRect(){
  const float lineWidth( static_cast<float>( m_settings.Get( ts_LineWidth ) ) );
  return truncated( Inflated( GetRect(), lineWidth ) );
}

bool ObjRectangle::HitTest( const Point& p ){
  return GetRect().Contains(p);
}

std::vector<Point> ObjRectangle::GetResizePoints(){
  return Corners( GetRect() );
}

std::vector<Point> ObjRectangle::GetAttachPoints(){
  return AttachPointsFromTri( GetTri() );
}

Object* ObjRectangle::Clone() const{
  return new ObjRectangle( GetTri(), m_settings );
}
