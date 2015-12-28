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

#include "objellipse.hh"
#include "objutil.hh"
#include "tools/settingid.hh"
#include "faintdc.hh"
const std::string s_TypeEllipse = "Ellipse";

FaintSettings GetEllipseSettings(){
  FaintSettings s;
  s.Set( ts_LineWidth, 1 );
  s.Set( ts_LineStyle, 1 );
  s.Set( ts_FillStyle, 0 );
  s.Set( ts_FgCol, faint::Color(0,0,0) );
  s.Set( ts_BgCol, faint::Color( 255, 255, 255 ) );
  s.Set( ts_SwapColors, false );
  return s;
}

ObjEllipse::ObjEllipse( const Tri& tri, const FaintSettings& settings )
  : Object(&s_TypeEllipse, tri, settings)
{}

void ObjEllipse::Draw( FaintDC& dc ){
  dc.Ellipse( GetTri(), m_settings );
}

void ObjEllipse::DrawMask( FaintDC& dc ){
  FaintSettings s(m_settings);
  s.Set( ts_FgCol, mask_edge );
  s.Set( ts_BgCol, mask_fill );
  dc.Ellipse( GetTri(), s );
}

IntRect ObjEllipse::GetRefreshRect(){
  const faint::coord width = m_settings.Get( ts_LineWidth );
  return truncated( Inflated(GetRect(), width ) );
}

bool ObjEllipse::HitTest( const Point& p ){
  return GetRect().Contains(p);
}

std::vector<Point> ObjEllipse::GetResizePoints(){
  return Corners(GetRect());
}

std::vector<Point> ObjEllipse::GetAttachPoints(){
  return AttachPointsFromTri( GetTri() );
}

Object* ObjEllipse::Clone() const{
  return new ObjEllipse( GetTri(), m_settings );
}
